/**************************************************************************
*
* Copyright (c) 2017, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <pthread.h>


#include "atmod.h"
#include "atchannel.h"
#include "at_tok.h"
#include "misc.h"
#include "at.h"
#include "aterr.h"
#include "atusb.h"

static int at_init(void* data, unsigned int datalen);
static int at_deinit(void* data, unsigned int datalen);
static int at_start(void* data, unsigned int datalen);
static int at_stop(void* data, unsigned int datalen);
static int at_set_info(void* data, unsigned int datalen);
static int at_get_info(void* data, unsigned int datalen);

static void onATReaderClosed();
static void onATTimeout();
static void onUnsolicited (const char *s, const char *sms_pdu);
static void waitForClose();
static int checkRadioState(void);
static int requestRadioPowerOnDown(int radio_mode);
static int requestEhrpdEnable(void *data, unsigned int datalen);
static int requestSIMStatus(void *data, unsigned int datalen);
static int requestOperator(void *data, unsigned int datalen);
static int requestOrSendDataCallList(void *data, unsigned int datalen);
static void requestMode(void *data, size_t datalen);
static void requestNetSearch(void *data, size_t datalen);
static void requestSignalStrength(void *data, size_t datalen);


static int 	fd = -1;

static int s_mcc = 0;
static int s_mnc = 0;


/* trigger change to this with s_state_cond */
static int s_closed = 0;

static pthread_mutex_t s_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_state_cond = PTHREAD_COND_INITIALIZER;

static struct at_info  atinfo;
static struct at_info* at = &atinfo;

static struct usb_info usb;

struct usbev_module usbdev_at = {
	._init 		= at_init,
	._deinit 	= at_deinit,
	._start 	= at_start,
	._stop 		= at_stop,
	._set_info 	= at_set_info,
	._get_info 	= at_get_info,
};


struct operator_item {
	char 	cimi[16];
	int	  	oper;
} oper_match[] = {
	//移动
	{ "CHINA MOBILE", 	CHINA_MOBILE },
	{ "CMCC", 			CHINA_MOBILE },
   	{ "46000", 			CHINA_MOBILE },
   	{ "46002", 			CHINA_MOBILE },
   	{ "46004", 			CHINA_MOBILE },   //中国移动物联网
   	{ "46007", 			CHINA_MOBILE },
	{ "46008", 			CHINA_MOBILE },
   	{ "46020", 			CHINA_MOBILE },
   	{ "46027", 			CHINA_MOBILE },

	//联通
	{ "CHN-UNICOM", 	CHINA_UNICOM },
	{ "UNICOM", 		CHINA_UNICOM },
   	{ "46001", 			CHINA_UNICOM },
   	{ "46006", 			CHINA_UNICOM },  //联通物联网
    { "46009", 			CHINA_UNICOM },
    
	//电信
	{ "CHN-UNICOM", 	CHINA_TELECOM },
	{ "46003", 			CHINA_TELECOM },
   	{ "46005", 			CHINA_TELECOM },
    { "46011", 			CHINA_TELECOM },
    { "2040",  			CHINA_TELECOM },
};

static char* _request_oper_map[] = {
	"CHINA_MOBILE",
	"CHINA_UNICOM",
	"CHINA_TELECOM",
	"UNKNOWN",
};

struct mode_item {
	char name[16];
	int  oper;
	int	 mode;
	
} mode_match[] =  {

	{ "NONE", 		CHINA_MOBILE,  MODE_NONE     },
	{ "NONE", 		CHINA_UNICOM,  MODE_NONE     },
	{ "NONE", 		CHINA_TELECOM, MODE_NONE     },
		
	{ "LTE FDD", 	CHINA_TELECOM, MODE_FDDLTE   },
	{ "LTE FDD", 	CHINA_UNICOM,  MODE_FDDLTE   },
   	{ "LTE TDD", 	CHINA_MOBILE,  MODE_TDLTE    },
   	
   	{ "UMTS", 		CHINA_MOBILE,  MODE_TDSCDMA  },
   	{ "UMTS", 		CHINA_UNICOM,  MODE_WCDMA    },
   	
   	{ "HSUPA", 		CHINA_UNICOM,  MODE_WCDMA    },   
   	{ "HSDPA", 		CHINA_MOBILE,  MODE_TDSCDMA  },
   	{ "EVDO", 		CHINA_TELECOM, MODE_EVDO     },
   	{ "TDSCDMA", 	CHINA_MOBILE,  MODE_TDSCDMA  },
   	{ "GPRS", 		CHINA_MOBILE,  MODE_GPRS     },
	{ "GPRS", 		CHINA_UNICOM,  MODE_GPRS     },
	{ "GPRS", 		CHINA_TELECOM, MODE_GPRS     },
	{ "EDGE", 		CHINA_MOBILE,  MODE_GPRS     },
	{ "EDGE", 		CHINA_UNICOM,  MODE_GPRS     },
	{ "EDGE", 		CHINA_TELECOM, MODE_GPRS     },
	{ "WCDMA", 		CHINA_UNICOM,  MODE_WCDMA    },
};

char *oper_request(SIM_OPERATOR request_oper) {
	return _request_oper_map[request_oper];
}


static 	int at_init(void* data, unsigned int datalen) {
	if(!data || datalen != sizeof(struct usb_info))
		return -1;
	memcpy(&usb, data, sizeof(struct usb_info));

	memset(at, 0, sizeof(struct at_info));

	int  ret = -1;
	char devName[32];

    at_set_on_reader_closed(onATReaderClosed);
    at_set_on_timeout(onATTimeout);

	memset(devName, 0, sizeof(devName));
	sprintf(devName, "/dev/ttyUSB%d", usb.match.atport);
	fd = open(devName, O_RDWR);
	if(-1 == fd) {
		goto err;
	}
	
	ret = setSerialBaud(fd, B115200);
	if(ret != 0)
		goto err;

	ret = setSerialRawMode(fd);
	if(ret != 0)
		goto err;

	s_closed = 0;
	ret = at_open(fd, onUnsolicited);

	if (ret < 0) {
		AT_DBG ("AT error %d on at_open\n", ret);
		return -1;
	}

/*
	//RIL_requestTimedCallback(initializeCallback, NULL, &TIMEVAL_0);
	mobile_init(NULL, 0);
	
	// Give initializeCallback a chance to dispatched, since
	// we don't presently have a cancellation mechanism
	sleep(1);
	s_closed = 1;

	waitForClose();
	//AT_DBG("Re-opening after close\n");
*/
	at->stat = E_AT_OPEN_SUCC;
	return 0;
	
err:
	at->stat = E_AT_OPEN_FAIL;
	return -1;
}
static int at_deinit(void* data, unsigned int datalen) {
	return 0;
}

static 	int at_start(void* data, unsigned int datalen) {

	int err = -1;
	ATResponse *p_response = NULL;

	/* Make ME Online */
	PHONE_FUNCTION phone_function = CFUN_ONLINE_MODE; 
	requestRadioPowerOnDown(phone_function);

	/* EhrpdEnable off needed for pppd*/
	requestEhrpdEnable(NULL, 0); 

	at_handshake(NULL, 0);

	requestNetSearch(NULL, 0);
	
	requestSIMStatus(NULL, 0);
	
	/* note: we don't check errors here. Everything important will
	   be handled in onATTimeout and onATReaderClosed */

	/*	atchannel is tolerant of echo but it must */
	/*	have verbose result codes */
	//at_send_command("ATE0Q0V1", NULL);
	at_send_command("ATE1", NULL);

	/*	No auto-answer */
	at_send_command("ATS0=0", NULL);

	/*	Extended errors */
	at_send_command("AT+CMEE=1", NULL);

	/*	set apn autometicly according to curent network operator */
	at_send_command("AT+NVRW=1,50058,\"01\"", NULL);

	//PDP_DEACTIVE
	//at_send_command("AT+CGACT=1,1", NULL);

	/* Auto:All band search*/
	at_send_command("AT+BNDPRF=896,1272,131072", NULL);

	/*	Network registration events */
	err = at_send_command("AT+CREG=2", &p_response);

	/* some handsets -- in tethered mode -- don't support CREG=2 */
	if (err < 0 || p_response->success == 0) {
		at_send_command("AT+CREG=1", NULL);
	}

	at_response_free(p_response);

	/*	GPRS registration events */
	at_send_command("AT+CGREG=1", NULL);

	/*	Call Waiting notifications */
	at_send_command("AT+CCWA=1", NULL);

	/*	Alternating voice/data off */
	at_send_command("AT+CMOD=0", NULL);

	/*	Not muted */
	//at_send_command("AT+CMUT=0", NULL);

	/*	+CSSU unsolicited supp service notifications */
	at_send_command("AT+CSSN=0,1", NULL);

	/*	no connected line identification */
	at_send_command("AT+COLP=0", NULL);

	/*	HEX character set */
	//at_send_command("AT+CSCS=\"HEX\"", NULL);

	/*	USSD unsolicited */
	at_send_command("AT+CUSD=1", NULL);

	/*	Enable +CGEV GPRS event notifications, but don't buffer */
	at_send_command("AT+CGEREP=1,0", NULL);

	/*	SMS PDU mode */
	at_send_command("AT+CMGF=0", NULL);

	requestOperator(NULL, 0);
	requestSignalStrength(NULL, 0);
	requestOrSendDataCallList(NULL, 0);
	requestMode(NULL, 0);


	// Give initializeCallback a chance to dispatched, since
	// we don't presently have a cancellation mechanism
   sleep(1);
   s_closed = 1;

   waitForClose();
	
	return 0;
}

static int at_stop(void* data, unsigned int datalen) {
	return 0;
}
static int at_set_info(void* data, unsigned int datalen) {
	return 0;
}

static 	int	at_get_info(void* data, unsigned int datalen) {
	if(!data || datalen != sizeof(struct at_info))
		return -1;
	memcpy((struct at_info*)data, at, sizeof(struct at_info));
	return 0;
}



static int isEhrpdeanable()
{
	ATResponse *p_response = NULL;
    int err = -1;
    char *line = NULL;
    char ret = -1;

    err = at_send_command_singleline("AT+EHRPDENABLE?", "+EHRPDENABLE:", &p_response);

    if (err < 0 || p_response->success == 0) {
        // assume radio is off
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextbool(&line, &ret);
    if (err < 0) goto error;

    at_response_free(p_response);

    return (int)ret;

error:

    at_response_free(p_response);
    return -1;

}

static int 
requestEhrpdEnable(void *data, unsigned int datalen) {

	ATResponse *p_response = NULL;
    int 		err = -1;
	
	if(!isEhrpdeanable()) 
		return 0;

	err = at_send_command("AT+EHRPDENABLE=0", &p_response);
	if (err < 0 || p_response->success == 0) {
		goto error;
	}
	
	at_response_free(p_response);
	return 0;
error:

    at_response_free(p_response);
    return -1;

}


/** returns 1 if on, 0 if off, and -1 on error */
static int checkRadioState(void)
{
    ATResponse *p_response = NULL;
    int err = -1;
    char *line = NULL;
    char ret = -1;

    err = at_send_command_singleline("AT+CFUN?", "+CFUN:", &p_response);

    if (err < 0 || p_response->success == 0) {
        // assume radio is off
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextbool(&line, &ret);
    if (err < 0) goto error;

    at_response_free(p_response);

    return (int)ret;

error:

    at_response_free(p_response);
    return -1;
}



static int requestRadioPowerOnDown(int radio_mode)
{
	int				count = 5;
    int 			err = -1;
	int				curr_radio_mode;
    ATResponse*		p_response = NULL;
	char			at_cmd[64];

	memset(at_cmd, 0, sizeof(at_cmd));

	curr_radio_mode = checkRadioState();
	if(curr_radio_mode == radio_mode) {
		return 0;
	}

	snprintf(at_cmd, sizeof(at_cmd)-1, "AT+CFUN=%d", radio_mode);  
	while(count-- < 0) {
    	err = at_send_command(at_cmd, &p_response);
		if (err < 0 || p_response->success == 0) {
			continue;
		}else {
			if (checkRadioState() == radio_mode) 
           		break;        		
		}
    }
    at_response_free(p_response);
    return 0;
}



/** Returns RUIM_NOT_READY on error */
static int
requestSIMStatus(void *data, unsigned int datalen)
{
    ATResponse *p_response = NULL;
    int err = -1;
    int ret = -1;
    char *cpinLine = NULL;
    char *cpinResult = NULL;

	//QCPIN?
    err = at_send_command_singleline("AT+CPIN?", "+CPIN:", &p_response);

    if (err != 0) {
        ret = SIM_NOT_READY;
        goto done;
    }

    switch (at_get_cme_error(p_response)) {
        case CME_SUCCESS:
            break;

        case CME_SIM_NOT_INSERTED:
            ret = SIM_ABSENT;
            goto done;

        default:
            ret = SIM_NOT_READY;
            goto done;
    }

    /* CPIN? has succeeded, now look at the result */

    cpinLine = p_response->p_intermediates->line;
    err = at_tok_start (&cpinLine);

    if (err < 0) {
        ret = SIM_NOT_READY;
        goto done;
    }

    err = at_tok_nextstr(&cpinLine, &cpinResult);

    if (err < 0) {
        ret = SIM_NOT_READY;
        goto done;
    }

	//AT_DBG("sim:%s\n", cpinResult);
    if (0 == strcmp (cpinResult, "SIM PIN")) {
        ret = SIM_PIN;
        goto done;
    } else if (0 == strcmp (cpinResult, "SIM PUK")) {
        ret = SIM_PUK;
        goto done;
    } else if (0 == strcmp (cpinResult, "PH-NET PIN")) {
        return SIM_NETWORK_PERSONALIZATION;
    } else if (0 != strcmp (cpinResult, "READY"))  {
        /* we're treating unsupported lock types as "sim absent" */
        ret = SIM_ABSENT;
        goto done;
    }

    at_response_free(p_response);
    p_response = NULL;
    cpinResult = NULL;

    ret = SIM_READY;
	at->stat = E_USB_HARDWARE_OK;
	at_response_free(p_response);
	return ret;
done:
	at->stat = E_USB_TTY_UNAVAILABLE;
	at->sim = ret;
	//AT_DBG("sim:%d\n", ret);
    at_response_free(p_response);
    return ret;
}


static int requestOrSendDataCallList(void *data, size_t datalen)
{
    ATResponse *p_response;
    ATLine *p_cur;
    int err;
    int n = 0;
    char *out;

    err = at_send_command_multiline ("AT+CGDCONT?", "+CGDCONT:", &p_response);
    if (err != 0 || p_response->success == 0) {

        return -1;
    }

    for (p_cur = p_response->p_intermediates; p_cur != NULL;
         p_cur = p_cur->p_next) {
        char *line = p_cur->line;
        int cid;

        err = at_tok_start(&line);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &cid);
        if (err < 0)
            goto error;


		//printf("cid = %d \n", cid);

		err = at_tok_nextstr(&line, &out);
        if (err < 0)
            goto error;

		//printf("type = %s \n", out);

		 // APN ignored for v5
        err = at_tok_nextstr(&line, &out);
        if (err < 0)
            goto error;

		//printf("apn = %s \n", out);

		err = at_tok_nextstr(&line, &out);
        if (err < 0)
            goto error;
		
		//printf("addresses = %s \n", out);
       

    }

    at_response_free(p_response);


    return 0;

error:

    at_response_free(p_response);
	return -1;
}

static void requestQueryNetworkSelectionMode(
                void *data , size_t datalen)
{
    int err;
    ATResponse *p_response = NULL;
    int response = 0;
    char *line;

	//AT+COPS=?
	//+COPS: (2,"CHN-UNICOM","UNICOM","46001",7),(3,"CHN-CT","CT","46011",7),(3,"CHINA MOBILE","CMCC","46000",7),,(0,1,2,3,4),(0,1,2)
	//AT+COPS?
	//+COPS: 0,2,"46001",7
	err = at_send_command_singleline("AT+COPS?", "+COPS:", &p_response);

    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);

    if (err < 0) {
        goto error;
    }

    err = at_tok_nextint(&line, &response);

    if (err < 0) {
        goto error;
    }
    at_response_free(p_response);
    return;
error:
    at_response_free(p_response);
    AT_DBG("requestQueryNetworkSelectionMode must never return error when radio is on");
}

static void requestSignalStrength(void *data, size_t datalen)
{
    ATResponse *p_response = NULL;
    int err = -1;
    char *line;
    int count = 0;
    int response = -1;

	//AT+CCSQ
    err = at_send_command_singleline("AT+CSQ", "+CSQ:", &p_response);

    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &response);
    if (err < 0) goto error;

	at->csq = response;
//	printf("csq:%d\n", response);

    at_response_free(p_response);
    return;

error:
    AT_DBG("requestSignalStrength must never return an error when radio is on\n");
    at_response_free(p_response);
}

static void requestNetSearch(void *data, size_t datalen)
{
	//切到4G模式  --- 先查询
	 if(usb.match.imodel == USB_HUAWEI)
	 	at_send_command("AT^SYSCFGEX=\"030201\",3FFFFFFF,1,2,7FFFFFFFFFFFFFFF,,", NULL);
	 
	 if(usb.match.imodel == USB_LONGSUNG)
	 at_send_command("AT+MODODREX=2", NULL);
}

static void requestMode(void *data, size_t datalen)
{
    ATResponse *p_response = NULL;
    int err = -1;
    char *line;
    int count = 0;
    char* response = NULL;

	//AT^HCSQ?  AT+PSRAT AT^SYSINFOEX　AT^SYSINFO
    err = at_send_command_singleline("AT+PSRAT", "+PSRAT:", &p_response);

    if (err < 0 || p_response->success == 0) {
		err = at_send_command_singleline("AT^SYSINFOEX", "^SYSINFOEX:", &p_response);

		if (err < 0 || p_response->success == 0) {
        	goto error;
		}

		int tmp[7];
		char curr_mode[32];
		memset(curr_mode, 0, sizeof(curr_mode));
		line = p_response->p_intermediates->line;
		char* p = strstr(line, ":");
		if( !p ) 
			goto error;
		
		sscanf(p, ":%d,%d,%d,%d,,%d,\"%s\"", &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], curr_mode);
		switch(tmp[4]) {
			case 1:
				at->mode = MODE_GPRS;
				memcpy(at->mode_name, "GPRS", 16);
				break;
			case 3: 
				at->mode = MODE_WCDMA;
				memcpy(at->mode_name, "WCDMA", 16);
				break;
			case 4:
				at->mode = MODE_TDSCDMA;
				memcpy(at->mode_name, "TDSCDMA", 16);
				break;
			case 6: 	
				if(at->oper == CHINA_MOBILE) {
					at->mode = MODE_TDLTE;
					memcpy(at->mode_name, "LTE TDD", 16);
				}
				else {
					at->mode = MODE_FDDLTE;
					memcpy(at->mode_name, "LTE FDD", 16);
				}
				break;
			default:
				break;

		}
		goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextstr(&line, &response);
    if (err < 0) goto error;

	unsigned int i = 0;
	for(i = 0; i < _ARRAY_SIZE(mode_match); i++) {
		if(strstr(response, mode_match[i].name) 
			&& at->oper == mode_match[i].oper) {
			at->mode= mode_match[i].mode;
			memcpy(at->mode_name, mode_match[i].name, 16);
			break;
		}
		at->mode = MODE_NONE;
	}
	//printf("mode:%s\n", response);

    at_response_free(p_response);
    return;

error:
//    AT_DBG("requestSignalStrength must never return an error when radio is on\n");
	

    at_response_free(p_response);
}


static int requestOperator(void *data, unsigned int datalen)
{
    int err = -1;
    int i;
    int skip;
    ATLine *p_cur;
    char *response[3];

    memset(response, 0, sizeof(response));

    ATResponse *p_response = NULL;

	//AT+COPS=3,0;+COPS?;+COPS=3,1;+COPS?;+COPS=3,2;+COPS?;
	//at+cimi 
	//AT+QCIMI
    //err = at_send_command_singleline("AT+CIMI", "+CIMI", &p_response);
    err = at_send_command_multiline("AT+COPS=3,0;+COPS?;+COPS=3,1;+COPS?;+COPS=3,2;+COPS?", "+COPS:", &p_response);

	//printf("####%s\n", p_response->p_intermediates->line);
    /* we expect 3 lines here:
     * +COPS: 0,0,"T - Mobile"
     * +COPS: 0,1,"TMO"
     * +COPS: 0,2,"310170"
     *  exp:
     *	+COPS: 0,0,"CHN-UNICOM",7
	 *	+COPS: 0,1,"UNICOM",7
	 *	+COPS: 0,2,"46001",7

	 	+COPS: 0,0,"CHINA MOBILE",7
	 	+COPS: 0,1,"CMCC",7
		+COPS: 0,2,"46000",7

		+COPS: 0,0,"CMCC",7
		+COPS: 0,1,"CMCC",7
		+COPS: 0,2,"46000",7

		//电信有问题
		+COPS: 0
		+COPS: 0
		+COPS: 0
     */
#if 0
    if (err < 0 || p_response->success == 0) {
        // assume radio is off
        goto error;
    }
	char resp[16] = { 0 };
    char* line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextstr(&line, &resp);
    if (err < 0) goto error;

	printf("resp:%s\n", resp);
#endif
#if 1

    for (i = 0, p_cur = p_response->p_intermediates
            ; p_cur != NULL
            ; p_cur = p_cur->p_next, i++
    ) {
        char *line = p_cur->line;

        err = at_tok_start(&line);
        if (err < 0) goto error;
		

        err = at_tok_nextint(&line, &skip);
        if (err < 0) goto error;

        // If we're unregistered, we may just get
        // a "+COPS: 0" response
        if (!at_tok_hasmore(&line)) {
            response[i] = NULL;
            continue;
        }

        err = at_tok_nextint(&line, &skip);
        if (err < 0) goto error;

	
        // a "+COPS: 0, n" response is also possible
        if (!at_tok_hasmore(&line)) {
            response[i] = NULL;
            continue;
        }

        err = at_tok_nextstr(&line, &(response[i]));
        if (err < 0) goto error;

		//AT_DBG("oper:%s\n", response[i]);

		unsigned int	j = 0;
		for(j = 0; j < _ARRAY_SIZE(oper_match); j++) {
			if(strstr(response[i], oper_match[j].cimi)) {
				at->oper = oper_match[j].oper;
				break;
			}
			at->oper = UNKNOWN;
		}
        // Simple assumption that mcc and mnc are 3 digits each
        if (strlen(response[i]) == 6) {
            if (sscanf(response[i], "%3d%3d", &s_mcc, &s_mnc) != 2) {
              //  AT_DBG("requestOperator expected mccmnc to be 6 decimal digits\n");
            }
        }
    }

    if (i != 3) {
        /* expect 3 lines exactly */
        goto error;
    }
#endif
#if 0
	int j = 0;
	for(j = 0; j < _ARRAY_SIZE(oper_match); j++) {
		if(strstr(resp, oper_match[j].cimi)) {
			at->oper = oper_match[j].oper;
			break;
		}
		at->oper = OPER_UNKNOWN;
	}
#endif

    at_response_free(p_response);

    return 0;
error:
    AT_DBG("requestOperator must not return error when radio is on\n");
    at_response_free(p_response);
	return -1;
}

static void waitForClose()
{
    pthread_mutex_lock(&s_state_mutex);

    while (s_closed == 0) {
        pthread_cond_wait(&s_state_cond, &s_state_mutex);
    }

    pthread_mutex_unlock(&s_state_mutex);
}

/* Called on command or reader thread */
static void onATReaderClosed()
{
    AT_DBG("AT channel closed\n");
    at_close();
    s_closed = 1;

    //setRadioState (RADIO_STATE_UNAVAILABLE);
}

/* Called on command thread */
static void onATTimeout()
{
    AT_DBG("AT channel timeout; closing\n");
    at_close();

    s_closed = 1;

    /* FIXME cause a radio reset here */

    //setRadioState (RADIO_STATE_UNAVAILABLE);
}


/**
 * Called by atchannel when an unsolicited line appears
 * This is called on atchannel's reader thread. AT commands may
 * not be issued here
 */
static void onUnsolicited (const char *s, const char *sms_pdu)
{
    char *line = NULL, *p;
    int err = -1;

    if (strStartsWith(s, "%CTZV:")) {
        /* TI specific -- NITZ time */
        char *response;

        line = p = strdup(s);
        at_tok_start(&p);

        err = at_tok_nextstr(&p, &response);

        if (err != 0) {
            AT_DBG("invalid NITZ line %s\n", s);
        } else {
 
        free(line);
       }
    } else if (strStartsWith(s,"+CRING:")
                || strStartsWith(s,"RING")
                || strStartsWith(s,"NO CARRIER")
                || strStartsWith(s,"+CCWA")
    ) {

    } else if (strStartsWith(s,"+CREG:")
                || strStartsWith(s,"+CGREG:")
    ) {


    } else if (strStartsWith(s, "+CMT:")) {

    } else if (strStartsWith(s, "+CDS:")) {

    } else if (strStartsWith(s, "+CGEV:")) {
        /* Really, we can ignore NW CLASS and ME CLASS events here,
         * but right now we don't since extranous
         * RIL_UNSOL_DATA_CALL_LIST_CHANGED calls are tolerated
         */
        /* can't issue AT commands here -- call on main thread */
     

    } else if (strStartsWith(s, "+CME ERROR: 150")) {
    
    } else if (strStartsWith(s, "+CTEC: ")) {
      
    } else if (strStartsWith(s, "+CCSS: ")) {
        int source = 0;
        line = p = strdup(s);
        if (!line) {
            AT_DBG("+CCSS: Unable to allocate memory");
            return;
        }
        if (at_tok_start(&p) < 0) {
            free(line);
            return;
        }
        if (at_tok_nextint(&p, &source) < 0) {
            AT_DBG("invalid +CCSS response: %s", line);
            free(line);
            return;
        }
//        SSOURCE(sMdmInfo) = source;
     
    } else if (strStartsWith(s, "+WSOS: ")) {
        char state = 0;
        int unsol;
        line = p = strdup(s);
        if (!line) {
            AT_DBG("+WSOS: Unable to allocate memory");
            return;
        }
        if (at_tok_start(&p) < 0) {
            free(line);
            return;
        }
        if (at_tok_nextbool(&p, &state) < 0) {
            AT_DBG("invalid +WSOS response: %s", line);
            free(line);
            return;
        }
        free(line);

    } else if (strStartsWith(s, "+WPRL: ")) {
        int version = -1;
        line = p = strdup(s);
        if (!line) {
            AT_DBG("+WPRL: Unable to allocate memory");
            return;
        }
        if (at_tok_start(&p) < 0) {
            AT_DBG("invalid +WPRL response: %s", s);
            free(line);
            return;
        }
        if (at_tok_nextint(&p, &version) < 0) {
            AT_DBG("invalid +WPRL response: %s", s);
            free(line);
            return;
        }
        free(line);
       // RIL_onUnsolicitedResponse(RIL_UNSOL_CDMA_PRL_CHANGED, &version, sizeof(version));
    } else if (strStartsWith(s, "+CFUN: 0")) {
       // setRadioState(RADIO_STATE_OFF);
    }
}



#if 0
#define REG_STATE_LEN 		15
#define REG_DATA_STATE_LEN 6
static void requestRegistrationState(int request, void *data ,
                                        size_t datalen)
{
	#define REQUEST_VOICE_REGISTRATION_STATE 1
	#define REQUEST_DATA_REGISTRATION_STATE  2
	
    int err;
    int *registration;
    char **responseStr = NULL;
    ATResponse *p_response = NULL;
    const char *cmd;
    const char *prefix;
    char *line;
    int i = 0, j, numElements = 0;
    int count = 3;
    int type, startfrom;

    AT_DBG("requestRegistrationState");
    if (request == REQUEST_VOICE_REGISTRATION_STATE) {
        cmd = "AT+CREG?";
        prefix = "+CREG:";
        numElements = REG_STATE_LEN;
    } else if (request == REQUEST_DATA_REGISTRATION_STATE) {
        cmd = "AT+CGREG?";
        prefix = "+CGREG:";
        numElements = REG_DATA_STATE_LEN;
    } else {
        assert(0);
        goto error;
    }

    err = at_send_command_singleline(cmd, prefix, &p_response);

    if (err != 0) goto error;

    line = p_response->p_intermediates->line;

    if (parseRegistrationState(line, &type, &count, &registration)) goto error;

    responseStr = malloc(numElements * sizeof(char *));
    if (!responseStr) goto error;
    memset(responseStr, 0, numElements * sizeof(char *));
    /**
     * The first '4' bytes for both registration states remain the same.
     * But if the request is 'DATA_REGISTRATION_STATE',
     * the 5th and 6th byte(s) are optional.
     */
     #if 0
    if (is3gpp2(type) == 1) {
        AT_DBG("registration state type: 3GPP2");
        // TODO: Query modem
        startfrom = 3;
        if(request == REQUEST_VOICE_REGISTRATION_STATE) {
            asprintf(&responseStr[3], "8");     // EvDo revA
            asprintf(&responseStr[4], "1");     // BSID
            asprintf(&responseStr[5], "123");   // Latitude
            asprintf(&responseStr[6], "222");   // Longitude
            asprintf(&responseStr[7], "0");     // CSS Indicator
            asprintf(&responseStr[8], "4");     // SID
            asprintf(&responseStr[9], "65535"); // NID
            asprintf(&responseStr[10], "0");    // Roaming indicator
            asprintf(&responseStr[11], "1");    // System is in PRL
            asprintf(&responseStr[12], "0");    // Default Roaming indicator
            asprintf(&responseStr[13], "0");    // Reason for denial
            asprintf(&responseStr[14], "0");    // Primary Scrambling Code of Current cell
      } else if (request == REQUEST_DATA_REGISTRATION_STATE) {
            asprintf(&responseStr[3], "8");   // Available data radio technology
      }
    } else { // type == RADIO_TECH_3GPP
        AT_DBG("registration state type: 3GPP");
        startfrom = 0;
        asprintf(&responseStr[1], "%x", registration[1]);
        asprintf(&responseStr[2], "%x", registration[2]);
        if (count > 3)
            asprintf(&responseStr[3], "%d", registration[3]);
    }
    asprintf(&responseStr[0], "%d", registration[0]);

    /**
     * Optional bytes for DATA_REGISTRATION_STATE request
     * 4th byte : Registration denial code
     * 5th byte : The max. number of simultaneous Data Calls
     */
    if(request == RIL_REQUEST_DATA_REGISTRATION_STATE) {
        // asprintf(&responseStr[4], "3");
        // asprintf(&responseStr[5], "1");
    }

    for (j = startfrom; j < numElements; j++) {
        if (!responseStr[i]) goto error;
    }
	
    free(registration);
    registration = NULL;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, responseStr, numElements*sizeof(responseStr));
    for (j = 0; j < numElements; j++ ) {
        free(responseStr[j]);
        responseStr[j] = NULL;
    }
	#endif
    sfree(responseStr);
    responseStr = NULL;
    at_response_free(p_response);

    return;
error:
    if (responseStr) {
        for (j = 0; j < numElements; j++) {
            free(responseStr[j]);
            responseStr[j] = NULL;
        }
        free(responseStr);
        responseStr = NULL;
    }
    AT_DBG("requestRegistrationState must never return an error when radio is on");
    at_response_free(p_response);
}



#endif



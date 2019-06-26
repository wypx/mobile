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
#include <mobile.h>
#include <at_id.h>

#define TTY_USB_FORMAT      "/dev/ttyUSB%d"
#define USB_DRIVER_PATH     "/proc/modules"

static s32 usb_init(void *data, u32 datalen);
static s32 usb_start(void *data, u32 datalen);
static s32 usb_get_info(void *data, u32 datalen);

/* trigger change to this with s_state_cond */
static s32 s_closed = 0;

static pthread_mutex_t s_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_state_cond = PTHREAD_COND_INITIALIZER;

static struct usb_info  usbinfo;
static struct usb_info* usb = &usbinfo;

static struct usb_item usbdevs[] = {
    /* LongSung Model */
    { USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_C5300V, "", "C5300V",           MODEM_LONGSUNG,  3, 0, 2, 2},
    { USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_C7500,  "", "EVDO USB MODEM",   MODEM_LONGSUNG,  3, 0, 2, 2},//C7500
    { USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U7500,  "", "HSPA USB MODEM",   MODEM_LONGSUNG,  1, 2, 0, 2},//U7500
    { USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U8300,  "", "U8300",            MODEM_LONGSUNG,  1, 3, 2, 2},
    { USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U8300W, "", "U8300W",           MODEM_LONGSUNG,  1, 3, 2, 2},
    { USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U8300C, "", "U8300C",           MODEM_LONGSUNG,  2, 1, 3, 2},
    { USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U9300C, "", "U9300C",           MODEM_LONGSUNG,  2, 1, 3, 2}, 
    { USB_MODEM_IDVENDOR_SANQ,      USB_MODEM_IDPRODUCT_LM9165, "", "LM9165",           MODEM_LONGSUNG,  1, 3, 2, 2},
    { USB_MODEM_IDVENDOR_SANQ,      USB_MODEM_IDPRODUCT_LM9115, "", "LM9115",           MODEM_LONGSUNG,  2, 3, 2, 2}, 

    /* Huawei Model */
    { USB_MODEM_IDVENDOR_HUAWEI, USB_MODEM_IDPRODUCT_MU709s_2,	 "", "MU709s-2",    MODEM_HUAWEI,  4, 2, 1, 0},
    { USB_MODEM_IDVENDOR_HUAWEI, USB_MODEM_IDPRODUCT_ME909u_521, "", "ME909u_521",  MODEM_HUAWEI,  4, 2, 1, 0},
    //  { USB_MODEM_IDVENDOR_HUAWEI, USB_MODEM_IDPRODUCT_ME909u_523, "", "ME909u_523",  MODEM_HUAWEI,  4, 2, 1,},
    { USB_MODEM_IDVENDOR_HUAWEI, USB_MODEM_IDPRODUCT_ME909s_821, "", "ME909s-821",  MODEM_HUAWEI,  4, 2, 1, 0},//ME909s ME906s
    { USB_MODEM_IDVENDOR_HUAWEI, USB_MODEM_IDPRODUCT_ME909s_121, "", "ME909s_121",  MODEM_HUAWEI,  4, 2, 1, 0},
    //ME909u ME906E cid=16
    //ME936 ME206V cid=11
};

static s8* _request_modem_map[] = {
    "MODEM_UNKOWN",
    "MODEM_LONGSUNG",
    "MODEM_HUAWEI",
    "MODEM_SIMCOM",
    "MODEM_NODECOM",
    "MODEM_NEOWAY",
};

static s8 * _request_oper_map[] = {
    "CHINA_UNKOWN",
    "CHINA_MOBILE",
    "CHINA_UNICOM",
    "CHINA_TELCOM",
};

s8 *mobile_modem_str(enum USB_MODEM modem) {
    return _request_modem_map[modem];
}

s8 *mobile_operator_str(enum SIM_OPERATOR oper) {
    return _request_oper_map[oper];
}

struct operator_item {
    s8  cimi[16];
    u32 operator;
} oper_match[] = {
    { "CHINA MOBILE",   CHINA_MOBILE },
    { "CMCC",           CHINA_MOBILE },
    { "46000",          CHINA_MOBILE },
    { "46002",          CHINA_MOBILE },
    { "46004",          CHINA_MOBILE },   /*NB-IOT*/
    { "46007",          CHINA_MOBILE },
    { "46008",          CHINA_MOBILE },
    { "46020",          CHINA_MOBILE },
    { "46027",          CHINA_MOBILE },

    { "CHN-UNICOM",     CHINA_UNICOM },
    { "UNICOM",         CHINA_UNICOM },
    { "46001",          CHINA_UNICOM },
    { "46006",          CHINA_UNICOM },  /*NB-IOT*/
    { "46009",          CHINA_UNICOM },

    { "CHN-UNICOM",     CHINA_TELCOM },
    { "46003",          CHINA_TELCOM },
    { "46005",          CHINA_TELCOM },
    { "46011",          CHINA_TELCOM },
    { "2040",           CHINA_TELCOM },
};

struct mode_item {
    s8  name[16];
    s32 operator;
    s32 mode;
} mode_match[] =  {
    { "NONE",       CHINA_MOBILE,  MODE_NONE    },
    { "NONE",       CHINA_UNICOM,  MODE_NONE    },
    { "NONE",       CHINA_TELCOM,  MODE_NONE    },

    { "LTE FDD",    CHINA_TELCOM,  MODE_FDDLTE  },
    { "LTE FDD",    CHINA_UNICOM,  MODE_FDDLTE  },
    { "LTE TDD",    CHINA_MOBILE,  MODE_TDLTE   },

    { "UMTS",       CHINA_MOBILE,  MODE_TDSCDMA },
    { "UMTS",       CHINA_UNICOM,  MODE_WCDMA   },

    { "HSUPA",      CHINA_UNICOM,  MODE_WCDMA   },
    { "HSDPA",      CHINA_MOBILE,  MODE_TDSCDMA },
    { "EVDO",       CHINA_TELCOM,  MODE_EVDO    },
    { "TDSCDMA",    CHINA_MOBILE,  MODE_TDSCDMA },
    { "GPRS",       CHINA_MOBILE,  MODE_GPRS    },
    { "GPRS",       CHINA_UNICOM,  MODE_GPRS    },
    { "GPRS",       CHINA_TELCOM,  MODE_GPRS    },
    { "EDGE",       CHINA_MOBILE,  MODE_GPRS    },
    { "EDGE",       CHINA_UNICOM,  MODE_GPRS    },
    { "EDGE",       CHINA_TELCOM,  MODE_GPRS    },
    { "WCDMA",      CHINA_UNICOM,  MODE_WCDMA   },
};

static void onATReaderClosed(void);
static void onATTimeout(void);
static void waitForClose(void);
static void onUnsolicited (const s8 *s, const s8 *sms_pdu);

static s32 usb_check_id_support(u32 vendorid, u32 productid) {

    u32 i;
    for(i = 0; i < _ARRAY_SIZE(usbdevs); i++) {

        if(productid == usbdevs[i].productid 
            && vendorid == usbdevs[i].vendorid) {
            memcpy(&(usb->item), &usbdevs[i], sizeof(struct usb_item));
            break;
        }
        usb->usb_status = E_DRIVER_ID_NOT_SUPPORT;
        if (i == (_ARRAY_SIZE(usbdevs)-1))
            return -1;
    }

    return 0;
}

static s32 usb_check_lsusb(void) {

    s8    line[256];
    FILE  *fp = NULL;
    s8    *p = NULL;

    fp = popen("lsusb", "r");
    if (!fp) {
        usb->usb_status = E_DRIVER_ID_NOT_SUPPORT;
        return -1;
    }

    u32 productid = 0;
    u32 vendorid = 0;

    while (!feof(fp)) {
        memset(line, 0, sizeof(line));
        if (!fgets(line, sizeof(line), fp)) {
            break;
        }

        if ((p = strstr(line, "ID"))) {
            sscanf(p, "ID %x:%x", &vendorid, &productid); 
            if (usb_check_id_support(vendorid, productid) < 0) {
                continue;
            } else {
                pclose(fp);
                return 0;
            }
        }
    }

    usb->usb_status = E_DRIVER_ID_NOT_SUPPORT;
    pclose(fp);
    return -1;
}

static s32 usb_check_ttyusb(void) {
    s32 rc = -1;
    s8  ttyPath[16];

    memset(ttyPath, 0, sizeof(ttyPath));
    snprintf(ttyPath, sizeof(ttyPath)-1, TTY_USB_FORMAT, usb->item.modem_port);

    /* ls /dev/ttyUSB# */
    rc = msf_chk_file_exist(ttyPath);
    if (rc != 0) {
        /* Drivers have been not installed, ttyUSB not exist,
           that is the drivers not support current productid */
        usb->usb_status = E_TTYUSB_NOT_FOUND;
        return -1;
    }
    return 0;
}

/* first check ttyUSB whther exist, if not 
then check the drivers whther are installed: 
usb_wwan.ko, usbserial.ko and option.ko */
static s32 usb_check_lsmod(void) {

    /*lsmod ==> cat /proc/modules */
    s32     rc = -1;

    FILE    *fp = NULL; 
    s8      line[128];
    s32     drivers = 0;

    rc = msf_chk_file_exist(USB_DRIVER_PATH);
    if (!rc) {
        usb->usb_status = E_DRIVER_NOT_INSTALLED;
        return -1;
    }

    fp = fopen(USB_DRIVER_PATH, "r");
    if (NULL == fp) {
        usb->usb_status = E_DRIVER_NOT_INSTALLED;
        return -1;
    }

    while (!feof(fp)) {
        memset(line, 0, sizeof(line));
        if (!fgets(line,  sizeof(line), fp)) {
            break;
        }

        if (strstr(line, "option") || 
           strstr(line, "usb_wwan") || 
           strstr(line, "usbserial")) {
            drivers++;
            /* Otherwise Drivers have not been installed completely or 
                No drivers have been installed */
            if(3 == drivers) {
                sfclose(fp);
                return 0;
            }
        } else {
            continue;
        }
    }

    usb->usb_status = E_DRIVER_NOT_INSTALLED;
    sfclose(fp);
    return -1;
}

void usb_modem_match(s8 *modem) {

    u32 i;
    for(i = 0; i < _ARRAY_SIZE(usbdevs); i++) {
        /*(same) USB ID canot recognise model, AT+CGMM needed for LongSung */
        if (msf_strstr(modem, usbdevs[i].modem_name) && 
            usb->item.productid == usbdevs[i].productid &&
            usb->item.vendorid == usbdevs[i].vendorid) {

            memcpy(&(usb->item), &usbdevs[i], sizeof(struct usb_item));
            usb->usb_status = E_TTYUSB_AVAILABLE;
            break;
        }
        usb->usb_status = E_TTYUSB_UNAVAILABLE;
    }
}

void usb_operator_match(s8 *cimi) {

    u32 i;
    for(i = 0; i < _ARRAY_SIZE(oper_match); i++) {
        /*(same) USB ID canot recognise model, AT+CGMM needed for LongSung */
        if (msf_strstr(cimi, oper_match[i].cimi)) {
            usb->sim_operator = oper_match[i].operator;
            break;
        }
        usb->sim_operator = CHINA_UNKOWN;
    }
}

/*澶栭儴瑕侀噴鏀剧敵璇风殑娑堟伅-at_response_free*/
void usb_process_network_mode(s8 *line) {

    s32 rc;
    s32 count = 0;
    s8 *response = NULL;

    if (usb->item.modem_type == MODEM_HUAWEI) {

        s32 tmp[7];
        s8 curr_mode[32];
        memset(curr_mode, 0, sizeof(curr_mode));
        s8 *p = msf_strstr(line, ":");

        if( !p ) return;
        
        sscanf(p, ":%d,%d,%d,%d,,%d,\"%s\"",
            &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], curr_mode);
        switch(tmp[4]) {
            case 1:
                usb->network_mode = MODE_GPRS;
                memcpy(usb->network_mode_str, "GPRS", strlen("GPRS"));
                break;
            case 3: 
                usb->network_mode = MODE_WCDMA;
                memcpy(usb->network_mode_str, "WCDMA", strlen("WCDMA"));
                break;
            case 4:
                usb->network_mode = MODE_TDSCDMA;
                memcpy(usb->network_mode_str, "TDSCDMA", strlen("TDSCDMA"));
                break;
            case 6:     
                if (usb->sim_operator == CHINA_MOBILE) {
                    usb->network_mode = MODE_TDLTE;
                    memcpy(usb->network_mode_str, "LTE TDD", strlen("LTE TDD"));
                }
                else {
                    usb->network_mode = MODE_FDDLTE;
                    memcpy(usb->network_mode_str, "LTE FDD", strlen("LTE TDD"));
                }
                break;
            default:
                break;
        }
    } else {
    
        rc = at_tok_start(&line);
        if (rc < 0) return;

        rc = at_tok_nextstr(&line, &response);
        if (rc < 0) return;

        u32 i = 0;
        for(i = 0; i < _ARRAY_SIZE(mode_match); i++) {
            if(msf_strstr(response, mode_match[i].name) 
                && usb->sim_operator == mode_match[i].operator) {
                usb->network_mode = mode_match[i].mode;
                memcpy(usb->item.modem_name, mode_match[i].name, 16);
                break;
            }
            usb->network_mode = MODE_NONE;
        }
    }
}

static s32 usb_init(void *data, u32 datalen) {

    s32 rc = -1;
    s8  dev_path[32];

    memset(usb, 0, sizeof(struct usb_info));
    usb->usb_fd = -1;
    usb->usb_status = -1;
    usb->item.modem_type = MODEM_UNKOWN;

    if (usb_check_lsmod() < 0) {
        MSF_MOBILE_LOG(DBG_ERROR, "Usb modem driver not installed.");
        return -1;
    }

    if (usb_check_lsusb() < 0) {
        MSF_MOBILE_LOG(DBG_ERROR, "Usb modem id app not matched.");
        return -1;
    }

    if (usb_check_ttyusb() < 0) {
        MSF_MOBILE_LOG(DBG_ERROR, "Usb modem ttyusb not found.");
        return -1;
    }
    
    at_set_on_reader_closed(onATReaderClosed);
    at_set_on_timeout(onATTimeout);

    memset(dev_path, 0, sizeof(dev_path));
    sprintf(dev_path, TTY_USB_FORMAT, usb->item.at_port);
    usb->usb_fd = open(dev_path, O_RDWR);
    if (-1 == usb->usb_fd) {
        goto err;
    }

    rc = msf_serial_baud(usb->usb_fd, B115200);
    if (rc != 0)
        goto err;

    rc = msf_serial_rawmode(usb->usb_fd);
    if (rc != 0) goto err;

    s_closed = 0;
    rc = at_open(usb->usb_fd, onUnsolicited);
    if (rc < 0) {
        MSF_MOBILE_LOG(DBG_ERROR, "AT error %d on at_open.", rc);
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
    usb->usb_status = E_TTYUSB_AVAILABLE;

    return 0;

err:
    return -1;
}


static s32 usb_start(void *data, u32 datalen) {

    mobile_at_modem_init();
    mobile_at_get_modem(usb_modem_match);
    mobile_at_set_radio(CFUN_ONLINE_MODE);
    mobile_at_set_ehrpd(EHRPD_CLOSE);

    usb->sim_status = mobile_at_get_sim();
    if (unlikely(usb->sim_status != SIM_READY))
        usb->usb_status = E_SIM_COMES_LOCKED;
    else
        usb->usb_status = E_SIM_COMES_READY;

    mobile_at_get_operator(usb_operator_match);
    mobile_at_get_signal();
    mobile_at_search_network(usb->item.modem_type, LS_AUTO);

    usleep(500);

    mobile_at_get_network_mode();

    mobile_at_get_network_3gpp();

    mobile_at_get_apns();

    // Give initializeCallback a chance to dispatched, since
    // we don't presently have a cancellation mechanism
    sleep(1);
    s_closed = 1;

    waitForClose();

    return 0;
}

static s32 usb_get_info(void *data, u32 datalen) {
    if (datalen != sizeof(struct usb_info))
        return -1;
    memcpy(data, usb, sizeof(struct usb_info));
    return 0;
}

/***************************AT Related Function******************************/

static void waitForClose() {

    pthread_mutex_lock(&s_state_mutex);

    while (s_closed == 0) {
        pthread_cond_wait(&s_state_cond, &s_state_mutex);
    }

    pthread_mutex_unlock(&s_state_mutex);
}

/* Called on command or reader thread */
static void onATReaderClosed(void) {
    MSF_MOBILE_LOG(DBG_DEBUG, "AT channel closed.");
    at_close();
    s_closed = 1;

    //setRadioState (RADIO_STATE_UNAVAILABLE);
}

/* Called on command thread */
static void onATTimeout() {
    MSF_MOBILE_LOG(DBG_DEBUG, "AT channel timeout; closing\n");
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
static void onUnsolicited (const s8 *s, const s8 *sms_pdu) {
    s8 *line = NULL, *p;
    s32 rc = -1;

    if (at_str_startwith(s, "%CTZV:")) {
        /* TI specific -- NITZ time */
        s8 *response;

        line = p = strdup(s);
        at_tok_start(&p);

        rc = at_tok_nextstr(&p, &response);

        if (rc != 0) {
            MSF_MOBILE_LOG(DBG_DEBUG, "invalid NITZ line %s.", s);
        } else {

        free(line);
       }
    } else if (at_str_startwith(s, "+CRING:")
                || at_str_startwith(s, "RING")
                || at_str_startwith(s, "NO CARRIER")
                || at_str_startwith(s, "+CCWA")) {

    } else if (at_str_startwith(s,"+CREG:")
                || at_str_startwith(s,"+CGREG:")) {


    } else if (at_str_startwith(s, "+CMT:")) {
    	//短信到达指示: +CMTI
    } else if (at_str_startwith(s, "+CDS:")) {
    	//新短信状态报告到达指示: +CDSI
    } else if (at_str_startwith(s, "+CME ERROR: 150")) {

    } else if (at_str_startwith(s, "+CMTI")) {
        /*鎺ユ敹鍒颁竴鏉℃柊鐨勭煭淇￠�氱煡*/
        //if(MODE_EVDO == mobile_mode)
        {
           // sprintf((char *)command, "AT^HCMGR=%d", );
        }
        //else
        {
           // sprintf((char *)command, "AT+CMGR=%d", );
        }

    } else if (at_str_startwith(s, "+CGDCONT:")) {

    }
    //^HCMGR +CMGR:

    //^SMMEMFULL", "%SMMEMFULL", "+SMMEMFULL"
    //^MODE:
    //^SYSINFO:", "%SYSINFO:", %SYSINFO:"
    //sscanf(p, ":%d,%d,%d,%d,%d", &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4]);
    //sscanf(last, ",%d", &tmp[6]);
    #if 0
    	mobile_srvStat = tmp[0]; 

		if (255 == tmp[1])
		{
			mobile_srvDomain = 3;
		}
		else
		{
			mobile_srvDomain = tmp[1];
		}
	 
		if(OPERATOR_CHINA_TELECOM == pDevDialParam->mobile_operator
			|| (M_HUAWEI_3G == shareMemParam->iModel ))
		{
			//when current mobile_operator is Telecom,current mode is only 3 kinds, MODE_EVDO，MODE_FDDLTE，MODE_CDMA1X
			if(4 == tmp[3] ||8 == tmp[3])
			{	
				mobile_mode = MODE_EVDO;
				if(0 == tmp[2])
				{
					mobile_reg3gInfo = CREG_STAT_REGISTED_LOCALNET;
				}
				else
				{
					mobile_reg3gInfo = CREG_STAT_REGISTED_ROAMNET;
				}
			}
			else if(9 == tmp[3])
			{
				mobile_mode = MODE_FDDLTE;
				if(0 == tmp[2])
				{
					mobile_reg4gInfo = CREG_STAT_REGISTED_LOCALNET;
				}
				else
				{
					mobile_reg4gInfo = CREG_STAT_REGISTED_ROAMNET;
				}
			}
			else if(2 == tmp[3])
			{
				mobile_mode = MODE_CDMA1X;
				if(0 == tmp[2])
				{
					mobile_reg2gInfo = CREG_STAT_REGISTED_LOCALNET;
				}
				else
				{
					mobile_reg2gInfo = CREG_STAT_REGISTED_ROAMNET;
				}
			}
			else if(5 == tmp[3])
			{
				mobile_mode = MODE_WCDMA;
			}
			else if(3 == tmp[3])
			{
				mobile_mode = MODE_GPRS;
			}
			else;


			mobile_srvStat = tmp[0]; 

			if (255 == tmp[1])
			{
				mobile_srvDomain = 3;
			}
			else
			{
				mobile_srvDomain = tmp[1];
			}
		}
		else
		{
			mobile_simStat = tmp[4];	

			if(1 == tmp[4])
			{
				mobile_simStat = SIM_VALID;
			}
			else if(255 == tmp[4])
			{
				mobile_simStat = SIM_NOTEXIST;
			}
			else  // 已知错误码240 255  后续和CPIN兼容使用
			{
				mobile_simStat = SIM_INVALID;  /* all invalid */
			}
		}
#endif
    //"^SYSINFOEX: 
    #if 0
    	if(!(p = strstr(line, ":")))
			return FALSE;
		sscanf(p, ":%d,%d,%d,%d,,%d,\"%s\"", &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], curr_mode);

		if(M_HUAWEI_4G == shareMemParam->iModel)
		{ 
			mobile_srvStat = tmp[0]; 

			if (255 == tmp[1])
			{
				mobile_srvDomain = 3;
			}
			else
			{
				mobile_srvDomain = tmp[1];
			}
			
			if(1 == tmp[4])
			{
				mobile_mode = MODE_GPRS;
				if(0 == tmp[2])
					mobile_reg2gInfo = CREG_STAT_REGISTED_LOCALNET;
				else
					mobile_reg2gInfo = CREG_STAT_REGISTED_ROAMNET;
			}
			else if(3 == tmp[4])
			{
				mobile_mode = MODE_WCDMA;
				if(0 == tmp[2])
					mobile_reg3gInfo = CREG_STAT_REGISTED_LOCALNET;
				else
					mobile_reg3gInfo = CREG_STAT_REGISTED_ROAMNET;
			}
			else if(4 == tmp[4])
			{
				mobile_mode = MODE_TDSCDMA;
				if(0 == tmp[2])
					mobile_reg3gInfo = CREG_STAT_REGISTED_LOCALNET;
				else
					mobile_reg3gInfo = CREG_STAT_REGISTED_ROAMNET;
			}
			else if(6 == tmp[4])
			{
				if(OPERATOR_CHINA_MOBILE== pDevDialParam->mobile_operator)
					mobile_mode = MODE_TDLTE;
				else if(OPERATOR_CHINA_UNICOM== pDevDialParam->mobile_operator)
					mobile_mode = MODE_FDDLTE;

				if(0 == tmp[2])
					mobile_reg4gInfo = CREG_STAT_REGISTED_LOCALNET;
				else
					mobile_reg4gInfo = CREG_STAT_REGISTED_ROAMNET;
			}
			
			
			if (1 == tmp[3])
			{
				mobile_simStat = SIM_VALID; 
			}
			else if(255 == tmp[3])
			{
				mobile_simStat = SIM_NOTEXIST;  
			}
			else //0  2 3 4 255 
			{
				mobile_simStat = SIM_INVALID;  /* all invalid */
			}
		}
    #endif
    
    //"^CCSQ" "+CSQ" "+CCSQ"
    //^HDRCSQ: ^HRSSILVL: ^CSQLVL: ^RSSILVL:
    //+PSRAT: NONE  LTE FDD  LTE TDD UMTS(TDSCDMA-WCDMA) 
        //HSUPA(WCDMA) HSDPA(WCDMA) TDSCDMA  GPRS EDGE(GPRS)
    // ^HCSQ: WCDMA 
    //+CFUN:
    //+COPS:
    //+CREG:", "+CGREG:", "+CEREG:
    //+CLIP:
    
    
}



struct msf_svc mobile_usb = {
    .init       = usb_init,
    .deinit     = NULL,
    .start      = usb_start,
    .stop       = NULL,
    .set_param  = NULL,
    .get_param  = usb_get_info,
};


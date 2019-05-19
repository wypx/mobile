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
static s32 usb_check_id_support(u32 vendorid, u32 productid);
static s32 usb_check_lsusb(void);
static s32 usb_check_ttyusb(void);
static s32 usb_check_lsmod(void);

static s32 s_mcc = 0;
static s32 s_mnc = 0;

/* trigger change to this with s_state_cond */
static s32 s_closed = 0;

static pthread_mutex_t s_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_state_cond = PTHREAD_COND_INITIALIZER;

static struct usb_info  usbinfo;
static struct usb_info* usb = &usbinfo;

static struct usb_item usbdevs[] = {
    /* LongSung Model */
    { USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_C5300V, "", "C5300V",           MODEM_LONGSUNG,  3, 0, 2,},
    { USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_C7500,  "", "EVDO USB MODEM",   MODEM_LONGSUNG, 3, 0, 2,},
    { USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U7500,  "", "HSPA USB MODEM",   MODEM_LONGSUNG, 1, 2, 0,},
    { USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U8300,  "", "U8300",            MODEM_LONGSUNG,  1, 3, 2,},
    { USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U8300W, "", "U8300W",           MODEM_LONGSUNG,  1, 3, 2,},
    { USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U8300C, "", "U8300C",           MODEM_LONGSUNG,  2, 1, 3,},
    { USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U9300C, "", "U9300C",           MODEM_LONGSUNG,  2, 1, 3,}, 
    { USB_MODEM_IDVENDOR_SANQ,      USB_MODEM_IDPRODUCT_LM9165, "", "LM9165",           MODEM_LONGSUNG,  1, 3, 2,},
    { USB_MODEM_IDVENDOR_SANQ,      USB_MODEM_IDPRODUCT_LM9115, "", "LM9115",           MODEM_LONGSUNG,  2, 3, 2,}, 

    /* Huawei Model */
    { USB_MODEM_IDVENDOR_HUAWEI, USB_MODEM_IDPRODUCT_MU709s_2,	 "", "MU709s-2",    MODEM_HUAWEI,  4, 2, 1,},
    { USB_MODEM_IDVENDOR_HUAWEI, USB_MODEM_IDPRODUCT_ME909u_521, "", "ME909u_521",  MODEM_HUAWEI,  4, 2, 1,},
    //  { USB_MODEM_IDVENDOR_HUAWEI, USB_MODEM_IDPRODUCT_ME909u_523, "", "ME909u_523",  MODEM_HUAWEI,  4, 2, 1,},
    { USB_MODEM_IDVENDOR_HUAWEI, USB_MODEM_IDPRODUCT_ME909s_821, "", "ME909s-821",  MODEM_HUAWEI,  4, 2, 1,},
    { USB_MODEM_IDVENDOR_HUAWEI, USB_MODEM_IDPRODUCT_ME909s_121, "", "ME909s_121",  MODEM_HUAWEI,  4, 2, 1,},
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


struct msf_svc mobile_usb = {
    .init       = usb_init,
    .deinit     = NULL,
    .start      = usb_start,
    .stop       = NULL,
    .set_param  = NULL,
    .get_param  = usb_get_info,
};


static void onATReaderClosed(void);
static void onATTimeout(void);
static void waitForClose(void);
static void onUnsolicited (const s8 *s, const s8 *sms_pdu);

static s32 usb_modem_init(void);
static s32 usb_query_modem_name(void);
static s32 usb_query_sim_status(void);
static s32 usb_query_sim_operator(void);
static s32 usb_query_network_signal(void);
static s32 usb_query_network_mode(void);
static s32 usb_query_network_mode_3gpp(void);
static s32 usb_query_apn_list(void);
static s32 usb_set_ehrpd_close(void);
static s32 usb_query_radio_status(void);
static s32 usb_set_radio_state(enum MOBILE_FUNC cfun);
static s32 usb_search_network(enum NETWORK_SERACH_TYPE s_mode);

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

static s32 usb_get_info(void *data, u32 datalen) {
    if (datalen != sizeof(struct usb_info))
        return -1;
    memcpy(data, usb, sizeof(struct usb_info));
    return 0;
}

static s32 usb_start(void *data, u32 datalen) {

    at_handshake();
    usb_modem_init();
    usb_query_modem_name();
    usb_set_radio_state(CFUN_ONLINE_MODE);
    usb_set_ehrpd_close();
    usb_query_sim_status();
    usb_query_sim_operator();
    usb_query_network_signal();
    usb_search_network(LS_AUTO);

    sleep(2);

    usb_query_network_mode();
    usb_query_network_mode_3gpp();
    usb_query_apn_list();

    // Give initializeCallback a chance to dispatched, since
    // we don't presently have a cancellation mechanism
    sleep(1);
    s_closed = 1;

    waitForClose();

    return 0;
}


static s32 usb_check_id_support(u32 vendorid, u32 productid) {

    for(u32 i = 0; i < _ARRAY_SIZE(usbdevs); i++) {

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


/***************************AT Related Function******************************/
static s32 usb_modem_init(void) {

    s32 rc;
    ATResponse *p_response = NULL;

    /* note: we don't check errors here. Everything important will
       be handled in onATTimeout and onATReaderClosed */

    /*  atchannel is tolerant of echo but it must */
    /*  have verbose result codes */
    at_send_command("ATE1", NULL);

    /*  No auto-answer */
    at_send_command("ATS0=0", NULL);

    /*  Extended errors */
    at_send_command("AT+CMEE=1", NULL);

    /*  set apn autometicly according to curent network operator */
    at_send_command("AT+NVRW=1,50058,\"01\"", NULL);

    //PDP_DEACTIVE
    //at_send_command("AT+CGACT=1,1", NULL);

    /* Auto:All band search*/
    at_send_command("AT+BNDPRF=896,1272,131072", NULL);

    /*  Network registration events */
    rc = at_send_command("AT+CREG=2", &p_response);

    /* some handsets -- in tethered mode -- don't support CREG=2 */
    if (rc < 0 || p_response->success == 0) {
        at_send_command("AT+CREG=1", NULL);
    }

    at_response_free(p_response);

    /*  GPRS registration events */
    at_send_command("AT+CGREG=1", NULL);

    /*	Call Waiting notifications */
    at_send_command("AT+CCWA=1", NULL);

    /*  Alternating voice/data off */
    at_send_command("AT+CMOD=0", NULL);

    /*  Not muted */
    //at_send_command("AT+CMUT=0", NULL);

    /*  +CSSU unsolicited supp service notifications */
    at_send_command("AT+CSSN=0,1", NULL);

    /*  no connected line identification */
    at_send_command("AT+COLP=0", NULL);

    /*  HEX character set */
    //at_send_command("AT+CSCS=\"HEX\"", NULL);

    /*  USSD unsolicited */
    at_send_command("AT+CUSD=1", NULL);

    /*  Enable +CGEV GPRS event notifications, but don't buffer */
    at_send_command("AT+CGEREP=1,0", NULL);

    /*  SMS PDU mode */
    at_send_command("AT+CMGF=0", NULL);

    return 0;
    }

static s32 usb_query_modem_name(void) {
    ATResponse *p_response = NULL;
    s32 rc;
    s8 *line;
    s8 *out;

    /* use ATI or AT+CGMM */
    rc = at_send_command_singleline("ATI", "Model:", &p_response);
    if (rc < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;

    rc= at_tok_start(&line);
    if (rc < 0) goto error;

    rc = at_tok_nextstr(&line, &out);
    if (rc < 0) goto error;

    for(u32 i = 0; i < _ARRAY_SIZE(usbdevs); i++) {
        /*(same) USB ID canot recognise model, AT+CGMM needed for LongSung */
        if (strstr(out, usbdevs[i].modem_name) && 
            usb->item.productid == usbdevs[i].productid &&
            usb->item.vendorid == usbdevs[i].vendorid) {

            memcpy(&(usb->item), &usbdevs[i], sizeof(struct usb_item));
            usb->usb_status = E_TTYUSB_AVAILABLE;
            break;
        }
        usb->usb_status = E_TTYUSB_UNAVAILABLE;
    }

    at_response_free(p_response);
    return 0;

error:
    usb->usb_status = E_TTYUSB_UNAVAILABLE;
    at_response_free(p_response);
    return -1;
}

static s32 usb_query_sim_status(void) {
    ATResponse *p_response = NULL;
    s32 rc;
    s8 *cpinLine = NULL;
    s8 *cpinResult = NULL;

    rc = at_send_command_singleline("AT+CPIN?", "+CPIN:", &p_response);
    if (rc != 0) {
        rc = at_send_command_singleline("AT+QCPIN?", "+QCPIN:", &p_response);
        if (rc != 0) {
            usb->sim_status = SIM_NOT_READY;
            goto done;
        }
    }

    switch (at_get_cme_error(p_response)) {
        case CME_SUCCESS:
            break;

        case CME_SIM_NOT_INSERTED:
            usb->sim_status = SIM_ABSENT;
            goto done;

        default:
            usb->sim_status = SIM_NOT_READY;
            goto done;
    }

    /* CPIN/QCPIN has succeeded, now look at the result */
    cpinLine = p_response->p_intermediates->line;
    rc = at_tok_start (&cpinLine);
    if (rc < 0) {
        usb->sim_status = SIM_NOT_READY;
        goto done;
    }

    rc = at_tok_nextstr(&cpinLine, &cpinResult);

    if (rc < 0) {
        usb->sim_status = SIM_NOT_READY;
        goto done;
    }

    //AT_DBG("sim:%s\n", cpinResult);
    if (0 == msf_strcmp(cpinResult, "SIM PIN")) {
        usb->sim_status = SIM_PIN;
        goto done;
    } else if (0 == msf_strcmp(cpinResult, "SIM PUK")) {
        usb->sim_status = SIM_PUK;
        goto done;
    } else if (0 == msf_strcmp(cpinResult, "PH-NET PIN")) {
        return SIM_NETWORK_PERSONALIZATION;
    } else if (0 != msf_strcmp(cpinResult, "READY"))  {
        /* we're treating unsupported lock types as "sim absent" */
        usb->sim_status = SIM_ABSENT;
        goto done;
    }

    at_response_free(p_response);
    p_response = NULL;
    cpinResult = NULL;

    usb->sim_status = SIM_READY;
    usb->usb_status = E_SIM_COMES_READY;
    at_response_free(p_response);
    return 0;
done:
    usb->usb_status = E_SIM_COMES_LOCKED;
    at_response_free(p_response);
    return -1;
}

static s32 usb_query_sim_operator(void) {
    s32 rc;
    s32 i;
    s32 skip;
    ATLine *p_cur;
    s8* response[3];

    memset(response, 0, sizeof(response));

    ATResponse *p_response = NULL;

    //AT+COPS=3,0;+COPS?;+COPS=3,1;+COPS?;+COPS=3,2;+COPS?;
    //at+cimi 
    //AT+QCIMI
    //err = at_send_command_singleline("AT+CIMI", "+CIMI", &p_response);
    rc = at_send_command_multiline("AT+COPS=3,0;+COPS?;+COPS=3,1;+COPS?;+COPS=3,2;+COPS?", "+COPS:", &p_response);

    //printf("####%s\n", p_response->p_intermediates->line);
    /* we expect 3 lines here:
     * +COPS: 0,0,"T - Mobile"
     * +COPS: 0,1,"TMO"
     * +COPS: 0,2,"310170"
     *  exp:
     *  +COPS: 0,0,"CHN-UNICOM",7
     *  +COPS: 0,1,"UNICOM",7
     *  +COPS: 0,2,"46001",7

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
            ; p_cur = p_cur->p_next, i++) {

        s8 *line = p_cur->line;

        rc = at_tok_start(&line);
        if (rc < 0) goto error;

        rc = at_tok_nextint(&line, &skip);
        if (rc < 0) goto error;

        // If we're unregistered, we may just get
        // a "+COPS: 0" response
        if (!at_tok_hasmore(&line)) {
            response[i] = NULL;
            continue;
        }

        rc = at_tok_nextint(&line, &skip);
        if (rc < 0) goto error;


        // a "+COPS: 0, n" response is also possible
        if (!at_tok_hasmore(&line)) {
            response[i] = NULL;
            continue;
        }

        rc = at_tok_nextstr(&line, &(response[i]));
        if (rc < 0) goto error;

        //AT_DBG("oper:%s\n", response[i]);

        unsigned int	j = 0;
        for(j = 0; j < _ARRAY_SIZE(oper_match); j++) {
            if(strstr(response[i], oper_match[j].cimi)) {
                usb->sim_operator = oper_match[j].operator;
                break;
            }
            usb->sim_operator = CHINA_UNKOWN;
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
    MSF_MOBILE_LOG(DBG_ERROR, "requestOperator must not return error when radio is on\n");
    at_response_free(p_response);
    return -1;
 }

static s32 usb_query_network_signal(void) {
    ATResponse *p_response = NULL;
    s32 rc;
    s8 *line;
    s32 count = 0;
    s32 response = -1;

    //AT+CCSQ
    rc = at_send_command_singleline("AT+CSQ", "+CSQ:", &p_response);
    if (rc < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;

    rc = at_tok_start(&line);
    if (rc < 0) goto error;

    rc = at_tok_nextint(&line, &response);
    if (rc < 0) goto error;

    usb->network_signal = response;

    at_response_free(p_response);
    return 0;

    error:
    at_response_free(p_response);
    return -1;
}

static s32 usb_query_network_mode(void) {

    ATResponse *p_response = NULL;
    s32 rc;
    s8 *line;
    s32 count = 0;
    s8 *response = NULL;

    //AT^HCSQ?  AT+PSRAT AT^SYSINFOEX　AT^SYSINFO
    rc = at_send_command_singleline("AT+PSRAT", "+PSRAT:", &p_response);

    if (rc < 0 || p_response->success == 0) {
        rc = at_send_command_singleline("AT^SYSINFOEX", "^SYSINFOEX:", &p_response);

        if (rc < 0 || p_response->success == 0) {
            goto error;
        }

        s32 tmp[7];
        s8 curr_mode[32];
        memset(curr_mode, 0, sizeof(curr_mode));
        line = p_response->p_intermediates->line;
        s8 *p = strstr(line, ":");
        if( !p ) 
            goto error;

        sscanf(p, ":%d,%d,%d,%d,,%d,\"%s\"", &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], curr_mode);
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
        goto error;
    }

    line = p_response->p_intermediates->line;

    rc = at_tok_start(&line);
    if (rc < 0) goto error;

    rc = at_tok_nextstr(&line, &response);
    if (rc < 0) goto error;

    unsigned int i = 0;
    for(i = 0; i < _ARRAY_SIZE(mode_match); i++) {
        if(strstr(response, mode_match[i].name) 
            && usb->sim_operator == mode_match[i].operator) {
            usb->network_mode = mode_match[i].mode;
            memcpy(usb->item.modem_name, mode_match[i].name, 16);
            break;
        }
        usb->network_mode = MODE_NONE;
    }
    //printf("mode:%s\n", response);

    at_response_free(p_response);
    return 0;

    error:
    //    AT_DBG("requestSignalStrength must never return an error when radio is on\n");

    at_response_free(p_response);
    return -1;
}


static s32 usb_query_network_mode_3gpp(void) {
    s32 rc;
    ATResponse *p_response = NULL;
    s32 response = 0;
    s8*line;

    //AT+COPS=?
    //+COPS: (2,"CHN-UNICOM","UNICOM","46001",7),(3,"CHN-CT","CT","46011",7),(3,"CHINA MOBILE","CMCC","46000",7),,(0,1,2,3,4),(0,1,2)
    //AT+COPS?
    //+COPS: 0,2,"46001",7
    rc = at_send_command_singleline("AT+COPS?", "+COPS:", &p_response);
    if (rc < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;

    rc = at_tok_start(&line);
    if (rc < 0) goto error;

    rc = at_tok_nextint(&line, &response);
    if (rc < 0) goto error;

    at_response_free(p_response);
    return 0;
    error:
    at_response_free(p_response);
    return -1;
}

static s32 usb_query_apn_list(void) {

    ATResponse *p_response;
    ATLine *p_cur;
    s32 rc;
    s32 n = 0;
    s8 *out;

    rc = at_send_command_multiline ("AT+CGDCONT?", "+CGDCONT:", &p_response);
    if (rc != 0 || p_response->success == 0) {

        return -1;
    }

    for (p_cur = p_response->p_intermediates; p_cur != NULL;
         p_cur = p_cur->p_next) {
        char *line = p_cur->line;
        int cid;

        rc = at_tok_start(&line);
        if (rc < 0) goto error;

        rc = at_tok_nextint(&line, &cid);
        if (rc < 0) goto error;


    //printf("cid = %d \n", cid);

    rc = at_tok_nextstr(&line, &out);
    if (rc < 0) goto error;

    //printf("type = %s \n", out);

     // APN ignored for v5
    rc = at_tok_nextstr(&line, &out);
    if (rc < 0) goto error;

    //printf("apn = %s \n", out);

    rc = at_tok_nextstr(&line, &out);
    if (rc < 0) goto error;

    //printf("addresses = %s \n", out);
       

    }

    at_response_free(p_response);
    return 0;

    error:
    at_response_free(p_response);
    return -1;
}


/* EhrpdEnable off needed by pppd*/
static s32 usb_set_ehrpd_close(void) {

    ATResponse *p_response = NULL;
    s32 rc;
    s8 *line = NULL;
    s8 ret = -1;

    rc = at_send_command_singleline("AT+EHRPDENABLE?", "+EHRPDENABLE:", &p_response);

    if (rc < 0 || p_response->success == 0) {
        // assume radio is off
        goto error;
    }

    line = p_response->p_intermediates->line;

    rc = at_tok_start(&line);
    if (rc < 0) goto error;

    rc = at_tok_nextbool(&line, &ret);
    if (rc < 0) goto error;

    at_response_free(p_response);

    if (ret == 1) {
    	rc = at_send_command("AT+EHRPDENABLE=0", &p_response);
    	if (rc < 0 || p_response->success == 0) {
    		goto error;
    	}
    }

    at_response_free(p_response);
    return 0;
    error:

    at_response_free(p_response);
    return -1;

    }

/** returns 1 if on, 0 if off, and -1 on error */
static s32 usb_query_radio_status(void) {
    ATResponse *p_response = NULL;
    s32 rc;
    s8 *line = NULL;
    s8 ret = -1;

    rc = at_send_command_singleline("AT+CFUN?", "+CFUN:", &p_response);
    if (rc < 0 || p_response->success == 0) {
        // assume radio is off
        goto error;
    }

    line = p_response->p_intermediates->line;

    rc = at_tok_start(&line);
    if (rc < 0) goto error;

    rc = at_tok_nextbool(&line, &ret);
    if (rc < 0) goto error;

    at_response_free(p_response);

    return (int)ret;

    error:

    at_response_free(p_response);
    return -1;
}

static s32 usb_set_radio_state(enum MOBILE_FUNC cfun) {
    s32 count = 5;
    s32 rc;
    enum MOBILE_FUNC curr_radio_mode = CFUN_LPM_MODE;
    ATResponse* p_response = NULL;
    s8 at_cmd[64];

    memset(at_cmd, 0, sizeof(at_cmd));

    curr_radio_mode = usb_query_radio_status();
    if(curr_radio_mode == cfun) {
        return 0;
    }

    snprintf(at_cmd, sizeof(at_cmd)-1, "AT+CFUN=%d", cfun);  
    while(count-- < 0) {
        rc = at_send_command(at_cmd, &p_response);
        if (rc < 0 || p_response->success == 0) {
        continue;
        }else {
            curr_radio_mode = usb_query_radio_status();
            if (curr_radio_mode == cfun) 
                break;
        }
    }
    at_response_free(p_response);
    return 0;
    }


static s32 usb_search_network(enum NETWORK_SERACH_TYPE s_mode) {
     if(usb->item.modem_type == MODEM_HUAWEI)
        at_send_command("AT^SYSCFGEX=\"030201\",3FFFFFFF,1,2,7FFFFFFFFFFFFFFF,,", NULL);
     
     if(usb->item.modem_type == MODEM_LONGSUNG)
     at_send_command("AT+MODODREX=2", NULL);

     return 0;
}

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

    //时区报告功能：AT+CTZ
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
          //设置自动应答前振铃次数：ATS0
          //呼叫等待：AT+CCWA

    } else if (at_str_startwith(s,"+CREG:")
                || at_str_startwith(s,"+CGREG:")) {


    } else if (at_str_startwith(s, "+CMT:")) {
    	//短信到达指示: +CMTI
    } else if (at_str_startwith(s, "+CDS:")) {
    	//新短信状态报告到达指示: +CDSI
    } else if (at_str_startwith(s, "+CME ERROR: 150")) {

    } else {

    }
}



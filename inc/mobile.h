/**************************************************************************
*
* Copyright (c) 2017-2018, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
#include <msf_svc.h>
#include <msf_network.h>
#include <msf_serial.h>
#include <msf_file.h>
#include <at_tok.h>
#include <at_channel.h>
#include <at_tok.h>
#include <client.h>

#define MSF_MOD_MOBILE "MOBILE"
#define MSF_MOBILE_LOG(level, ...) \
    msf_log_write(level, MSF_MOD_MOBILE, MSF_FUNC_FILE_LINE, __VA_ARGS__)

enum MOBILE_ERR {
    E_MOBILE_STATE_INIT     = 0,
    E_DRIVER_NOT_INSTALLED  = 1,/* lsmod driver in /proc/moudules not installed */
    E_DRIVER_ID_NOT_FOUND,      /* lsusb show id file not found*/
    E_DRIVER_ID_NOT_SUPPORT,    /* lsusb show id not supported for app*/
    E_TTYUSB_NOT_FOUND ,        /* ttyUSB not exist */
    E_TTYUSB_OPEN_FAIL,         /* ttyUSB AT intf open failed */
    E_TTYUSB_UNAVAILABLE,       /* ttyUSB can be opened, but at(AT+CGMM) is blocked*/
    E_TTYUSB_AVAILABLE,         /* ttyUSB AT intf test successful*/

    E_SIM_NOT_RECOGNOSE,        /* SIM is not supported*/
    E_SIM_NOT_INSERT,           /* SIM is not inerted*/
    E_SIM_NOT_FASTED,           /* SIM is not fasted*/
    E_SIM_COMES_READY,          /* SIM is ready*/
    E_SIM_COMES_LOCKED,         /* SIM need pin or puk */
    E_SIM_OPER_UNKNOWN,         /* SIM operator unknown */
    E_SIM_RECOGNOSED,           /* SIM is ready and operator is known*/
};

enum ECHO_STATE {
    ECHO_CLOSED,
    ECHO_OPEN,
} MSF_PACKED_MEMORY;

enum MOBILE_FUNC {
    /* minimum functionality, dsiable RF but reserve SIM card power supply, 
    *previous mode must be offline */
    CFUN_LPM_MODE          = 0, 
    CFUN_ONLINE_MODE       = 1, /* full functionality (Default) */
    CFUN_OFFLINE_MODE      = 4, /* disable phone both transmit and receive RF circuits */
    CFUN_FTM_MODE          = 5, /* Factory Test Mode */
    CFUN_RESTART_MODE      = 6, /* Reset mode */ 

    /* note:AT+CFUN=6 must be used after setting AT+CFUN=7 
    If module in offline mode, must execute AT+CFUN=6 or restart module to online mode.*/
    CFUN_RADIO_MODE         = 7, /* Offline Mode */
    /* note:If AT+CFUN=0/4 is used after setting AT+CFUN=7, 
    * module will restart to online mode */
};

/* for "^CPIN": PIN & PUK management */
#define CPIN_READY      1
#define CPIN_PIN        2
#define CPIN_PUK        3
#define CPIN_PIN2       4
#define CPIN_PUK2       5

enum SIM_STATE {
    SIM_UNKOWN      = 0,
    SIM_VALID       = 1,
    SIM_INVALID     = 4,
    SIM_ROAM        = 240,
    SIM_NOTEXIST    = 255,
};

enum SIM_Status{
    SIM_ABSENT      = 0,
    SIM_NOT_READY   = 1,
    SIM_READY       = 2, /* SIM_READY means the radio state is RADIO_STATE_SIM_READY */
    SIM_PIN         = 3,
    SIM_PUK         = 4,
    SIM_NETWORK_PERSONALIZATION = 5,
    RUIM_ABSENT     = 6,
    RUIM_NOT_READY  = 7,
    RUIM_READY      = 8,
    RUIM_PIN        = 9,
    RUIM_PUK        = 10,
    RUIM_NETWORK_PERSONALIZATION = 11
} MSF_PACKED_MEMORY;

enum SIM_OPERATOR {
    CHINA_UNKOWN    = 0,
    CHINA_MOBILE    = 1,
    CHINA_UNICOM    = 2,
    CHINA_TELCOM    = 3,
} MSF_PACKED_MEMORY;

/* Huawei for AT^SYSINFOEX, "^SYSCONFIG" (TD) | "^SYSCFG" (WCDMA): 
 * system mode config parameters  
 * LongSung for ^MODODREX or ^MODODR */
enum NETWORK_SERACH_TYPE {
    /* LongSung Modem */
    LS_UTMS_ONLY            = 1, /* Not Support Now */
    LS_AUTO                 = 2, /* LTE > CDMA >HDR >TDS >WCDMA >GSM */
    LS_GSM_ONLY             = 3, /* GSM + CDMA ONLY */
    LS_GSM_PREFERED         = 4, /* TDS Pre (TDS > GSM > LTE >WCDMA >HDR >CDMA) */
    LS_LTE_ONLY             = 5, /* LTE ONLY */
    LS_TDSCDMA_ONLY         = 6, /* TDSCDMA ONLY */
    LS_TDSCDMA_WCDMA        = 7, /* TDCMDA_WCDMA */
    LS_TDSCDMA_GSM_WCDMA    = 8, /* TDCMDA_GSM_WCDMA */
    LS_TDSCDMA_WCDMA_LTE    = 9, /* TDCMDA_WCDMA_LTE */
    LS_HDR_ONLY             = 10,/* HDR ONLY */
    LS_LTE_PREFERED         = 11,/* LTE Pre (LTE > HDR >CDMA >TDS >WCDMA >GSM) */
    LS_HDR_PREFERED         = 12,/* HDR Pre (HDR >CDMA >LTE >TDS >WCDMA > GSM)*/
    LS_HDR_LTE              = 13,/* HDR and LTE */
    LS_HDR_CDMA             = 14,/* CDMA and HDR*/
    LS_CDMA_ONLY            = 15,/* CDMA and HDR*/

    /* Huawei Modem */
    HW_AUTO         = 2,   /* Automatic */
    HW_CDMA         = 3,   /* CDMA (not supported currently) */
    HW_HDR          = 4,   /* HDR (not supported currently) */
    HW_HYBRID       = 5,   /* CDMA/HDR HYBRID (not supported currently) */
    HW_CDMA1X       = 11,
    HW_EVDO         = 12,
    HW_GSM          = 13,
    HW_WCDMA        = 14,
    HW_TDSCDMA      = 15,
    HW_NOCHANGE     = 16,
    HW_TDLTE        = 17,
    HW_FDDLTE       = 18,
};

enum NETWORK_MODE {
    MODE_NONE       = 0,    /*NO SERVICE*/
    MODE_CDMA1X     = 2,    /*CDMA*/
    MODE_GPRS       = 3,    /*GSM/GPRS*/
    MODE_EVDO       = 4,    /*HDR(EVDO)*/
    MODE_WCDMA      = 5,    /*WCDMA*/
    MODE_GSWCDMA    = 7,    /*GSM/WCDMA*/
    MODE_HYBRID     = 8,    /*HYBRID(CDMA and EVDO mixed)*/
    MODE_TDLTE      = 9,    /*TDLTE*/
    MODE_FDDLTE     = 10,   /*FDDLTE(2)*/
    MODE_TDSCDMA    = 15    /*TD-SCDMA*/
};

/* service status*/ 
enum SRV_STATUS {
    SRV_NO_SERVICE  = 0,
    SRV_RESTRICTED_SERVICE,
    SRV_VALID_SERVICE,
    SRV_RESTRICTED_REGIONAL_SERVICE,
    SRV_POWER_SAVING_HIBERATE_SERVICE,
};

/* service domain*/
enum SRV_DOMAIN {
    SRV_DOMAIN_NONE,
    SRV_DOMAIN_CS,
    SRV_DOMAIN_PS,
    SRV_DOMAIN_CS_PS,
    SRV_DOMAIN_CS_PS_SEARCH,
};

enum PDP_STATUS {
    PDP_DEACTIVE,
    PDP_ACTIVE,
};

enum DIAL_USB_TYPE {
    DIAL_TTY_USB,
    DIAL_TTY_ACM,
};
enum USB_MODEM {
    MODEM_UNKOWN,
    MODEM_LONGSUNG,
    MODEM_HUAWEI,
    MODEM_SIMCOM,
    MODEM_NODECOM,
    MODEM_NOEWAY,
};

struct usb_item {
    u32 vendorid;
    u32 productid;
    u8  manufact[32];
    s8  modem_name[16]; /* CGMR */
    u8  modem_type;
    u8  at_port;    /* port id not more than 255*/
    u8  modem_port;
    u8  debug_port;
} MSF_PACKED_MEMORY ;

struct usb_info {
    u8   usb_status;
    u8   usb_type;
    s32  usb_fd;
    struct usb_item item;

    u8   sim_status;
    u8   sim_operator;
    u8   network_signal;
    u8   network_mode;
    u8   network_search_mode;
    u8   network_mode_str[32];
} MSF_PACKED_MEMORY;

s8 *mobile_error_request(enum MOBILE_ERR err);
s8 *mobile_modem_str(enum USB_MODEM modem);
s8 *mobile_operator_str(enum SIM_OPERATOR oper);

/******************* DIAL **************************/
enum DIAL_TYPE {
    DIAL_AUTO   = 0,
    DIAL_MANUAL = 1,
};

enum AUTH_TYPE {
    AUTH_NONE,
    AUTH_PAP,
    AUTH_CHAP,
    AUTH_PAP_CHAP,
};

enum DIAL_STATE {
    DIAL_INIT,
    DIAL_EXECUTING,
    DIAL_FAILED,
    DIAL_SUCCESSFUL,
};

struct dial_info {
    u8      dial_enable;
    u8      dial_type;  /* auto: 1  manual:2 */

    /* dial config */
    time_t  start_time;
    time_t  stop_time;
    u16     mtu;
    u8      protocal;
    u8      epname;

    enum DIAL_STATE stat;
    pid_t   ppp_pid;
    s8      ip_addr[MAX_IPADDR_LEN];
    s8      gate_way[MAX_IPADDR_LEN];
    s8      net_mask[MAX_IPADDR_LEN];
    s8      dns_primary[MAX_IPADDR_LEN];
    s8      dns_second[MAX_IPADDR_LEN];
    s8      test_domain[MAX_IPADDR_LEN];
    u8      test_result;
} MSF_PACKED_MEMORY;

/******************* SMS **************************/
struct sms_item {
    struct list_head sms_head;
    s8 peer_phone_num[16];
    s8 msg[256];
} MSF_PACKED_MEMORY;

struct sms_queue {
    u32 sms_num;
    struct list_head sms_list;
} MSF_PACKED_MEMORY;

struct mobile {
    pid_t pid;
    u8 verbose;
    u8 sms_enable;
    u8 backuping;
    u8 upgrading;

    struct dial_info dial;
    
    u8 sim_stat;
    u8 sim_operator;
    u8 sim_number[32];

    u8 networdk_search_mode;
    u8 network_mode;
    u8 network_signal;
    u8 register_stat;

    u8 reserved_2[9];
    u8 service_stat;
    u8 service_domain;
    u8 modem_type;
    u8 modem_id[32];
    u8 modem_name[32];

    s8 apn[32]; /* eg. "public.vpdn.hz" */
    u8 reserved_3[62];
} MSF_PACKED_MEMORY;


enum mobile_mod_idx {
    MOBILE_MOD_USB,
    MOBILE_MOD_DIAL,
    MOBILE_MOD_SMS,
    MOBILE_MOD_MAX,
};

extern struct msf_svc  mobile_usb;
extern struct msf_svc  mobile_dial;
extern struct msf_svc  mobile_sms;
extern struct msf_svc* mobile_module[];


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

#define DEFAULT_MTU     1400
#define MAX_AT_RESPONSE 0x1000
#define PPP_INTERFACE   "ppp0"

static s32 sFD;     /* file desc of AT channel */
static s8 sATBuffer[MAX_AT_RESPONSE+1];
static s8 *sATBufferCur = NULL;


static s32 s_lac = 0;
static s32 s_cid = 0;

static s32 dial_init(void* data, u32 datalen);
static s32 dial_get_info(void* data, u32 datalen);
static s32 dial_start(void* data, u32 datalen);

struct msf_svc mobile_dial = {
    .init       = dial_init,
    .deinit     = NULL,
    .start      = dial_start,
    .stop       = NULL,
    .set_param   = NULL,
    .get_param   = dial_get_info,
};

static struct dial_info    dialinfo;
static struct dial_info*   dial = &dialinfo;
static struct usb_info     usb;

typedef struct __attribute__((__packed__)) {
    u32 local_oper;
    u32 local_mode;
    s8  local_number[32];
    s8  local_user[32];
    s8  local_pass[32];
    s8  local_apn[32];
} local_dial_param;


static local_dial_param dialparam[] = {
 { CHINA_TELCOM, MODE_EVDO,   "#777", 	"card", "card", ""     },   /* EVDO mode */
 { CHINA_UNICOM, MODE_WCDMA,  "*99#", 	"user", "user", "3gnet"},   /* WCDMA mode */
 { CHINA_MOBILE, MODE_TDSCDMA,"*98*1#", "user", "user", "cmnet"},   /* TDSCDMA mode */
 { CHINA_MOBILE, MODE_TDLTE,  "*98*1#",	"card", "card", "cmnet"},   /* TDLTE mode */
 { CHINA_TELCOM, MODE_FDDLTE, "*99#", 	"user", "user", "ctlte"},   /* FDDLTE mode */
 { CHINA_UNICOM, MODE_FDDLTE, "*98*1#", "user", "user", "cuinet"},  /* FDDLTE mode */

 { CHINA_MOBILE, MODE_TDLTE,  "*98*1#",	"card", "card", "cmwap"},   /* TDLTE mode */
 { CHINA_MOBILE, MODE_TDLTE,  "*98*1#",	"card", "card", "cmmtm"},   /* TDLTE mode */

 { CHINA_TELCOM, MODE_EVDO,   "#777", 	"ctnet@mycdma.cn", "vnet.mobi", ""},/*evdo mode */
 { CHINA_TELCOM, MODE_FDDLTE, "*98*1#", "user", "user", "ctm2m"},   /* FDDLTE mode */
 { CHINA_TELCOM, MODE_FDDLTE, "*98*1#",	"user", "user", "ctnet"},   /* FDDLTE mode */
 { CHINA_TELCOM, MODE_FDDLTE, "*98*1#",	"user", "user", "ctwap"},   /* FDDLTE mode */

 
 { CHINA_UNICOM, MODE_FDDLTE, "*98*1#", "user", "user", "uninet"},  /* FDDLTE mode */
 { CHINA_UNICOM, MODE_FDDLTE, "*98*1#",	"user", "user", "uniwap"},  /* FDDLTE mode */
 { CHINA_UNICOM, MODE_FDDLTE, "*98*1#",	"user", "user", "wonet" },  /* FDDLTE mode */

};

void dial_param_init(void) {
    memset(dial, 0, sizeof(struct dial_info));
    dial->dial_enable = MSF_TRUE;
    dial->dial_type = DIAL_AUTO;
    dial->epname = DIAL_TTY_USB;
    dial->mtu = 1460;
    dial->protocal = AUTH_PAP_CHAP;
    memcpy(dial->test_domain, "luotang.me", strlen("luotang.me"));
    dial->stat = DIAL_INIT;
}

static s32 dial_init(void *data, u32 datalen) {
    if (!data || datalen != sizeof(struct usb_info))
        return -1;

    memset(&usb, 0, sizeof(struct usb_info));
    memcpy(&usb,  data, sizeof(struct usb_info));

    dial_param_init();

    return 0;
}

static s32 dial_get_info(void *data, u32 datalen) {
    if (!data || datalen != sizeof(struct dial_info))
        return -1;

    s32 count = 20;
    s8  ip[32];
    s32 rc = -1;
    do {
        memset(dial->ip_addr, 0, sizeof(dial->ip_addr));
        rc = get_ipaddr_by_intf(PPP_INTERFACE, dial->ip_addr, sizeof(dial->ip_addr));
        if (0 == rc) {
            MSF_MOBILE_LOG(DBG_DEBUG, "pppd dial ok\n");
            dial->stat = 1;
            break;
        }
        sleep(2);
        dial->stat = 0;
        count--;
    } while(count > 0);

    memcpy((struct dial_info*)data, dial, sizeof(struct dial_info));

    return 0;
}

static s32 dial_start(void *data, u32 datalen) {

    dial->ppp_pid = -1;

    s32 i = 0;
    s8  *argv[56];
    s8  chat_script[512] = {0};
    s8  chat_disconnect[512] = {0};
    s8  port_str[16];

    memset(port_str, 0, sizeof(port_str));

    u32 j;
    for (j = 0; j < _ARRAY_SIZE(dialparam); j++) {
        if (usb.sim_operator == dialparam[j].local_oper && 
            usb.network_mode == dialparam[j].local_mode) {
            break;
        }
    }

    sprintf(chat_script,
        "chat -v  TIMEOUT 15 ABORT BUSY  ABORT 'NO ANSWER' '' ATH0 OK AT 'OK-+++\\c-OK' ATDT%s CONNECT ",
        dialparam[j].local_number);

        sprintf(chat_disconnect,
        "chat -v ABORT 'BUSY' ABORT 'ERROR' ABORT 'NO DIALTONE' '' '\\K' '' '+++ATH' SAY '\nPDP context detached\n'");


    argv[i++] = "pppd";
    argv[i++] = "modem";
    argv[i++] = "crtscts";
    argv[i++] = "debug";
    argv[i++] = "nodetach";

    snprintf(port_str, sizeof(port_str) -1, 
        "/dev/ttyUSB%d", 
        usb.item.modem_port);

    argv[i++] =   port_str;

    argv[i++] = "115200";
    argv[i++] = "connect";
    argv[i++] = chat_script;
    argv[i++] = "noipdefault";
    argv[i++] = "default-asyncmap";
    argv[i++] = "lcp-max-configure";
    argv[i++] = "30";
    argv[i++] = "defaultroute";
    argv[i++] = "hide-password";//show-password	
    argv[i++] = "nodeflate";
    argv[i++] = "nopcomp";
    argv[i++] = "novj";
    //argv[i++] = "noccp";
    argv[i++] = "novjccomp";
    //ipcp-accept-local
    //ipcp-accept-remote

    argv[i++] = "mtu";
    argv[i++] = "1400";

    /*
    //CHAP_PROTOCAL
    argv[i++] = "refuse-pap";
    argv[i++] = "refuse-eap";
    argv[i++] = "auth";
    argv[i++] = "require-chap";
    argv[i++] = "user";
    argv[i++] =  "user";
    argv[i++] = "password";
    argv[i++] =  "user";
    argv[i++] = "noauth";

    //PAP_PROTOCOL
    argv[i++] = "refuse-chap";
    argv[i++] = "refuse-eap";
    argv[i++] = "auth";
    argv[i++] = "require-pap";
    argv[i++] = "user";
    argv[i++] =  "user";
    argv[i++] = "password";
    argv[i++] = "user";
    argv[i++] = "noauth";*/

    argv[i++] = "noauth";
    argv[i++] = "user";
    argv[i++] = dialparam[j].local_user;
    argv[i++] = "password";
    argv[i++] =  dialparam[j].local_pass;


    argv[i++] = "usepeerdns";
    argv[i++] = "lcp-echo-interval";
    argv[i++] = "60";
    argv[i++] = "lcp-echo-failure";
    argv[i++] = "3";
    argv[i++] = "maxfail";
    argv[i++] = "0";
    argv[i++] = "disconnect";
    argv[i++] = chat_disconnect;
    argv[i++] = (s8  *)0;

    if ((dial->ppp_pid = vfork()) < 0) {

        MSF_MOBILE_LOG(DBG_ERROR, "Can not fork, exit now.");
        return -1;
    }

    if (dial->ppp_pid == 0) {
        (void)execvp("pppd", argv);
    } else {
        return dial->ppp_pid; /* pid pppd*/
    }

    return 0;
}


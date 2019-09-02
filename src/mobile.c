/**************************************************************************
*
* Copyright (c) 2017-2019, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
/*
 *  ┏┓   ┏┓
 * ┏┛┻━━━┛┻┓
 * ┃       ┃
 * ┃ ━　　　┃
 * ┃┳┛  ┗┳ ┃
 * ┃       ┃
 * ┃   ┻   ┃
 * ┃       ┃
 * ┗━┓   ┏━┛
 *   ┃   ┃   NO 996!
 *   ┃   ┃   NO BUG!
 *   ┃   ┗━━━┓
 *   ┃       ┣┓
 *   ┃       ┏┛
 *   ┗┓┓┏━┳┓┏┛
 *    ┃┫┫ ┃┫┫
 *    ┗┻┛ ┗┻┛
 */
#include <mobile.h>

static s8 *g_errmsg_map[] = {
    "MOBILE_STATE_INIT",        /* Mobile init state*/
    "DRIVER_NOT_INSTALLED",     /* lsmod driver in /proc/moudules not installed */
    "DRIVER_ID_NOT_FOUND",      /* lsusb show id file not found*/
    "DRIVER_ID_NOT_SUPPORT",    /* lsusb show id not supported for app*/
    "TTYUSB_NOT_FOUND",         /* ttyUSB not exist */
    "TTYUSB_OPEN_FAIL",         /* ttyUSB AT intf open failed */
    "TTYUSB_UNAVAILABLE",       /* ttyUSB can be opened, but at(AT+CGMM) is blocked*/
    "TTYUSB_AVAILABLE",         /* ttyUSB AT intf test successful*/

    "SIM_NOT_RECOGNOSE",        /* SIM is not supported*/
    "SIM_NOT_INSERT",           /* SIM is not inerted*/
    "SIM_NOT_FASTED",           /* SIM is not fasted*/
    "SIM_COMES_READY",          /* SIM is ready*/
    "SIM_COMES_LOCKED",         /* SIM need pin or puk */
    "SIM_OPER_UNKNOWN",         /* SIM operator unknown */
    "SIM_RECOGNOSED",           /* SIM is ready and operator is known*/

};
 s8 *mobile_error_request(enum MOBILE_ERR err) {
    return g_errmsg_map[err];
}

static void mobile_show_version(void) {
    MSF_MOBILE_LOG(DBG_DEBUG, BOLD YELLOW"#####################################################");
    MSF_MOBILE_LOG(DBG_DEBUG, BOLD RED   "# Name    : Mobile Process                          #");
    MSF_MOBILE_LOG(DBG_DEBUG, BOLD GREEN "# Author  : luotang.me                              #");
    MSF_MOBILE_LOG(DBG_DEBUG, BOLD BROWN "# Version : 1.1                                     #");
    MSF_MOBILE_LOG(DBG_DEBUG, BOLD BLUE  "# Time    : 2018-01-27 - 2019-05-03                 #");
    MSF_MOBILE_LOG(DBG_DEBUG, BOLD PURPLE"# Content : Hardware - SIM - Dial - SMS             #");
    MSF_MOBILE_LOG(DBG_DEBUG, BOLD GRAY  "#####################################################");
}

s32 mobile_req_scb(s8 *data, u32 len, u32 cmd) {
    MSF_MOBILE_LOG(DBG_INFO, "Mobile seq service callback data(%p) len(%d) cmd(%d).",
                       data, len, cmd);
    return 0;
}

s32 mobile_ack_scb(s8 *data, u32 len, u32 cmd)
{
    MSF_MOBILE_LOG(DBG_INFO, "Mobile ack service callback data(%p) len(%d) cmd(%d).",
                       data, len, cmd);

    return 0;
}

s32 sig_pppd_exit(void *lparam)
{

    s32 status;
    s32 exitNo = 0;
    pid_t *pid = (pid_t*)lparam;
    //AT_ERR("4G net_valid: %d\n", net_valid());

    if (*pid > 0) {
        kill(*pid, SIGTERM);//SIGTERM  SIGKILL
    }

    *pid = waitpid(*pid, &status, 0);
    if (*pid != -1) {
        MSF_MOBILE_LOG(DBG_DEBUG, "Process %d exit.", (s32)*pid);
    }

    if (WIFEXITED(status)){
        exitNo = WEXITSTATUS(status);
        MSF_MOBILE_LOG(DBG_DEBUG, "Normal termination, exit status(%d).", exitNo);
    }
    return 0;
}

static s32 pppid = 0;
void sig_handler(s32 sig) {

    if (SIGINT == sig || SIGKILL == sig  || SIGSEGV == sig) {
        sig_pppd_exit(&pppid);
        MSF_MOBILE_LOG(DBG_DEBUG, "pppd exit.");
        exit(0);
        
    }
}

#define server_host "192.168.58.132"
#define SERVER_PORT "9999"

s32 main(s32 argc, s8 **argv) {

    s32 rc = -1;
    struct client_param param;
    memset(&param, 0, sizeof(param));
    param.name = MSF_MOD_MOBILE;
    param.cid = RPC_MOBILE_ID;
    param.host = LOCAL_HOST_V4;
    param.port = SERVER_PORT;
    param.req_scb = mobile_req_scb;
    param.ack_scb = mobile_ack_scb;

    rc = client_agent_init(&param);
    if (rc < 0) {
        MSF_MOBILE_LOG(DBG_DEBUG, "IPC agent init faild, try later.");
        //return -1;
    }
   
    mobile_show_version();

    //signal(SIGINT,  sig_handler);
    ///signal(SIGKILL, sig_handler);
    //signal(SIGSEGV, sig_handler);

    struct usb_info usb;
    struct dial_info dial;

    memset(&dial, 0, sizeof(struct dial_info));
    memset(&usb,  0, sizeof(struct usb_info));

    rc = mobile_module[MOBILE_MOD_USB]->init(NULL, 0);
    mobile_module[MOBILE_MOD_USB]->get_param(&usb, sizeof(usb));
    if (rc < 0) {
        MSF_MOBILE_LOG(DBG_DEBUG, "Usb status: %s.", 
                    mobile_error_request(usb.usb_status));
        return -1;
    }
    rc = mobile_module[MOBILE_MOD_USB]->start(NULL, 0);
    mobile_module[MOBILE_MOD_USB]->get_param(&usb, sizeof(usb));

    MSF_MOBILE_LOG(DBG_DEBUG, "##################  HardWare Info  ######################");
    MSF_MOBILE_LOG(DBG_DEBUG, "AT PORT       : %d",   usb.item.at_port);
    MSF_MOBILE_LOG(DBG_DEBUG, "MEDEM PORT    : %d",   usb.item.modem_port);
    MSF_MOBILE_LOG(DBG_DEBUG, "DEBUG PORT    : %d",   usb.item.debug_port);
    MSF_MOBILE_LOG(DBG_DEBUG, "VENDDOR ID    : 0x%x", usb.item.vendorid);
    MSF_MOBILE_LOG(DBG_DEBUG, "PRODUCT ID    : 0x%x", usb.item.productid);
    MSF_MOBILE_LOG(DBG_DEBUG, "USB MODEM     : %s",   usb.item.modem_name);
    MSF_MOBILE_LOG(DBG_DEBUG, "USB IMODEL    : %s",   mobile_modem_str(usb.item.modem_type));
    MSF_MOBILE_LOG(DBG_DEBUG, "RESULT        : %s",   mobile_error_request(usb.usb_status));

    MSF_MOBILE_LOG(DBG_DEBUG, "##################  SIM INFO ########################");
    MSF_MOBILE_LOG(DBG_DEBUG, "AT TEST: %s",      mobile_error_request(usb.usb_status));
    MSF_MOBILE_LOG(DBG_DEBUG, "SIM    : %s",      mobile_operator_str(usb.sim_operator));
    MSF_MOBILE_LOG(DBG_DEBUG, "CSQ    : %d",      usb.network_signal);
    MSF_MOBILE_LOG(DBG_DEBUG, "MODE   : %s",    usb.network_mode_str);

    if (usb.usb_status != E_TTYUSB_AVAILABLE ||
        usb.usb_status != E_SIM_RECOGNOSED)
        return -1;

    mobile_module[MOBILE_MOD_DIAL]->init(&usb, sizeof(usb));
    mobile_module[MOBILE_MOD_DIAL]->start(NULL, 0);
    mobile_module[MOBILE_MOD_DIAL]->get_param(&dial, sizeof(dial));

    MSF_MOBILE_LOG(DBG_DEBUG, "##################  4G DIALING  #####################");
    MSF_MOBILE_LOG(DBG_DEBUG, "RESULT   : %d",  dial.stat);
    MSF_MOBILE_LOG(DBG_DEBUG, "IP       : %s",  dial.ip_addr);
    MSF_MOBILE_LOG(DBG_DEBUG, "DNS1     : %s",  dial.dns_primary);
    MSF_MOBILE_LOG(DBG_DEBUG, "DNS2     : %s",  dial.dns_second);
    MSF_MOBILE_LOG(DBG_DEBUG, "GateWay  : %s",  dial.gate_way);

    while(1) {
        mobile_module[MOBILE_MOD_DIAL]->get_param(&dial, sizeof(dial));
        if(dial.stat)
            break;
        else
            sleep(2);
    }

    pppid = dial.ppp_pid;
    MSF_MOBILE_LOG(DBG_DEBUG, "##################  4G NET TEST #######################");
    MSF_MOBILE_LOG(DBG_DEBUG, "TEST RESULT  : %d",  dial.test_result);
    MSF_MOBILE_LOG(DBG_DEBUG, "TEST DOMAIN  : %s",  dial.test_domain);

    MSF_MOBILE_LOG(DBG_DEBUG, "##################  TEST OVER  ########################");

    while (1) 
      sleep(2);

    client_agent_deinit();

    return 0;
}




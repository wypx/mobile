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

static s8 *g_request_errmsg_map[] = {
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
    return g_request_errmsg_map[err];
}

/**
 * Call from RIL to us to make a RIL_REQUEST
 *
 * Must be completed with a call to RIL_onRequestComplete()
 *
 * RIL_onRequestComplete() may be called from any thread, before or after
 * this function returns.
 *
 * Will always be called from the same thread, so returning here implies
 * that the radio is ready to process another command (whether or not
 * the previous command has completed).
 */
static void onRequest (s32 request, void *data, u32  datalen) {

}
static void mobile_show_info(void) {
    MSF_MOBILE_LOG(DBG_DEBUG, "#####################################################");
    MSF_MOBILE_LOG(DBG_DEBUG, "# Name    : Mobile Process                          #");
    MSF_MOBILE_LOG(DBG_DEBUG, "# Author  : luotang.me                              #");
    MSF_MOBILE_LOG(DBG_DEBUG, "# Version : 1.1                                     #");
    MSF_MOBILE_LOG(DBG_DEBUG, "# Time    : 2018-01-27 - 2019-05-03                 #");
    MSF_MOBILE_LOG(DBG_DEBUG, "# Content : Hardware - SIM - Dial - NetPing         #");
    MSF_MOBILE_LOG(DBG_DEBUG, "#####################################################");
}

s32 sig_pppd_exit(void *lparam) {

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

s32 main(s32 argc, s8 **argv) {

    log_init("logger/MOBILE.log");
    
    mobile_show_info();

    //signal(SIGINT,  sig_handler);
    ///signal(SIGKILL, sig_handler);
    //signal(SIGSEGV, sig_handler);

    struct usb_info         usb;
    struct dial_info        dial;

    memset(&dial, 0, sizeof(struct dial_info));
    memset(&usb,  0, sizeof(struct usb_info));
    
    mobile_module[MOBILE_MOD_USB]->init(NULL, 0);
    mobile_module[MOBILE_MOD_USB]->start(NULL, 0);
    mobile_module[MOBILE_MOD_USB]->get_param(&usb, sizeof(usb));
    //if(usb.usb_status != E_USB_TTY_EXIST)
    //  return -1;

    MSF_MOBILE_LOG(DBG_DEBUG, "##################  HardWare Info  ######################");
    MSF_MOBILE_LOG(DBG_DEBUG, "AT PORT       : %d\n",   usb.item.at_port);
    MSF_MOBILE_LOG(DBG_DEBUG, "MEDEM PORT    : %d\n",   usb.item.modem_port);
    MSF_MOBILE_LOG(DBG_DEBUG, "DEBUG PORT    : %d\n",   usb.item.debug_port);
    MSF_MOBILE_LOG(DBG_DEBUG, "VENDDOR ID    : 0x%x\n", usb.item.vendorid);
    MSF_MOBILE_LOG(DBG_DEBUG, "PRODUCT ID    : 0x%x\n", usb.item.productid);
    MSF_MOBILE_LOG(DBG_DEBUG, "USB MODEM     : %s\n",   usb.item.modem_name);
    MSF_MOBILE_LOG(DBG_DEBUG, "USB IMODEL    : %s\n",   mobile_modem_str(usb.item.modem_type));
    MSF_MOBILE_LOG(DBG_DEBUG, "RESULT        : %s\n",   mobile_error_request(usb.usb_status));

    //if(usb.usb_status != E_USB_TTY_AVAILABLE)
    //  return -1;

    MSF_MOBILE_LOG(DBG_DEBUG, "\n##################  SIM INFO ########################\n");
    MSF_MOBILE_LOG(DBG_DEBUG, "AT TEST: %s\n",  mobile_error_request(usb.usb_status));
    MSF_MOBILE_LOG(DBG_DEBUG, "SIM    : %s\n",  mobile_operator_str(usb.sim_operator));
    MSF_MOBILE_LOG(DBG_DEBUG, "CSQ    : %d\n",  usb.network_signal);
    MSF_MOBILE_LOG(DBG_DEBUG, "MODE   : %s\n\n",    usb.network_mode_str);

    if(usb.usb_status != E_TTYUSB_AVAILABLE)
        return -1;

    mobile_module[MOBILE_MOD_DIAL]->init(&usb, sizeof(usb)); //check net
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

    return 0;
}




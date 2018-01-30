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
#include <sys/wait.h>

#include "atmod.h"
#include "atrunner.h"
#include "misc.h"
#include "atusb.h"
#include "atdial.h"
#include "at.h"
#include "aterr.h"
#include "atnet.h"


//
/*** Callback methods from the RIL library to us ***/

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
static void onRequest (int request, void *data, unsigned int  datalen)
{
	
}
static void PrtVersion(void)
{
    printf("#########################################################\n");
	printf("#  Name	    :  Mobile Process                           #\n");
	printf("#  Author   :  luotang.me                               #\n");
	printf("#  Version  :  1.0                                      #\n");
	printf("#  Time     :  2018-01-27                               #\n");
	printf("#  Content  :  Hardware - SIM - Dial - NetPing          #\n");
	printf("#########################################################\n");
}

int sig_pppd_exit(void* lparam) {

	int status;
	int exitNo = 0;
	pid_t* pid = lparam;
	//AT_ERR("4G net_valid: %d\n", net_valid());

	if(*pid > 0)
		kill(*pid, SIGTERM);//SIGTERM  SIGKILL

	*pid = waitpid(*pid, &status, 0);
	if (*pid != -1) {
		AT_ERR("process %d exit\n", (int)*pid);
	}

	if (WIFEXITED(status)){
		exitNo = WEXITSTATUS(status);
		AT_ERR("normal termination, exit status =%d\n", exitNo);
	}
	return 0;
}

static int pppid = 0;
void sig_handler(int sig) {

	if (SIGINT == sig 
	 || SIGKILL == sig 
	 || SIGSEGV == sig) {
		 sig_pppd_exit(&pppid);
         exit(0);
    }
}

int main (int argc, char **argv)
{
	PrtVersion();

	signal(SIGINT,  sig_handler);
	signal(SIGKILL, sig_handler);
	signal(SIGSEGV, sig_handler);

    struct usb_info 	usb;
	struct at_info 		at;
	struct dial_info 	dial;
	struct net_info 	net;

	memset(&net,  0, sizeof(struct net_info));
	memset(&dial, 0, sizeof(struct dial_info));
	memset(&at,   0, sizeof(struct at_info));
	memset(&usb,  0, sizeof(struct usb_info));

	printf("\n##################  CHECK INFO  ########################\n");
	
	usb_module[MOD_USBDEV_DETECT]->_init(NULL, 0);
	usb_module[MOD_USBDEV_DETECT]->_get_info(&usb, sizeof(usb));
	if(usb.ustat != E_USB_TTY_EXIST)
		return -1;
	
	usb_module[MOD_USBDEV_AT]->_init(&usb, sizeof(usb));
	usb_module[MOD_USBDEV_AT]->_get_info(&at, sizeof(at));
	if (at.stat != E_AT_OPEN_SUCC)
		return -1;
	
	/* Test AT and AT+CGMM */
	usb_module[MOD_USBDEV_DETECT]->_start(NULL, 0);
	usb_module[MOD_USBDEV_DETECT]->_get_info(&usb, sizeof(usb));

	printf("\n##################  HardWare Info  ######################\n");
	printf("AT PORT       : %d\n", 		usb.match.atport);
	printf("MEDEM PORT    : %d\n", 		usb.match.modemport);
	printf("DEBUG PORT    : %d\n", 		usb.match.debugport);
	printf("VENDDOR ID    : 0x%x\n", 	usb.match.vendorid);
	printf("PRODUCT ID    : 0x%x\n", 	usb.match.productid);
	printf("USB MODEM     : %s\n", 		usb.match.usbname);
	printf("USB IMODEL    : %s\n", 		model_request(usb.match.imodel));
	printf("RESULT        : %s\n", 		error_request(usb.ustat));

	if(usb.ustat != E_USB_TTY_AVAILABLE)
		return -1;


	usb_module[MOD_USBDEV_AT]->_start(NULL, 0);
	usb_module[MOD_USBDEV_AT]->_get_info(&at, sizeof(at));

	printf("\n##################  SIM INFO ########################\n");
	printf("AT TEST: %s\n", 	error_request(at.stat));
	printf("SIM    : %s\n", 	oper_request(at.oper));
	printf("CSQ    : %d\n", 	at.csq);
	printf("MODE   : %s\n\n", 	at.mode_name);
	
	if(at.stat != E_USB_HARDWARE_OK)
		return -1;

	usb_module[MOD_USBDEV_DIAL]->_init(&at, sizeof(at)); //check net
	usb_module[MOD_USBDEV_DIAL]->_start(&usb, sizeof(usb));
	usb_module[MOD_USBDEV_DIAL]->_get_info(&dial, sizeof(dial));
	
	printf("\n##################  4G DIALING  #####################\n");
	printf("RESULT    : %d\n", 		dial.stat);
	printf("IP    	  : %s\n", 		dial.ip);
	printf("DNS1      : %s\n", 		dial.dns1);
	printf("DNS2      : %s\n", 		dial.dns2);
	printf("GateWay   : %s\n\n", 	dial.gateway);

	while(1) {
		usb_module[MOD_USBDEV_DIAL]->_get_info(&dial, sizeof(dial));
		if(dial.stat)
			break;
		else
			sleep(2);
	}

	pppid = dial.ppid;
	usb_module[MOD_USBDEV_NET]->_init(NULL, 0);
	usb_module[MOD_USBDEV_NET]->_start(NULL, 0);
	usb_module[MOD_USBDEV_NET]->_get_info(&net, sizeof(struct net_info));
	
	printf("\n##################  4G NET TEST #######################\n");
	printf("TEST RESULT  : %d\n", 		net.result);
	printf("TEST DNS     : %s\n", 		net.test_dns1);
	printf("TEST DOMAIN  : %s\n", 		net.test_domain);


	printf("\n##################  TEST OVER  #########################\n\n");
	
	while(1) {
		sleep(2);
	}
    return 0;
}




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

#include "atmod.h"
#include "atdial.h"
#include "misc.h"
#include <string.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include "at.h"
#include "atusb.h"


#define MAX_AT_RESPONSE 0x1000


static int sFD;     /* file desc of AT channel */
static char sATBuffer[MAX_AT_RESPONSE+1];
static char *sATBufferCur = NULL;


static int s_lac = 0;
static int s_cid = 0;


static 	int dial_init(void* data, unsigned int datalen);
static 	int dial_get_info(void* data, unsigned int datalen);
static 	int dial_start(void* data, unsigned int datalen);

static int get_ipaddr(char* if_name, char* ip, int len);


struct usbev_module usbdev_dial = {
	._init 		= dial_init,
	._deinit 	= NULL,
	._start 	= dial_start,
	._stop 		= NULL,
	._set_info 	= NULL,
	._get_info 	= dial_get_info,
};

static struct dial_info 	dialinfo;
static struct dial_info* 	dial = &dialinfo;

static struct at_info 		at;
static struct usb_info 		usb;

static 	int dial_init(void* data, unsigned int datalen) {
	//mainLoop(data);
	if(!data || datalen != sizeof(struct at_info))
		return -1;

	memset(&at, 0, sizeof(struct at_info));
	memcpy(&at,  data, sizeof(struct at_info));
	
	return 0;
}

static 	int	dial_get_info(void* data, unsigned int datalen) {
	if(datalen != sizeof(struct dial_info))
			return -1;

	#define PPP_IF	"ppp0"

	int		count = 20;
	char	ip[16];
	int		ret = -1;
	do {
		memset(ip, 0, sizeof(ip));
		ret = get_ipaddr(PPP_IF, ip, sizeof(ip));
		if(0 == ret) {
			//AT_DBG("pppd dial ok\n");
			dial->stat = 1;
			break;
		}
		sleep(2);
		dial->stat = 0;
		count--;
	}while(count > 0);
	
	memcpy(dial->ip, ip, sizeof(ip));
	memcpy((struct dial_info*)data, dial, sizeof(struct dial_info));
	
	return 0;
}



#define MIN(a,b)		((a) > (b) ? (b) : (a))

static int get_ipaddr(char* if_name, char* ip, int len) {


	int s = -1;
	struct ifreq ifr;
	struct sockaddr_in* ptr = NULL;
	struct in_addr addr_temp;

	if(len < 16) {
		AT_DBG("The ip need 16 byte !\n");
		return -1;
	}

	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}

	strncpy(ifr.ifr_name, if_name, MIN(16, strlen(if_name)));
	
	if(ioctl(s, SIOCGIFADDR, &ifr) < 0) {
		//AT_DBG("get_ipaddr ioctl errno=%d \n", errno);
		close(s);
		return -1;
	}
	ptr = (struct sockaddr_in *) & ifr.ifr_ifru.ifru_netmask;
	addr_temp = ptr->sin_addr;
	snprintf(ip, len, "%s", inet_ntoa(addr_temp/*ptr->sin_addr*/));
	close(s);
	return 0;
}
	

static 	int	dial_start(void* data, unsigned int datalen) {

	if(!data || datalen != sizeof(struct usb_info))
		return -1;
	memcpy(&usb, data, sizeof(struct usb_info));
	dial->ppid = -1;

	int 	i = 0;
	char* 	argv[56];
	char 	modem_chat_script[512] = {0};
	char 	modem_chat_disconnect[512] = {0};
	char* 	modem_ppp_telephone = NULL;
	char	port_str[16];

	memset(port_str, 0, sizeof(port_str));


	if(at.oper == CHINA_UNICOM)
		modem_ppp_telephone =  FDDLTE_UNICOM_DEF_DIALNUM;	
	else if(at.oper == CHINA_MOBILE)
		modem_ppp_telephone =  TDLTE_DEF_DIALNUM;	
	else
		modem_ppp_telephone = FDDLTE_TELECOM_DEF_DIALNUM;


	sprintf(modem_chat_script,
		"chat -v  TIMEOUT 15 ABORT BUSY  ABORT 'NO ANSWER' '' ATH0 OK AT 'OK-+++\\c-OK' ATDT%s CONNECT ",
		modem_ppp_telephone);

   	sprintf(modem_chat_disconnect,
		"chat -v ABORT 'BUSY' ABORT 'ERROR' ABORT 'NO DIALTONE' '' '\\K' '' '+++ATH' SAY '\nPDP context detached\n'");


	argv[i++] = "pppd";
	argv[i++] = "modem";	
	argv[i++] = "crtscts";	
	argv[i++] = "debug";	
	argv[i++] = "nodetach";	

	/*
	if(at->mode == USB_HUAWEI)
		modem_ppp_telephone =  FDDLTE_UNICOM_DEF_DIALNUM;	
	else if(at->mode== USB_LONGSUNG)
		modem_ppp_telephone =  TDLTE_DEF_DIALNUM;	*/

	snprintf(port_str, sizeof(port_str) -1, 
		"/dev/ttyUSB%d", 
		usb.match.modemport);
	//argv[i++] = "/dev/ttyUSB4";		// 9607模块的拨号节点是 ttyUSB4	
	//argv[i++] = "/dev/ttyUSB2";		// 9603模块的拨号节点是 ttyUSB2		
	//argv[i++] = "/dev/ttyUSB2";		// U8300模块的拨号节点是 ttyUSB3	
	//argv[i++] = "/dev/ttyUSB2";		// HUAWEI模块的拨号节点是 ttyUSB2	
	argv[i++] =   port_str;

	argv[i++] = "115200";	
	argv[i++] = "connect";	
	argv[i++] = modem_chat_script;		
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
	argv[i++] = "noauth";	*/

	argv[i++] = "noauth";	
	argv[i++] = "user";		

	if(at.oper == CHINA_UNICOM)
		argv[i++] =  FDDLTE_UNICOM_DFT_NAME;	
	else if(at.oper == CHINA_MOBILE)
		argv[i++] =  TDLTE_DFT_NAME;	
	else
		argv[i++] =  FDDLTE_TELECOM_DFT_NAME;
	
	argv[i++] = "password";		
	if(at.oper == CHINA_UNICOM)
		argv[i++] =  FDDLTE_UNICOM_DFT_PASSWD;	
	else if(at.oper == CHINA_MOBILE)
		argv[i++] =  TDLTE_DFT_PASSWD;	
	else
		argv[i++] =  FDDLTE_TELECOM_DFT_PASSWD;

	argv[i++] = "usepeerdns";	
	argv[i++] = "lcp-echo-interval";	
	argv[i++] = "60";	
	argv[i++] = "lcp-echo-failure";	
	argv[i++] = "3";	
	argv[i++] = "maxfail";	
	argv[i++] = "0";	
	argv[i++] = "disconnect";	
	argv[i++] = modem_chat_disconnect;	
	argv[i++] = (char *)0;


	/*起一个新的进程来用于拨号进程*/
	if((dial->ppid = vfork()) < 0) 
	{
		AT_DBG("Can not fork, exit now\n");
		return -1;
	}
	if( dial->ppid == 0)
	{
		(void)execvp("pppd", argv);
	}
	else
	{
		return dial->ppid; /* pid pppd*/
	}
	
	return 0;
}





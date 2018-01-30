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
#include "atusb.h"
#include "atchannel.h"
#include "at_tok.h"
#include "misc.h"
#include "aterr.h"

static int usb_init(void* data, unsigned int datalen);
static int usb_start(void* data, unsigned int datalen);
static int usb_get_info(void* data, unsigned int datalen);

static int usb_check_id_support(unsigned int vendorid, unsigned int productid);
static int usb_check_lsusb(void);
static int usb_check_ttyusb(void);
static int usb_check_lsmod(void);
static int requestATest(void);
static int requestModelName(void);


static char* _request_model_map[] = {
	"USB_UNKNOWN",
	"USB_LONGSUNG",
	"USB_HUAWEI",
	"USB_SIMCOM",
	"USB_NODECOM",
	"USB_NEOWAY",
};

char *model_request(usbdev_model request_model){
	return _request_model_map[request_model];
}

static struct usb_info  usbinfo;
static struct usb_info* usb = &usbinfo;

struct usbdev_match usbdevs[] = {
	/* LongSung Model */
	{ USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_C5300V, "", "C5300V", 	 	 USB_LONGSUNG,  3, 0, 2,},
	{ USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_C7500, 	"", "EVDO USB MODEM", USB_LONGSUNG, 3, 0, 2,},
	{ USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U7500, 	"", "HSPA USB MODEM", USB_LONGSUNG, 1, 2, 0,},
	{ USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U8300, 	"", "U8300",  		 USB_LONGSUNG,  1, 3, 2,},
	{ USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U8300W, "", "U8300W", 		 USB_LONGSUNG,  1, 3, 2,},
	{ USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U8300C,	"", "U8300C", 		 USB_LONGSUNG,  2, 1, 3,},
	{ USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U9300C,	"", "U9300C", 		 USB_LONGSUNG,  2, 1, 3,}, 
	{ USB_MODEM_IDVENDOR_SANQ, 		USB_MODEM_IDPRODUCT_LM9165,	"", "LM9165", 		 USB_LONGSUNG,  1, 3, 2,},
	{ USB_MODEM_IDVENDOR_SANQ, 		USB_MODEM_IDPRODUCT_LM9115,	"", "LM9115", 		 USB_LONGSUNG,  2, 3, 2,}, 
	/* U9300C 手动测试发现 2,3可用  */

	/* Huawei Model */
	{ USB_MODEM_IDVENDOR_HUAWEI, USB_MODEM_IDPRODUCT_MU709s_2,	 "", "MU709s-2",   	USB_HUAWEI,  4, 2, 1,},
	{ USB_MODEM_IDVENDOR_HUAWEI, USB_MODEM_IDPRODUCT_ME909u_521, "", "ME909u_521", 	USB_HUAWEI,  4, 2, 1,},
//	{ USB_MODEM_IDVENDOR_HUAWEI, USB_MODEM_IDPRODUCT_ME909u_523, "", "ME909u_523", 	USB_HUAWEI,  4, 2, 1,},
	{ USB_MODEM_IDVENDOR_HUAWEI, USB_MODEM_IDPRODUCT_ME909s_821, "", "ME909s-821", 	USB_HUAWEI,  4, 2, 1,},
	{ USB_MODEM_IDVENDOR_HUAWEI, USB_MODEM_IDPRODUCT_ME909s_121, "", "ME909s_121", 	USB_HUAWEI,  4, 2, 1,},
};


struct usbev_module usbdev_detect = {
	._init 		= usb_init,
	._deinit 	= NULL,
	._start 	= usb_start,
	._stop 		= NULL,
	._set_info 	= NULL,
	._get_info 	= usb_get_info,
};

static 	int usb_init(void* data, unsigned int datalen) {
	memset(usb, 0, sizeof(struct usb_info));
	usb->ustat = -1;
	usb->fd = -1;
	usb->match.imodel = USB_UNKNOWN;

	do {
		if (E_USB_ID_APP_SUPPORT != usb_check_lsusb())
			break;
		
		usb->ustat = E_USB_ID_FOUND;

		if (E_USB_DRIVER_INSTALLED !=  usb_check_lsmod())
			break;
		
		if (E_USB_TTY_EXIST != usb_check_ttyusb())
			break;
	} while(0);

	return 0;
}

static 	int	usb_get_info(void* data, unsigned int datalen) {
	if(datalen != sizeof(struct usb_info))
		return -1;
	memcpy((struct usb_info*)data, usb, sizeof(struct usb_info));
	return 0;
}
static 	int usb_start(void* data, unsigned int datalen) {

	requestATest();
	requestModelName();
	return 0;
}

static int usb_check_id_support(unsigned int vendorid, unsigned int productid) {

	unsigned int i = 0;
	for(i = 0; i < _ARRAY_SIZE(usbdevs); i++) {
	
		if(productid == usbdevs[i].productid 
			&& vendorid == usbdevs[i].vendorid) {
			memcpy(&(usb->match), &usbdevs[i], sizeof(struct usbdev_match));
			usb->ustat = E_USB_ID_APP_SUPPORT;
			break;
		}
		usb->ustat = E_USB_ID_APP_NOT_SUPPORT;
	}

	return usb->ustat;
}


static int usb_check_lsusb(void) {

	char 	line[256];
	FILE* 	fp = NULL;
	char* 	p = NULL;

	fp = popen("lsusb", "r");
	if (!fp) {
	  usb->ustat = E_USB_ID_FILE_NOT_FOUND;
	  return usb->ustat;
	}

	unsigned int productid = 0;
	unsigned int vendorid = 0;

	while (!feof(fp)) { 
		memset(line,	 0, sizeof(line));

		fgets(line, sizeof(line), fp);
		
		if((p = strstr(line, "ID"))) {
			sscanf(p, "ID %x:%x", &vendorid, &productid); 
			
			//productid = strtol(prodid_str, NULL, 16);
			//vendorid  = strtol(vendorid_str, NULL, 16);
			if(E_USB_ID_APP_SUPPORT != usb_check_id_support(vendorid, productid)) {
				continue;
			} else {
				break;
			}
		}
	}
   
    pclose(fp);
	
	return usb->ustat;
}

static int usb_check_ttyusb(void) {
	int 		ret = -1;
	char 		ttyPath[16];
	
	memset(ttyPath, 0, sizeof(ttyPath));
	snprintf(ttyPath, sizeof(ttyPath)-1, tty_usb_path, usb->match.modemport);

	/* ls /dev ==>  /dev/ttyUSB# */
	ret = check_file_exist(ttyPath);
	if(ret) {
		/* Drivers have been installed, but ttyUSB not exist,
		   that is the drivers not support current productid */
		usb->ustat = E_USB_ID_BSP_SUPPORT;
		usb->ustat = E_USB_TTY_EXIST;
	}else {
		usb->ustat = E_USB_ID_BSP_NOT_SUPPORT;
		usb->ustat = E_USB_TTY_NOT_EXIST;
	}
	return usb->ustat;
}

/* first check ttyUSB whther exist, if not 
   then check the drivers whther are installed: 
   usb_wwan.ko, usbserial.ko and option.ko */
static 	int	usb_check_lsmod(void) {

	/*lsmod ==> cat /proc/modules */
	int 		ret = -1;
	
	FILE*	 	fp = NULL; 
	char 	 	line[128];
	int			drivers = 0;

	ret = check_file_exist(usb_driver_path);
	if(ret) {
		do {
			fp = fopen(usb_driver_path, "r");
			if(NULL == fp) {
				usb->ustat = E_USB_DRIVER_FILE_OPEN_FAIL;
				break;
			}
			while (!feof(fp)) {	
				memset(line, 0, sizeof(line));
				fgets(line,  sizeof(line), fp); 
				
				if(strstr(line, "option") || 
				   strstr(line, "usb_wwan") || 
				   strstr(line, "usbserial")) {
					drivers++;
					if(3 == drivers)
						break;
				} else {
					continue;
				}
			}
		}while(0);
	}else {
		usb->ustat = E_USB_DRIVER_FILE_NOT_FOUND;
	}
	if(3 == drivers) {  
		usb->ustat = E_USB_DRIVER_INSTALLED;
	}else {
		/* Drivers have not been installed completely or No drivers have been installed */
		usb->ustat = E_USB_DRIVER_NOT_INSTALLED;
	}
	sfclose(fp);
	return usb->ustat;
}

static int requestATest(void) {

	ATResponse *p_response = NULL;
    int err = -1;
    char *line;
    char *out;

	//at+cgmm
    err = at_send_command_singleline("AT", "AT", &p_response);

    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;

//	AT_DBG("line:%s\n", line);

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextstr(&line, &out);
    if (err < 0) goto error;

	//AT_DBG("at_query:%s\n", out);

	if(strstr(out, "OK"))
		usb->ustat = E_AT_TEST_OK;
	else
		usb->ustat = E_AT_TEST_ERR;
	
    at_response_free(p_response);
    return 0;

error:
   // AT_DBG("requestSignalStrength must never return an error when radio is on");
    at_response_free(p_response);

	return -1;

}


static int requestModelName(void)
{
	ATResponse *p_response = NULL;
    int err = -1;
    char *line;
    char *out;
	unsigned int i = 0;

	//at+cgmm
    err = at_send_command_singleline("ATI", "Model:", &p_response);

    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;

//	AT_DBG("line:%s\n", line);

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextstr(&line, &out);
    if (err < 0) goto error;

	//AT_DBG("model:%s\n", out);
	
	for(i = 0; i < _ARRAY_SIZE(usbdevs); i++) {
		/*(same) USB ID canot recognise model, AT+CGMM needed for LongSung */
		if(strstr(out, usbdevs[i].usbname) && 
			usb->match.productid == usbdevs[i].productid &&
			usb->match.vendorid == usbdevs[i].vendorid) {					
			memcpy(&(usb->match), &usbdevs[i], sizeof(struct usbdev_match));
			usb->ustat = E_USB_TTY_AVAILABLE;
			break;
		}
		usb->ustat = E_USB_TTY_UNAVAILABLE;
	}	

    at_response_free(p_response);
    return 0;

error:
   // AT_DBG("requestSignalStrength must never return an error when radio is on");
    usb->ustat = E_USB_TTY_UNAVAILABLE;
    at_response_free(p_response);
	return -1;
}




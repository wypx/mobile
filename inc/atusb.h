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


#include <stdio.h>

/* 龙旗 */
#define USB_MODEM_IDVENDOR_LONGCHEER 0x1c9e

/* 龙尚 */
#define USB_MODEM_IDVENDOR_LONGSUNG USB_MODEM_IDVENDOR_LONGCHEER
#define USB_MODEM_IDPRODUCT_C5300 	0x9e00
#define USB_MODEM_IDPRODUCT_C5300V 	0x9e00
#define USB_MODEM_IDPRODUCT_C7500 	0x1c9e	
#define USB_MODEM_IDPRODUCT_LS 		0x9607	//other

#define USB_MODEM_IDPRODUCT_U6300 	0x9603
#define USB_MODEM_IDPRODUCT_U7500 	0x9603
#define USB_MODEM_IDPRODUCT_U8300 	0x9b05  //8300,LM91XX
#define USB_MODEM_IDPRODUCT_U8301 	0x9b05
#define USB_MODEM_IDPRODUCT_U8300W 	0x9b05
#define USB_MODEM_IDPRODUCT_U8300C 	0x9b05
#define USB_MODEM_IDPRODUCT_U9300C 	0x9b3c

/* 三旗 */
#define USB_MODEM_IDVENDOR_SANQ USB_MODEM_IDVENDOR_LONGCHEER
#define USB_MODEM_IDPRODUCT_LM9105 	0x9b05
#define USB_MODEM_IDPRODUCT_LM9115 	0x9b05
#define USB_MODEM_IDPRODUCT_LM9165 	0x9b05
#define USB_MODEM_IDPRODUCT_LM9265 	0x9b05
#define USB_MODEM_IDPRODUCT_LM9206 	0x9b05

/* 华为 */
#define USB_MODEM_IDVENDOR_HUAWEI 	0x12d1
#define USB_MODEM_IDPRODUCT_EM560 	0x1d09
#define USB_MODEM_IDPRODUCT_EM660 	0x1001
#define USB_MODEM_IDPRODUCT_EM660C 	0x1001
#define USB_MODEM_IDPRODUCT_EM770 	0x1001
#define USB_MODEM_IDPRODUCT_EM770W 	0x1404
#define USB_MODEM_IDPRODUCT_MC509 	0x1404
#define USB_MODEM_IDPRODUCT_MC509_V2 	0x1404
#define USB_MODEM_IDPRODUCT_MU609_1506 	0x1506
#define USB_MODEM_IDPRODUCT_MU609_1573 	0x1573
#define USB_MODEM_IDPRODUCT_MU709s_2 	0x1c25
#define USB_MODEM_IDPRODUCT_E261 		0x140c
#define USB_MODEM_IDPRODUCT_ME909u_521 	0x1573
#define USB_MODEM_IDPRODUCT_ME909u_523 	0x1573
#define USB_MODEM_IDPRODUCT_ME909s_821 	0x15c1 
#define USB_MODEM_IDPRODUCT_ME909s_121 	0x15c1
#define USB_MODEM_IDPRODUCT_EM820W 		0x1404

/* 中兴 */
#define USB_MODEM_IDVENDOR_ZTE 		0x19d2
#define USB_MODEM_IDPRODUCT_ME3760 	0x0199
#define USB_MODEM_IDPRODUCT_MC2716 	0xffed

/* QUANTA */
#define USB_MODEM_IDVENDOR_QUANTA 		0x0408
#define USB_MODEM_IDPRODUCT_MobileGenie 0xea47

/* 鼎桥 */
#define USB_MODEM_IDVENDOR_TDTECH 	0x12d1
#define USB_MODEM_IDPRODUCT_EM350 	0x1506

/* 国民技术 */
#define USB_MODEM_IDVENDOR_GUOMIN 	0x258d
#define USB_MODEM_IDPRODUCT_NM304 	0x1101
#define USB_MODEM_IDPRODUCT_NM304B 	0x2000

/* QUALCOMM */
#define USB_MODEM_IDVENDOR_QUALCOMM 0x05c6

/* QUECTEL */
#define USB_MODEM_IDVENDOR_QUECTEL USB_MODEM_IDVENDOR_QUALCOMM
#define USB_MODEM_IDPRODUCT_UC20 	0x9003
#define USB_MODEM_IDPRODUCT_EC20C 	0x9215
#define USB_MODEM_IDPRODUCT_EC20CE 	0x9215

/* QUECTEL_V2 */
#define USB_MODEM_IDVENDOR_QUECTEL_V2 	0x2c7c
#define USB_MODEM_IDPRODUCT_EC20CEF 	0x0125

/* REMO */
#define USB_MODEM_IDVENDOR_REMO USB_MODEM_IDVENDOR_QUALCOMM
#define USB_MODEM_IDPRODUCT_R1523 		0x9025

/* SIERRA */
#define USB_MODEM_IDVENDOR_SIERRA 		0x1199
#define USB_MODEM_IDPRODUCT_MC7354 		0x68a2
#define USB_MODEM_IDPRODUCT_MC7354_68C0 0x68c0

/* CINTERION */
#define USB_MODEM_IDVENDOR_CINTERION 	0x1e2d
#define USB_MODEM_IDPRODUCT_PLS8_E 		0x0060

/* SIMCOM */
#define USB_MODEM_IDVENDOR_SIMCOM 		0x1e0e
#define USB_MODEM_IDPRODUCT_SIM7100C 	0x9001
#define USB_MODEM_IDPRODUCT_SIM7100CE 	0x9001

/* YUGE */
#define USB_MODEM_IDVENDOR_YUGE 		0x257a
#define USB_MODEM_IDVENDOR_YUGE_QUALCOMM USB_MODEM_IDVENDOR_QUALCOMM
#define USB_MODEM_IDPRODUCT_CLM910 		0x9025
#define USB_MODEM_IDPRODUCT_CLM920 		0x9025
#define USB_MODEM_IDPRODUCT_CLM920_SV 	0x9025

/* FORGE */
#define USB_MODEM_IDVENDOR_FORGE USB_MODEM_IDVENDOR_QUALCOMM
#define USB_MODEM_IDPRODUCT_SLM630B 	0x9025

/* INNOFIDEI */
#define USB_MODEM_IDVENDOR_INNOFIDEI 	0x1d53
#define USB_MODEM_IDPRODUCT_E7B01_04_C 	0x3d53
#define USB_MODEM_IDPRODUCT_E5530 		0x3d53


#define tty_usb_path 	"/dev/ttyUSB%d"
#define usb_driver_path "/proc/modules"



/* USB modem tty设备类型 */
typedef enum {
    USB_TTY_USB = 0,
    USB_TTY_ACM	= 1,
}usbdev_type;

typedef enum {
	USB_UNKNOWN 	= 0,
    USB_LONGSUNG 	= 1,
    USB_HUAWEI		= 2,
    USB_SIMCOM		= 3,
    USB_NODECOM		= 4,
 	USB_NEOWAY		= 5,
}usbdev_model;

struct usbdev_match {
	unsigned int 	vendorid;  		 /* 厂商ID */
	unsigned int 	productid;		 /* 产品ID */
	char 			manufacturer[16];/* 厂商信息 */

	char 			usbname[16];	 /* CGMR */
	int	 			imodel;
	
	int	  			atport;			 /* AT指令接口号 */
	int	  			modemport;		 /* 数据接口号 */
	int	  			debugport;		 /* 调试口或者诊断口接口号 */
};

struct usb_info {
	struct usbdev_match match;
		
	int	 			mode_support;
	int	 			net_support;
	int	 			netswitch_support;
	usbdev_type		utype;
	int	 			ustat;		/* 搜索结果 */
	int				fd;
	
	char 			res[32];
};

char *model_request(usbdev_model request_model);




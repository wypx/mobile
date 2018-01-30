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


typedef enum {
	//step 1  
	E_USB_TTY_EXIST			= 0,
	E_USB_TTY_NOT_EXIST,			//ttyUSB not exist- E_USB_DRIVER_NOT_INSTALLED/E_USB_DRIVER_NOT_SUPPORT (BSP)
	E_USB_TTY_UNAVAILABLE,			//ttyUSB resource cannot be used   (Hardware)
	E_USB_TTY_AVAILABLE,			//Test AT+CGMM command ok

	//step 2
	E_USB_DRIVER_FILE_FOUND = 4,	//File Path proc/moudules exist
	E_USB_DRIVER_FILE_NOT_FOUND,
	E_USB_DRIVER_FILE_OPEN_FAIL,
	E_USB_DRIVER_INSTALLED,
	E_USB_DRIVER_NOT_INSTALLED,		//USB driver has not been installed	(BSP) 

	//step 3
	E_USB_ID_FILE_FOUND		= 9,
	E_USB_ID_FILE_NOT_FOUND,		//ID FILE not found				(BSP)
	E_USB_ID_FILE_OEPN_FAIL,		//Open id file failed				(APP)

	//step 4
	E_USB_ID_FOUND			= 12,	//USB ID can be found				(APP)
	E_USB_ID_NOT_FOUND, 			//USB ID cannot be found			(APP)
	E_USB_DEV_NOT_POWER_ON ,		//USB DEV has not power on 		(Hardware)
	E_USB_ID_APP_SUPPORT,			//USB ID is supported yet			(APP)
	E_USB_ID_APP_NOT_SUPPORT,		//USB ID not supported yet			(APP)
	E_USB_ID_BSP_SUPPORT,			//USB ID is supported yet			(BSP)
	E_USB_ID_BSP_NOT_SUPPORT,		//USB ID not supported yet			(BSP)

	//step6
	E_AT_OPEN_SUCC			= 19,
	E_AT_OPEN_FAIL,
	
	//step 7
	E_AT_TEST_OK			= 21,
	E_AT_TEST_ERR,
	
	//step 8
	E_USB_HARDWARE_OK		= 23,
	
	
	//step 9
	E_USB_BAD_WORKING		= 25,	//USB bad working,canot recognised sim card(hardware)
	E_SIM_NOT_INSERT,				//SIM card not insert to the dev
	E_SIM_NOT_FASTED,				//SIM card be in  poor contact with dev 
	E_SIM_BAD,						//SIM card is bad enough,cannot be recognised
	E_SIM_READY,					//SIM is ready ,ok
	E_SIM_LOCKED,					//SIM need pin or puk

	E_SIM_OPER_UNKNOWN		= 30,
	E_SIM_OPER_KNOWN,

	E_REG_NONE				= 32,
	E_REG_ERR,
	E_REG_SUCC,
	
}usbdev_error;

char *error_request(usbdev_error request_stat);



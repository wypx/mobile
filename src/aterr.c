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

 #include "aterr.h"
 
 static char* _request_errmsg_map[] = {
	 "E_USB_TTY_EXIST",
	 "E_USB_TTY_NOT_EXIST / E_USB_ID_BSP_SUPPORT",
	 "E_USB_TTY_UNAVAILABLE",
	 "E_USB_TTY_AVAILABLE",
	 
	 "E_USB_DRIVER_FILE_FOUND",
	 "E_USB_DRIVER_FILE_NOT_FOUND",
	 "E_USB_DRIVER_FILE_OPEN_FAIL",
	 "E_USB_DRIVER_INSTALLED",
	 "E_USB_DRIVER_NOT_INSTALLED",
 
	 "E_USB_ID_FILE_FOUND",
	 "E_USB_ID_FILE_NOT_FOUND",
	 "E_USB_ID_FILE_OEPN_FAIL",
	 
	 "E_USB_ID_FOUND",
	 "E_USB_ID_NOT_FOUND / E_USB_DEV_NOT_POWER_ON",
	 "E_USB_ID_NOT_FOUND / E_USB_DEV_NOT_POWER_ON",
	 "E_USB_ID_APP_SUPPORT",
	 "E_USB_ID_APP_NOT_SUPPORT",
	 "E_USB_TTY_NOT_EXIST / E_USB_ID_BSP_SUPPORT",
	 "E_USB_ID_BSP_NOT_SUPPORT",

	"E_AT_OPEN_SUCC",
	"E_AT_OPEN_FAIL",
	
	"E_AT_TEST_OK",
	"E_AT_TEST_ERR",
	
	"E_USB_HARDWARE_OK / SIM READY",
	
	 "E_RES"
 };
 
 static char* _request_errmsg_map_zh[] = {
	 "ttyUSB口存在",
	 "ttyUSB口不存在 / 内核不支持改USB ID",
	 "ttyUSB口不可用(测试AT, AT+CGMM不通过)",
	 "ttyUSB口可用(测试AT, AT+CGMM通过)",
	 
	 "驱动文件找到",
	 "驱动文件未找到",
	 "驱动文件打开失败",
	 "驱动已经安装",
	 "驱动未安装",
 
	 "USB ID文件找到",
	 "USB ID文件未找到",
	 "USB ID文件打开失败",
	 
	 "USB ID找到",
	 "USB ID未找到 / USB 未上电",
	 "USB ID未找到 / USB 未上电",
	 "USB 应用支持",
	 "USB 应用不支持",
	 "USB口不存在 / 内核不支持该 USB ID",
	 "内核不支持该 USB ID",

	"AT 口打开成功",
	"AT 口打开失败",
	
	"AT 测试指令成功",
	"AT 测试指令失败",
	
	"4G 模块硬件目测OK / 识别SIM卡成功",
	
	 "其他"
 };
 
 
 char *error_request(usbdev_error request_stat)
 {
	 return _request_errmsg_map[request_stat];
 }



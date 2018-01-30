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
#include <string.h>
#include <unistd.h>

typedef int (*usb_dev_cb)(void* data, unsigned int datalen);

struct usbev_module {
	usb_dev_cb 	_init;
	usb_dev_cb	_start;
	usb_dev_cb	_stop;
	usb_dev_cb	_set_info;
	usb_dev_cb  _get_info;
	usb_dev_cb	_deinit;
};

typedef enum {
	MOD_USBDEV_DETECT =	0,
	MOD_USBDEV_AT,
	MOD_USBDEV_DIAL,
	MOD_USBDEV_NET,
	MOD_USBDEV_SMS,
	MOD_USBDEV_MAX,
}usbdev_mod_idx;

extern struct usbev_module  usbdev_detect;
extern struct usbev_module  usbdev_at;
extern struct usbev_module  usbdev_dial;
extern struct usbev_module  usbdev_net;
extern struct usbev_module  usbdev_sms;

extern struct usbev_module* usb_module[];


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


struct usbev_module* usb_module[] = {
	&usbdev_detect,
	&usbdev_at,
	&usbdev_dial,
	&usbdev_net,
	&usbdev_sms,
	NULL,
};

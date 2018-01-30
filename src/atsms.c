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


static int sms_init(void* data, unsigned int datalen);
static int sms_deinit(void* data, unsigned int datalen);
static int sms_start(void* data, unsigned int datalen);
static int sms_stop(void* data, unsigned int datalen);
static int sms_set_info(void* data, unsigned int datalen);
static int sms_get_info(void* data, unsigned int datalen);
static int sms_test(void* data, unsigned int datalen);



struct usbev_module usbdev_sms = {
	._init 		= sms_init,
	._deinit 	= sms_deinit,
	._start 	= sms_start,
	._stop 		= sms_stop,
	._set_info 	= sms_set_info,
	._get_info 	= sms_get_info,
};


static int sms_init(void* data, unsigned int datalen) {
	return 0;
}
static int sms_deinit(void* data, unsigned int datalen) {
	return 0;
}
static int sms_start(void* data, unsigned int datalen) {
	return 0;
}
static int sms_stop(void* data, unsigned int datalen) {
	return 0;
}
static int sms_set_info(void* data, unsigned int datalen) {
	return 0;
}
static int sms_get_info(void* data, unsigned int datalen) {
	return 0;
}
static int sms_test(void* data, unsigned int datalen) {
	return 0;
}




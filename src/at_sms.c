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

enum SMS_MODE {
    SMS_PDU,
    SMS_TEXT,
};

struct sms {
    s8 sms_center[16];
    s8 phone_list[8];
    s8 operator_resp;
};

static s32 sms_init(void *data, u32 datalen);
static s32 sms_deinit(void *data, u32 datalen);
static s32 sms_start(void *data, u32 datalen);
static s32 sms_stop(void *data, u32 datalen);
static s32 sms_set_info(void *data, u32 datalen);
static s32 sms_get_info(void *data, u32 datalen);

struct msf_svc mobile_sms = {
    .init           = sms_init,
    .deinit         = sms_deinit,
    .start          = sms_start,
    .stop           = sms_stop,
    .set_param      = sms_set_info,
    .get_param      = sms_get_info,
};


static s32 sms_init(void* data, u32 datalen) {
    return 0;
}
static s32 sms_deinit(void *data, u32 datalen) {
    return 0;
}
static s32 sms_start(void *data, u32 datalen) {
    return 0;
}
static s32 sms_stop(void *data, u32 datalen) {
    return 0;
}
static s32 sms_set_info(void *data, u32 datalen) {
    return 0;
}
static s32 sms_get_info(void *data, u32 datalen) {
    return 0;
}

/**************************************************************************
*
* Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
#ifndef __MSF_AT_ERRNO_H__
#define __MSF_AT_ERRNO_H__

#include <iostream>

namespace MSF {
namespace MOBILE {

enum MobileErrno 
{
    MOBILE_E_SUCCESS                    = 0,
    MOBILE_E_DRIVER_NOT_INSTALLED       = 1,    /* Driver lsmod in /proc/moudules not installed */
    MOBILE_E_DRIVER_ID_NOT_FOUND        = 2,    /* Driver lsusb show id file not found*/
    MOBILE_E_DRIVER_ID_NOT_SUPPORT      = 3,    /* Driver lsusb id not supported in apploacation */

    MOBILE_E_TTY_USB_NOT_FOUND          = 4,    /* ttyUSB not exist */
    MOBILE_E_TTY_USB_OPEN_FAIL          = 5,    /* ttyUSB AT intf open failed */
    MOBILE_E_TTY_USB_UNAVAILABLE        = 6,    /* ttyUSB opened successful, but (AT+CGMM) blocked*/
    MOBILE_E_TTY_USB_AVAILABLE          = 7,    /* ttyUSB AT intf test successful*/

    MOBILE_E_RADIO_NOT_AVAILABLE        = 8,    /* If radio did not start or is resetting */

    MOBILE_E_SIM_PIN2                   = 9,    /* Operation requires SIM PIN2 to be entered */
    MOBILE_E_SIM_PUK2                   = 10,   /* Operation requires SIM PIN2 to be entered */
    MOBILE_E_SIM_BUSY                   = 11,   /* SIM is busy */
    MOBILE_E_SIM_FULL                   = 12,   /* The target EF is full */
    MOBILE_E_SIM_ABSENT                 = 13,   /* fail to set the location where CDMA subscription
                                                    shall be retrieved because of SIM or RUIM card absent */
    MOBILE_E_SIM_ERR                    = 14,   /* SIM card maybe is bad for running too long */
    MOBILE_E_SIM_NOT_INSERT             = 15,   /* SIM card maybe not inserted */
    MOBILE_E_SIM_NOT_FASTED             = 16,   /* SIM card maybe not fasted */
    MOBILE_E_SIM_READY                  = 17,   /* SIM card is ready */

    MOBILE_E_OPERATOR_NOT_SUPPORT       = 18,   /* Operator maybe not support by Modem, Band... */
    MOBILE_E_OPERATOR_RECOGNOSED        = 19,   /* Operator has been recognosed */


    MOBILE_E_OP_NOT_ALLOWED_DURING_VOICE_CALL = 20, /* data ops are not allowed during voice
                                                       call on a Class C GPRS device */
    MOBILE_E_OP_NOT_ALLOWED_BEFORE_REG_TO_NW = 21,  /* data ops are not allowed before device
                                                       registers in network */
    MOBILE_E_SMS_SEND_FAIL_RETRY = 22,              /* fail to send sms and need retry */

    MOBILE_E_MODE_NOT_SUPPORTED = 23,               /* HW does not support preferred network type */
   
    MOBILE_E_SYSTEM_ERR = 24,                       /* Hit platform or system error */
    MOBILE_E_MODEM_ERR = 25,                        /* Vendor RIL got unexpected or incorrect response
                                                        from modem for this request */
    
    MOBILE_E_NETWORK_ERR = 26,                      /* Received error from network */
    MOBILE_E_NETWORK_NOT_FOUND = 27,                /* Network cannot be found */
    MOBILE_E_NETWORK_NOT_READY = 28,                /* Network is not ready to perform the request */
    MOBILE_E_NETWORK_REJECT = 29,                   /* Request is rejected by network */

    MOBILE_E_INVALID_SMS_FORMAT = 30,               /* Invalid sms format */
    MOBILE_E_ENCODING_ERR = 31,                     /* Message not encoded properly */
    MOBILE_E_INVALID_SMSC_ADDRESS = 32,             /* SMSC address specified is invalid */

    /* OEM specific error codes. To be used by OEM when they don't want to reveal
     * specific error codes which would be replaced by Generic failure.*/
    MOBILE_E_OEM_ERROR_1    = 501,
    MOBILE_E_OEM_ERROR_2    = 502,
    MOBILE_E_OEM_ERROR_3    = 503,
    MOBILE_E_OEM_ERROR_4    = 504,
    MOBILE_E_OEM_ERROR_5    = 505,
    MOBILE_E_OEM_ERROR_6    = 506,
    MOBILE_E_OEM_ERROR_7    = 507,
    MOBILE_E_OEM_ERROR_8    = 508,
    MOBILE_E_OEM_ERROR_9    = 509,
    MOBILE_E_OEM_ERROR_10   = 510,
};


//see https://android.googlesource.com/platform/hardware/ril/


std::string ErrCodeParse(enum MobileErrno e);

}
}
#endif
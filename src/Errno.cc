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
#include "Errno.h"

namespace MSF {
namespace MOBILE {

static const std::string kErrMsgMap[] = {
    "MOBILE_STATE_INIT",     /* Mobile init state*/
    "DRIVER_NOT_INSTALLED",  /* lsmod driver in /proc/moudules not installed */
    "DRIVER_ID_NOT_FOUND",   /* lsusb show id file not found*/
    "DRIVER_ID_NOT_SUPPORT", /* lsusb show id not supported for app*/
    "TTYUSB_NOT_FOUND",      /* ttyUSB not exist */
    "TTYUSB_OPEN_FAIL",      /* ttyUSB AT intf open failed */
    "TTYUSB_UNAVAILABLE", /* ttyUSB can be opened, but at(AT+CGMM) is blocked*/
    "TTYUSB_AVAILABLE",   /* ttyUSB AT intf test successful*/
    "SIM_NOT_RECOGNOSE",  /* SIM is not supported*/
    "SIM_NOT_INSERT",     /* SIM is not inerted*/
    "SIM_NOT_FASTED",     /* SIM is not fasted*/
    "SIM_COMES_READY",    /* SIM is ready*/
    "SIM_COMES_LOCKED",   /* SIM need pin or puk */
    "SIM_OPER_UNKNOWN",   /* SIM operator unknown */
    "SIM_RECOGNOSED",     /* SIM is ready and operator is known*/
};

const std::string &ErrCodeParse(const MobileErrno & e) { return kErrMsgMap[e]; }
}  // namespace MOBILE
}  // namespace MSF
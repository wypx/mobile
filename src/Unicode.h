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
#ifndef MOBILE_SRC_UNICODE_H_
#define MOBILE_SRC_UNICODE_H_

#include <cstdint>

namespace Mobile {

uint32_t StrUnicode2GB(const char *src, char *dst, uint32_t n);
uint32_t StrGB2Unicode(const char *str, char *dst, uint32_t n);

}  // namespace Mobile
#endif
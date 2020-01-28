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
#ifndef _AT_TOK_H_
#define _AT_TOK_H_

namespace MSF {
namespace MOBILE {

int AtTokStart(char **p_cur);
int AtTokNextInt(char **p_cur, int *p_out);
int AtTokNextHexInt(char **p_cur, int *p_out);
int AtTokNextBool(char **p_cur, char *p_out);
int AtTokNextStr(char **p_cur, char **out);
int AtTokNextHasMore(char **p_cur);
int AtStrStartWith(const char *line, const char *prefix);
char * AtFindNextEOL(char *cur);
}
}
#endif

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
#ifndef AT_TOK_H
#define AT_TOK_H

#include <msf_utils.h>

s32 at_tok_start(s8 **p_cur);
s32 at_tok_nextint(s8 **p_cur, s32 *p_out);
s32 at_tok_nexthexint(s8 **p_cur, s32 *p_out);
s32 at_tok_nextbool(s8 **p_cur, s8 *p_out);
s32 at_tok_nextstr(s8 **p_cur, s8 **out);
s32 at_tok_hasmore(s8 **p_cur);
s32 at_str_startwith(const s8 *line, const s8 *prefix);

#endif /*AT_TOK_H */

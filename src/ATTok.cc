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
#include "ATTok.h"

#include <base/GccAttr.h>
#include <string.h>
#include <unistd.h>

#include <cctype>
#include <cstdlib>

namespace MSF {
namespace MOBILE {

/**
 * Starts tokenizing an AT response string
 * returns -1 if this is not a valid response string, 0 on success.
 * updates *p_cur with current position
 */
int AtTokStart(char **p_cur) {
  if (unlikely(*p_cur == nullptr)) {
    return -1;
  }
  // skip prefix
  // consume "^[^:]:"
  *p_cur = strchr(*p_cur, ':');
  if (*p_cur == nullptr) {
    return -1;
  }
  (*p_cur)++;

  return 0;
}

static inline void skipWhiteSpace(char **p_cur) {
  while (**p_cur != '\0' && isspace(**p_cur)) {
    (*p_cur)++;
  }
}

static inline void skipNextComma(char **p_cur) {
  while (**p_cur != '\0' && **p_cur != ',') {
    (*p_cur)++;
  }
  if (**p_cur == ',') {
    (*p_cur)++;
  }
}

static inline char *nextTok(char **p_cur) {
  char *ret = nullptr;
  skipWhiteSpace(p_cur);

  if (*p_cur == nullptr) {
    ret = nullptr;
  } else if (**p_cur == '"') {
    (*p_cur)++;
    ret = strsep(p_cur, "\"");
    skipNextComma(p_cur);
  } else {
    ret = strsep(p_cur, ",");
  }

  return ret;
}

/**
 * Parses the next integer in the AT response line and places it in *p_out
 * returns 0 on success and -1 on fail
 * updates *p_cur
 * "base" is the same as the base param in strtol
 */
static int AtTokNextIntBase(char **p_cur, int *p_out, int base, int uns) {
  char *ret = nextTok(p_cur);
  if (ret == nullptr) {
    return -1;
  } else {
    long l;
    char *end;

    if (uns)
      l = strtoul(ret, &end, base);
    else
      l = strtol(ret, &end, base);

    *p_out = (int)l;
    if (end == ret) {
      return -1;
    }
  }

  return 0;
}

/**
 * Parses the next base 10 integer in the AT response line
 * and places it in *p_out
 * returns 0 on success and -1 on fail
 * updates *p_cur
 */
int AtTokNextInt(char **p_cur, int *p_out) {
  if (unlikely(*p_cur == nullptr)) {
    return -1;
  }
  return AtTokNextIntBase(p_cur, p_out, 10, 0);
}

/**
 * Parses the next base 16 integer in the AT response line
 * and places it in *p_out
 * returns 0 on success and -1 on fail
 * updates *p_cur
 */
int AtTokNextHexInt(char **p_cur, int *p_out) {
  if (unlikely(*p_cur == nullptr)) {
    return -1;
  }
  return AtTokNextIntBase(p_cur, p_out, 16, 1);
}

int AtTokNextBool(char **p_cur, char *p_out) {
  int ret;
  int result;

  ret = AtTokNextInt(p_cur, &result);
  if (ret < 0) {
    return -1;
  }
  // booleans should be 0 or 1
  if (!(result == 0 || result == 1)) {
    return -1;
  }
  if (p_out != NULL) {
    *p_out = (char)result;
  }
  return ret;
}

int AtTokNextStr(char **p_cur, char **p_out) {
  *p_out = nextTok(p_cur);
  return 0;
}

/** returns 1 on "has more tokens" and 0 if no */
int AtTokHasMore(char **p_cur) {
  return !(*p_cur == nullptr || **p_cur == '\0');
}

int AtStrStartWith(const char *line, const char *prefix) {
  for (; *line != '\0' && *prefix != '\0'; line++, prefix++) {
    if (*line != *prefix) {
      return 0;
    }
  }
  return *prefix == '\0';
}

/**
 * Returns a pointer to the end of the next line
 * special-cases the "> " SMS prompt
 *
 * returns nullptr if there is no complete line
 */
char *AtFindNextEOL(char *cur) {
  if (cur[0] == '>' && cur[1] == ' ' && cur[2] == '\0') {
    /* SMS prompt character...not \r terminated */
    return cur + 2;
  }

  // Find next newline
  while (*cur != '\0' && *cur != '\r' && *cur != '\n') cur++;

  return *cur == '\0' ? nullptr : cur;
}
}  // namespace MOBILE
}  // namespace MSF
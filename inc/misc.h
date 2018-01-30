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

#define DEBUG 1
#ifdef DEBUG
#define AT_DBG(fmt, args...) 	printf("[%s:%d]$ " fmt, __FILE__, __LINE__, ##args)
#define AT_INFO(fmt, args...) 	printf("[%s:%d]$ " fmt, __FILE__, __LINE__, ##args)
#define AT_ERR(fmt, args...) 	printf("[%s:%d]$ " fmt, __FILE__, __LINE__, ##args)
#define AT_WARN(fmt, args...) 	printf("[%s:%d]$ " fmt, __FILE__, __LINE__, ##args)
#else
#define AT_DBG(fmt, args...)
#endif


#ifndef _ARRAY_SIZE
#define _ARRAY_SIZE(x) (sizeof (x) / sizeof (x[0]))
#endif

#define sfclose(fp) do { \
		if ((fp) != NULL) { \
			fclose((fp)); \
			(fp) = NULL;} \
		} while(0) 

/** returns 1 if line starts with prefix, 0 if it does not */
int strStartsWith(const char *line, const char *prefix);
int setSerialBaud(int fd, int speed);
int setSerialRawMode(int fd);
int check_file_exist(char* filename);
int serial_write(int fd, void *buf, const int size);
int serial_read(int fd, void *buf, const int size);



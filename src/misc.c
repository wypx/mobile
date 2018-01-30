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


#include "misc.h"
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

/** returns 1 if line starts with prefix, 0 if it does not */
int strStartsWith(const char *line, const char *prefix)
{
    for ( ; *line != '\0' && *prefix != '\0' ; line++, prefix++) {
        if (*line != *prefix) {
            return 0;
        }
    }

    return *prefix == '\0';
}

int setSerialBaud(int fd, int speed) {
	struct termios opt;
	
	tcgetattr(fd,&opt);
	cfsetispeed(&opt,speed);
	cfsetospeed(&opt,speed);
	return tcsetattr(fd,TCSANOW,&opt);
}

int setSerialRawMode(int fd) {
	struct termios opt;
	
	tcgetattr(fd,&opt);
	opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); 		/* input */
	opt.c_iflag &= ~(IXON|IXOFF|ICRNL|INLCR|IGNCR);	/* ²»½« CR(0D) Ó³Éä³É NR(0A) */
	opt.c_oflag &= ~OPOST;							/* output */
	return tcsetattr(fd,TCSANOW,&opt);
}

int check_file_exist(char* filename) {
	int ret = -1;
	
	if(filename != NULL) {
		ret = access(filename, R_OK | W_OK); 
	}
	return (ret >= 0) ? 1 : 0;
}

int serial_write(int fd, void *buf, const int size)
{
	if(fd < 0) return -1;

#if 0
	printf("    serial_send: > ");
	int i=0;
	for(; i<size; i++) printf("%02X ", *((unsigned char *)buf+i));
	printf("\n");
#endif

	int cur=0;
	int len=size;
	int written;
	while (cur < len)
	{
		do
		{
			written = write(fd, (char*)buf + cur, len - cur);
		}
		while (written < 0 && errno == EINTR);

		if (written < 0)
		{
			return -1;
		}
		cur += written;
	}
	return cur;
}

int serial_read(int fd, void *buf, const int size)
{
	if(fd<0) return -1;

	fd_set rset;
	struct timeval tm;
	int n=0;
	unsigned int curTime, startTime;
	startTime=(unsigned int)time(0);

	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	tm.tv_sec=3;
	tm.tv_usec=0;

	if(select(fd+1, &rset, NULL, NULL, &tm)>0)
	{
		do
		{
			usleep(100*1000);
			n=read(fd, buf, size);

			curTime=time(0);
			if(abs(curTime-startTime>=3)) break;
		}
		while(n<0);
	}

#if  0
	if(n>0) printf("    serial_read: < ");
	int i=0;
	for(; i<n; i++) printf("%02X ", *((unsigned char *)buf+i));
	if(n>0) printf("\n");
#endif

	if(n<0) return -1;
	return n;
}





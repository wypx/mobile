#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#define DEBUG_PRT

int SetSerialBaud(int fd,int speed)
{
	struct termios opt;
	tcgetattr(fd,&opt);
	cfsetispeed(&opt,speed);
	cfsetospeed(&opt,speed);
	return tcsetattr(fd,TCSANOW,&opt);
}

int SetSerialRawMode(int fd)
{
	struct termios opt;
	tcgetattr(fd,&opt);
	opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /* input */
	opt.c_iflag &= ~(IXON|IXOFF|ICRNL|INLCR|IGNCR);	/* 不将 CR(0D) 映射成 NR(0A) */
	opt.c_oflag &= ~OPOST;	/* output */
	return tcsetattr(fd,TCSANOW,&opt);
}

int serial_write(int fd, const void *buf, const int size)
{
	if(fd<0) return -1;

#ifdef DEBUG_PRT
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
			written = write(fd, buf + cur, len - cur);
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

#ifdef DEBUG_PRT
	if(n>0) printf("    serial_read: < ");
	int i=0;
	for(; i<n; i++) printf("%02X ", *((unsigned char *)buf+i));
	if(n>0) printf("\n");
#endif

	if(n<0) return -1;
	return n;
}

void *serial_run(void *z)
{
	int *fd=(int *)z;
	char buf[512];
	int n=-1;
	while(1)
	{
		memset(buf, 0, 512);
		n=serial_read(*fd, buf, 512);
		if(n>0)
			printf("====%s====\n", buf);
	}
}

int main(int argc, char **argv)
{
	if(argc!=2)
	{
		printf("test <tty>\n");
		return 1;
	}
	
	signal(SIGTSTP, SIG_IGN);

	int fd;

	if((fd=open(argv[1], O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK))<0)
	{
		printf("tty cannot open!\n");
		return 2;
	}

#if 1
	SetSerialBaud(fd, B115200);
	SetSerialRawMode(fd);
#endif
	
	pthread_t tid;
	pthread_create(&tid, NULL, serial_run, (void*)&fd);

	char buf[512];
	int bufLen=0;
	#if 0
	while(1)
	{
		buf[0] = 'A';
		buf[1] = 'T';
		buf[2] = '\r';
		buf[3] = '\n';
		serial_write(fd, buf, 4);
	}
	#else
	while(fgets(buf, 512, stdin)!=NULL)
	{
		bufLen=strlen(buf)-1;
		buf[bufLen]='\0';
		strcat(buf, "\r\n");
		serial_write(fd, buf, bufLen+2);
	}
	#endif
}

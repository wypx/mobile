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


#include "atnet.h"
#include "atmod.h"
#include "misc.h"

#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/ioctl.h>

static 	int net_init(void* data, unsigned int datalen);
static 	int net_get_info(void* data, unsigned int datalen);
static 	int net_start(void* data, unsigned int datalen);

static int get_dns(char* dns1, char* dns2, int len);
static int in_cksum(unsigned short *buf, int sz);
static int net_ping(const char *host);
static int net_valid();

/* 
  Ping Function:
  to ensure network is valid
  ping luotang.me and dns addr*/

enum {
  DEFDATALEN = 56,
  MAXIPLEN = 60,
  MAXICMPLEN = 76,
  MAXPACKET = 65468,
  MAX_DUP_CHK = (8 * 128),
  MAXWAIT = 10,
  PINGINTERVAL = 1		  /* second */
};


struct usbev_module usbdev_net = {
	._init 		= net_init,
	._deinit 	= NULL,
	._start 	= net_start,
	._stop 		= NULL,
	._set_info 	= NULL,
	._get_info 	= net_get_info,
};


struct net_info netinfo;
struct net_info* net = &netinfo;

#define Test_Domain 	"luotang.me"

static 	int net_init(void* data, unsigned int datalen) 
{
	memset(net, 0, sizeof(struct net_info));
	(void)get_dns(net->test_dns1, net->test_dns2, sizeof(net->test_dns1));
	memcpy(net->test_domain, Test_Domain, strlen(Test_Domain));

	return 0;
}

static 	int net_get_info(void* data, unsigned int datalen) {
	if(datalen != sizeof(struct net_info))
		return -1;
	memcpy((struct net_info*)data, net, sizeof(struct net_info));
	return  0;
}

static 	int net_start(void* data, unsigned int datalen)
{	
	net->result = net_valid();
	return 0;
}


static int  get_dns(char* dns1, char* dns2, int len)
{
	char 	buf[128], name[50];
	char* 	dns[2];
	int 	dns_index = 0;

	#define RESOLV_FILE		"/etc/resolv.conf"

	memset(buf, 0, sizeof(buf));
	memset(name, 0, sizeof(name));
	memset(dns, 0, sizeof(dns));

	if (!dns1 || !dns2) {
		return -1;
	}

	FILE * fp = fopen(RESOLV_FILE, "r");
	if(NULL == fp) {
		AT_ERR("can not open file /etc/resolv.conf\n");
		return -1;
	}

	dns[0] = dns1;
	dns[1] = dns2;

	while (fgets(buf, sizeof(buf), fp)) {
		sscanf(buf, "%s", name);
		if (!strcmp(name, "nameserver")) {
			sscanf(buf, "%s%s", name, dns[dns_index++]);
		}

		if (dns_index >= 2) {
			break;
		}
	}

	fclose(fp);

	return 0;
}

/* common routines */
static int in_cksum(unsigned short *buf, int sz)
{
	int nleft = sz;
	int sum = 0;
	unsigned short *w = buf;
	unsigned short ans = 0;

	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1) {
		*(unsigned char *) (&ans) = *(unsigned char *) w;
		sum += ans;
	}

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	ans = ~sum;
	return (ans);
}

static int net_get_host_addr(const char* host, struct sockaddr_in* remote_addr) {
	
	int 	s = -1; 
	struct 	addrinfo hints;
	struct 	addrinfo *result = NULL;
	struct 	addrinfo *rp = NULL;
	char   	ipbuf[16];
	
	memset(ipbuf,  0, sizeof(ipbuf));
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family 	= AF_UNSPEC;	 /* Allow IPv4 and IPv6 */
	hints.ai_socktype 	= SOCK_STREAM;	  /* Socket Type */

	s = getaddrinfo(host, 0, &hints, &result);
	if (0 != s) {
		AT_DBG("getaddrinfo: %s\n", gai_strerror(s));
		return -1;
	}
	/* Some domain has more than one ip addr*/
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		remote_addr = (struct sockaddr_in *)rp->ai_addr;
		//AT_DBG("host addr:%s\n", inet_ntop(AF_INET, &remote_addr->sin_addr, ipbuf, 16));
	}

	freeaddrinfo(result);  
	return 0;
}

static int net_ping(const char *host)
{
	int ret = -1;
	struct sockaddr_in pingaddr;
	struct icmp *pkt;
	int pingsock, c;
	char packet[DEFDATALEN + MAXIPLEN + MAXICMPLEN];

	memset(&pingaddr, 0, sizeof(struct sockaddr_in));

	ret = net_get_host_addr(host, &pingaddr);
	if(ret != 0) {
		return -1;
	}
	pingaddr.sin_family = AF_INET;

	
	pingsock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if(pingsock < 0) {
		AT_DBG("socket error\n");
		return -1;
	}

	pkt = (struct icmp *) packet;
	memset(pkt, 0, sizeof(packet));
	pkt->icmp_type = ICMP_ECHO;
	pkt->icmp_cksum = in_cksum((unsigned short *) pkt, sizeof(packet));

	c = sendto(pingsock, packet, DEFDATALEN + ICMP_MINLEN, 0,
			   (struct sockaddr *) &pingaddr, sizeof(struct sockaddr_in));

	if (c < 0) {
		close(pingsock);
		AT_DBG("sendto error\n");
		return -1;
	}
	
	struct timeval tv_out;
	tv_out.tv_sec = 2;
	tv_out.tv_usec = 0;
	setsockopt(pingsock, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));

	/* listen for replies */
	while (1) {
		struct sockaddr_in from;
		socklen_t fromlen = sizeof(from);

		if ((c = recvfrom(pingsock, packet, sizeof(packet), 0,
						  (struct sockaddr *) &from, &fromlen)) < 0) {
			if(EAGAIN == errno) {
				ret = -1;
			}
			break;
		}

		if (c >= 76) {			/* ip + icmp 76????*/
			struct iphdr *iphdr = (struct iphdr *) packet;

			pkt = (struct icmp *) (packet + (iphdr->ihl << 2));	/* skip ip hdr */
			if(pkt->icmp_type == ICMP_DEST_UNREACH)
			{
				AT_DBG("ping host unreachable\n");
				ret = -1;
			}
			if (pkt->icmp_type == ICMP_ECHOREPLY)
				break;
		}
	}
	close(pingsock);
	return ret;
}

static int net_valid() {
	
	int 	count = 2;
	
	int 	failed_time = 0;
	int		net_valid = 0;

	char*	act_if = NULL;
	
	do {
		if( net_ping(net->test_dns1) != 0) {
			failed_time++;
			net_valid = 0;
			AT_DBG("ping dns %s is error\n", net->test_dns1);
			break;
		} else {
			failed_time = 0;
			AT_DBG("ping dns %s is ok\n", net->test_dns1);
			net_valid = 1;
		}

		if( net_ping(net->test_domain) != 0) {
			failed_time++;
			net_valid = 0;
			AT_DBG("ping %s is error\n", net->test_domain);
			break;
		} else {
			failed_time = 0;
			AT_DBG("ping %s is ok\n", Test_Domain);
			net_valid = 1;
		}
	
		count--;
	}while(count > 0);

	return net_valid;
}



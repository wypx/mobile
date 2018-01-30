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

struct ipc_gprs_ip_configuration {
    unsigned char unk0;
    unsigned char field_flag;
    unsigned char unk1;
    unsigned char ip[4];
    unsigned char dns1[4];
    unsigned char dns2[4];
    unsigned char gateway[4];
    unsigned char subnet_mask[4];
    unsigned char unk2[4];
};


struct dial_info {
	 int	 stat;
	 pid_t	 ppid;
	 char	 ip[16];
	 char	 dns1[16];
	 char	 dns2[16];
	 char	 gateway[16];
};


// Default MTU value
#define DEFAULT_MTU 1400

// 各运营商拨号号码、用户名、密码
#define EVDO_DFT_DIALNUM		"#777"			///< 电信
#define EVDO_DFT_NAME			"card"   		//  "ctnet@mycdma.cn" 
#define EVDO_DFT_PASSWD			"card"   		// "vnet.mobi"

#define WCDMA_DFT_DIALNUM		"*99#"			///< 联通
#define WCDMA_DFT_NAME			"user"
#define WCDMA_DFT_PASSWD		"user"
#define WCDMA_DFT_APN			"3gnet"

#define TDSCDMA_DFT_DIALNUM		"*98*1#"		///< 移动
#define TDSCDMA_DFT_NAME		"user"
#define TDSCDMA_DFT_PASSWD		"user"
#define TDSCDMA_DFT_APN			"cmnet"

#define TDLTE_DEF_DIALNUM		"*98*1#"		///< 移动 TDLTE
#define TDLTE_DFT_NAME   		"card"
#define TDLTE_DFT_PASSWD 		"card"
#define TDLTE_DFT_APN			"cmnet"         //cmwap cmmtm

#define FDDLTE_TELECOM_DEF_DIALNUM		"*99#"	///< 电信 FDDLTE
#define FDDLTE_TELECOM_DFT_NAME   		"user"
#define FDDLTE_TELECOM_DFT_PASSWD 		"user"
#define FDDLTE_TELECOM_DFT_APN			"ctlte" //ctm2m ctnet ctwap
 
#define FDDLTE_UNICOM_DEF_DIALNUM		"*98*1#" ///< 联通 FDDLTE
#define FDDLTE_UNICOM_DFT_NAME   		"user"
#define FDDLTE_UNICOM_DFT_PASSWD 		"user"
#define FDDLTE_UNICOM_DFT_APN			"cuinet" //uninet uniwap wonet



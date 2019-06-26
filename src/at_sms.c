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
#include <mobile.h>

enum SMS_MODE {
    SMS_PDU,
    SMS_TEXT,
};

struct sms {
    s8 sms_center[16];
    s8 phone_list[8];
    s8 operator_resp;
};

static s32 sms_bytes_tostring(const u8 *pstSrc, s8 *pstDst, u32 uSrcLen);
static s32 sms_encode_pdu(const struct sms_param *pstSms, s8 *pstDst);
static s32 sms_sting_tobytes(const s8 *pstSrc, u8 *pstDst, u32 uSrcLen);
static s32 sms_decode_pdu(const s8 *pstSrc, struct sms_param *pstSms);

static s32 sms_init(void *data, u32 datalen);
static s32 sms_deinit(void *data, u32 datalen);
static s32 sms_start(void *data, u32 datalen);
static s32 sms_stop(void *data, u32 datalen);
static s32 sms_set_info(void *data, u32 datalen);
static s32 sms_get_info(void *data, u32 datalen);

struct msf_svc mobile_sms = {
    .init           = sms_init,
    .deinit         = sms_deinit,
    .start          = sms_start,
    .stop           = sms_stop,
    .set_param      = sms_set_info,
    .get_param      = sms_get_info,
};

/* 将源串每8个字节分为一组, 压缩成7个字节
 * 循环该处理过程, 直至源串被处理完
 * 如果分组不到8字节, 也能正确处理 */
u32 sms_encode_7bit(const s8 *pstSrc, u8 *pstDst, u32 uSrcLen) {

    u32 uSrcCnt = 0;
    u32 uDstCnt = 0;
    s32 iCurChar = 0;/*当前处理的字符0-7*/
    u8 ucLeftLen = 0;

    while (uSrcCnt < uSrcLen) {
        /*取源字符串的计数值的最低3位*/
        iCurChar = uSrcCnt & 7;
        if (iCurChar == 0) {
            /*保存第一个字节*/
            ucLeftLen = *pstSrc;
        } else {
            /*其他字节将其右边部分与残余数据相加,得到一个目标编码字节*/
            *pstDst = (*pstSrc << (8 - iCurChar)) | ucLeftLen;
            /*将该字节剩下的左边部分,作为残余数据保存起来*/
            ucLeftLen = *pstSrc >> iCurChar;
            /*修改目标串的指针和计数值dst++*/
            uDstCnt++;
        }
        /*修改源串的指针和计数值*/
        pstSrc++;
        uSrcCnt++;
    }

    return uDstCnt;
}


/* 将源数据每7个字节分一组,解压缩成8个字节
 * 循环该处理过程, 直至源数据被处理完
 * 如果分组不到7字节,也能正确处理 */
s32 sms_decode_7bit(const s8 *pstSrc, u8 *pstDst, u32 uSrcLen) {

    u32 uSrcCnt = 0;
    u32 uDstCnt = 0;
    s32 iCurChar = 0;/*当前处理的字符0-6*/
    u8 ucLeftLen = 0;

    while (uSrcCnt < uSrcLen) {
        /* 将源字节右边部分与残余数据相加
         * 去掉最高位,得到一个目标解码字节*/
        *pstDst = ((*pstDst << iCurChar) | ucLeftLen) & 0x7f;
        /*将该字节剩下的左边部分,作为残余数据保存起来*/
        ucLeftLen = *pstDst >> (7 - iCurChar);
        pstDst++;
        uDstCnt++;
        //修改字节计数值
        iCurChar++;
        /*到了一组的最后一个字节*/
        if (iCurChar == 7) {
            //额外得到一个目标解码字节
            *pstDst = ucLeftLen;
            pstDst++;
            uDstCnt++;
            iCurChar = 0;
            ucLeftLen = 0;
        }
        pstDst++;
        uSrcCnt++;
    }
    *pstDst = 0;
    return uDstCnt;
}

/* 可打印字符串转换为字节数据
 * 如: "C8329BFD0E01"-->{0xC8,0x32,0x9B,0xFD,0x0E,0x01}
 */
static s32 sms_bytes_tostring(const u8 *pstSrc, s8 *pstDst, u32 uSrcLen) {
    u32 i;

    for (i = 0; i < uSrcLen; i += 2 , pstSrc++, pstDst++) {
        /*输出高4位*/
        if (*pstSrc >= '0' && *pstSrc <= '9') {
            *pstDst = (*pstSrc - '0') << 4;
        } else {
            *pstDst = (*pstSrc - 'A' + 10) << 4;
        }
        pstSrc++;
        /*输出低4位*/
        if (*pstSrc >= '0' && *pstSrc <= '9') {
            *pstDst |= *pstSrc - '0';
        } else {
            *pstDst |= *pstSrc - 'A' + 10;
        }
    }
    return uSrcLen / 2;
}

/* 字节数据转换为可打印字符串
 * 如:{0xC8,0x32,0x9B,0xFD,0x0E,0x01}-->"C8329BFD0E01"
 */
static s32 sms_sting_tobytes(const s8 *pstSrc, u8 *pstDst, u32 uSrcLen) {
    u32 i;
    const s8 tab[] = "0123456789ABCDEF";/* 0x0-0xf的字符查找表 */
    for (i = 0; i < uSrcLen; i++) {
        /*输出低4位*/
        *pstDst++ = tab[*pstSrc >> 4];
        /*输出高4位*/
        *pstDst++ = tab[*pstSrc & 0x0f];
        pstSrc++;
    }
    *pstDst = '\0';
    return uSrcLen * 2;
}

/* 大家已经注意到PDU串中的号码和时间,都是两两颠倒的字符串
 * 利用下面两个函数可进行正反变换：
 * 正常顺序的字符串转换为两两颠倒的字符串,若长度为奇数,补'F'凑成偶数
 * 如: "8613851872468"-->"683158812764F8"
 */
s32 sms_invert_numbers(const s8 *pSrc, s8 *pDst, u32 uSrcLen)
{
    s32 nDstLength;
    s8 ch;
    u32 i = 0;
    nDstLength = uSrcLen;
    //两两颠倒
    for (i = 0; i < uSrcLen; i += 2) {
        ch = *pSrc++;//保存先出现的字符
        *pDst++ = *pSrc++;//复制后出现的字符
        *pDst++ = ch;//复制先出现的字符
    }
    //源串长度是奇数吗？
    if (uSrcLen & 1) {
        *(pDst - 2) = 'F';//补'F'
        nDstLength++;//目标串长度加1
    }
    *pDst = '\0';
    return nDstLength;
}

/* 两两颠倒的字符串转换为正常顺序的字符串
 * 如:"683158812764F8"-->"8613851872468"
 */
s32 sms_serialize_numbers(const s8 *pSrc, s8 *pDst, u32 uSrcLen) {
    u32 nDstLength;
    s8 ch;
    u32 i;
    nDstLength = uSrcLen;
    //两两颠倒
    for (i = 0; i < uSrcLen; i += 2) {
        ch = *pSrc++;//保存先出现的字符
        *pDst++ = *pSrc++;//复制后出现的字符
        *pDst++ = ch;//复制先出现的字符
    }
    //最后的字符是'F'吗？
    if (*(pDst - 1) == 'F') {
        pDst--;
        nDstLength--;//目标字符串长度减1
    }
    *pDst = '\0';
    return nDstLength;
}

u32 sms_encode_8bit(const s8 *pSrc, u8 *pDst, u32 uSrcLen) {
    memcpy(pDst, pSrc, uSrcLen);
    return uSrcLen;
}

u32 sms_decode_8bit(const u8 *pSrc, s8 *pDst, u32 uSrcLen) {
    memcpy(pDst, pSrc, uSrcLen);
    *pDst = '\0';
    return uSrcLen;
}

static s32 sms_encode_pdu(const struct sms_param *pstSms, s8 *pDst) {

    s32 nLength;//内部用的串长度
    s32 nDstLength;//目标PDU串长度
    u8 buf[256];//内部用的缓冲区

    //SMSC地址信息段
    nLength = strlen(pstSms->sca);//SMSC地址字符串的长度
    buf[0] = (s8) ((nLength & 1) == 0 ? nLength : nLength + 1) / 2 + 1;//SMSC地址信息长度
    buf[1] = 0x91;//固定:用国际格式号码
    nDstLength = sms_bytes_tostring(buf, pDst, 2);//转换2个字节到目标PDU串
    nDstLength += sms_invert_numbers(pstSms->sca, &pDst[nDstLength], nLength);//转换SMSC到目标PDU串

    //TPDU段基本参数、目标地址等nLength=strlen(pSrc->TPA);//TP-DA地址字符串的长度
    buf[0] = 0x11;//是发送短信(TP-MTI=01)，TP-VP用相对格式(TP-VPF=10)
    buf[1] = 0;//TP-MR=0
    buf[2] = (s8) nLength;//目标地址数字个数(TP-DA地址字符串真实长度)
    buf[3] = 0x91;//固定:用国际格式号码

    nDstLength += sms_bytes_tostring(buf, &pDst[nDstLength], 4);//转换4个字节到目标PDU串
    nDstLength += sms_invert_numbers(pstSms->tpa, &pDst[nDstLength], nLength);//转换	TP-DA到目标PDU串

    //TPDU段协议标识、编码方式、用户信息等
    nLength = strlen(pstSms->tp_ud);//用户信息字符串的长度
    buf[0] = pstSms->tp_pid;//协议标识(TP-PID)
    buf[1] = pstSms->tp_dcs;//用户信息编码方式(TP-DCS)
    buf[2] = 0;//有效期(TP-VP)为5分钟

    if (pstSms->tp_dcs == GSM_7BIT) {
        //7-bit编码方式
        buf[3] = nLength;//编码前长度
        nLength = sms_encode_7bit(pstSms->tp_ud, &buf[4], nLength + 1) + 4;//转换TP-DA到目标PDU串
    }
    else if (pstSms->tp_dcs == GSM_UCS2)
    {
        //UCS2编码方式
        buf[3] = at_strGB2Unicode(pstSms->tp_ud, &buf[4], nLength);//转换TP-DA到目标PDU串
        nLength = buf[3] + 4;//nLength等于该段数据长度
    }
    else
    {
        //8-bit编码方式
        buf[3]=sms_encode_8bit(pstSms->tp_ud,&buf[4],nLength);//转换TP-DA到目标PDU串
        nLength=buf[3]+4;//nLength等于该段数据长度
    }
    nDstLength += sms_bytes_tostring(buf, &pDst[nDstLength], nLength);//转换该段数据到目标PDU串
    return nDstLength;
}

static s32 sms_decode_pdu(const s8 *pSrc, struct sms_param *pstSms) {

    u32 nDstLength;//目标PDU串长度
    u8 tmp;//内部用的临时字节变量
    s8 buf[256];//内部用的缓冲区
    s8 TP_MTI_MMS_RP = 0;
    u8 udhl = 0;/*数据头长度,只用于长短信*/
    //SMSC地址信息段
    sms_sting_tobytes(pSrc, (u8*)&tmp, 2);//取长度
    tmp = (tmp - 1) * 2;//SMSC号码串长度
    pSrc += 4;//指针后移
    sms_serialize_numbers(pSrc, pstSms->sca, tmp);//转换SMSC号码到目标PDU串
    pSrc += tmp;//指针后移
    //TPDU段基本参数、回复地址等
    sms_sting_tobytes(pSrc, (u8*)&tmp, 2);//取基本参数
    TP_MTI_MMS_RP = tmp;
    pSrc += 2;//指针后移
    //包含回复地址,取回复地址信息
    sms_sting_tobytes(pSrc, (u8*)&tmp, 2);//取长度
    if (tmp & 1) {
        tmp += 1;//调整奇偶性
    }
    pSrc += 4;//指针后移
    sms_serialize_numbers(pSrc, pstSms->tpa, tmp);//取TP-RA号码
    pSrc += tmp;//指针后移
    //TPDU段协议标识、编码方式、用户信息等
    sms_sting_tobytes(pSrc, (u8*) &pstSms->tp_pid, 2);//取协议标识TP-PID)
    pSrc += 2;//指针后移
    sms_sting_tobytes(pSrc, (u8*) &pstSms->tp_dcs, 2);//取编码方式(TP-DCS)
    pSrc += 2;//指针后移
    sms_serialize_numbers(pSrc, pstSms->tp_scts, 14);//服务时间戳字符串   (TP_SCTS)
    pSrc += 14;//指针后移
    sms_sting_tobytes(pSrc, (u8*)&tmp, 2);//用户信息长度(TP-UDL)
    pSrc += 2;//指针后移

    /*有UDHI*/
    if(TP_MTI_MMS_RP & 0x40) {
        sms_sting_tobytes(pSrc, &udhl, 2);
        pSrc += 2;
        pSrc += udhl*2;
        tmp -= (((udhl<<1)+2) >> 1);
    }

    if (pstSms->tp_dcs == GSM_7BIT) {
        // 7-bit解码
        nDstLength = sms_sting_tobytes(pSrc, (u8*)buf, tmp & 7 ? (int) tmp * 7 / 4 + 2
                : (s32) tmp * 7 / 4);//格式转换
        sms_decode_7bit(buf, (u8*)pstSms->tp_ud, nDstLength);//转换到TP-DU
        nDstLength = tmp;
    } else if (pstSms->tp_dcs == GSM_UCS2) {
        // UCS2解码
        nDstLength = sms_sting_tobytes(pSrc, (u8*)buf, tmp * 2);//格式转换
        nDstLength = at_strUnicode2GB((u8*)buf, pstSms->tp_ud, nDstLength);//转换到TP-DU
    } else {
        //8-bit解码
        nDstLength = sms_sting_tobytes(pSrc, (u8*)buf, tmp * 2);//格式转换
        nDstLength = sms_decode_8bit((u8*)buf, pstSms->tp_ud, nDstLength);//转换到TP-DU
    }
    //返回目标字符串长度
    return nDstLength;
}

s32 sms_chkfragment(const s8 *msg)
{
    if(NULL == msg)
    {
        printf("check_sms_length: input msg NULL\n");
        return false;
    }

    u32 i = 0;
    s32 chn_num = 0;
    s32 eng_num = 0;

    for(i = 0; i < strlen(msg); i++)
    {
        if(msg[i] > 0xA0)
        {
            chn_num ++;
        }
        else
        {
            eng_num++;
        }

        if((chn_num + 2*eng_num) >= 139 && (0 == chn_num % 2))
        {
            return (chn_num+ eng_num);
        }
    }

    return true;
}


enum SMS_FORMAT smsformat;

static s32 sms_sendmsg(const s8 *phone, const s8 *msg) {

#if 0
    s32 rc;
    s32 nSmscLength;
    s32 nPduLength;
    s8 pdu[512] = { 0 };
    s8 printfBuf[256] = { 0 };
    s8 atCommand[32] = { 0 };
    struct sms_param stSms;
    s32 mobile_mode;

    if (SMS_TEXT_MODE == smsFormat)
    {
        writeline(s_fd, "AT^HSMSSS=0,0,6,0"); /* set SMS param */
        nSmscLength = at_strGB2Unicode(msg, (u8*)pdu, strlen(msg));
        sprintf(printfBuf, "%d, %d", nSmscLength, strlen(msg));
        atPrintf(printfBuf);
        sprintf(atCommand, "AT^HCMGS=\"%s\"", phone);
        writeline(s_fd, atCommand);
        //rc = waitATResponse(">", (s8*)NULL, 0, true, 2000);
        if (rc == 0)
        {
         //   rc = writeCtrlZ(s_fd, pdu, nSmscLength);
        }

    } else {
        nPduLength = sms_encode_pdu(&stSms, pdu);
        sms_sting_tobytes(pdu, &nSmscLength, 2);
        nSmscLength++;

        printf("the pdu after encode is: %s\n", pdu);
        sprintf(printfBuf, "nPduLength[%d], nSmscLength[%d]", nPduLength, nSmscLength);

        if(MODE_EVDO == mobile_mode)
        {
            sprintf(atCommand, "AT^HCMGS=%d", nPduLength / 2 - nSmscLength);	/* SMS send */
        }
        else
        {
            sprintf(atCommand, "AT+CMGS=%d", nPduLength / 2 - nSmscLength);	/* SMS send */
        }

        writeline(s_fd, atCommand);

        /* wait \r\n> */
        //rc = waitATResponse(">", (char*)NULL, 0, TRUE, 2000);
        if (rc == 0)
        {
           // rc = writeCtrlZ(s_fd, pdu, nPduLength);
        }
        else
        {
           // rc = writeCtrlZ(s_fd, pdu, nSmscLength);
        }
    }
#endif
    return 0;
}

#if 0
static s32 ATReadSMS(const char* line1, const char* line2, int index, int line2Len, s32 * smsStatus, s32 bNewSms)
{
    /*
     ����TD��TXTģʽ�½�����Ϣ����: (text mode)
     +CMT: "1065815401",,"09/03/11,12:14:06+02"
     messsageContent
     */
    char msg[164];                       /*����Ϣֻ����70�ֽ�*/
    char phoneNum[32];
    char printfBuf[256];
    const char *pTmp1 = NULL;
    const char *pTmp2 = NULL;
    struct sms_param smParam;
    int i = 0;
    int smsEncForm = 0;


    memset(msg, 0, sizeof(msg));
    memset(phoneNum, 0, sizeof(phoneNum));
    memset(printfBuf, 0, sizeof(printfBuf));
    memset(&smParam, 0, sizeof(struct sms_param));

    pTmp1 = pTmp2 = line1;
    if (SMS_TEXT_MODE == smsFormat)
    {
        /*
        <CR><LF>^HCMGR: <callerID>, <year>, <month>, <day>, <hour>, <minute>, <second>, 
            <lang>, <format>, <length>, <prt>,<prv>,<type>,<stat> 
        <CR><LF><msg><CTRL+Z><CR><LF>
        */
        /*ȥ��^HCMGR:*/
        pTmp1 += 7;
        pTmp2 = strchr(pTmp1, ',');
        strncpy(phoneNum, pTmp1, (pTmp2 - pTmp1));
        //strncpy((char *)(smsStatus->phoneNum), phoneNum, MIN(sizeof(smsStatus->phoneNum) - 1, strlen(phoneNum)));
        //strncpy((char *)(smsStatus->recvTime), pTmp2 + 1, 4);


        for (i = 0; i <5;i++)
        {
            pTmp2 = strchr(pTmp2 + 1, ',');
            /* <month>, <day>, <hour>, <minute>, <second> */
            strncpy((char *)(&(smsStatus->recvTime[i * 2 + 4])), pTmp2 + 1, 2);
        }

        if(NULL == (pTmp2 = strchr(pTmp2 + 1, ',')))
            return -1;
        if(NULL == (pTmp2 = strchr(pTmp2 + 1, ',')))
            return -1;
        pTmp2++;
        smsEncForm = atoi(pTmp2);

        line2Len = line2Len > 140  ? 140 : line2Len;		

        if (1 == smsEncForm)	/* 1-- ASCII */
        {
            /*һ���������70������modify 0318*/
            strncpy(msg, line2, line2Len);
        }
        else if (4 == smsEncForm)	/* 6-- UNICODE */
        {
        	strUnicode2GB((unsigned char *)line2, msg, line2Len - 2); /* delete the last 2 byte 0x00 0x1a */
        }		
        }
        /*PUDģʽ*/
        else
        {
        initSmParam(&smParam, "", "");
        gsmDecodePdu(line2, &smParam);
        strcpy(phoneNum, smParam.TPA);
        strcpy(msg, smParam.TP_UD);
        strncpy((char *)(smsStatus->phoneNum), phoneNum, MIN(sizeof(smsStatus->phoneNum) - 1, strlen(phoneNum)));
        smsStatus->recvTime[0] = '2';						/* ����20xx ͷ */
        smsStatus->recvTime[1] = '0';
        strncpy((char *)(&smsStatus->recvTime[2]), smParam.TP_SCTS, 12);	/* SMS received time */		
    }

}
#endif

static s32 sms_init(void* data, u32 datalen) {
    return 0;
}
static s32 sms_deinit(void *data, u32 datalen) {
    return 0;
}
static s32 sms_start(void *data, u32 datalen) {
    return 0;
}
static s32 sms_stop(void *data, u32 datalen) {
    return 0;
}
static s32 sms_set_info(void *data, u32 datalen) {
    return 0;
}
static s32 sms_get_info(void *data, u32 datalen) {
    return 0;
}

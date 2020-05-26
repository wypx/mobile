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
#include <base/Logger.h>

#include "ATChannel.h"
#include "Mobile.h"
#include "Unicode.h"

#include "Sms.h"

using namespace MSF::BASE;
using namespace MSF::MOBILE;

namespace MSF {
namespace MOBILE {

// User SMS Code
#define GSM_7BIT 0
#define GSM_8BIT 4
#define GSM_UCS2 8

bool SMSManager::AddMsg(const std::string &msg) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (sms_queue_.size() > max_msg_num_) {
    MSF_ERROR << "SMS exceed max msg num: " << max_msg_num_;
    return false;
  }
  sms_queue_.emplace(msg);
  return true;
}

SMS & SMSManager::DelMsg() {
  std::lock_guard<std::mutex> lock(mutex_);
  SMS &sms = sms_queue_.front();
  sms_queue_.pop();
  return sms;
}

/* 将源串每8个字节分为一组, 压缩成7个字节
 * 循环该处理过程, 直至源串被处理完
 * 如果分组不到8字节, 也能正确处理 */
uint32_t SMSManager::Encode7Bit(char *src, char *dst, uint32_t srcLen) {
  uint32_t uSrcCnt = 0;
  uint32_t uDstCnt = 0;
  uint32_t iCurChar = 0; /*当前处理的字符0-7*/
  uint32_t ucLeftLen = 0;

  while (uSrcCnt < srcLen) {
    /*取源字符串的计数值的最低3位*/
    iCurChar = uSrcCnt & 7;
    if (iCurChar == 0) {
      /*保存第一个字节*/
      ucLeftLen = *src;
    } else {
      /*其他字节将其右边部分与残余数据相加,得到一个目标编码字节*/
      *dst = (*src << (8 - iCurChar)) | ucLeftLen;
      /*将该字节剩下的左边部分,作为残余数据保存起来*/
      ucLeftLen = *src >> iCurChar;
      /*修改目标串的指针和计数值dst++*/
      uDstCnt++;
    }
    /*修改源串的指针和计数值*/
    src++;
    uSrcCnt++;
  }
  return uDstCnt;
}

/* 可打印字符串转换为字节数据
 * 如: "C8329BFD0E01"-->{0xC8,0x32,0x9B,0xFD,0x0E,0x01}
 */
uint32_t SMSManager::Byte2String(char *src, char *dst, uint32_t srcLen) {
  uint32_t i;

  for (i = 0; i < srcLen; i += 2, src++, dst++) {
    /*输出高4位*/
    if (*src >= '0' && *src <= '9') {
      *dst = (*src - '0') << 4;
    } else {
      *dst = (*src - 'A' + 10) << 4;
    }
    src++;
    /*输出低4位*/
    if (*src >= '0' && *src <= '9') {
      *dst |= *src - '0';
    } else {
      *dst |= *src - 'A' + 10;
    }
  }
  return srcLen / 2;
}

/* 将源数据每7个字节分一组,解压缩成8个字节
 * 循环该处理过程, 直至源数据被处理完
 * 如果分组不到7字节,也能正确处理 */
uint32_t SMSManager::Decode7Bit(char *src, char *dst, uint32_t srcLen) {
  uint32_t uSrcCnt = 0;
  uint32_t uDstCnt = 0;
  uint32_t iCurChar = 0; /*当前处理的字符0-6*/
  uint32_t ucLeftLen = 0;

  while (uSrcCnt < srcLen) {
    /* 将源字节右边部分与残余数据相加
     * 去掉最高位,得到一个目标解码字节*/
    *dst = ((*dst << iCurChar) | ucLeftLen) & 0x7f;
    /*将该字节剩下的左边部分,作为残余数据保存起来*/
    ucLeftLen = *dst >> (7 - iCurChar);
    dst++;
    uDstCnt++;
    //修改字节计数值
    iCurChar++;
    /*到了一组的最后一个字节*/
    if (iCurChar == 7) {
      //额外得到一个目标解码字节
      *dst = ucLeftLen;
      dst++;
      dst++;
      iCurChar = 0;
      ucLeftLen = 0;
    }
    dst++;
    uSrcCnt++;
  }
  *dst = 0;
  return uDstCnt;
}

/* 字节数据转换为可打印字符串
 * 如:{0xC8,0x32,0x9B,0xFD,0x0E,0x01}-->"C8329BFD0E01"
 */
uint32_t SMSManager::String2Byte(char *src, char *dst, uint32_t srcLen) {
  uint32_t i;
  const char tab[] = "0123456789ABCDEF"; /* 0x0-0xf的字符查找表 */
  for (i = 0; i < srcLen; i++) {
    /*输出低4位*/
    *dst++ = tab[*src >> 4];
    /*输出高4位*/
    *dst++ = tab[*src & 0x0f];
    src++;
  }
  *dst = '\0';
  return srcLen * 2;
}

/* 大家已经注意到PDU串中的号码和时间,都是两两颠倒的字符串
 * 利用下面两个函数可进行正反变换：
 * 正常顺序的字符串转换为两两颠倒的字符串,若长度为奇数,补'F'凑成偶数
 * 如: "8613851872468"-->"683158812764F8"
 */
uint32_t SMSManager::InvetNumbers(char *src, char *dst, uint32_t srcLen) {
  uint32_t nDstLength = srcLen;
  char ch;
  uint32_t i = 0;

  //两两颠倒
  for (i = 0; i < srcLen; i += 2) {
    ch = *src++;      //保存先出现的字符
    *dst++ = *src++;  //复制后出现的字符
    *dst++ = ch;      //复制先出现的字符
  }
  //源串长度是奇数吗？
  if (srcLen & 1) {
    *(dst - 2) = 'F';  //补'F'
    nDstLength++;      //目标串长度加1
  }
  *dst = '\0';
  return nDstLength;
}

/* 两两颠倒的字符串转换为正常顺序的字符串
 * 如:"683158812764F8"-->"8613851872468"
 */
uint32_t SMSManager::SerializeNumbers(char *src, char *dst, uint32_t srcLen) {
  uint32_t nDstLength = srcLen;
  char ch;
  uint32_t i;
  //两两颠倒
  for (i = 0; i < srcLen; i += 2) {
    ch = *src++;      //保存先出现的字符
    *dst++ = *src++;  //复制后出现的字符
    *dst++ = ch;      //复制先出现的字符
  }
  //最后的字符是'F'吗？
  if (*(dst - 1) == 'F') {
    dst--;
    nDstLength--;  //目标字符串长度减1
  }
  *dst = '\0';
  return nDstLength;
}

uint32_t SMSManager::Encode8Bit(char *src, char *dst, uint32_t srcLen) {
  memcpy(dst, src, srcLen);
  return srcLen;
}

uint32_t SMSManager::Decode8Bit(char *src, char *dst, uint32_t srcLen) {
  memcpy(dst, src, srcLen);
  *dst = '\0';
  return srcLen;
}

uint32_t SMSManager::EncodePdu(char *dst) {
  uint32_t nLength;     //内部用的串长度
  uint32_t nDstLength;  //目标PDU串长度
  char buf[256];        //内部用的缓冲区

  // SMSC地址信息段
  nLength = strlen(sca_);  // SMSC地址字符串的长度
  buf[0] = (char)((nLength & 1) == 0 ? nLength : nLength + 1) / 2 +
           1;                             // SMSC地址信息长度
  buf[1] = 0x91;                          //固定:用国际格式号码
  nDstLength = Byte2String(buf, dst, 2);  //转换2个字节到目标PDU串
  nDstLength +=
    InvetNumbers(sca_, &dst[nDstLength], nLength);  //转换SMSC到目标PDU串

  // TPDU段基本参数、目标地址等nLength=strlen(pSrc->tpa_);//TP-DA地址字符串的长度
  buf[0] = 0x11;  //是发送短信(TP-MTI=01)，TP-VP用相对格式(TP-VPF=10)
  buf[1] = 0;     // TP-MR=0
  buf[2] = (char)nLength;  //目标地址数字个数(TP-DA地址字符串真实长度)
  buf[3] = 0x91;           //固定:用国际格式号码

  nDstLength += Byte2String(buf, &dst[nDstLength], 4);  //转换4个字节到目标PDU串
  nDstLength +=
      InvetNumbers(tpa_, &dst[nDstLength], nLength);  //转换	TP-DA到目标PDU串

  // TPDU段协议标识、编码方式、用户信息等
  nLength = strlen(tp_ud_);  //用户信息字符串的长度
  buf[0] = tp_pid_;          //协议标识(TP-PID)
  buf[1] = tp_dcs_;          //用户信息编码方式(TP-DCS)
  buf[2] = 0;               //有效期(TP-VP)为5分钟

  if (tp_dcs_ == GSM_7BIT) {
    // 7-bit编码方式
    buf[3] = nLength;  //编码前长度
    nLength =
        Encode7Bit(tp_ud_, &buf[4], nLength + 1) + 4;  //转换TP-DA到目标PDU串
  } else if (tp_dcs_ == GSM_UCS2) {
    // UCS2编码方式
    buf[3] = StrGB2Unicode(tp_ud_, &buf[4], nLength);  //转换TP-DA到目标PDU串
    nLength = buf[3] + 4;  // nLength等于该段数据长度
  } else {
    // 8-bit编码方式
    buf[3] = Encode8Bit(tp_ud_, &buf[4], nLength);  //转换TP-DA到目标PDU串
    nLength = buf[3] + 4;  // nLength等于该段数据长度
  }
  nDstLength +=
      Byte2String(buf, &dst[nDstLength], nLength);  //转换该段数据到目标PDU串
  return nDstLength;
}

uint32_t SMSManager::DecodePdu(char *src) {
  uint32_t nDstLength;  //目标PDU串长度
  char tmp;             //内部用的临时字节变量
  char buf[256];        //内部用的缓冲区
  char TP_MTI_MMS_RP = 0;
  char udhl = 0; /*数据头长度,只用于长短信*/
  // SMSC地址信息段
  String2Byte(src, (char *)&tmp, 2);  //取长度
  tmp = (tmp - 1) * 2;                // SMSC号码串长度
  src += 4;                           //指针后移
  SerializeNumbers(src, sca_, tmp);    //转换SMSC号码到目标PDU串
  src += tmp;                         //指针后移
  // TPDU段基本参数、回复地址等
  String2Byte(src, (char *)&tmp, 2);  //取基本参数
  TP_MTI_MMS_RP = tmp;
  src += 2;  //指针后移
  //包含回复地址,取回复地址信息
  String2Byte(src, (char *)&tmp, 2);  //取长度
  if (tmp & 1) {
    tmp += 1;  //调整奇偶性
  }
  src += 4;                         //指针后移
  SerializeNumbers(src, tpa_, tmp);  //取TP-RA号码
  src += tmp;                       //指针后移
  // TPDU段协议标识、编码方式、用户信息等
  String2Byte(src, (char *)&tp_pid_, 2);  //取协议标识TP-PID)
  src += 2;                              //指针后移
  String2Byte(src, (char *)&tp_dcs_, 2);  //取编码方式(TP-DCS)
  src += 2;                              //指针后移
  SerializeNumbers(src, tp_scts_, 14);    //服务时间戳字符串   (tp_scts_)
  src += 14;                             //指针后移
  String2Byte(src, (char *)&tmp, 2);     //用户信息长度(TP-UDL)
  src += 2;                              //指针后移

  /*有UDHI*/
  if (TP_MTI_MMS_RP & 0x40) {
    String2Byte(src, (char *)&udhl, 2);
    src += 2;
    src += udhl * 2;
    tmp -= (((udhl << 1) + 2) >> 1);
  }

  if (tp_dcs_ == GSM_7BIT) {
    // 7-bit解码
    nDstLength = String2Byte(
        src, buf,
        tmp & 7 ? (int)tmp * 7 / 4 + 2 : (int)tmp * 7 / 4);  //格式转换
    Decode7Bit(buf, tp_ud_, nDstLength);                      //转换到TP-DU
    nDstLength = tmp;
  } else if (tp_dcs_ == GSM_UCS2) {
    // UCS2解码
    nDstLength = String2Byte(src, buf, tmp * 2);         //格式转换
    nDstLength = StrUnicode2GB(buf, tp_ud_, nDstLength);  //转换到TP-DU
  } else {
    // 8-bit解码
    nDstLength = String2Byte(src, buf, tmp * 2);      //格式转换
    nDstLength = Decode8Bit(buf, tp_ud_, nDstLength);  //转换到TP-DU
  }
  //返回目标字符串长度
  return nDstLength;
}

int SMSManager::CheckFragment(const char *msg, const uint32_t len) {
  uint32_t msg_idx = 0;
  uint32_t chn_num = 0;
  uint32_t eng_num = 0;

  for (msg_idx = 0; msg_idx < len; msg_idx++) {
    if (msg[msg_idx] > 0xA0) {
      chn_num++;
    } else {
      eng_num++;
    }

    if ((chn_num + 2 * eng_num) >= 139 && (0 == chn_num % 2)) {
      return (chn_num + eng_num);
    }
  }

  return -1;
}

int SMSManager::SendSMSMsg(const char *phone, const char *msg) {
  int ret;
  int nSmscLength;
  int nPduLength;
  char pdu[512] = {0};
  char printfBuf[256] = {0};
  char atCommand[32] = {0};
  int mobile_mode;

  if (SMS_TEXT == format_) {
    ch_->WriteLine("AT^HSMSSS=0,0,6,0", kCtrlEnter.c_str()); /* set SMS param */
    nSmscLength = StrGB2Unicode(msg, pdu, strlen(msg));
    sprintf(printfBuf, "%d, %ld", nSmscLength, strlen(msg));
    MSF_DEBUG << printfBuf;
    sprintf(atCommand, "AT^HCMGS=\"%s\"", phone);
    ch_->WriteLine(atCommand, kCtrlEnter.c_str());
    // ret = waitATResponse(">", nullptr, 0, true, 2000);
    if (ret == 0) {
      ch_->WriteLine(pdu, kCtrlZ.c_str());
      // ret = ch_->WriteLine(pdu, nSmscLength);
    }

  } else {
    nPduLength = EncodePdu(pdu);
    String2Byte(pdu, (char *)&nSmscLength, 2);
    nSmscLength++;

    printf("the pdu after encode is: %s\n", pdu);
    sprintf(printfBuf, "nPduLength[%d], nSmscLength[%d]", nPduLength,
            nSmscLength);

    if (MODE_EVDO == mobile_mode) {
      sprintf(atCommand, "AT^HCMGS=%d",
              nPduLength / 2 - nSmscLength); /* SMS send */
    } else {
      sprintf(atCommand, "AT+CMGS=%d",
              nPduLength / 2 - nSmscLength); /* SMS send */
    }
  
    ch_->WriteLine(atCommand, kCtrlEnter.c_str());

    /* wait \r\n> */
    // ret = waitATResponse(">", nullptr, 0, true, 2000);
    if (ret == 0) {
      //    ret = ch_->WriteLine(pdu, nPduLength);
    } else {
      //    ret = ch_->WriteLine(pdu, nSmscLength);
    }
  }
  return 0;
}

int SMSManager::ATReadSMSThread(char *line1, char *line2, int index,
                                int line2Len, int *smsStatus, int bNewSms) {
  /*
   ����TD��TXTģʽ�½�����Ϣ����: (text mode)
   +CMT: "1065815401",,"09/03/11,12:14:06+02"
   messsageContent
   */
  char msg[164]; /*����Ϣֻ����70�ֽ�*/
  char phoneNum[32];
  char printfBuf[256];
  const char *pTmp1 = NULL;
  const char *pTmp2 = NULL;
  int i = 0;
  int smsEncForm = 0;

  memset(msg, 0, sizeof(msg));
  memset(phoneNum, 0, sizeof(phoneNum));
  memset(printfBuf, 0, sizeof(printfBuf));

  pTmp1 = pTmp2 = line1;
  if (SMS_TEXT == format_) {
    /*
    <CR><LF>^HCMGR: <callerID>, <year>, <month>, <day>, <hour>, <minute>,
    <second>,
        <lang>, <format_>, <length>, <prt>,<prv>,<type>,<stat>
    <CR><LF><msg><CTRL+Z><CR><LF>
    */
    /*ȥ��^HCMGR:*/
    pTmp1 += 7;
    pTmp2 = strchr(pTmp1, ',');
    strncpy(phoneNum, pTmp1, (pTmp2 - pTmp1));
    // strncpy((char *)(smsStatus->phoneNum), phoneNum,
    // MIN(sizeof(smsStatus->phoneNum) - 1, strlen(phoneNum)));
    // strncpy((char *)(smsStatus->recvTime), pTmp2 + 1, 4);

    for (i = 0; i < 5; i++) {
      pTmp2 = strchr(pTmp2 + 1, ',');
      /* <month>, <day>, <hour>, <minute>, <second> */
      // strncpy((char *)(&(smsStatus->recvTime[i * 2 + 4])), pTmp2 + 1, 2);
    }

    if (nullptr == (pTmp2 = strchr(pTmp2 + 1, ','))) return -1;
    if (nullptr == (pTmp2 = strchr(pTmp2 + 1, ','))) return -1;
    pTmp2++;
    smsEncForm = atoi(pTmp2);

    line2Len = line2Len > 140 ? 140 : line2Len;

    if (1 == smsEncForm) /* 1-- ASCII */
    {
      /*һ���������70������modify 0318*/
      strncpy(msg, line2, line2Len);
    } else if (4 == smsEncForm) /* 6-- UNICODE */
    {
      StrUnicode2GB(line2, msg,
                    line2Len - 2); /* delete the last 2 byte 0x00 0x1a */
    }
    /*PUDģʽ*/
    else {
      DecodePdu(line2);
      strcpy(phoneNum, tpa_);
      strcpy(msg, tp_ud_);
    }
    // strncpy((char *)(smsStatus->phoneNum), phoneNum,
    // MIN(sizeof(smsStatus->phoneNum) - 1, strlen(phoneNum)));
    // smsStatus->recvTime[0] = '2'; /* ����20xx ͷ
    // */
    // smsStatus->recvTime[1] = '0';
    // strncpy((char *)(&smsStatus->recvTime[2]), smParam.tp_scts_, 12);	/* SMS
    // received time */
  }
  return 0;
}
}  // namespace MOBILE
}  // namespace MSF

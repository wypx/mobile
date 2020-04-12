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
#ifndef MOBILE_APP_SMS_H
#define MOBILE_APP_SMS_H

#include <iostream>
#include <list>
#include <mutex>

namespace MSF {
namespace MOBILE {

/**
 * 其他源码参考:
 * https://github.com/nicolas-s/ofono/tree/master/drivers/atmodem
 *
 * */

enum SMSMode { SMS_PDU, SMS_TEXT };

enum SMS_STORAGE_AREA { ME_AREA = 0, SM_AREA };

enum SMS_CLEAN_MODE {
  SMS_CLEAN_READ = 1,          /* 清除已读短信 */
  SMS_CLEAN_READ_SEND,         //清除已读和已发送短信
  SMS_CLEAN_READ_SEND_UNSEND,  //清除已读,已发送和未发送短信
  SMS_CLEAN_READ_UNREAD_SEND_UNSEND  //清除已读、未读、已发送和未发送短信
};

struct SMS {
  std::string smsMsg; /* the encoded SMS message (max 255 bytes) */
  uint8_t resp;

  SMS(const std::string &msg) : smsMsg(std::move(msg)) {}
};

struct ATChannel;
class SMSManager {
 public:
  bool addMsg(const std::string &msg);
  struct SMS delMsg();

 private:
  std::mutex _mutex;
  std::string _smsCenter; /* Max len 16*/
  uint32_t _maxMsgNum;
  std::list<struct SMS> _smsMsgList;      /* SMS message queue */
  std::list<std::string> _whitePhoneList; /* White phone list to recieve msg*/

  char sca[16];     /*SMSC:Service central number*/
  char tpa[16];     /*Dst or resp number (TP-DA or TP-RA)*/
  char tp_pid;      /*TP-PID:User protocol id*/
  char tp_dcs;      /*TP-DCS:User sms encode*/
  char tp_scts[16]; /*TP_SCTS:Service time stamp*/
  char tp_ud[161];  /*TP-UD:Origin user info*/
  char index;       /*Sms message index*/

  enum SMSMode format;

  ATChannel *_ch;

  uint32_t byte2String(char *src, char *dst, uint32_t srcLen);
  uint32_t encode7bit(char *src, char *dst, uint32_t srcLen);
  uint32_t encode8bit(char *src, char *dst, uint32_t srcLen);
  uint32_t encodePdu(char *dst);
  uint32_t decode7bit(char *src, char *dst, uint32_t srcLen);
  uint32_t decode8bit(char *src, char *dst, uint32_t srcLen);
  uint32_t decodePdu(char *src);
  uint32_t string2Byte(char *src, char *dst, uint32_t srcLen);

  uint32_t invetNumbers(char *src, char *dst, uint32_t srcLen);
  uint32_t serializeNumbers(char *src, char *dst, uint32_t srcLen);

  int checkFragment(const char *msg, const uint32_t len);

  int sendSMSMsg(const char *phone, const char *msg);

  int ATReadSMSThread(char *line1, char *line2, int index, int line2Len,
                      int *smsStatus, int bNewSms);
};
}  // namespace MOBILE
}  // namespace MSF
#endif
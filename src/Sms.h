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
#ifndef MOBILE_SRC_SMS_H_
#define MOBILE_SRC_SMS_H_

#include <iostream>
#include <queue>
#include <mutex>

namespace MSF {
namespace MOBILE {

/**
 * 其他源码参考:
 * https://github.com/nicolas-s/ofono/tree/master/drivers/atmodem
 **/
enum SMSMode { SMS_PDU, SMS_TEXT };

enum SMS_STORAGE_AREA { ME_AREA = 0, SM_AREA };

enum SMS_CLEAN_MODE {
  SMS_CLEAN_READ = 1,          /* 清除已读短信 */
  SMS_CLEAN_READ_SEND,         /* 清除已读和已发送短信 */
  SMS_CLEAN_READ_SEND_UNSEND,  /* 清除已读,已发送和未发送短信 */
  SMS_CLEAN_READ_UNREAD_SEND_UNSEND  /* 清除已读,未读,已发送和未发送短信 */
};

struct SMS {
  uint32_t idx_ = 0;
  std::string msg_; /* the encoded SMS message (max 255 bytes) */
  uint32_t resp_ = 0;
  SMS(const std::string &msg) : msg_(std::move(msg)) {}
};

struct ATChannel;
class SMSManager {
 public:
  SMSManager(uint32_t max_msg_num) : max_msg_num_(max_msg_num) { }
  bool AddMsg(const std::string &msg);
  SMS & DelMsg();

 private:
  std::mutex mutex_;
  std::string sms_center_; /* Max len 16*/
  uint32_t max_msg_num_;
  std::queue<struct SMS> sms_queue_; /* SMS message queue */
  std::list<std::string> white_phones_; /* White phone list to recieve msg */

  char sca_[16];     /* SMSC:Service central number */
  char tpa_[16];     /* Dst or resp number (TP-DA or TP-RA) */
  char tp_pid_;      /* TP-PID:User protocol id */
  char tp_dcs_;      /* TP-DCS:User sms encode */
  char tp_scts_[16]; /* TP_SCTS:Service time stamp */
  char tp_ud_[161];  /* TP-UD:Origin user info */
  char index_;       /* Sms message index */

  SMSMode format_;
  ATChannel *ch_;

  uint32_t Byte2String(char *src, char *dst, uint32_t srcLen);
  uint32_t Encode7Bit(char *src, char *dst, uint32_t srcLen);
  uint32_t Encode8Bit(char *src, char *dst, uint32_t srcLen);
  uint32_t EncodePdu(char *dst);
  uint32_t Decode7Bit(char *src, char *dst, uint32_t srcLen);
  uint32_t Decode8Bit(char *src, char *dst, uint32_t srcLen);
  uint32_t DecodePdu(char *src);
  uint32_t String2Byte(char *src, char *dst, uint32_t srcLen);

  uint32_t InvetNumbers(char *src, char *dst, uint32_t srcLen);
  uint32_t SerializeNumbers(char *src, char *dst, uint32_t srcLen);

  int CheckFragment(const char *msg, const uint32_t len);

  int SendSMSMsg(const char *phone, const char *msg);

  int ATReadSMSThread(char *line1, char *line2, int index, int line2Len,
                      int *smsStatus, int bNewSms);
};
}  // namespace MOBILE
}  // namespace MSF
#endif
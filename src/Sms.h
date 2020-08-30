/**************************************************************************
 *
 * Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
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

#include <functional>
#include <iostream>
#include <list>
#include <mutex>
#include <queue>

namespace mobile {

/**
 * 其他源码参考:
 * https://github.com/nicolas-s/ofono/tree/master/drivers/atmodem
 **/
enum SMSMode { SMS_PDU, SMS_TEXT };

enum SMS_STORAGE_AREA { ME_AREA = 0, SM_AREA };

enum SMS_CLEAN_MODE {
  SMS_CLEAN_READ = 1,         /* 清除已读短信 */
  SMS_CLEAN_READ_SEND,        /* 清除已读和已发送短信 */
  SMS_CLEAN_READ_SEND_UNSEND, /* 清除已读,已发送和未发送短信 */
  SMS_CLEAN_READ_UNREAD_SEND_UNSEND /* 清除已读,未读,已发送和未发送短信 */
};

/**
 * MMSmsState:
 * @SMS_STATE_UNKNOWN: State unknown or not reportable.
 * @SMS_STATE_STORED: The message has been neither received nor yet sent.
 * @SMS_STATE_RECEIVING: The message is being received but is not yet complete.
 * @SMS_STATE_RECEIVED: The message has been completely received.
 * @SMS_STATE_SENDING: The message is queued for delivery.
 * @SMS_STATE_SENT: The message was successfully sent.
 *
 * State of a given SMS.
 *
 * Since: 1.0
 */
typedef enum { /*< underscore_name=sms_state >*/
               SMS_STATE_UNKNOWN = 0,
               SMS_STATE_STORED = 1,
               SMS_STATE_RECEIVING = 2,
               SMS_STATE_RECEIVED = 3,
               SMS_STATE_SENDING = 4,
               SMS_STATE_SENT = 5,
} SmsState;

/**
 * MMSmsStorage:
 * @SMS_STORAGE_UNKNOWN: Storage unknown.
 * @SMS_STORAGE_SM: SIM card storage area.
 * @SMS_STORAGE_ME: Mobile equipment storage area.
 * @SMS_STORAGE_MT: Sum of SIM and Mobile equipment storages
 * @SMS_STORAGE_SR: Status report message storage area.
 * @SMS_STORAGE_BM: Broadcast message storage area.
 * @SMS_STORAGE_TA: Terminal adaptor message storage area.
 *
 * Storage for SMS messages.
 *
 * Since: 1.0
 */
typedef enum { /*< underscore_name=sms_storage >*/
               SMS_STORAGE_UNKNOWN = 0,
               SMS_STORAGE_SM = 1,
               SMS_STORAGE_ME = 2,
               SMS_STORAGE_MT = 3,
               SMS_STORAGE_SR = 4,
               SMS_STORAGE_BM = 5,
               SMS_STORAGE_TA = 6,
} SmsStorage;

struct SMS {
  uint32_t idx_ = 0;
  std::string msg_; /* the encoded SMS message (max 255 bytes) */
  uint32_t resp_ = 0;
  SMS(const std::string &msg) : msg_(std::move(msg)) {}
};

typedef std::function<int(const char *s, const char *ctrl)> WriteLineCallback;

class SMSManager {
 public:
  SMSManager(uint32_t max_msg_num) : max_msg_num_(max_msg_num) {}
  bool AddMsg(const std::string &msg);
  SMS &DelMsg();

  void RegisterWriter(const WriteLineCallback &cb) {
    write_line_ = std::move(cb);
  }

 private:
  std::mutex mutex_;
  std::string sms_center_; /* Max len 16*/
  uint32_t max_msg_num_;
  std::queue<struct SMS> sms_queue_;    /* SMS message queue */
  std::list<std::string> white_phones_; /* White phone list to recieve msg */

  char sca_[16];     /* SMSC:Service central number */
  char tpa_[16];     /* Dst or resp number (TP-DA or TP-RA) */
  char tp_pid_;      /* TP-PID:User protocol id */
  char tp_dcs_;      /* TP-DCS:User sms encode */
  char tp_scts_[16]; /* TP_SCTS:Service time stamp */
  char tp_ud_[161];  /* TP-UD:Origin user info */
  char index_;       /* Sms message index */

  SMSMode format_;
  WriteLineCallback write_line_;

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
}  // namespace mobile
#endif
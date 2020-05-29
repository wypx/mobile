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
#ifndef MOBILE_APP_ATCMD_H
#define MOBILE_APP_ATCMD_H

#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "Mobile.h"

using namespace mobile;

namespace mobile {

static const uint32_t kMaxATCmdLen = 256;

typedef std::function<void(void *)> ModemMatch;

enum AtRequestType {
  REQUEST_RADIO_POWER,
  REQUEST_RESET_RADIO,
  REQUEST_SET_BAND_MODE,
  REQUEST_QUERY_AVAILABLE_BAND_MODE,
  REQUEST_GET_SIM_STATUS,
  REQUEST_SIGNAL_STRENGTH,
  REQUEST_GET_CURRENT_CALLS,
  REQUEST_DIAL,
  REQUEST_DTMF,
  REQUEST_DTMF_START,
  REQUEST_DTMF_STOP,
  REQUEST_OPERATOR,
  REQUEST_VOICE_REGISTRATION_STATE,
  REQUEST_DATA_REGISTRATION_STATE,
  REQUEST_SEND_SMS,
  REQUEST_WRITE_SMS_TO_SIM,
  REQUEST_DELETE_SMS_ON_SIM,
  REQUEST_ANSWER_CALL,
};

struct ATResponse;
typedef std::function<int(const char *, ATResponse **)> SendCmdCb;
typedef std::function<int(const char *, ATResponse **)> SendCmdNumbericCb;
typedef std::function<int(const char *, const char *, ATResponse **)>
    SendCmdSingleLineCb;
typedef std::function<int(const char *, const char *, ATResponse **)>
    SendCmdMultiLineCb;
typedef std::function<ATResponse *()> AllocResponceCb;
typedef std::function<void(ATResponse *)> FreeResponceCb;
typedef std::function<ATCmeError(const ATResponse *)> GetCmeError;

class ATCmdManager {
 public:
  ATCmdManager() = default;
  ~ATCmdManager() {}
  virtual bool initModem();
  virtual bool resetModem();
  virtual bool getModem(ModemMatch cb);
  virtual RadioMode getRadioState();
  virtual bool setRadioState(enum RadioMode cfun);
  virtual bool setChipErrorLevel(const int level);
  virtual int getSIMStatus();
  virtual int getSIMLock();
  virtual int setSIMUnlockPinCode(const std::string &pincode);
  virtual int setNewPinCode(const std::string &oldcode,
                            const std::string &newcode);
  virtual bool setEhrpd();

  virtual int getOperator();
  bool listAllOperators();
  bool listOperatorByNumberic();
  bool setAutoRegister();
  virtual int getNetSearchType();
  virtual int setNetSearchType();
  virtual bool getNetMode();
  virtual int getNet3GPP();

  virtual bool getNetStatus(enum NetWorkType type);

  bool setPDPStatus(uint32_t pdpStatus);
  bool setQosRequestBrief();
  bool getSMSCenter();
  bool setSMSStorageArea();
  bool setSMSFormat();
  bool hasSMS();
  bool readSMS();
  bool clearSMS();

  bool setAPNS();
  bool getAPNS();
  bool clearAPNS();
  bool setAuthInfo();
  virtual int setSignalNotify();
  virtual int getSignal();
  virtual int getLSCellInfo();
  virtual int getClccInfo();
  bool setCallerId();
  bool setTeStatus();

  virtual int setWapen();
  virtual int setDtmf();
  virtual int setAudio();
  virtual int setTTS();
  virtual int answerCall();
  virtual int setCallInfo();
  virtual int getCallInfo();

  void SetSendCommandCb(SendCmdCb send, SendCmdNumbericCb sendNum,
                        SendCmdSingleLineCb sendSLine,
                        SendCmdMultiLineCb sendMLine, AllocResponceCb allocRsp,
                        FreeResponceCb freeRsp, GetCmeError getErr) {
    send_command_numberic_ = send;
    send_command_numberic_ = sendNum;
    send_command_singleline_ = sendSLine;
    send_command_mutiline_ = sendMLine;
    alloc_resp_ = allocRsp;
    free_resp_ = freeRsp;
    get_cme_error_ = getErr;
  }

 private:
  SendCmdCb send_command_;
  SendCmdNumbericCb send_command_numberic_;
  SendCmdSingleLineCb send_command_singleline_;
  SendCmdMultiLineCb send_command_mutiline_;
  AllocResponceCb alloc_resp_;
  FreeResponceCb free_resp_;
  GetCmeError get_cme_error_;
};
}  // namespace mobile
#endif
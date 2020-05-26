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
#ifndef MOBILE_APP_ATCHANNEL_H
#define MOBILE_APP_ATCHANNEL_H

#include <condition_variable>
#include <functional>
#include <iostream>

#include "ATCmd.h"

namespace MSF {
namespace MOBILE {

/**
 * Ref android source:
 * https://android.googlesource.com/platform/hardware/ril/
 * */

#ifdef __cplusplus
extern "C" {
#endif

#define TTY_USB_FORMAT "/dev/ttyUSB%d"

static const std::string kTTyUsbPrefix = "/dev/ttyUSB";
static const std::string kCtrlZ = "\032";
static const std::string kCtrlEnter = "\r";

typedef enum {
  AT_SUCCESS = 0,
  AT_ERROR_GENERIC = 1,
  AT_ERROR_COMMAND_PENDING = 2,
  AT_ERROR_CHANNEL_CLOSED = 3,
  AT_ERROR_TIMEOUT = 4,
  /* AT commands may not be issued from
   * reader thread (or unsolicited response callback */
  AT_ERROR_INVALID_THREAD = 5,
  /* eg an at_send_command_singleline that
   * did not get back an intermediate response */
  AT_ERROR_INVALID_RESPONSE = 6,
  AT_ERROR_UNKNOWN = 7
} ATErrno;

typedef enum {
  NO_RESULT,  /* no intermediate response expected */
  NUMERIC,    /* a single intermediate response starting with a 0-9 */
  SINGLELINE, /* a single intermediate response starting with a prefix */
  MULTILINE   /* multiple line intermediate response
               starting with a prefix */
} ATCmdType;

/** a singly-lined list of intermediate responses */
typedef struct ATLine {
  struct ATLine *next_;
  char *line_;
} ATLine;

/** Free this with at_response_free() */
typedef struct ATResponse {
  int success_;           /* true if final response indicates
                                 success (eg "OK") */
  char *final_resp_;      /* eg OK, ERROR */
  ATLine *intermediates_; /* any intermediate responses */
} ATResponse;

/**
 * a user-provided unsolicited response handler function
 * this will be called from the reader thread, so do not block
 * "s" is the line, and "sms_pdu" is either NULL or the PDU response
 * for multi-line TS 27.005 SMS PDU responses (eg +CMT:)
 */
typedef std::function<void(const char *line, const char *smsPdu)>
    ATUnsolHandler;

/* This callback is invoked on the reader thread (like ATUnsolHandler)
   when the input stream closes before you call at_close
   (not when you call at_close())
   You should still call at_close()
   It may also be invoked immediately from the current thread if the read
   channel is already closed */
typedef std::function<void()> ATReaderClosedHandler;

/* This callback is invoked on the command thread.
   You should reset or handshake here to avoid getting out of sync */
typedef std::function<void()> ATReaderTimeOutHandler;

class ATChannel {
 public:
  ATChannel(ATUnsolHandler h1, ATReaderClosedHandler h2,
            ATReaderTimeOutHandler h3);
  ~ATChannel();

  int HandShake(void);

  int SendCommandSms(const char *cmd, const char *pdu, const char *rspPrefix,
                     ATResponse **outRsp);

  const std::string &ParseATErrno(const ATErrno code) const;
  void ReaderLoop();
  void SenderLoop();

  int WriteLine(const char *s, const char *ctrl);

  /* Use friend class also ok */
  void RegisterATCommandCb(ATCmdManager *acm) {
    if (acm != nullptr) {
      acm->SetSendCommandCb(
          std::bind(&ATChannel::SendCommand, this, std::placeholders::_1,
                    std::placeholders::_2),
          std::bind(&ATChannel::SendCommandNumeric, this, std::placeholders::_1,
                    std::placeholders::_2),
          std::bind(&ATChannel::SendCommandSingleLine, this,
                    std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3),
          std::bind(&ATChannel::SendCommandMultiLine, this,
                    std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3),
          std::bind(&ATChannel::AllocResponce, this),
          std::bind(&ATChannel::FreeResponce, this, std::placeholders::_1),
          std::bind(&ATChannel::GetCmeError, this, std::placeholders::_1));
    }
  }

 private:
  ATUnsolHandler unsol_handler_;
  ATReaderClosedHandler close_handler_;
  ATReaderTimeOutHandler timeout_handler_;

  int read_fd_;
  bool read_closed_;
  std::mutex mutex_; /* Protected current pending command */
  std::condition_variable cond_;
  std::mutex mutex_cond_;
  ATResponse *rsp_;
  const char *rsp_prefix_;
  const char *sms_pdu_;
  ATCmdType type_;

  void ClearPendingCommand();

  int SendCommandNoLock(const char *cmd, ATCmdType type, const char *rspPrefix,
                        const char *smsPdu, uint64_t timeoutMsec,
                        ATResponse **outRsp);

  int SendCommandFull(const char *cmd, ATCmdType type, const char *rspPrefix,
                      const char *smsPdu, uint64_t timeoutMsec,
                      ATResponse **outRsp);

  ATResponse *AllocResponce();
  void FreeResponce(ATResponse *rsp);
  ATCmeError GetCmeError(const ATResponse *rsp);

  int SendCommand(const char *cmd, ATResponse **outRsp);

  int SendCommandSingleLine(const char *cmd, const char *rspPrefix,
                            ATResponse **outRsp);
  int SendCommandNumeric(const char *cmd, ATResponse **outRsp);

  int SendCommandMultiLine(const char *cmd, const char *rspPrefix,
                           ATResponse **outRsp);

  bool IsSMSUnsolicited(const char *line);
  void ProcessUnsolLine(const char *line);

  void HandleUnsolicited(const char *line);

  bool IsFinalResponseSuccess(const char *line);
  bool IsFinalResponse(const char *line);
  void HandleFinalResponse(const char *line);

  bool IsFinalResponseError(const char *line);

  void AddIntermediate(const char *line);
  void ReverseIntermediates(ATResponse *p_response);

  void ProcessLine(const char *line);

  char *ReadLine() const;

  bool ReaderOpen();
  void ReaderClose();
};

#ifdef __cplusplus
}
#endif
}  // namespace MOBILE
}  // namespace MSF
#endif

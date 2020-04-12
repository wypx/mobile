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

#include "AtCmd.h"

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

#define CTRL_Z "\032"
#define CTRL_ENTER "\r"

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
  struct ATLine *p_next;
  char *line;
} ATLine;

/** Free this with at_response_free() */
typedef struct ATResponse {
  int success;             /* true if final response indicates
                                 success (eg "OK") */
  char *finalResponse;     /* eg OK, ERROR */
  ATLine *p_intermediates; /* any intermediate responses */
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

  int handShake(void);

  int sendCommandSms(const char *cmd, const char *pdu, const char *rspPrefix,
                     ATResponse **outRsp);

  const std::string &parseATErrno(const ATErrno code) const;
  void readerLoop();
  void senderLoop();

  int writeLine(const char *s, const char *ctrl);

  /* Use friend class also ok */
  void registSendCmdCb(ATCmdManager *acm) {
    if (acm != nullptr) {
      acm->setSendCommandCb(
          std::bind(&ATChannel::sendCommand, this, std::placeholders::_1,
                    std::placeholders::_2),
          std::bind(&ATChannel::sendCommandNumeric, this, std::placeholders::_1,
                    std::placeholders::_2),
          std::bind(&ATChannel::sendCommandSingleLine, this,
                    std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3),
          std::bind(&ATChannel::sendCommandMultiLine, this,
                    std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3),
          std::bind(&ATChannel::allocResponce, this),
          std::bind(&ATChannel::freeResponce, this, std::placeholders::_1),
          std::bind(&ATChannel::getCmeError, this, std::placeholders::_1));
    }
  }

 private:
  ATUnsolHandler _unsolHandler;
  ATReaderClosedHandler _closeHandler;
  ATReaderTimeOutHandler _timeoutHandler;

  int _readFd;
  bool _readClosed;
  std::mutex _mutex; /* Protected current pending command */
  std::condition_variable _cond;
  std::mutex _mutexCond;
  ATResponse *_rsp;
  const char *_rspPrefix;
  const char *_smsPdu;
  ATCmdType _type;

  void clearPendingCommand();

  int sendCommandNoLock(const char *cmd, ATCmdType type, const char *rspPrefix,
                        const char *smsPdu, uint64_t timeoutMsec,
                        ATResponse **outRsp);

  int sendCommandFull(const char *cmd, ATCmdType type, const char *rspPrefix,
                      const char *smsPdu, uint64_t timeoutMsec,
                      ATResponse **outRsp);

  ATResponse *allocResponce();
  void freeResponce(ATResponse *rsp);
  ATCmeError getCmeError(const ATResponse *rsp);

  int sendCommand(const char *cmd, ATResponse **outRsp);

  int sendCommandSingleLine(const char *cmd, const char *rspPrefix,
                            ATResponse **outRsp);
  int sendCommandNumeric(const char *cmd, ATResponse **outRsp);

  int sendCommandMultiLine(const char *cmd, const char *rspPrefix,
                           ATResponse **outRsp);

  bool isSMSUnsolicited(const char *line);
  void processUnsolLine(const char *line);

  void handleUnsolicited(const char *line);

  bool isFinalResponseSuccess(const char *line);
  bool isFinalResponse(const char *line);
  void handleFinalResponse(const char *line);

  bool isFinalResponseError(const char *line);

  void addIntermediate(const char *line);
  void reverseIntermediates(ATResponse *p_response);

  void processLine(const char *line);

  char *readLine() const;

  bool readerOpen();
  void readerClose();
};

#ifdef __cplusplus
}
#endif
}  // namespace MOBILE
}  // namespace MSF
#endif

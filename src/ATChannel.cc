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
#include <fcntl.h>
#include <mutex>

#include <base/utils.h>
#include <base/gcc_attr.h>
#include <base/serial.h>
#include <base/thread.h>
#include <base/time_utils.h>

#include "ATChannel.h"
#include "ATTok.h"

using namespace MSF;
using namespace Mobile;
namespace Mobile {

#define HANDSHAKE_RETRY_COUNT 8
#define HANDSHAKE_TIMEOUT_MSEC 250

#define MAX_AT_RESPONSE (8 * 1024)
static char kATBuffer[MAX_AT_RESPONSE + 1];
static char *kATBufferCur;

/** add an intermediate response to sp_response*/
void ATChannel::AddIntermediate(const char *line) {
  ATLine *p_new = (ATLine *)malloc(sizeof(ATLine));

  if (!p_new) return;

  p_new->line_ = strdup(line);

  /* note: this adds to the head of the list, so the list
     will be in reverse order of lines received. the order is flipped
     again before passing on to the command issuer */
  p_new->next_ = rsp_->intermediates_;
  rsp_->intermediates_ = p_new;
}

/**
 * returns 1 if line is a final response indicating error
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static const char *kFinalResponsesError[] = {
    "ERROR",     "+CMS ERROR:", "+CME ERROR:", "NO CARRIER", /* sometimes! */
    "NO ANSWER", "NO DIALTONE", };
bool ATChannel::IsFinalResponseError(const char *line) {
  for (uint32_t i = 0; i < MSF_ARRAY_SIZE(kFinalResponsesError); ++i) {
    if (AtStrStartWith(line, kFinalResponsesError[i])) {
      return true;
    }
  }
  return false;
}

/**
 * returns 1 if line is a final response indicating success
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static const char *kFinalResponsesSuccess[] = {
    "OK", "CONNECT" /* some stacks start up data on another channel */
};

bool ATChannel::IsFinalResponseSuccess(const char *line) {
  for (uint32_t i = 0; i < MSF_ARRAY_SIZE(kFinalResponsesSuccess); ++i) {
    if (AtStrStartWith(line, kFinalResponsesSuccess[i])) {
      return true;
    }
  }
  return false;
}

/**
 * returns 1 if line is a final response, either  error or success
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
bool ATChannel::IsFinalResponse(const char *line) {
  return IsFinalResponseSuccess(line) || IsFinalResponseError(line);
}

/** assumes s_commandmutex is held */
void ATChannel::HandleFinalResponse(const char *line) {
  rsp_->final_resp_ = strdup(line);
  cond_.notify_one();
}

void ATChannel::HandleUnsolicited(const char *line) {
  if (unsol_handler_ != nullptr) {
    unsol_handler_(line, nullptr);
  }
}

/**
 * The line reader places the intermediate responses in reverse order
 * here we flip them back
 */
void ATChannel::ReverseIntermediates(ATResponse *p_response) {
  ATLine *pcur, *pnext;

  pcur = p_response->intermediates_;
  p_response->intermediates_ = nullptr;

  while (pcur != nullptr) {
    pnext = pcur->next_;
    pcur->next_ = p_response->intermediates_;
    p_response->intermediates_ = pcur;
    pcur = pnext;
  }
}

void ATChannel::ClearPendingCommand() {
  FreeResponce(rsp_);
  rsp_ = nullptr;
  rsp_prefix_ = nullptr;
  sms_pdu_ = nullptr;
}

/**
 * Internal send_command implementation
 * Doesn't lock or call the timeout callback
 *
 * timeoutMsec == 0 means infinite timeout
 */
int ATChannel::SendCommandNoLock(const char *cmd, ATCmdType type,
                                 const char *rspPrefix, const char *smsPdu,
                                 uint64_t timeoutMsec, ATResponse **outRsp) {
  int ret = 0;
  /* clean last responce */
  if (unlikely(rsp_ != nullptr)) {
    ret = AT_ERROR_COMMAND_PENDING;
    ClearPendingCommand();
    return ret;
  }

  ret = WriteLine(cmd, kCtrlEnter.c_str());
  if (ret < 0) {
    ClearPendingCommand();
    return ret;
  }

  type_ = type;
  rsp_prefix_ = rspPrefix;
  sms_pdu_ = smsPdu;
  rsp_ = AllocResponce();

  std::unique_lock<std::mutex> lock(mutex_cond_);
  while (rsp_->final_resp_ == nullptr && !read_closed_) {
    if (timeoutMsec != 0) {
      // https://zh.cppreference.com/w/cpp/thread/condition_variable/wait_for
      using namespace std::chrono_literals;
      cond_.wait_for(lock, timeoutMsec * 1ms);
    } else {
      cond_.wait(lock);
    }

    if (errno == ETIMEDOUT) {
      ret = AT_ERROR_TIMEOUT;
      ClearPendingCommand();
      return ret;
    }
  }

  if (outRsp == nullptr) {
    FreeResponce(rsp_);
  } else {
    /* line reader stores intermediate responses in reverse order */
    ReverseIntermediates(rsp_);
    *outRsp = rsp_;
  }

  rsp_ = nullptr;

  if (unlikely(read_closed_)) {
    ret = AT_ERROR_CHANNEL_CLOSED;
    ClearPendingCommand();
    return ret;
  }
  ClearPendingCommand();
  return 0;
}

/**
 * Internal send_command implementation
 *
 * timeoutMsec == 0 means infinite timeout
 */
int ATChannel::SendCommandFull(const char *cmd, ATCmdType type,
                               const char *rspPrefix, const char *smsPdu,
                               uint64_t timeoutMsec, ATResponse **outRsp) {
  int ret;

  // if (0 != pthread_equal(s_tid_reader, pthread_self())) {
  //     /* cannot be called from reader thread */
  //     return AT_ERROR_INVALID_THREAD;
  // }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    ret = SendCommandNoLock(cmd, type, rspPrefix, smsPdu, timeoutMsec, outRsp);
  }

  if (ret == AT_ERROR_TIMEOUT && timeout_handler_ != nullptr) {
    timeout_handler_();
  }

  return ret;
}

/**
 * Issue a single normal AT command with no intermediate response expected
 *
 * "command" should not include \r
 * pp_outResponse can be NULL
 *
 * if non-NULL, the resulting ATResponse * must be eventually freed with
 * at_response_free
 */
int ATChannel::SendCommand(const char *cmd, ATResponse **outRsp) {
  return SendCommandFull(cmd, NO_RESULT, nullptr, nullptr, 0, outRsp);
}

int ATChannel::SendCommandSingleLine(const char *cmd, const char *rspPrefix,
                                     ATResponse **outRsp) {
  int ret;

  ret = SendCommandFull(cmd, SINGLELINE, rspPrefix, nullptr, 0, outRsp);
  if (ret == 0 && outRsp != nullptr && (*outRsp)->success_ > 0 &&
      (*outRsp)->intermediates_ == nullptr) {
    /* successful command must have an intermediate response */
    FreeResponce(*outRsp);
    *outRsp = nullptr;
    return AT_ERROR_INVALID_RESPONSE;
  }

  return ret;
}

int ATChannel::SendCommandNumeric(const char *cmd, ATResponse **outRsp) {
  int ret;

  ret = SendCommandFull(cmd, NUMERIC, nullptr, nullptr, 0, outRsp);
  if (ret == 0 && outRsp != nullptr && (*outRsp)->success_ > 0 &&
      (*outRsp)->intermediates_ == nullptr) {
    /* successful command must have an intermediate response */
    FreeResponce(*outRsp);
    *outRsp = nullptr;
    return AT_ERROR_INVALID_RESPONSE;
  }

  return ret;
}

int ATChannel::SendCommandMultiLine(const char *cmd, const char *rspPrefix,
                                    ATResponse **outRsp) {
  return SendCommandFull(cmd, MULTILINE, rspPrefix, nullptr, 0, outRsp);
}

int ATChannel::SendCommandSms(const char *cmd, const char *pdu,
                              const char *rspPrefix, ATResponse **outRsp) {
  int ret;

  ret = SendCommandFull(cmd, SINGLELINE, rspPrefix, pdu, 0, outRsp);
  if (ret == AT_SUCCESS && outRsp != nullptr && (*outRsp)->success_ > 0 &&
      (*outRsp)->intermediates_ == nullptr) {
    /* successful command must have an intermediate response */
    FreeResponce(*outRsp);
    *outRsp = nullptr;
    return AT_ERROR_INVALID_RESPONSE;
  }

  return ret;
}

/**
 * Periodically issue an AT command and wait for a response.
 * Used to ensure channel has start up and is active
 */
int ATChannel::HandShake(void) {
  // if (0 != pthread_equal(MSF::CurrentThread::tid(), pthread_self())) {
  //     /* cannot be called from reader thread */
  //     return AT_ERROR_INVALID_THREAD;
  // }
  int ret;
  for (int i = 0; i < HANDSHAKE_RETRY_COUNT; ++i) {
    /* some stacks start with verbose off */
    ret = SendCommandFull("ATE0Q0V1", NO_RESULT, nullptr, nullptr,
                          HANDSHAKE_TIMEOUT_MSEC, nullptr);
    if (ret == 0) {
      break;
    }
  }

  if (ret == 0) {
    /* pause for a bit to let the input buffer drain any unmatched OK's
       (they will appear as extraneous unsolicited responses) */
    SleepMsec(HANDSHAKE_TIMEOUT_MSEC);
  }
  return 0;
}

ATChannel::ATChannel(ATUnsolHandler h1, ATReaderClosedHandler h2,
                     ATReaderTimeOutHandler h3, ATInstallEventHandler h4)
    : unsol_handler_(std::move(h1)),
      close_handler_(std::move(h2)),
      timeout_handler_(std::move(h3)),
      install_at_handler_(std::move(h4)),
      read_closed_(false),
      mutex_(),
      mutex_cond_(),
      rsp_(nullptr),
      rsp_prefix_(nullptr),
      sms_pdu_(nullptr) {
  kATBufferCur = kATBuffer;
}

// EventLoop *atcmd_loop_;

ATChannel::~ATChannel() { ReaderClose(); }

void ATChannel::Initialize() {
  if (ReaderOpen()) {
    install_at_handler_(read_fd_);
    // loop_->runInLoop(std::bind(&ATChannel::ReaderLoop, this));
  } else {
    // loop_->runInLoop(std::bind(&ATChannel::Initialize, this));
  }
}

/**
 * Returns error code from response
 * Assumes AT+CMEE=1 (numeric) mode
 */
ATCmeError ATChannel::GetCmeError(const ATResponse *rsp) {
  int ret;
  int err;
  char *p_cur;

  if (rsp->success_ > 0) {
    return CME_SUCCESS;
  }

  if (rsp->final_resp_ == nullptr ||
      !AtStrStartWith(rsp->final_resp_, "+CME ERROR:")) {
    return CME_ERROR_NON_CME;
  }

  p_cur = rsp->final_resp_;
  err = AtTokStart(&p_cur);
  if (err < 0) {
    return CME_ERROR_NON_CME;
  }

  err = AtTokNextInt(&p_cur, &ret);
  if (err < 0) {
    return CME_ERROR_NON_CME;
  }

  return static_cast<ATCmeError>(ret);
}

ATResponse *ATChannel::AllocResponce() {
  return (ATResponse *)calloc(1, sizeof(ATResponse));
}

void ATChannel::FreeResponce(ATResponse *rsp) {
  if (unlikely(rsp == nullptr)) return;

  ATLine *p_line = rsp->intermediates_;

  while (p_line != nullptr) {
    ATLine *p_toFree;

    p_toFree = p_line;
    p_line = p_line->next_;

    free(p_toFree->line_);
    free(p_toFree);
  }

  free(rsp->final_resp_);
  free(rsp);
}

/**
 * Sends string s to the radio with a \r appended.
 * Returns AT_ERROR_* on error, 0 on success
 *
 * This function exists because as of writing, android libc does not
 * have buffered stdio.
 */
int ATChannel::WriteLine(const char *s, const char *ctrl) {
  size_t cur = 0;
  size_t len = strlen(s);
  ssize_t written;

  if (unlikely(read_fd_ < 0 || read_closed_)) {
    LOG(ERROR) << "AT channel has beed closed.";
    return AT_ERROR_CHANNEL_CLOSED;
  }

  LOG(DEBUG) << ">> " << s;

  /* the main string */
  while (cur < len) {
    do {
      written = write(read_fd_, s + cur, len - cur);
    } while (written < 0 && errno == EINTR);

    if (written < 0) {
      return AT_ERROR_GENERIC;
    }
    cur += written;
  }

  /* the \r  or ctrl z*/
  do {
    written = write(read_fd_, ctrl, 1);
  } while ((written < 0 && errno == EINTR) || (written == 0));

  if (written < 0) {
    return AT_ERROR_GENERIC;
  }
  return AT_SUCCESS;
}

const std::string &ATChannel::ParseATErrno(const ATErrno code) const {
  static std::string kATErrStr[] = {
      "AT_SUCCESS",               "AT_ERROR_GENERIC",
      "AT_ERROR_COMMAND_PENDING", "AT_ERROR_CHANNEL_CLOSED",
      "AT_ERROR_TIMEOUT",         "AT_ERROR_INVALID_THREAD",
      "AT_ERROR_INVALID_RESPONSE"};
  return kATErrStr[code];
}

bool ATChannel::ReaderOpen() {
  read_fd_ = open("/dev/ttyUSB2", O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (read_fd_ < 0) {
    switch (errno) {
      case EINTR:
        // Recurse because this is a recoverable error.
        ReaderOpen();
        break;
      case ENFILE:
      case EMFILE:
        LOG(ERROR) << "Too many file handles open.";
      default:
        break;
    }
    return false;
  }

  if (SetSerialBaud(read_fd_, B115200) != 0) {
    close(read_fd_);
    read_fd_ = -1;
    return false;
  }

  if (SetSerialRawMode(read_fd_) != 0) {
    close(read_fd_);
    read_fd_ = -1;
    return false;
  }

  return true;
}

/**
 *  This callback is invoked on the reader thread (like ATUnsolHandler)
 *  when the input stream closes before you call at_close
 *  (not when you call at_close())
 *  You should still call at_close()
 */
/* FIXME is it ok to call this from the reader and the command thread? */
void ATChannel::ReaderClose() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!read_closed_) {
    if (read_fd_ > 0) {
      close(read_fd_);
      read_fd_ = -1;
    }

    read_closed_ = true;
    cond_.notify_one();
    /* the reader thread should eventually die */

    if (close_handler_ != nullptr) {
      close_handler_();
    }
  }
}

/**
 * returns 1 if line is the first line in (what will be) a two-line
 * SMS unsolicited response
 */
static const char *kSsmsUnsoliciteds[] = {"+CMT:", "+CDS:", "+CBM:"};

bool ATChannel::IsSMSUnsolicited(const char *line) {
  for (uint32_t i = 0; i < MSF_ARRAY_SIZE(kSsmsUnsoliciteds); ++i) {
    if (AtStrStartWith(line, kSsmsUnsoliciteds[i])) {
      return true;
    }
  }
  return false;
}

void ATChannel::ProcessUnsolLine(const char *line) {
  char *line1;
  const char *line2;

  // The scope of string returned by 'readline()' is valid only
  // till next call to 'readline()' hence making a copy of line
  // before calling readline again.
  line1 = strdup(line);
  line2 = ReadLine();

  if (unlikely(line2 == nullptr)) {
    LOG(FATAL) << "Read channel got some fatal error: " << errno;
    free(line1);
    ReaderClose();
    return;
  }

  if (unsol_handler_ != nullptr) {
    unsol_handler_(line1, line2);
  }
  free(line1);
}

void ATChannel::ProcessLine(const char *line) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (rsp_ == nullptr) {
    /* no command pending */
    HandleUnsolicited(line);
  } else if (IsFinalResponseSuccess(line)) {
    rsp_->success_ = true;
    HandleFinalResponse(line);
  } else if (IsFinalResponseError(line)) {
    rsp_->success_ = false;
    HandleFinalResponse(line);
  } else if (sms_pdu_ != nullptr && 0 == strcmp(line, "> ")) {
    // See eg. TS 27.005 4.3
    // Commands like AT+CMGS have a "> " prompt
    WriteLine(sms_pdu_, kCtrlZ.c_str());
    sms_pdu_ = nullptr;
  } else
    switch (type_) {
      case NO_RESULT:
        HandleUnsolicited(line);
        break;
      case NUMERIC:
        if (rsp_->intermediates_ == nullptr && isdigit(line[0])) {
          AddIntermediate(line);
        } else {
          /* either we already have an intermediate response or
             the line doesn't begin with a digit */
          HandleUnsolicited(line);
        }
        break;
      case SINGLELINE:
        if (rsp_->intermediates_ == NULL && AtStrStartWith(line, rsp_prefix_)) {
          AddIntermediate(line);
        } else {
          /* we already have an intermediate response */
          HandleUnsolicited(line);
        }
        break;
      case MULTILINE:
        if (AtStrStartWith(line, rsp_prefix_)) {
          AddIntermediate(line);
        } else {
          HandleUnsolicited(line);
        }
        break;

      default: /* this should never be reached */
        LOG(ERROR) << "Unsupported AT command type: " << type_;
        HandleUnsolicited(line);
        break;
    }
}

/**
 * Reads a line from the AT channel, returns NULL on timeout.
 * Assumes it has exclusive read access to the FD
 *
 * This line is valid only until the next call to readline
 *
 * This function exists because as of writing, android libc does not
 * have buffered stdio.
 */
char *ATChannel::ReadLine() const {
  ssize_t count;
  char *p_read = nullptr;
  char *p_eol = nullptr;
  char *ret;

  /* this is a little odd. I use *skATBufferCur == 0 to
   * mean "buffer consumed completely". If it points to a character, than
   * the buffer continues until a \0 */
  if (*kATBufferCur == '\0') {
    /* empty buffer */
    kATBufferCur = kATBuffer;
    *kATBufferCur = '\0';
    p_read = kATBuffer;
  } else {/* *skATBufferCur != '\0' */
    /* there's data in the buffer from the last read */

    // skip over leading newlines
    while (*kATBufferCur == '\r' || *kATBufferCur == '\n') kATBufferCur++;

    p_eol = AtFindNextEOL(kATBufferCur);
    if (p_eol == nullptr) {
      /* a partial line. move it up and prepare to read more */
      size_t len;

      len = strlen(kATBufferCur);

      memmove(kATBuffer, kATBufferCur, len + 1);
      p_read = kATBuffer + len;
      kATBufferCur = kATBuffer;
    }
    /* Otherwise, (p_eol !- NULL) there is a complete line  */
    /* that will be returned the while () loop below        */
  }

  while (p_eol == nullptr) {
    if (0 == MAX_AT_RESPONSE - (p_read - kATBuffer)) {
      LOG(ERROR) << "Input line exceeded buffer";
      /* ditch buffer and start over again */
      kATBufferCur = kATBuffer;
      *kATBufferCur = '\0';
      p_read = kATBuffer;
    }

    do {
      count = read(read_fd_, p_read, MAX_AT_RESPONSE - (p_read - kATBuffer));
    } while (count < 0 && errno == EINTR);

    if (count > 0) {
      // AT_DUMP( "<< ", p_read, count );

      p_read[count] = '\0';
      // skip over leading newlines
      while (*kATBufferCur == '\r' || *kATBufferCur == '\n') kATBufferCur++;

      p_eol = AtFindNextEOL(kATBufferCur);
      p_read += count;
    } else if (count <= 0) {
      /* read error encountered or EOF reached */
      if (count == 0) {
        LOG(ERROR) << "ATChannel: EOF reached";
      } else {
        LOG(ERROR) << "ATChannel: read error :" << strerror(errno);
      }
      return nullptr;
    }
  }

  /* a full line in the buffer. Place a \0 over the \r and return */
  ret = kATBufferCur;
  *p_eol = '\0';
  kATBufferCur = p_eol + 1; /* this will always be <= p_read,    */
                            /* and there will be a \0 at *p_read */

  // MSF_MOBILE_LOG(DBG_DEBUG,"AT< %s\n", ret);
  return ret;
}

void ATChannel::ReaderLoop() {
  const char *line = ReadLine();
  if (unlikely(line == nullptr)) {
    LOG(FATAL) << "Read channel got some fatal error: " << errno;
    ReaderClose();
  }

  if (unlikely(IsSMSUnsolicited(line))) {
    ProcessUnsolLine(line);
  } else {
    ProcessLine(line);
  }
}

}  // namespace Mobile

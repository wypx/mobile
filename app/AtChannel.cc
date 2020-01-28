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
#include <base/Utils.h>
#include <base/Time.h>
#include <base/Serial.h>
#include <base/Thread.h>
#include <base/GccAttr.h>

#include "AtTok.h"
#include "ATChannel.h"

#include <fcntl.h>
#include <mutex>


using namespace MSF::BASE;
using namespace MSF::TIME;
using namespace MSF::MOBILE;

namespace MSF {
namespace MOBILE {

#define HANDSHAKE_RETRY_COUNT 8
#define HANDSHAKE_TIMEOUT_MSEC 250

#define MAX_AT_RESPONSE (8 * 1024)
static char _ATBuffer[MAX_AT_RESPONSE+1];
static char *_ATBufferCur;

/** add an intermediate response to sp_response*/
void ATChannel::addIntermediate(const char *line)
{
    ATLine *p_new = (ATLine *) malloc(sizeof(ATLine));

    if (!p_new) return;

    p_new->line = strdup(line);

    /* note: this adds to the head of the list, so the list
       will be in reverse order of lines received. the order is flipped
       again before passing on to the command issuer */
    p_new->p_next = _rsp->p_intermediates;
    _rsp->p_intermediates = p_new;
}


/**
 * returns 1 if line is a final response indicating error
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static const char * g_finalResponsesError[] = {
    "ERROR",
    "+CMS ERROR:",
    "+CME ERROR:",
    "NO CARRIER", /* sometimes! */
    "NO ANSWER",
    "NO DIALTONE",
};
bool ATChannel::isFinalResponseError(const char *line)
{
    for (size_t i = 0 ; i < MSF_ARRAY_SIZE(g_finalResponsesError) ; ++i) {
        if (AtStrStartWith(line, g_finalResponsesError[i])) {
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
static const char * g_finalResponsesSuccess[] = {
    "OK",
    "CONNECT"       /* some stacks start up data on another channel */
};

bool ATChannel::isFinalResponseSuccess(const char *line)
{
    for (size_t i = 0 ; i < MSF_ARRAY_SIZE(g_finalResponsesSuccess) ; ++i) {
        if (AtStrStartWith(line, g_finalResponsesSuccess[i])) {
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
bool ATChannel::isFinalResponse(const char *line)
{
    return isFinalResponseSuccess(line) || isFinalResponseError(line);
}

/** assumes s_commandmutex is held */
void ATChannel::handleFinalResponse(const char *line)
{
    _rsp->finalResponse = strdup(line);
    _cond.notify_one();
}

void ATChannel::handleUnsolicited(const char *line)
{
    if (_unsolHandler != nullptr) {
        _unsolHandler(line, nullptr);
    }
}

/**
 * The line reader places the intermediate responses in reverse order
 * here we flip them back
 */
void ATChannel::reverseIntermediates(ATResponse *p_response)
{
    ATLine *pcur,*pnext;

    pcur = p_response->p_intermediates;
    p_response->p_intermediates = nullptr;

    while (pcur != nullptr) {
        pnext = pcur->p_next;
        pcur->p_next = p_response->p_intermediates;
        p_response->p_intermediates = pcur;
        pcur = pnext;
    }
}


void ATChannel::clearPendingCommand()
{
    freeResponce(_rsp);
    _rsp = nullptr;
    _rspPrefix = nullptr;
    _smsPdu = nullptr;
}

/**
 * Internal send_command implementation
 * Doesn't lock or call the timeout callback
 *
 * timeoutMsec == 0 means infinite timeout
 */
int ATChannel::sendCommandNoLock(const char *cmd, ATCmdType type,
                    const char *rspPrefix, const char *smsPdu,
                    uint64_t timeoutMsec, ATResponse **outRsp)
{
    int ret = 0;
    /* clean last responce */
    if (unlikely(_rsp != nullptr)) {
        ret = AT_ERROR_COMMAND_PENDING;
        clearPendingCommand();
        return ret;
    }

    ret = writeLine(cmd, CTRL_ENTER);
    if (ret < 0) {
        clearPendingCommand();
        return ret;
    }

    _type = type;
    _rspPrefix = rspPrefix;
    _smsPdu = smsPdu;
    _rsp = allocResponce();

    std::unique_lock<std::mutex> lock(_mutexCond);
    while (_rsp->finalResponse == nullptr && !_readClosed) {
        if (timeoutMsec != 0) {
            //https://zh.cppreference.com/w/cpp/thread/condition_variable/wait_for
            using namespace std::chrono_literals;
            _cond.wait_for(lock, timeoutMsec * 1ms);
        } else {
             _cond.wait(lock);
        }

        if (errno == ETIMEDOUT) {
            ret = AT_ERROR_TIMEOUT;
            clearPendingCommand();
            return ret;
        }
    }

    if (outRsp == nullptr) {
        freeResponce(_rsp);
    } else {
        /* line reader stores intermediate responses in reverse order */
        reverseIntermediates(_rsp);
        *outRsp = _rsp;
    }

    _rsp = nullptr;

    if (unlikely(_readClosed)) {
        ret = AT_ERROR_CHANNEL_CLOSED;
        clearPendingCommand();
        return ret;
    }
    clearPendingCommand();
    return 0;
}

/**
 * Internal send_command implementation
 *
 * timeoutMsec == 0 means infinite timeout
 */
int ATChannel::sendCommandFull(const char *cmd, ATCmdType type,
                    const char *rspPrefix, const char *smsPdu,
                    uint64_t timeoutMsec, ATResponse **outRsp)
{
    int ret;

    // if (0 != pthread_equal(s_tid_reader, pthread_self())) {
    //     /* cannot be called from reader thread */
    //     return AT_ERROR_INVALID_THREAD;
    // }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        ret = sendCommandNoLock(cmd, type,
                    rspPrefix, smsPdu,
                    timeoutMsec, outRsp);
    }

    if (ret == AT_ERROR_TIMEOUT && _timeoutHandler != nullptr) {
        _timeoutHandler();
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
int ATChannel::sendCommand(const char *cmd, ATResponse **outRsp)
{
    return sendCommandFull(cmd, NO_RESULT, nullptr, nullptr, 0, outRsp);
}

int ATChannel::sendCommandSingleLine(const char *cmd, 
                                const char *rspPrefix,
                                ATResponse **outRsp)
{
    int ret;

    ret = sendCommandFull(cmd, SINGLELINE, rspPrefix, nullptr, 0, outRsp);
    if (ret == 0 && outRsp != nullptr
        && (*outRsp)->success > 0
        && (*outRsp)->p_intermediates == nullptr) {
        /* successful command must have an intermediate response */
        freeResponce(*outRsp);
        *outRsp = nullptr;
        return AT_ERROR_INVALID_RESPONSE;
    }

    return ret;
}

int ATChannel::sendCommandNumeric(const char *cmd, ATResponse **outRsp)
{
    int ret;

    ret = sendCommandFull(cmd, NUMERIC, nullptr, nullptr, 0, outRsp);
    if (ret == 0 && outRsp != nullptr
        && (*outRsp)->success > 0
        && (*outRsp)->p_intermediates == nullptr
    ) {
        /* successful command must have an intermediate response */
        freeResponce(*outRsp);
        *outRsp = nullptr;
        return AT_ERROR_INVALID_RESPONSE;
    }

    return ret;
}


int ATChannel::sendCommandMultiLine(const char *cmd,
                                const char *rspPrefix,
                                ATResponse **outRsp)
{
    return sendCommandFull (cmd, MULTILINE, rspPrefix, nullptr, 0, outRsp);
}

int ATChannel::sendCommandSms(const char *cmd, const char *pdu,
                            const char *rspPrefix, ATResponse **outRsp)
{
    int ret;

    ret = sendCommandFull(cmd, SINGLELINE, rspPrefix, pdu, 0, outRsp);
    if (ret == AT_SUCCESS && outRsp != nullptr
        && (*outRsp)->success > 0
        && (*outRsp)->p_intermediates == nullptr) {
        /* successful command must have an intermediate response */
        freeResponce(*outRsp);
        *outRsp = nullptr;
        return AT_ERROR_INVALID_RESPONSE;
    }

    return ret;
}

/**
 * Periodically issue an AT command and wait for a response.
 * Used to ensure channel has start up and is active
 */
int ATChannel::handShake(void)
{
    // if (0 != pthread_equal(MSF::CurrentThread::tid(), pthread_self())) {
    //     /* cannot be called from reader thread */
    //     return AT_ERROR_INVALID_THREAD;
    // }
    int ret;
    for (int i = 0; i < HANDSHAKE_RETRY_COUNT; ++i) {
        /* some stacks start with verbose off */
        ret = sendCommandFull ("ATE0Q0V1", NO_RESULT,
                    nullptr, nullptr, HANDSHAKE_TIMEOUT_MSEC, nullptr);
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

ATChannel::ATChannel(ATUnsolHandler h1, ATReaderClosedHandler h2, ATReaderTimeOutHandler h3)
         :  _unsolHandler(std::move(h1)),
            _closeHandler(std::move(h2)),
            _timeoutHandler(std::move(h3)),
            _readClosed(false),
            _mutex(),
            _mutexCond(),
            _rsp(nullptr),
            _rspPrefix(nullptr),
            _smsPdu(nullptr)
{
    _ATBufferCur = _ATBuffer;
}

ATChannel::~ATChannel()
{
    readerClose();
}

/**
 * Returns error code from response
 * Assumes AT+CMEE=1 (numeric) mode
 */
ATCmeError ATChannel::getCmeError(const ATResponse *rsp)
{
    int ret;
    int err;
    char *p_cur;

    if (rsp->success > 0) {
        return CME_SUCCESS;
    }

    if (rsp->finalResponse == nullptr
        || !AtStrStartWith(rsp->finalResponse, "+CME ERROR:")) {
        return CME_ERROR_NON_CME;
    }

    p_cur = rsp->finalResponse;
    err = AtTokStart(&p_cur);
    if (err < 0) {
        return CME_ERROR_NON_CME;
    }

    err = AtTokNextInt(&p_cur, &ret);
    if (err < 0) {
        return CME_ERROR_NON_CME;
    }

    return (ATCmeError)ret;
}

ATResponse* ATChannel::allocResponce()
{
    return (ATResponse *) calloc(1, sizeof(ATResponse));
}

void ATChannel::freeResponce(ATResponse *rsp)
{
    if (unlikely(rsp == nullptr)) return;

    ATLine *p_line = rsp->p_intermediates;

    while (p_line != nullptr) {
        ATLine *p_toFree;

        p_toFree = p_line;
        p_line = p_line->p_next;

        free(p_toFree->line);
        free(p_toFree);
    }

    free (rsp->finalResponse);
    free (rsp);
}


/**
 * Sends string s to the radio with a \r appended.
 * Returns AT_ERROR_* on error, 0 on success
 *
 * This function exists because as of writing, android libc does not
 * have buffered stdio.
 */
int ATChannel::writeLine(const char *s, const char *ctrl)
{
    size_t cur = 0;
    size_t len = strlen(s);
    ssize_t written;

    if (unlikely(_readFd < 0 || _readClosed)) { 
        MSF_ERROR << "AT channel has beed closed.";
        return AT_ERROR_CHANNEL_CLOSED;
    } 

    MSF_DEBUG << ">> " << s;

    /* the main string */
    while (cur < len) {
        do {
            written = write (_readFd, s + cur, len - cur);
        } while (written < 0 && errno == EINTR);

        if (written < 0) {
            return AT_ERROR_GENERIC;
        }
        cur += written;
    }

    /* the \r  or ctrl z*/
    do {
        written = write (_readFd, ctrl , 1);
    } while ((written < 0 && errno == EINTR) || (written == 0));

    if (written < 0) {
        return AT_ERROR_GENERIC;
    }
    return AT_SUCCESS;
}

const std::string & ATChannel::parseATErrno(const ATErrno code) const
{
    static std::string _g_ATErrStr[] = {
        "AT_SUCCESS",
        "AT_ERROR_GENERIC",
        "AT_ERROR_COMMAND_PENDING",
        "AT_ERROR_CHANNEL_CLOSED",
        "AT_ERROR_TIMEOUT",
        "AT_ERROR_INVALID_THREAD",
        "AT_ERROR_INVALID_RESPONSE"
    };
    return _g_ATErrStr[code];
}

bool ATChannel::readerOpen()
{
    _readFd = open("/dev/ttyUSB2", O_RDWR);
    if (_readFd < 0) {
        return false;
    }
    
    if (setSerialBaud(_readFd, B115200) != 0) {
        close(_readFd);
        _readFd = -1;
        return false;
    }

    if (setSerialRawMode(_readFd) != 0) {
        close(_readFd);
        _readFd = -1;
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
void ATChannel::readerClose()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_readClosed) {
        if (_readFd > 0) {
            close(_readFd);
            _readFd = -1;
        }

        _readClosed = true;
        _cond.notify_one();
        /* the reader thread should eventually die */

        if (_closeHandler != nullptr) {
            _closeHandler();
        }
    }
    
}

/**
 * returns 1 if line is the first line in (what will be) a two-line
 * SMS unsolicited response
 */
static const char * g_smsUnsoliciteds[] = {
    "+CMT:",
    "+CDS:",
    "+CBM:"
};

bool ATChannel::isSMSUnsolicited(const char *line)
{
    for (size_t i = 0 ; i < MSF_ARRAY_SIZE(g_smsUnsoliciteds) ; ++i) {
        if (AtStrStartWith(line, g_smsUnsoliciteds[i])) {
            return true;
        }
    }
    return false;
}

void ATChannel::processUnsolLine(const char *line)
{
    char *line1;
    const char *line2;

    // The scope of string returned by 'readline()' is valid only
    // till next call to 'readline()' hence making a copy of line
    // before calling readline again.
    line1 = strdup(line);
    line2 = readLine();

    if (unlikely(line2 == nullptr)) {
        MSF_FATAL << "Read channel got some fatal error: " << errno;
        free(line1);
        readerClose();
        return;
    }

    if (_unsolHandler != nullptr) {
        _unsolHandler(line1, line2);
    }
    free(line1);
}

void ATChannel::processLine(const char *line)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_rsp == nullptr) {
        /* no command pending */
        handleUnsolicited(line);
    } else if (isFinalResponseSuccess(line)) {
        _rsp->success = true;
        handleFinalResponse(line);
    } else if (isFinalResponseError(line)) {
        _rsp->success = false;
        handleFinalResponse(line);
    } else if (_smsPdu != nullptr && 0 == strcmp(line, "> ")) {
        // See eg. TS 27.005 4.3
        // Commands like AT+CMGS have a "> " prompt
        writeLine(_smsPdu, CTRL_Z);
        _smsPdu = nullptr;
    } else switch (_type) {
        case NO_RESULT:
            handleUnsolicited(line);
            break;
        case NUMERIC:
            if (_rsp->p_intermediates == nullptr
                && isdigit(line[0])) {
                addIntermediate(line);
            } else {
                /* either we already have an intermediate response or
                   the line doesn't begin with a digit */
                handleUnsolicited(line);
            }
            break;
        case SINGLELINE:
            if (_rsp->p_intermediates == NULL
                && AtStrStartWith(line, _rspPrefix)
            ) {
                addIntermediate(line);
            } else {
                /* we already have an intermediate response */
                handleUnsolicited(line);
            }
            break;
        case MULTILINE:
            if (AtStrStartWith(line, _rspPrefix)) {
                addIntermediate(line);
            } else {
                handleUnsolicited(line);
            }
        break;

        default: /* this should never be reached */
            MSF_ERROR << "Unsupported AT command type: " << _type;
            handleUnsolicited(line);
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
char * ATChannel::readLine() const
{
    ssize_t count;
    char *p_read = nullptr;
    char *p_eol = nullptr;
    char *ret;

    /* this is a little odd. I use *s_ATBufferCur == 0 to
     * mean "buffer consumed completely". If it points to a character, than
     * the buffer continues until a \0 */
    if (*_ATBufferCur == '\0') {
        /* empty buffer */
        _ATBufferCur = _ATBuffer;
        *_ATBufferCur = '\0';
        p_read = _ATBuffer;
    } else {   /* *s_ATBufferCur != '\0' */
        /* there's data in the buffer from the last read */

        // skip over leading newlines
        while (*_ATBufferCur == '\r' || *_ATBufferCur == '\n')
            _ATBufferCur++;

        p_eol = AtFindNextEOL(_ATBufferCur);
        if (p_eol == nullptr) {
            /* a partial line. move it up and prepare to read more */
            size_t len;

            len = strlen(_ATBufferCur);

            memmove(_ATBuffer, _ATBufferCur, len + 1);
            p_read = _ATBuffer + len;
            _ATBufferCur = _ATBuffer;
        }
        /* Otherwise, (p_eol !- NULL) there is a complete line  */
        /* that will be returned the while () loop below        */
    }

    while (p_eol == nullptr) {
        if (0 == MAX_AT_RESPONSE - (p_read - _ATBuffer)) {
            MSF_ERROR << "Input line exceeded buffer";
            /* ditch buffer and start over again */
            _ATBufferCur = _ATBuffer;
            *_ATBufferCur = '\0';
            p_read = _ATBuffer;
        }

        do {
            count = read(_readFd, p_read,
                            MAX_AT_RESPONSE - (p_read - _ATBuffer));
        } while (count < 0 && errno == EINTR);

        if (count > 0) {
            // AT_DUMP( "<< ", p_read, count );

            p_read[count] = '\0';
            // skip over leading newlines
            while (*_ATBufferCur == '\r' || *_ATBufferCur == '\n')
                _ATBufferCur++;

            p_eol = AtFindNextEOL(_ATBufferCur);
            p_read += count;
        } else if (count <= 0) {
            /* read error encountered or EOF reached */
            if(count == 0) {
                MSF_ERROR << "ATChannel: EOF reached";
            } else {
                MSF_ERROR << "ATChannel: read error :" << strerror(errno);
            }
            return nullptr;
        }
    }

    /* a full line in the buffer. Place a \0 over the \r and return */
    ret = _ATBufferCur;
    *p_eol = '\0';
    _ATBufferCur = p_eol + 1; /* this will always be <= p_read,    */
                                /* and there will be a \0 at *p_read */

    // MSF_MOBILE_LOG(DBG_DEBUG,"AT< %s\n", ret);
    return ret;
}

void ATChannel::readerLoop()
{
    const char * line = readLine();
    if (unlikely(line == nullptr)) {
        MSF_FATAL << "Read channel got some fatal error: " << errno;
        readerClose();
    }

    if (unlikely(isSMSUnsolicited(line))) {
        processUnsolLine(line);
    } else {
        processLine(line);
    }
}

}
}
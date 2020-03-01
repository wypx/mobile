/**************************************************************************
 *
 * Copyright (c) 2017, luotang.me <wypx520@gmail.com>, OPERATOR.
 * All rights reserved.
 *
 * Distributed under the terms of the GNU General Public License v2.
 *
 * This software is provided 'as is' with no explicit or implied warranties
 * in respect of its properties, including, but not limited to, correctness
 * and/or fitness for purpose.
 *
 **************************************************************************/
#include "Dial.h"

#include <base/GccAttr.h>
#include <base/Logger.h>
#include <base/Utils.h>
#include <signal.h>
#include <sys/wait.h>

#include "Modem.h"

namespace MSF {
namespace MOBILE {

static char const *kPPPd = "pppd";
static char const *kModem = "modem";
static char const *kCrtscts = "crtscts";
static char const *kDebug = "debug";
static char const *kNoDetach = "nodetach";

static char const *kBaudrate = "115200";
static char const *kConnect = "connect";
static char const *kNoIpDefault = "noipdefault";
static char const *kAsyncmap = "default-asyncmap";
static char const *kLcpMaxKey = "lcp-max-configure";
static char const *kLcpMaxVal = "30";
static char const *kDefaultRoute = "defaultroute";
static char const *kHidePassWord = "hide-password"; /* show-password */
static char const *kNoDeflate = "nodeflate";
static char const *kNopcomp = "nopcomp";
static char const *kNovj = "novj";
static MSF_UNUSED_CHECK char const *kNoccp = "noccp";
static char const *kNovjCcomp = "novjccomp";

static char const *kMtuKey = "mtu";
static char const *kMtuVal = "1400";

static char const *kNoAuth = "noauth";
static char const *kAuth = "auth";
static char const *kUser = "user";
static char const *kPassWord = "password";

static char const *kRefusePap = "refuse-pap";
static char const *kRefuseChap = "refuse-pap";
static char const *kRefuseEap = "refuse-eap";
static char const *kRequirePap = "require-pap";
static char const *kRequireChap = "require-chap";

static char const *kUsePeerDns = "usepeerdns";
static char const *kLcpEchoIntervalKey = "lcp-echo-interval";
static char const *kLcpEchoIntervalVal = "60";
static char const *kLcpEchoFailureKey = "lcp-echo-failure";
static char const *kLcpEchoFailureVal = "3";
static char const *kMaxFailKey = "maxfail";
static char const *kMaxFailVal = "0";
static char const *kDisconnect = "disconnect";
static char const *kPPPdEnd = nullptr;

struct LocalDialParam {
  enum Operator oper;
  enum NetMode mode;
  char number[32];
  char user[32];
  char pass[32];
  char apn[32];
} _g_dilaParam_[] = {
    {OPERATOR_TELCOM, MODE_EVDO, "#777", "card", "card", ""}, /* EVDO mode */
    {OPERATOR_UNICOM, MODE_WCDMA, "*99#", "user", "user",
     "3gnet"}, /* WCDMA mode */
    {OPERATOR_MOBILE, MODE_TDSCDMA, "*98*1#", "user", "user",
     "cmnet"}, /* TDSCDMA mode */
    {OPERATOR_MOBILE, MODE_TDLTE, "*98*1#", "card", "card",
     "cmnet"}, /* TDLTE mode */
    {OPERATOR_TELCOM, MODE_FDDLTE, "*99#", "user", "user",
     "ctlte"}, /* FDDLTE mode */
    {OPERATOR_UNICOM, MODE_FDDLTE, "*98*1#", "user", "user",
     "cuinet"}, /* FDDLTE mode */
    {OPERATOR_MOBILE, MODE_TDLTE, "*98*1#", "card", "card",
     "cmwap"}, /* TDLTE mode */
    {OPERATOR_MOBILE, MODE_TDLTE, "*98*1#", "card", "card",
     "cmmtm"}, /* TDLTE mode */
    {OPERATOR_TELCOM, MODE_EVDO, "#777", "ctnet@mycdma.cn", "vnet.mobi",
     ""}, /*evdo mode */
    {OPERATOR_TELCOM, MODE_FDDLTE, "*98*1#", "user", "user",
     "ctm2m"}, /* FDDLTE mode */
    {OPERATOR_TELCOM, MODE_FDDLTE, "*98*1#", "user", "user",
     "ctnet"}, /* FDDLTE mode */
    {OPERATOR_TELCOM, MODE_FDDLTE, "*98*1#", "user", "user",
     "ctwap"}, /* FDDLTE mode */
    {OPERATOR_UNICOM, MODE_FDDLTE, "*98*1#", "user", "user",
     "uninet"}, /* FDDLTE mode */
    {OPERATOR_UNICOM, MODE_FDDLTE, "*98*1#", "user", "user",
     "uniwap"}, /* FDDLTE mode */
    {OPERATOR_UNICOM, MODE_FDDLTE, "*98*1#", "user", "user",
     "wonet"}, /* FDDLTE mode */
};

Dial::Dial()
    : _enable(true),
      _authType(AUTH_PAP_CHAP),
      _mtu(1460),
      _epName(DIAL_TTY_USB),
      _dialType(DIAL_AUTO_PERSIST),
      _dialStat(DIAL_INIT),
      _testDomian("luotang.me") {}
void Dial::addPlan(time_t startSime, time_t stopSime) {
  //检查是否重复区间
  _plans.push_back(std::make_pair(startSime, stopSime));
}

void Dial::delPlan(time_t startSime, time_t stopSime) {}

bool Dial::killPPPID() { return true; }

bool Dial::reDialPPP() { return true; }

int Dial::sigPPPExist(void *args) {
  int status;
  int exitNo = 0;
  pid_t *pid = (pid_t *)args;

  if (*pid > 0) {
    kill(*pid, SIGKILL);  // SIGTERM  SIGKILL
  }

  *pid = waitpid(*pid, &status, 0);
  if (*pid != -1) {
    MSF_INFO << "Process: " << *pid << " exist";
  }

  if (WIFEXITED(status)) {
    exitNo = WEXITSTATUS(status);
    MSF_INFO << "Normal termination, exit status:" << exitNo;
  }
  return 0;
}

void Dial::sigHandler(int sig) {
  MSF_INFO << "Catch signal: " << sig;
  switch (sig) {
    case SIGINT:
    case SIGKILL:
    case SIGSEGV:
    case SIGBUS:
      sigPPPExist(&_pppId);
      MSF_DEBUG << "pppd exit.";
      exit(0);
    default:
      break;
  };
}

bool Dial::dialPPP(DialCb cb) {
  int i = 0;
  const char *pppArgv[56];
  char chatConnScript[512] = {0};
  char chatDisconScript[512] = {0};

  _pppId = -1;

  uint32_t j;
  for (j = 0; j < MSF_ARRAY_SIZE(_g_dilaParam_); ++j) {
    if (_modem->getModemInfo()->_simOper == _g_dilaParam_[j].oper &&
        _modem->getNetMode() == _g_dilaParam_[j].mode) {
      break;
    }
  }
  if (unlikely(j == MSF_ARRAY_SIZE(_g_dilaParam_))) {
    cb(-1, nullptr);
    return false;
  }

  snprintf(chatConnScript, sizeof(chatConnScript) - 1,
           "chat -v  TIMEOUT 15 ABORT BUSY  ABORT 'NO ANSWER' '' ATH0 OK AT "
           "'OK-+++\\c-OK' ATDT%s CONNECT ",
           _g_dilaParam_[j].number);

  snprintf(chatDisconScript, sizeof(chatDisconScript) - 1,
           "chat -v ABORT 'BUSY' ABORT 'ERROR' ABORT 'NO DIALTONE' '' '\\K' '' "
           "'+++ATH' SAY '\nPDP context detached\n'");

  /* PPPd 参数
   * https://blog.csdn.net/wangc_farsight/article/details/89279765
   * https://docs.oracle.com/cd/E56344_01/html/E54077/pppd-1m.html
   * */
  pppArgv[i++] = kPPPd;
  pppArgv[i++] = kModem;
  pppArgv[i++] = kCrtscts;
  pppArgv[i++] = kDebug;
  pppArgv[i++] = kNoDetach;

  char portStr[32] = {0};
  snprintf(portStr, sizeof(portStr) - 1, MODEM_TTY_USB_PREFIX,
           _modem->getModemInfo()->_dialPort);

  pppArgv[i++] = portStr;

  pppArgv[i++] = kBaudrate;
  pppArgv[i++] = kConnect;
  pppArgv[i++] = chatConnScript;
  pppArgv[i++] = kNoIpDefault;
  pppArgv[i++] = kAsyncmap;
  pppArgv[i++] = kLcpMaxKey;
  pppArgv[i++] = kLcpMaxVal;
  pppArgv[i++] = kDefaultRoute;
  pppArgv[i++] = kHidePassWord;
  pppArgv[i++] = kNoDeflate;
  pppArgv[i++] = kNopcomp;
  pppArgv[i++] = kNovj;
  // argv[i++] = "noccp";
  pppArgv[i++] = kNovjCcomp;
  // ipcp-accept-local
  // ipcp-accept-remote

  pppArgv[i++] = kMtuKey;
  pppArgv[i++] = kMtuVal;

  if (_authType == AUTH_NONE) {
    pppArgv[i++] = kNoAuth;
  } else if (_authType == AUTH_PAP) {
    pppArgv[i++] = kRefuseChap;
    pppArgv[i++] = kRefuseEap;
    pppArgv[i++] = kAuth;
    pppArgv[i++] = kRequirePap;
  } else if (_authType == AUTH_CHAP) {
    pppArgv[i++] = kRefusePap;
    pppArgv[i++] = kRefuseEap;
    pppArgv[i++] = kAuth;
    pppArgv[i++] = kRequireChap;
  } else if (_authType == AUTH_PAP_CHAP) {
  }

  pppArgv[i++] = kUser;
  pppArgv[i++] = _g_dilaParam_[j].user;
  pppArgv[i++] = kPassWord;
  pppArgv[i++] = _g_dilaParam_[j].pass;

  pppArgv[i++] = kUsePeerDns;
  pppArgv[i++] = kLcpEchoIntervalKey;
  pppArgv[i++] = kLcpEchoIntervalVal;
  pppArgv[i++] = kLcpEchoFailureKey;
  pppArgv[i++] = kLcpEchoFailureVal;
  pppArgv[i++] = kMaxFailKey;
  pppArgv[i++] = kMaxFailVal;
  pppArgv[i++] = kDisconnect;
  pppArgv[i++] = chatDisconScript;
  pppArgv[i++] = kPPPdEnd;

  if ((_pppId = vfork()) < 0) {
    MSF_ERROR << "Can not fork, exit now.";
    return false;
  }

  if (_pppId == 0) {
    /* https://blog.csdn.net/diehuojiang5959/article/details/101620215
     * https://blog.csdn.net/wtguo1022/article/details/80882891 */
    int ret = execvp(kPPPd, const_cast<char **>(pppArgv));
    /* The execvp function returns only if an error occurs. */
    MSF_ERROR << "Execvp failed with error code: " << ret;
    abort();
  } else {
    return _pppId;
  }
}
}  // namespace MOBILE
}  // namespace MSF

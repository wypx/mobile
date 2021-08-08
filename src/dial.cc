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
#include "dial.h"

#include <base/gcc_attr.h>
#include <base/logging.h>
#include <base/utils.h>
#include <signal.h>
#include <sys/wait.h>

#include "modem.h"

namespace mobile {

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
static __attribute_unused__ char const *kNoccp = "noccp";
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

static struct LocalDialParam {
  Operator oper_;
  NetMode mode_;
  char number_[32];
  char user_[32];
  char pass_[32];
  char apn_[32];
} kDefaultDialParam[] = {
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
    : enable_(true),
      auth_type_(AUTH_PAP_CHAP),
      mtu_(1460),
      ep_name_(TTY_USB),
      dial_type_(DIAL_AUTO_PERSIST),
      dial_stat_(DIAL_INIT),
      test_domain_("luotang.me") {}

void Dial::AddPlan(time_t start, time_t stop) {
  //检查是否重复区间
  plans_.push_back(std::make_pair(start, stop));
}

void Dial::DelPlan(time_t start, time_t stop) {}

bool Dial::KillPPPID() { return true; }

bool Dial::ReDialPPP() { return true; }

int Dial::SigPPPExist(void *args) {
  int status;
  int exitNo = 0;
  pid_t *pid = (pid_t *)args;

  if (*pid > 0) {
    kill(*pid, SIGKILL);  // SIGTERM  SIGKILL
  }

  *pid = waitpid(*pid, &status, 0);
  if (*pid != -1) {
    LOG(INFO) << "Process: " << *pid << " exist";
  }

  if (WIFEXITED(status)) {
    exitNo = WEXITSTATUS(status);
    LOG(INFO) << "Normal termination, exit status:" << exitNo;
  }
  return 0;
}

void Dial::SigHandler(int sig) {
  LOG(INFO) << "Catch signal: " << sig;
  switch (sig) {
    case SIGINT:
    case SIGKILL:
    case SIGSEGV:
    case SIGBUS:
      SigPPPExist(&pppid_);
      LOG(L_DEBUG) << "pppd exit.";
      exit(0);
    default:
      break;
  };
}

bool Dial::DialPPP(DialCb cb) {
  int i = 0;
  const char *pppArgv[56];
  char chatConnScript[512] = {0};
  char chatDisconScript[512] = {0};

  pppid_ = -1;

  uint32_t j;
  for (j = 0; j < MSF_ARRAY_SIZE(kDefaultDialParam); ++j) {
    if (modem_->modem_info()->sim_operator_ == kDefaultDialParam[j].oper_ &&
        modem_->net_mode() == kDefaultDialParam[j].mode_) {
      break;
    }
  }
  if (unlikely(j == MSF_ARRAY_SIZE(kDefaultDialParam))) {
    cb(-1, nullptr);
    return false;
  }

  snprintf(chatConnScript, sizeof(chatConnScript) - 1,
           "chat -v  TIMEOUT 15 ABORT BUSY  ABORT 'NO ANSWER' '' ATH0 OK AT "
           "'OK-+++\\c-OK' ATDT%s CONNECT ",
           kDefaultDialParam[j].number_);

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
           modem_->modem_info()->dial_port_);

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

  if (auth_type_ == AUTH_NONE) {
    pppArgv[i++] = kNoAuth;
  } else if (auth_type_ == AUTH_PAP) {
    pppArgv[i++] = kRefuseChap;
    pppArgv[i++] = kRefuseEap;
    pppArgv[i++] = kAuth;
    pppArgv[i++] = kRequirePap;
  } else if (auth_type_ == AUTH_CHAP) {
    pppArgv[i++] = kRefusePap;
    pppArgv[i++] = kRefuseEap;
    pppArgv[i++] = kAuth;
    pppArgv[i++] = kRequireChap;
  } else if (auth_type_ == AUTH_PAP_CHAP) {
  }

  pppArgv[i++] = kUser;
  pppArgv[i++] = kDefaultDialParam[j].user_;
  pppArgv[i++] = kPassWord;
  pppArgv[i++] = kDefaultDialParam[j].pass_;

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

  if ((pppid_ = vfork()) < 0) {
    LOG(ERROR) << "Can not fork, exit now.";
    return false;
  }

  if (pppid_ == 0) {
    /* https://blog.csdn.net/diehuojiang5959/article/details/101620215
     * https://blog.csdn.net/wtguo1022/article/details/80882891 */
    int ret = execvp(kPPPd, const_cast<char **>(pppArgv));
    /* The execvp function returns only if an error occurs. */
    LOG(ERROR) << "Execvp failed with error code: " << ret;
    abort();
  } else {
    return pppid_;
    // 子pid
    // "/var/run/%s.pid"
  }
}

void net_ip_forward(int val) {
  // int v = val ? 1 : 0;

  // sysprintf("echo %d > /proc/sys/net/ipv4/ip_forward", v);
  // iptable -t nat -A POSTROUTING -o ppp0 -s IP -j SNAT --to PPP_IP
}

}  // namespace mobile

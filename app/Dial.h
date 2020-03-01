/**************************************************************************
 *
 * Copyright (c) 2019, luotang.me <wypx520@gmail.com>, China.
 * All rights reserved.
 *
 * Distributed under the terms of the GNU General Public License v2.
 *
 * This software is provided 'as is' with no explicit or implied warranties
 * in respect of its properties, including, but not limited to, correctness
 * and/or fitness for purpose.
 *
 **************************************************************************/
#ifndef __MSF_AT_DIAL_H__
#define __MSF_AT_DIAL_H__

#include <unistd.h>

#include <ctime>
#include <functional>
#include <iostream>
#include <mutex>
#include <utility>
#include <vector>

#include "Modem.h"

using namespace MSF::MOBILE;

namespace MSF {
namespace MOBILE {

#define DEFAULT_MTU 1460
#define PPP_INTERFACE "ppp0"

typedef std::function<void(const int, void *)> DialCb;

class Dial {
 public:
  Dial();
  ~Dial();

  const bool enable() const { return _enable; }
  void setEnable(const bool enable) { _enable = enable; }

  void setDialType(const enum DialType dialType) { _dialType = dialType; }
  void setAuthType(const enum AuthType authType) { _authType = authType; }
  void addPlan(time_t startSime, time_t stopSime);
  void delPlan(time_t startSime, time_t stopSime);
  void setMtu(const uint16_t mtu = DEFAULT_MTU) { _mtu = mtu; }
  void setEpName(const enum DialUsbType epName) { _epName = epName; }

  const std::string &getIpAddr() const { return _ipAddr; }
  const std::string &getGateWay() const { return _gateWay; }
  const std::string &getNetMask() const { return _netMask; }
  const std::string &getPrimaryDns() const { return _dnsPrimay; }
  const std::string &getSecondDns() const { return _dnsSecond; }

 private:
  Modem *_modem;
  bool _enable;
  /**
   * Data profile to modem
   */
  /* true to enable the profile, 0 to disable, 1 to enable */
  bool _profileEnabled;

  /* id of the data profile, data cid */
  int _profileId;
  /* the APN to connect to */
  std::string _apn;
  /** one of the PDP_type values in TS 27.007 section 10.1.1.
   * For example, "IP", "IPV6", "IPV4V6", or "PPP".*/
  std::string _protocol;
  /** authentication protocol used for this PDP context
   * (None: 0, PAP: 1, CHAP: 2, PAP&CHAP: 3) */
  enum AuthType _authType;

  /* the username for APN, or NULL */
  std::string _user;
  /* the password for APN, or NULL */
  std::string _pass;
  /* the profile type, TYPE_COMMON-0, TYPE_3GPP-1, TYPE_3GPP2-2 */
  int _profileType;

  /* the period in seconds to limit the maximum connections */
  uint32_t _maxConnsTime;
  /* the maximum connections during maxConnsTime */
  uint32_t _maxConns;
  /** the required wait time in seconds after a successful UE initiated
   * disconnect of a given PDN connection before the device can send
   * a new PDN connection request for that given PDN
   */
  uint32_t _waitTime;

  /** maximum transmission unit (MTU) size in bytes */
  uint16_t _mtu;

  enum DialUsbType _epName;

  enum DialType _dialType;
  /* Dial plans in one day */
  typedef std::pair<time_t, time_t> PlanItems;
  std::vector<PlanItems> _plans;

  pid_t _pppId;

  enum DialStat _dialStat;
  std::string _ipAddr;
  std::string _gateWay;
  std::string _netMask;
  std::string _dnsPrimay;
  std::string _dnsSecond;

  std::string _testDomian;
  int _testRetCode;

  std::mutex _mutex;

  void sigHandler(int sig);
  int sigPPPExist(void *args);
  bool killPPPID();
  bool reDialPPP();
  bool dialPPP(DialCb cb);
};
}  // namespace MOBILE
}  // namespace MSF
#endif
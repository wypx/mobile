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
#ifndef MOBILE_SRC_DIAL_H_
#define MOBILE_SRC_DIAL_H_

#include <unistd.h>

#include <ctime>
#include <functional>
#include <iostream>
#include <mutex>
#include <utility>
#include <vector>

#include "Modem.h"

using namespace mobile;
namespace mobile {

static const uint32_t kDefaultMTU = 1460;
static const std::string kPPPInterface = "ppp0";

typedef std::function<void(const int, void *)> DialCb;

class Dial {
 public:
  Dial();
  ~Dial();

  const bool enable() const { return enable_; }
  void set_enable(const bool enable) { enable_ = enable; }

  void set_dial_type(const DialType type) { dial_type_ = type; }
  void set_auth_type(const AuthType type) { auth_type_ = type; }

  const uint16_t mtu() const { return mtu_; }
  void set_mtu(const uint16_t mtu = kDefaultMTU) { mtu_ = mtu; }

  const TTyType ep_name() const { return ep_name_; }
  void set_ep_name(const TTyType ep) { ep_name_ = ep; }

  void AddPlan(time_t start, time_t stop);
  void DelPlan(time_t start, time_t stop);

  const std::string &ip_addr() const { return ip_addr_; }
  const std::string &gateway() const { return gateway_; }
  const std::string &netmask() const { return netmask_; }
  const std::string &primary_dns() const { return primary_dns_; }
  const std::string &second_dns() const { return second_dns_; }

 private:
  Modem *modem_;
  bool enable_;
  /**
   * Data profile to modem
   */
  /* true to enable the profile, 0 to disable, 1 to enable */
  bool profile_enabled_;

  /* id of the data profile, data cid */
  int profile_id_;

  /* the APN to connect to */
  std::string apn_;

  /** one of the PDP_type values in TS 27.007 section 10.1.1.
   * For example, "IP", "IPV6", "IPV4V6", or "PPP".*/
  std::string protocol_;

  /** authentication protocol used for this PDP context
   * (None: 0, PAP: 1, CHAP: 2, PAP&CHAP: 3) */
  AuthType auth_type_;

  /* the username for APN, or NULL */
  std::string user_;

  /* the password for APN, or NULL */
  std::string pass_;

  /* the profile type, TYPE_COMMON-0, TYPE_3GPP-1, TYPE_3GPP2-2 */
  int profile_type_;

  /* the period in seconds to limit the maximum connections */
  uint32_t max_conns_time_;

  /* the maximum connections during max_conns_time_ */
  uint32_t max_conns_;

  /** the required wait time in seconds after a successful UE initiated
   * disconnect of a given PDN connection before the device can send
   * a new PDN connection request for that given PDN
   */
  uint32_t wait_time_;

  /** maximum transmission unit (MTU) size in bytes */
  uint16_t mtu_;

  TTyType ep_name_;
  DialType dial_type_;

  /* Dial plans in one day */
  typedef std::pair<time_t, time_t> PlanItems;
  std::vector<PlanItems> plans_;

  pid_t pppid_;

  DialStat dial_stat_;
  std::string ip_addr_;
  std::string gateway_;
  std::string netmask_;
  std::string primary_dns_;
  std::string second_dns_;

  std::string test_domain_;
  int test_code_;

  std::mutex mutex_;

  void SigHandler(int sig);
  int SigPPPExist(void *args);
  bool KillPPPID();
  bool ReDialPPP();
  bool DialPPP(DialCb cb);
};
}  // namespace mobile
#endif
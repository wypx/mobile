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
#ifndef MOBILE_SRC_MODEM_H_
#define MOBILE_SRC_MODEM_H_

#include <iostream>

#include "Mobile.h"

namespace MSF {
namespace MOBILE {

enum ModemType {
  MODEM_UNKOWN,
  MODEM_LONGSUNG,
  MODEM_HUAWEI,
  MODEM_ZTE,
  MODEM_INTEL,
  MODEM_SIMCOM,
  MODEM_NODECOM,
  MODEM_NOEWAY,
  MODEM_QUECTEL
};

enum Operator {
  OPERATOR_MOBILE = 0,
  OPERATOR_UNICOM = 1,
  OPERATOR_TELCOM = 2,
  OPERATOR_UNKOWN = 3
};

/* for "^CPIN": PIN & PUK management */
#define CPIN_READY 1
#define CPIN_PIN 2
#define CPIN_PUK 3
#define CPIN_PIN2 4
#define CPIN_PUK2 5

enum SIMState {
  SIM_UNKOWN = 0,
  SIM_VALID = 1,
  SIM_INVALID = 4,
  SIM_ROAM = 240,
  SIM_NOTEXIST = 255,
};

enum SIMStatus {
  SIM_ABSENT = 0,
  SIM_NOT_READY = 1,
  SIM_READY = 2, /* SIM_READY means the radio state is RADIO_STATE_SIM_READY */
  SIM_PIN = 3,
  SIM_PUK = 4,
  SIM_NETWORK_PERSONALIZATION = 5,
  RUIM_ABSENT = 6,
  RUIM_NOT_READY = 7,
  RUIM_READY = 8,
  RUIM_PIN = 9,
  RUIM_PUK = 10,
  RUIM_NETWORK_PERSONALIZATION = 11
};

enum EchoStatus {
  ECHO_CLOSED,
  ECHO_OPEN,
};

/* Fixed modem infomations */
struct ModemInfo {
  const uint32_t vendor_id_;
  const uint32_t product_id_;
  const std::string manufact_;
  const std::string modem_name_; /* CGMR */
  const ModemType modem_type_;
  const uint8_t at_port_; /* port id not more than 255*/
  const uint8_t dial_port_;
  const uint8_t dbg_port_;
  const uint8_t apn_cid_; /* data apn cid */

  std::string sim_number_;
  Operator sim_operator_;

  ModemInfo(const uint32_t vendorId, const uint32_t productId,
            const std::string &modemName, const ModemType modemType,
            const uint8_t atPort, const uint8_t dialPort, const uint8_t dbgPort,
            const uint8_t apnCid)
      : vendor_id_(vendorId),
        product_id_(productId),
        modem_name_(modemName),
        modem_type_(modemType),
        at_port_(atPort),
        dial_port_(dialPort),
        dbg_port_(dbgPort),
        apn_cid_(apnCid) {}
};

class ATChannel;

class Modem {
 public:
  Modem() { }
  virtual ~Modem() { }

  bool Init();
  const NetMode net_mode() const { return net_mode_; }
  const ModemInfo *modem_info() const { return modem_; }

  bool ProbeDevice();
  virtual bool CheckSimcard();
  virtual bool CheckCellfun();
  virtual bool StartDial();
  virtual bool StopDial();

  void UnsolHandler(const char *line, const char *smsPdu);
  void ReaderCloseHandler();
  void ReadTimeoutHandler();

  bool CheckIdSupport(const uint32_t vendorId, const uint32_t productId);
  bool CheckUsbDriver();
  bool CheckSerialMod();
  bool CheckSerialPort();
  void MatchModem(const char *modemStr);
  const std::string &PasreModem() const;
  Operator MatchOperator(const char *cimi);
  const std::string &PasreOperator(Operator op) const;
  void MatchNetMode(const char *netStr);

 private:
  MobileErrno errno_;
  ModemInfo *modem_;
  SIMStatus sim_stat_;

  uint32_t net_signal_;
  NetMode net_mode_;
  uint32_t net_search_mode_;
  uint8_t net_mode_str_[32];

  ATChannel *ch_;
};

}  // namespace MOBILE
}  // namespace MSF
#endif
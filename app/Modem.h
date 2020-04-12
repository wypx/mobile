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
#ifndef MOBILE_APP_MODEM_H
#define MOBILE_APP_MODEM_H

#include <iostream>

#include "Errno.h"
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
  const uint32_t _vendorId;
  const uint32_t _productId;
  const std::string _manufact;
  const std::string _modemName; /* CGMR */
  const ModemType _modemType;
  const uint8_t _atPort; /* port id not more than 255*/
  const uint8_t _dialPort;
  const uint8_t _dbgPort;
  const uint8_t _apnCid; /*data apn cid*/

  std::string _simNum;
  enum Operator _simOper;

  ModemInfo(const uint32_t vendorId, const uint32_t productId,
            const std::string &modemName, const enum ModemType modemType,
            const uint8_t atPort, const uint8_t dialPort, const uint8_t dbgPort,
            const uint8_t apnCid)
      : _vendorId(vendorId),
        _productId(productId),
        _modemName(modemName),
        _modemType(modemType),
        _atPort(atPort),
        _dialPort(dialPort),
        _dbgPort(dbgPort),
        _apnCid(apnCid) {}
};

class ATChannel;
class Modem {
 public:
  Modem();
  virtual ~Modem();
  bool init();

  const enum NetMode getNetMode() const { return _netMode; }
  const struct ModemInfo *getModemInfo() const { return _modem; }

 protected:
  void probeDev();
  virtual bool checkSim();
  virtual bool checkCfun();
  virtual bool startDial();
  virtual bool stopDial();

  MobileErrno _errno;
  struct ModemInfo *_modem;
  enum SIMStatus _simStat;

  uint32_t _signal;
  enum NetMode _netMode;
  uint32_t _netSearchMode;
  uint8_t _netModeStr[32];

  ATChannel *_ch;

  void unsolHandler(const char *line, const char *smsPdu);
  void readerCloseHandler();
  void readTimeoutHandler();

  bool checkIdSupport(const uint32_t vendorId, const uint32_t productId);
  bool checkLsUsb();
  bool checkLsMod();
  bool checkSerial();
  void matchModem(const char *modemStr);
  const std::string &pasreModem() const;
  enum Operator matchOperator(const char *cimi);
  const std::string &pasreOperator(enum Operator op) const;
  void matchNetMode(const char *netStr);
};
}  // namespace MOBILE
}  // namespace MSF
#endif
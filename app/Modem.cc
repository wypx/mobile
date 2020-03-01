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
#include "Modem.h"

#include <base/File.h>
#include <base/Utils.h>

#include "AtChannel.h"
#include "AtTok.h"
#include "Errno.h"
#include "Idx.h"

using namespace MSF::BASE;
using namespace MSF::MOBILE;

namespace MSF {
namespace MOBILE {

static const std::string _ModemPasrer_[] = {
    "MODEM_UNKOWN",   /* 未知 */
    "MODEM_LONGSUNG", /* 龙尚 */
    "MODEM_HUAWEI",   /* 华为 */
    "MODEM_ZTE",      /* 中兴 */
    "MODEM_INTEL",    /* Intel */
    "MODEM_SIMCOM",   /* SIMCOM */
    "MODEM_NODECOM",  /* 诺控 */
    "MODEM_NEOWAY",   /* 有方 */
    "MODEM_QUECTEL"   /* 移远 */
};

const std::string &Modem::pasreModem() const {
  return _ModemPasrer_[_modem->_modemType];
}

static struct OperatorItem {
  char cimi[16];
  enum Operator op;
} _OperMatch_[] = {
    {"OPERATOR MOBILE", OPERATOR_MOBILE}, {"CMCC", OPERATOR_MOBILE},
    {"46000", OPERATOR_MOBILE},           {"46002", OPERATOR_MOBILE},
    {"46004", OPERATOR_MOBILE}, /*NB-IOT*/
    {"46007", OPERATOR_MOBILE},           {"46008", OPERATOR_MOBILE},
    {"46020", OPERATOR_MOBILE},           {"46027", OPERATOR_MOBILE},
    {"CHN-UNICOM", OPERATOR_UNICOM},      {"UNICOM", OPERATOR_UNICOM},
    {"46001", OPERATOR_UNICOM},           {"46006", OPERATOR_UNICOM}, /*NB-IOT*/
    {"46009", OPERATOR_UNICOM},           {"CHN-UNICOM", OPERATOR_TELCOM},
    {"46003", OPERATOR_TELCOM},           {"46005", OPERATOR_TELCOM},
    {"46011", OPERATOR_TELCOM},           {"2040", OPERATOR_TELCOM},
};

enum Operator Modem::matchOperator(const char *cimiStr) {
  for (uint32_t i = 0; i < MSF_ARRAY_SIZE(_OperMatch_); ++i) {
    /*(same) USB ID canot recognise model, AT+CGMM needed for LongSung */
    if (strstr(cimiStr, _OperMatch_[i].cimi)) {
      return _OperMatch_[i].op;
      break;
    }
  }
  return OPERATOR_UNKOWN;
}

static const std::string _OperParser_[] = {
    "OPERATOR_MOBILE",
    "OPERATOR_UNICOM",
    "OPERATOR_TELCOM",
    "OPERATOR_UNKOWN",
};

const std::string &Modem::pasreOperator(enum Operator op) const {
  return _OperParser_[op];
}

struct NetModeItem {
  char name[16];
  enum Operator op;
  enum NetMode mode;
} _ModeMath_[] = {
    {"NONE", OPERATOR_MOBILE, MODE_NONE},
    {"NONE", OPERATOR_UNICOM, MODE_NONE},
    {"NONE", OPERATOR_TELCOM, MODE_NONE},
    {"LTE FDD", OPERATOR_TELCOM, MODE_FDDLTE},
    {"LTE FDD", OPERATOR_UNICOM, MODE_FDDLTE},
    {"LTE TDD", OPERATOR_MOBILE, MODE_TDLTE},
    {"UMTS", OPERATOR_MOBILE, MODE_TDSCDMA},
    {"UMTS", OPERATOR_UNICOM, MODE_WCDMA},
    {"HSUPA", OPERATOR_UNICOM, MODE_WCDMA},
    {"HSDPA", OPERATOR_MOBILE, MODE_TDSCDMA},
    {"EVDO", OPERATOR_TELCOM, MODE_EVDO},
    {"TDSCDMA", OPERATOR_MOBILE, MODE_TDSCDMA},
    {"GPRS", OPERATOR_MOBILE, MODE_GPRS},
    {"GPRS", OPERATOR_UNICOM, MODE_GPRS},
    {"GPRS", OPERATOR_TELCOM, MODE_GPRS},
    {"EDGE", OPERATOR_MOBILE, MODE_GPRS},
    {"EDGE", OPERATOR_UNICOM, MODE_GPRS},
    {"EDGE", OPERATOR_TELCOM, MODE_GPRS},
    {"WCDMA", OPERATOR_UNICOM, MODE_WCDMA},
};

static ModemInfo *_usbModems_[] = {
    new ModemInfo(USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U8300,
                  "U8300", MODEM_LONGSUNG, 1, 3, 2, 2),
    new ModemInfo(USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U8300W,
                  "U8300W", MODEM_LONGSUNG, 1, 3, 2, 2),
    new ModemInfo(USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U8300C,
                  "U8300C", MODEM_LONGSUNG, 2, 1, 3, 2),
    new ModemInfo(USB_MODEM_IDVENDOR_LONGCHEER, USB_MODEM_IDPRODUCT_U9300C,
                  "U9300C", MODEM_LONGSUNG, 2, 1, 3, 2),
    new ModemInfo(USB_MODEM_IDVENDOR_HUAWEI, USB_MODEM_IDPRODUCT_ME909u_521,
                  "ME909u_521", MODEM_HUAWEI, 4, 2, 1, 0),
    new ModemInfo(USB_MODEM_IDVENDOR_HUAWEI, USB_MODEM_IDPRODUCT_ME909s_821,
                  "ME909s-821", MODEM_HUAWEI, 4, 2, 1, 0),
    new ModemInfo(USB_MODEM_IDVENDOR_HUAWEI, USB_MODEM_IDPRODUCT_ME909s_121,
                  "ME909s_121", MODEM_HUAWEI, 4, 2, 1, 0)};

bool Modem::checkIdSupport(const uint32_t vendorId, const uint32_t productId) {
  for (uint32_t i = 0; i < MSF_ARRAY_SIZE(_usbModems_); ++i) {
    if (productId == _usbModems_[i]->_productId &&
        vendorId == _usbModems_[i]->_vendorId) {
      _modem = _usbModems_[i];
      return true;
    }
    _errno = MOBILE_E_DRIVER_ID_NOT_SUPPORT;
    if (i == (MSF_ARRAY_SIZE(_usbModems_) - 1)) return false;
  }
  return true;
}

bool Modem::checkLsUsb() {
  char line[256];
  FILE *fp = nullptr;
  char *p = nullptr;

  fp = popen("lsusb", "r");
  if (!fp) {
    _errno = MOBILE_E_DRIVER_ID_NOT_SUPPORT;
    return false;
  }

  uint32_t productId = 0;
  uint32_t vendorId = 0;

  while (!feof(fp)) {
    memset(line, 0, sizeof(line));
    if (!fgets(line, sizeof(line), fp)) {
      break;
    }

    if ((p = strstr(line, "ID"))) {
      sscanf(p, "ID %x:%x", &vendorId, &productId);
      if (!checkIdSupport(vendorId, vendorId)) {
        continue;
      } else {
        pclose(fp);
        return true;
      }
    }
  }

  _errno = MOBILE_E_DRIVER_ID_NOT_SUPPORT;
  pclose(fp);
  return false;
}

/* first check ttyUSB whther exist, if not
 * then check the drivers whther are installed:
 * usb_wwan.ko, usbserial.ko and option.ko */
#define USB_DRIVER_PATH "/proc/modules"
bool Modem::checkLsMod() {
  /*lsmod ==> cat /proc/modules */
  FILE *fp = NULL;
  char line[128];
  uint32_t drivers = 0;

  if (!IsFileExist(USB_DRIVER_PATH)) {
    _errno = MOBILE_E_DRIVER_NOT_INSTALLED;
    return false;
  }

  fp = fopen(USB_DRIVER_PATH, "r");
  if (nullptr == fp) {
    _errno = MOBILE_E_DRIVER_NOT_INSTALLED;
    return -1;
  }

  while (!feof(fp)) {
    memset(line, 0, sizeof(line));
    if (!fgets(line, sizeof(line), fp)) {
      break;
    }

    if (strstr(line, "option") || strstr(line, "usb_wwan") ||
        strstr(line, "usbserial")) {
      drivers++;
      /* Otherwise Drivers have not been installed completely or
          No drivers have been installed */
      if (3 == drivers) {
        fclose(fp);
        return 0;
      }
    } else {
      continue;
    }
  }

  _errno = MOBILE_E_DRIVER_NOT_INSTALLED;
  fclose(fp);
  return false;
}

bool Modem::checkSerial() {
  char ttyPath[16] = {0};
  snprintf(ttyPath, sizeof(ttyPath) - 1, TTY_USB_FORMAT, _modem->_dbgPort);

  /* ls /dev/ttyUSB# */
  if (!IsFileExist(ttyPath)) {
    /* Drivers have been not installed, ttyUSB not exist,
       that is the drivers not support current productid */
    _errno = MOBILE_E_TTY_USB_NOT_FOUND;
    return false;
  }
  return true;
}

void Modem::matchModem(const char *modemStr) {
  for (uint32_t i = 0; i < MSF_ARRAY_SIZE(_usbModems_); ++i) {
    /*(same) USB ID canot recognise model, AT+CGMM needed for LongSung */
    if (strstr(modemStr, _usbModems_[i]->_modemName.c_str()) &&
        _modem->_productId == _usbModems_[i]->_productId &&
        _modem->_vendorId == _usbModems_[i]->_vendorId) {
      /* Update modem pointer to specific item */
      _modem = _usbModems_[i];
      _errno = MOBILE_E_TTY_USB_AVAILABLE;
      break;
    }
    _errno = MOBILE_E_TTY_USB_UNAVAILABLE;
  }
}

void Modem::matchNetMode(const char *netStr) {
  char *pNetStr = (char *)netStr;
  switch (_modem->_modemType) {
    case MODEM_HUAWEI: {
      int tmp[7];
      char currMode[32] = {0};
      char *p = strstr(pNetStr, ":");
      if (unlikely(!p)) {
        MSF_ERROR << "Parse network mode failed";
        return;
      }

      sscanf(p, ":%d,%d,%d,%d,,%d,\"%s\"", &tmp[0], &tmp[1], &tmp[2], &tmp[3],
             &tmp[4], currMode);

      switch (tmp[4]) {
        case 1:
          _netMode = MODE_GPRS;
          break;
        case 3:
          _netMode = MODE_WCDMA;
          break;
        case 4:
          _netMode = MODE_TDSCDMA;
          break;
        case 6:
          _netMode =
              (_modem->_simOper == OPERATOR_MOBILE) ? MODE_TDLTE : MODE_FDDLTE;
          break;
        default:
          break;
      }
    } break;
    case MODEM_LONGSUNG:
    case MODEM_NOEWAY:
    case MODEM_NODECOM:
    case MODEM_SIMCOM: {
      char *response = nullptr;
      int ret = AtTokStart(&pNetStr);
      if (ret < 0) return;

      ret = AtTokNextStr(&pNetStr, &response);
      if (ret < 0) return;

      for (uint32_t i = 0; i < MSF_ARRAY_SIZE(_ModeMath_); ++i) {
        if (strstr(response, _ModeMath_[i].name) &&
            (_modem->_simOper == _ModeMath_[i].op)) {
          _netMode = _ModeMath_[i].mode;
          break;
        }
        _netMode = MODE_NONE;
      }
    }

    default:
      break;
  }
}

static struct UnsolParser {
  std::string _unsolATLine;
  ATUnsolHandler _handler;
} _unsolATLine[] = {
    {"%CTZV", nullptr}, /* TI specific -- NITZ time */
    {"+CRING:", nullptr},
    {"RING:", nullptr},
    {"NO CARRIER:", nullptr},
    {"+CCWA:", nullptr},
    {"+CREG:", nullptr},
    {"+CGREG:", nullptr},
    {"+CMT:", nullptr},
    {"+CDS:", nullptr},
    {"+CME ERROR: 150:", nullptr},
    {"+CGDCONT:", nullptr},

    /*接收到一条新的短信通知*/
    // if(MODE_EVDO == _netMode)
    // {
    // sprintf((char *)command, "AT^HCMGR=%d", );
    // },
    // else
    // {
    // sprintf((char *)command, "AT+CMGR=%d", );
    // },
    {"+CMTI:", nullptr},
};

/**
 * Called by atchannel when an unsolicited line appears
 * This is called on atchannel's reader thread. AT commands may
 * not be issued here
 */
void Modem::unsolHandler(const char *line, const char *smsPdu) {
//^HCMGR +CMGR:

//^SMMEMFULL", "%SMMEMFULL", "+SMMEMFULL"
//^MODE:
//^SYSINFO:", "%SYSINFO:", %SYSINFO:"
// sscanf(p, ":%d,%d,%d,%d,%d", &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4]);
// sscanf(last, ",%d", &tmp[6]);
#if 0
    	mobile_srvStat = tmp[0]; 

		if (255 == tmp[1])
		{
			mobile_srvDomain = 3;
		}
		else
		{
			mobile_srvDomain = tmp[1];
		}
	 
		if(OPERATOR_OPERATOR_TELECOM == pDevDialParam->mobile_operator
			|| (M_HUAWEI_3G == shareMemParam->iModel ))
		{
			//when current mobile_operator is Telecom,current mode is only 3 kinds, MODE_EVDO��MODE_FDDLTE��MODE_CDMA1X
			if(4 == tmp[3] ||8 == tmp[3])
			{	
				mobile_mode = MODE_EVDO;
				if(0 == tmp[2])
				{
					mobile_reg3gInfo = CREG_STAT_REGISTED_LOCALNET;
				}
				else
				{
					mobile_reg3gInfo = CREG_STAT_REGISTED_ROAMNET;
				}
			}
			else if(9 == tmp[3])
			{
				mobile_mode = MODE_FDDLTE;
				if(0 == tmp[2])
				{
					mobile_reg4gInfo = CREG_STAT_REGISTED_LOCALNET;
				}
				else
				{
					mobile_reg4gInfo = CREG_STAT_REGISTED_ROAMNET;
				}
			}
			else if(2 == tmp[3])
			{
				mobile_mode = MODE_CDMA1X;
				if(0 == tmp[2])
				{
					mobile_reg2gInfo = CREG_STAT_REGISTED_LOCALNET;
				}
				else
				{
					mobile_reg2gInfo = CREG_STAT_REGISTED_ROAMNET;
				}
			}
			else if(5 == tmp[3])
			{
				mobile_mode = MODE_WCDMA;
			}
			else if(3 == tmp[3])
			{
				mobile_mode = MODE_GPRS;
			}
			else;


			mobile_srvStat = tmp[0]; 

			if (255 == tmp[1])
			{
				mobile_srvDomain = 3;
			}
			else
			{
				mobile_srvDomain = tmp[1];
			}
		}
		else
		{
			mobile_simStat = tmp[4];	

			if(1 == tmp[4])
			{
				mobile_simStat = SIM_VALID;
			}
			else if(255 == tmp[4])
			{
				mobile_simStat = SIM_NOTEXIST;
			}
			else  // ��֪������240 255  ������CPIN����ʹ��
			{
				mobile_simStat = SIM_INVALID;  /* all invalid */
			}
		}
#endif
//"^SYSINFOEX:
#if 0
    	if(!(p = strstr(line, ":")))
			return FALSE;
		sscanf(p, ":%d,%d,%d,%d,,%d,\"%s\"", &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], curr_mode);

		if(M_HUAWEI_4G == shareMemParam->iModel)
		{ 
			mobile_srvStat = tmp[0]; 

			if (255 == tmp[1])
			{
				mobile_srvDomain = 3;
			}
			else
			{
				mobile_srvDomain = tmp[1];
			}
			
			if(1 == tmp[4])
			{
				mobile_mode = MODE_GPRS;
				if(0 == tmp[2])
					mobile_reg2gInfo = CREG_STAT_REGISTED_LOCALNET;
				else
					mobile_reg2gInfo = CREG_STAT_REGISTED_ROAMNET;
			}
			else if(3 == tmp[4])
			{
				mobile_mode = MODE_WCDMA;
				if(0 == tmp[2])
					mobile_reg3gInfo = CREG_STAT_REGISTED_LOCALNET;
				else
					mobile_reg3gInfo = CREG_STAT_REGISTED_ROAMNET;
			}
			else if(4 == tmp[4])
			{
				mobile_mode = MODE_TDSCDMA;
				if(0 == tmp[2])
					mobile_reg3gInfo = CREG_STAT_REGISTED_LOCALNET;
				else
					mobile_reg3gInfo = CREG_STAT_REGISTED_ROAMNET;
			}
			else if(6 == tmp[4])
			{
				if(OPERATOR_OPERATOR_MOBILE== pDevDialParam->mobile_operator)
					mobile_mode = MODE_TDLTE;
				else if(OPERATOR_OPERATOR_UNICOM== pDevDialParam->mobile_operator)
					mobile_mode = MODE_FDDLTE;

				if(0 == tmp[2])
					mobile_reg4gInfo = CREG_STAT_REGISTED_LOCALNET;
				else
					mobile_reg4gInfo = CREG_STAT_REGISTED_ROAMNET;
			}
			
			
			if (1 == tmp[3])
			{
				mobile_simStat = SIM_VALID; 
			}
			else if(255 == tmp[3])
			{
				mobile_simStat = SIM_NOTEXIST;  
			}
			else //0  2 3 4 255 
			{
				mobile_simStat = SIM_INVALID;  /* all invalid */
			}
		}
#endif

  //"^CCSQ" "+CSQ" "+CCSQ"
  //^HDRCSQ: ^HRSSILVL: ^CSQLVL: ^RSSILVL:
  //+PSRAT: NONE  LTE FDD  LTE TDD UMTS(TDSCDMA-WCDMA)
  // HSUPA(WCDMA) HSDPA(WCDMA) TDSCDMA  GPRS EDGE(GPRS)
  // ^HCSQ: WCDMA
  //+CFUN:
  //+COPS:
  //+CREG:", "+CGREG:", "+CEREG:
  //+CLIP:
}

void Modem::readerCloseHandler() {}

void Modem::readTimeoutHandler() {}

bool Modem::init() {
  if (!checkLsMod()) {
    MSF_ERROR << "Usb modem driver not installed.";
    return false;
  }

  if (!checkLsUsb()) {
    MSF_ERROR << "Usb modem id app not matched.";
    return false;
  }

  if (!checkSerial()) {
    MSF_ERROR << "Usb modem ttyusb not found.";
    return false;
  }

  _ch = new ATChannel(std::bind(&Modem::unsolHandler, this,
                                std::placeholders::_1, std::placeholders::_2),
                      std::bind(&Modem::readerCloseHandler, this),
                      std::bind(&Modem::readTimeoutHandler, this));

  /* Give initializeCallback a chance to dispatched, since
   * we don't presently have a cancellation mechanism */
  usleep(500);
}

static int usb_start(void *data, uint32_t datalen) {
  // mobile_at_modem_init();
  // mobile_at_get_modem(usb_modem_match);
  // mobile_at_set_radio(CFUN_ONLINE_MODE);
  // mobile_at_set_ehrpd(EHRPD_CLOSE);

  // usb->sim_status = mobile_at_get_sim();
  // if (unlikely(usb->sim_status != SIM_READY))
  //     usb->usb_status = E_SIM_COMES_LOCKED;
  // else
  //     usb->usb_status = E_SIM_COMES_READY;

  // mobile_at_get_operator(usb_operator_match);
  // mobile_at_get_signal();
  // mobile_at_search_network(usb->item.modem_type, LS_AUTO);

  // usleep(500);

  // mobile_at_get_network_mode();

  // mobile_at_get_network_3gpp();

  // mobile_at_get_apns();

  // // Give initializeCallback a chance to dispatched, since
  // // we don't presently have a cancellation mechanism
  // sleep(1);
  // s_closed = 1;

  // waitForClose();

  return 0;
}
}  // namespace MOBILE
}  // namespace MSF
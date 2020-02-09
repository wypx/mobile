/**************************************************************************
*
* Copyright (c) 2017-2018, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
#ifndef __AT_MOBILE_H__
#define __AT_MOBILE_H__

#include <base/GccAttr.h>
#include <base/Os.h>
#include <base/Noncopyable.h>
#include <base/MemPool.h>
#include <event/EventLoop.h>
#include <event/EventStack.h>
#include <client/AgentClient.h>

#include <string>
#include <list>
#include <vector>
#include <tuple>

using namespace MSF::BASE;
using namespace MSF::EVENT;
using namespace MSF::AGENT;

namespace MSF {
namespace MOBILE {

#define MODEM_TTY_USB_PREFIX "/dev/ttyUSB%d"

typedef enum {
    CME_ERROR_NON_CME = -1,
    CME_SUCCESS = 0,
    CME_SIM_NOT_INSERTED = 10
} ATCmeError;

enum EhrpdStatus {
  EHRPD_CLOSE = 0,
  EHRPD_OPEN  = 1,
};

enum ChipErrLevel {
  CHIP_ERROR_NULL = 0,
  CHIP_ERROR_NUMERIC,
  CHIP_ERROR_LONG,
};

enum PdpStatus {
  PDP_DEACTIVE,
  PDP_ACTIVE,
};

enum DialUsbType {
  DIAL_TTY_USB,
  DIAL_TTY_ACM,
};

enum DialType {
  DIAL_AUTO_PERSIST = 0,  /* Always dial to keep online */
  DIAL_AUTO_PLANS   = 1,  /* Dial as scheduled */
  DIAL_MANUALLY     = 2,  /* Dial manual when test*/
};

enum AuthType {
  AUTH_NONE,
  AUTH_PAP,
  AUTH_CHAP,
  AUTH_PAP_CHAP,
};

enum DialStat {
  DIAL_INIT,
  DIAL_EXECUTING,
  DIAL_FAILED,
  DIAL_SUCCESS,
};

enum CallIdStatus {
  CALLER_ID_CLOSE = 0,
  CALLER_ID_OPEN  = 1
};

enum RadioMode {
  /* minimum functionality, dsiable RF but reserve SIM card power supply, 
   * previous mode must be offline */
  RADIO_LPM_MODE          = 0, /* Radio explictly powered off (eg CFUN=0) */
  RADIO_ONLINE_MODE       = 1, /* full functionality (Default) */
  RADIO_OFFLINE_MODE      = 4, /* disable phone both transmit and receive RF circuits */
  RADIO_FTM_MODE          = 5, /* Factory Test Mode */
  RADIO_RESTART_MODE      = 6, /* Reset mode */ 
  /* note:AT+CFUN=6 must be used after setting AT+CFUN=7 
   * If module in offline mode, must execute AT+CFUN=6 or restart module to online mode.*/
  RADIO_OFF_MODE          = 7, /* Offline Mode, Radio unavailable (eg, resetting or not booted) */
  /* note:If AT+CFUN=0/4 is used after setting AT+CFUN=7, 
   * module will restart to online mode */
};

typedef enum {
  CARD_STATE_ABSENT     = 0,
  CARD_STATE_PRESENT    = 1,
  CARD_STATE_ERROR      = 2,
  CARD_STATE_RESTRICTED = 3  /* card is present but not usable due to carrier restrictions.*/
} CardState;

struct LceStatusInfo {
  uint8_t lce_status;              /* LCE service status:
                                    * -1 = not supported;
                                    * 0 = stopped;
                                    * 1 = active.
                                    */
  uint32_t actual_interval_ms;    /* actual LCE reporting interval,
                                  * meaningful only if LCEStatus = 1.*/
};

/* service status*/ 
enum ServiceStatus {
  SERVICE_NOT_FOUND  = 0,
  SERVICE_RESTRICTED_SERVICE,
  SERVICE_VALID_SERVICE,
  SERVICE_RESTRICTED_REGIONAL_SERVICE,
  SERVICE_POWER_SAVING_HIBERATE_SERVICE,
};

/* service domain*/
enum ServiceDomain {
  /* No restriction at all including voice/SMS/USSD/SS/AV64 and packet data. */
  SERVICE_DOMAIN_NONE,
      /* Block all voice/SMS/USSD/SS/AV64 including emergency call due to restriction.*/
  SERVICE_DOMAIN_CS_NORMAL,
  /* Block packet data access due to restriction. */
  SERVICE_DOMAIN_PS_NORMAL,
  SERVICE_DOMAIN_CS_PS_ALL,
  /* Not registered, but MT is currently searching */
  SERVICE_DOMAIN_CS_PS_SEARCH,
};

/* Huawei for AT^SYSINFOEX, "^SYSCONFIG" (TD) | "^SYSCFG" (WCDMA): 
 * system mode config parameters  
 * LongSung for ^MODODREX or ^MODODR */
enum NETWORK_SERACH_TYPE {
  /* LongSung Modem */
  LS_UTMS_ONLY            = 1, /* Not Support Now */
  LS_AUTO                 = 2, /* LTE > CDMA >HDR >TDS >WCDMA >GSM */
  LS_GSM_ONLY             = 3, /* GSM + CDMA ONLY */
  LS_GSM_PREFERED         = 4, /* TDS Pre (TDS > GSM > LTE >WCDMA >HDR >CDMA) */
  LS_LTE_ONLY             = 5, /* LTE ONLY */
  LS_TDSCDMA_ONLY         = 6, /* TDSCDMA ONLY */
  LS_TDSCDMA_WCDMA        = 7, /* TDCMDA_WCDMA */
  LS_TDSCDMA_GSM_WCDMA    = 8, /* TDCMDA_GSM_WCDMA */
  LS_TDSCDMA_WCDMA_LTE    = 9, /* TDCMDA_WCDMA_LTE */
  LS_HDR_ONLY             = 10,/* HDR ONLY */
  LS_LTE_PREFERED         = 11,/* LTE Pre (LTE > HDR >CDMA >TDS >WCDMA >GSM) */
  LS_HDR_PREFERED         = 12,/* HDR Pre (HDR >CDMA >LTE >TDS >WCDMA > GSM)*/
  LS_HDR_LTE              = 13,/* HDR and LTE */
  LS_HDR_CDMA             = 14,/* CDMA and HDR*/
  LS_CDMA_ONLY            = 15,/* CDMA and HDR*/

  /* Huawei Modem */
  HW_AUTO         = 2,   /* Automatic */
  HW_CDMA         = 3,   /* CDMA (not supported currently) */
  HW_HDR          = 4,   /* HDR (not supported currently) */
  HW_HYBRID       = 5,   /* CDMA/HDR HYBRID (not supported currently) */
  HW_CDMA1X       = 11,
  HW_EVDO         = 12,
  HW_GSM          = 13,
  HW_WCDMA        = 14,
  HW_TDSCDMA      = 15,
  HW_NOCHANGE     = 16,
  HW_TDLTE        = 17,
  HW_FDDLTE       = 18,
};

typedef enum {
  RADIO_TECH_UNKNOWN = 0,
  RADIO_TECH_GPRS = 1,
  RADIO_TECH_EDGE = 2,
  RADIO_TECH_UMTS = 3,
  RADIO_TECH_IS95A = 4,
  RADIO_TECH_IS95B = 5,
  RADIO_TECH_1xRTT =  6,
  RADIO_TECH_EVDO_0 = 7,
  RADIO_TECH_EVDO_A = 8,
  RADIO_TECH_HSDPA = 9,
  RADIO_TECH_HSUPA = 10,
  RADIO_TECH_HSPA = 11,
  RADIO_TECH_EVDO_B = 12,
  RADIO_TECH_EHRPD = 13,
  RADIO_TECH_LTE = 14,
  RADIO_TECH_HSPAP = 15,  /* HSPA+ */
  RADIO_TECH_GSM = 16,    /* Only supports voice */
  RADIO_TECH_TD_SCDMA = 17,
  RADIO_TECH_IWLAN = 18,
  RADIO_TECH_LTE_CA = 19
} RadioTechnology;

typedef enum {
  RADIO_TECH_3GPP = 1, /* 3GPP Technologies - GSM, WCDMA */
  RADIO_TECH_3GPP2 = 2 /* 3GPP2 Technologies - CDMA */
} RadioTechnologyFamily;

typedef enum {
  BAND_MODE_UNSPECIFIED = 0,      //"unspecified" (selected by baseband automatically)
  BAND_MODE_EURO = 1,             //"EURO band" (GSM-900 / DCS-1800 / WCDMA-IMT-2000)
  BAND_MODE_USA = 2,              //"US band" (GSM-850 / PCS-1900 / WCDMA-850 / WCDMA-PCS-1900)
  BAND_MODE_JPN = 3,              //"JPN band" (WCDMA-800 / WCDMA-IMT-2000)
  BAND_MODE_AUS = 4,              //"AUS band" (GSM-900 / DCS-1800 / WCDMA-850 / WCDMA-IMT-2000)
  BAND_MODE_AUS_2 = 5,            //"AUS band 2" (GSM-900 / DCS-1800 / WCDMA-850)
  BAND_MODE_CELL_800 = 6,         //"Cellular" (800-MHz Band)
  BAND_MODE_PCS = 7,              //"PCS" (1900-MHz Band)
  BAND_MODE_JTACS = 8,            //"Band Class 3" (JTACS Band)
  BAND_MODE_KOREA_PCS = 9,        //"Band Class 4" (Korean PCS Band)
  BAND_MODE_5_450M = 10,          //"Band Class 5" (450-MHz Band)
  BAND_MODE_IMT2000 = 11,         //"Band Class 6" (2-GMHz IMT2000 Band)
  BAND_MODE_7_700M_2 = 12,        //"Band Class 7" (Upper 700-MHz Band)
  BAND_MODE_8_1800M = 13,         //"Band Class 8" (1800-MHz Band)
  BAND_MODE_9_900M = 14,          //"Band Class 9" (900-MHz Band)
  BAND_MODE_10_800M_2 = 15,       //"Band Class 10" (Secondary 800-MHz Band)
  BAND_MODE_EURO_PAMR_400M = 16,  //"Band Class 11" (400-MHz European PAMR Band)
  BAND_MODE_AWS = 17,             //"Band Class 15" (AWS Band)
  BAND_MODE_USA_2500M = 18        //"Band Class 16" (US 2.5-GHz Band)
} RadioBandMode;

enum NetMode {
  MODE_NONE       = 0,    /*NO SERVICE*/
  MODE_CDMA1X     = 2,    /*CDMA*/
  MODE_GPRS       = 3,    /*GSM/GPRS*/
  MODE_EVDO       = 4,    /*HDR(EVDO)*/
  MODE_WCDMA      = 5,    /*WCDMA*/
  MODE_GSWCDMA    = 7,    /*GSM/WCDMA*/
  MODE_HYBRID     = 8,    /*HYBRID(CDMA and EVDO mixed)*/
  MODE_TDLTE      = 9,    /*TDLTE*/
  MODE_FDDLTE     = 10,   /*FDDLTE(2)*/
  MODE_TDSCDMA    = 15    /*TD-SCDMA*/
};

typedef enum {
  PREF_NET_TYPE_GSM_WCDMA                = 0, /* GSM/WCDMA (WCDMA preferred) */
  PREF_NET_TYPE_GSM_ONLY                 = 1, /* GSM only */
  PREF_NET_TYPE_WCDMA                    = 2, /* WCDMA  */
  PREF_NET_TYPE_GSM_WCDMA_AUTO           = 3, /* GSM/WCDMA (auto mode, according to PRL) */
  PREF_NET_TYPE_CDMA_EVDO_AUTO           = 4, /* CDMA and EvDo (auto mode, according to PRL) */
  PREF_NET_TYPE_CDMA_ONLY                = 5, /* CDMA only */
  PREF_NET_TYPE_EVDO_ONLY                = 6, /* EvDo only */
  PREF_NET_TYPE_GSM_WCDMA_CDMA_EVDO_AUTO = 7, /* GSM/WCDMA, CDMA, and EvDo (auto mode) */
  PREF_NET_TYPE_LTE_CDMA_EVDO            = 8, /* LTE, CDMA and EvDo */
  PREF_NET_TYPE_LTE_GSM_WCDMA            = 9, /* LTE, GSM/WCDMA */
  PREF_NET_TYPE_LTE_CMDA_EVDO_GSM_WCDMA  = 10, /* LTE, CDMA, EvDo, GSM/WCDMA */
  PREF_NET_TYPE_LTE_ONLY                 = 11, /* LTE only */
  PREF_NET_TYPE_LTE_WCDMA                = 12, /* LTE/WCDMA */
  PREF_NET_TYPE_TD_SCDMA_ONLY            = 13, /* TD-SCDMA only */
  PREF_NET_TYPE_TD_SCDMA_WCDMA           = 14, /* TD-SCDMA and WCDMA */
  PREF_NET_TYPE_TD_SCDMA_LTE             = 15, /* TD-SCDMA and LTE */
  PREF_NET_TYPE_TD_SCDMA_GSM             = 16, /* TD-SCDMA and GSM */
  PREF_NET_TYPE_TD_SCDMA_GSM_LTE         = 17, /* TD-SCDMA,GSM and LTE */
  PREF_NET_TYPE_TD_SCDMA_GSM_WCDMA       = 18, /* TD-SCDMA, GSM/WCDMA */
  PREF_NET_TYPE_TD_SCDMA_WCDMA_LTE       = 19, /* TD-SCDMA, WCDMA and LTE */
  PREF_NET_TYPE_TD_SCDMA_GSM_WCDMA_LTE   = 20, /* TD-SCDMA, GSM/WCDMA and LTE */
  PREF_NET_TYPE_TD_SCDMA_GSM_WCDMA_CDMA_EVDO_AUTO  = 21, /* TD-SCDMA, GSM/WCDMA, CDMA and EvDo */
  PREF_NET_TYPE_TD_SCDMA_LTE_CDMA_EVDO_GSM_WCDMA   = 22  /* TD-SCDMA, LTE, CDMA, EvDo GSM/WCDMA */
} PreferredNetworkType;

typedef enum {
  CALL_STATE_ACTIVE   = 0,
  CALL_STATE_HOLDING  = 1,
  CALL_STATE_DIALING  = 2,                /* MO call only */
  CALL_STATE_ALERTING = 3,                /* MO call only */
  CALL_STATE_INCOMING = 4,                /* MT call only */
  CALL_STATE_WAITING  = 5                 /* MT call only */
} CallState;

struct ApnItem {
    int             cid;        /* Context ID, uniquely identifies this call */
    int             active;     /* 0=inactive, 1=active/physical link down, 2=active/physical link up */
    char *          type;       /* One of the PDP_type values in TS 27.007 section 10.1.1.
                                   For example, "IP", "IPV6", "IPV4V6", or "PPP". */
    char *          ifname;     /* The network interface name */
    char *          apn;        /* ignored */
    char *          address;    /* An address, e.g., "192.0.1.3" or "2001:db8::1". */
};

struct MobileConf {
    std::vector<ApnItem> apnList_; /* eg. "public.vpdn.hz" - cid */

    uint32_t     logLevel_;

    bool        smsEnbale_;
    bool        smsAlarm_;
    bool        smdControl_;
    std::list<std::string>  smsWhiteList_;      /* 支持接收告警和控制命令的SIM卡号列表 */
    std::list<uint32_t>     smsAlarmType_;      /* 支持的告警类型 */
    std::list<uint32_t>     smdControlType_;    /* 支持的控制类型 */
};

struct MobileState {
    bool        backuping_;
    bool        upgrading_;
  
    uint8_t     simState_;
    uint8_t     regState_;
    uint8_t     serviceState_;
    uint8_t     serviceDomain_;

    uint8_t     signal_;
    uint8_t     searchMode_;
    enum NetMode netMode_;

    std::string ipAddr_;
    std::string netMask_;
    std::string gateWay_;
    std::list<std::string> dnsList_;
};

class ATChannel;
class ATCmdManager;
class Mobile : public Noncopyable
{
    public:
        Mobile(const std::string & config = std::string());
        ~Mobile();

        void debugInfo();

        bool loadConfig();
        void start();

        /* Unix server addr */
        void setAgent(const std::string & addr)
        {
          // agent_->setAgentServer(addr);
        }
        /* Tcp/Udp ip addr and port */
        void setAgent(const std::string & addr, const uint16_t port)
        {
          // agent_->setAgentServer(addr, port);
        }
    private:
        std::string config_;
        std::string logFile_;

        OsInfo os_;
        EventStack *stack_;
        ATChannel *channel_;
        ATCmdManager *acm_;
        MemPool *pool_;
        AgentClient   *agent_;
        bool    quit_;
        pid_t   pppId_;

        void onRequestCb(char *data, const uint32_t len, const AgentCommand cmd);
};

}
}
#endif
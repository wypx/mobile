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
#include <mobile.h>

#define MAX_AT_CMD_LEN 256
/*
 * AT Command Releated In at_cmd.c
 * 
 */

s32 mobile_at_modem_init(void) {

    s32 rc;
    ATResponse *p_response = NULL;

    at_handshake();

    /* note: we don't check errors here. Everything important will
     be handled in onATTimeout and onATReaderClosed */

    /*  atchannel is tolerant of echo but it must */
    /*  have verbose result codes */
    at_send_command("ATE1", NULL);
    //at_send_command("ATE0", NULL);

    /*  No auto-answer */
    at_send_command("ATS0=0", NULL);

    /*  Extended errors */
    at_send_command("AT+CMEE=1", NULL);

    /*  set apn autometicly according to curent network operator */
    at_send_command("AT+NVRW=1,50058,\"01\"", NULL);

    //PDP_DEACTIVE
    //at_send_command("AT+CGACT=1,1", NULL);

    /* Auto:All band search*/
    at_send_command("AT+BNDPRF=896,1272,131072", NULL);

    /*  Network registration events */
    rc = at_send_command("AT+CREG=2", &p_response);

    /* some handsets -- in tethered mode -- don't support CREG=2 */
    if (rc < 0 || p_response->success == 0) {
      at_send_command("AT+CREG=1", NULL);
    }

    at_response_free(p_response);

    /*  GPRS registration events */
    at_send_command("AT+CGREG=1", NULL);

    /*  Call Waiting notifications */
    at_send_command("AT+CCWA=1", NULL);

    /*  Alternating voice/data off */
    at_send_command("AT+CMOD=0", NULL);

    /*  Not muted */
    //at_send_command("AT+CMUT=0", NULL);

    /*  +CSSU unsolicited supp service notifications */
    at_send_command("AT+CSSN=0,1", NULL);

    /*  no connected line identification */
    at_send_command("AT+COLP=0", NULL);

    /*  HEX character set */
    //at_send_command("AT+CSCS=\"HEX\"", NULL);

    /*  USSD unsolicited */
    at_send_command("AT+CUSD=1", NULL);

    /*  Enable +CGEV GPRS event notifications, but don't buffer */
    at_send_command("AT+CGEREP=1,0", NULL);

    /*  SMS PDU mode */
    at_send_command("AT+CMGF=0", NULL);

    return rc;
}

s32 mobile_at_set_chip_error_level(u32 chiplevel) {

    //deault: CHIP_ERROR_NUMERIC
    s32 rc;
    s8 atcmd[MAX_AT_CMD_LEN] = { 0 };

    snprintf(atcmd, sizeof(atcmd), "AT+CMEE=%d", chiplevel);

    rc = at_send_command(atcmd, NULL);

    return rc;
}

s32 mobile_at_reset_module(void) {

    s32 rc;

    rc = at_send_command("AT^RESET", NULL);

    return rc;
}


s32 mobile_at_get_modem(mobile_modem_match_f match_func) {

   ATResponse *p_response = NULL;
   s32 rc;
   s8 *line;
   s8 *out;

   /* use ATI or AT+CGMM or AT+LCTSW */
   rc = at_send_command_singleline("ATI", "Model:", &p_response);
   if (rc < 0 || p_response->success == 0) {
       goto error;
   }

   line = p_response->p_intermediates->line;

   rc= at_tok_start(&line);
   if (rc < 0) goto error;

   rc = at_tok_nextstr(&line, &out);
   if (rc < 0) goto error;

   match_func(out);

   at_response_free(p_response);
   return 0;

error:
   at_response_free(p_response);
   return -1;
}

/** returns 1 if on, 0 if off, and -1 on error */
s32 mobile_get_radio(void) {

    ATResponse *p_response = NULL;
    s32 rc;
    s8 *line = NULL;
    s8 ret = -1;

    rc = at_send_command_singleline("AT+CFUN?", "+CFUN:", &p_response);
    if (rc < 0 || p_response->success == 0) {
        // assume radio is off
        goto error;
    }

    line = p_response->p_intermediates->line;

    rc = at_tok_start(&line);
    if (rc < 0) goto error;

    rc = at_tok_nextbool(&line, &ret);
    if (rc < 0) goto error;

    at_response_free(p_response);
    return (s32)ret;

error:
    at_response_free(p_response);
    return -1;
}

s32 mobile_at_set_radio(enum MOBILE_FUNC cfun) {

    s32 rc;
    s32 count = 5;
    ATResponse* p_response = NULL;
    enum MOBILE_FUNC curr_radio_mode = CFUN_LPM_MODE;
    s8 atcmd[MAX_AT_CMD_LEN] = { 0 };

    curr_radio_mode = mobile_get_radio();
    if (curr_radio_mode == cfun) {
        return 0;
    }

    snprintf(atcmd, sizeof(atcmd)-1, "AT+CFUN=%d", cfun);
    while(count-- < 0) {
        rc = at_send_command(atcmd, &p_response);
        if (rc < 0 || p_response->success == 0) {
            continue;
        } else {
            curr_radio_mode = mobile_get_radio();
            if (curr_radio_mode == cfun) 
                break;
        }
    }
    at_response_free(p_response);
    return 0;
}

s32 mobile_at_get_sim(void) {

    ATResponse *p_response = NULL;
    s32 rc;
    s8 *cpinLine = NULL;
    s8 *cpinResult = NULL;

    rc = at_send_command_singleline("AT+CPIN?", "+CPIN:", &p_response);
    if (rc != 0) {
        rc = at_send_command_singleline("AT+QCPIN?", "+QCPIN:", &p_response);
        if (rc != 0) {
            rc = SIM_NOT_READY;
            goto done;
        }
    }

    switch (at_get_cme_error(p_response)) {
        case CME_SUCCESS:
            break;

        case CME_SIM_NOT_INSERTED:
            rc = SIM_ABSENT;
            goto done;

        default:
            rc = SIM_NOT_READY;
            goto done;
    }

    /* CPIN/QCPIN has succeeded, now look at the result */
    cpinLine = p_response->p_intermediates->line;
    rc = at_tok_start (&cpinLine);
    if (rc < 0) {
        rc = SIM_NOT_READY;
        goto done;
    }

    rc = at_tok_nextstr(&cpinLine, &cpinResult);
    if (rc < 0) {
        rc = SIM_NOT_READY;
        goto done;
    }

    //AT_DBG("sim:%s\n", cpinResult);
    if (0 == msf_strcmp(cpinResult, "SIM PIN")) {
        rc = SIM_PIN;
        goto done;
    } else if (0 == msf_strcmp(cpinResult, "SIM PUK")) {
        rc = SIM_PUK;
        goto done;
    } else if (0 == msf_strcmp(cpinResult, "PH-NET PIN")) {
        rc = SIM_NETWORK_PERSONALIZATION;
        goto done;
    } else if (0 != msf_strcmp(cpinResult, "READY"))  {
        /* we're treating unsupported lock types as "sim absent" */
        rc = SIM_ABSENT;
        goto done;
    }

    at_response_free(p_response);
    p_response = NULL;
    cpinResult = NULL;

    rc = SIM_READY;
    at_response_free(p_response);
    return rc;
done:
    at_response_free(p_response);
    return rc;
}

s32 mobile_at_get_simlock(void) {
    s32 rc = -1;

    rc = at_send_command("AT+CLCK=\"SC\",2", NULL);
    if (rc != 0) {
        /*Telecom*/
        rc = at_send_command("AT+QCLCK=\"SC\",2", NULL);
        if (rc != 0) {
            return -1;
        }
    }
    return 0;
}

s32 mobile_at_set_unlock_pincode(s8             *pincode) {

    s32 rc = -1;
    s8 atcmd[MAX_AT_CMD_LEN] = { 0 };

    snprintf(atcmd, sizeof(atcmd)-1, "AT+CPIN=\"%s\"", pincode);
    rc = at_send_command(atcmd, NULL);
    if (rc != 0) {
        //error info
    }
    return rc;
}

s32 mobile_at_set_new_pincode(s8 *oldcode, s8 *newcode) {

    s32 rc = -1;
    s8 atcmd[MAX_AT_CMD_LEN] = { 0 };

    snprintf(atcmd, sizeof(atcmd), "AT+CPIN=\"%s\",\"%s\"", oldcode, newcode);
    rc = at_send_command(atcmd, NULL);
    if (rc != 0) {
        //error info
    }
    return rc;
}


/* Ehrpd enable off is needed by pppd dial,
 * but quickly dial do not need it*/
s32 mobile_at_set_ehrpd(enum EHRPD_STATE enehrpd) {

    ATResponse *p_response = NULL;
    s32 rc;
    s8 *line = NULL;
    s8 ret = -1;
    s8 atcmd[MAX_AT_CMD_LEN] = { 0 };
    
    rc = at_send_command_singleline("AT+EHRPDENABLE?", "+EHRPDENABLE:", &p_response);
    if (rc < 0 || p_response->success == 0) {
        // assume radio is off
        goto error;
    }

    line = p_response->p_intermediates->line;

    rc = at_tok_start(&line);
    if (rc < 0) goto error;

    rc = at_tok_nextbool(&line, &ret);
    if (rc < 0) goto error;

    at_response_free(p_response);

    /*ret equal 1(true) represent is on*/
    if (ret == enehrpd) {
        at_response_free(p_response);
        return  0;
    }

    snprintf(atcmd, sizeof(atcmd)-1, "AT+EHRPDENABLE=%d", enehrpd);
    rc = at_send_command(atcmd, &p_response);
    if (rc < 0 || p_response->success == 0) {
        goto error;
    }

    at_response_free(p_response);
    return 0;
error:
    at_response_free(p_response);
    return -1;
}

s32 mobile_at_get_search_network(void) {

    s32 rc = -1;
    s8 atcmd[MAX_AT_CMD_LEN] = { 0 };
    
    snprintf(atcmd, sizeof(atcmd), "AT+MODODREX?");

    //huawei 3g
    snprintf(atcmd, sizeof(atcmd), "AT^ANTMODE");
    //u7500
    snprintf(atcmd, sizeof(atcmd), "AT+MODODR?");
    //c7500
    snprintf(atcmd, sizeof(atcmd), "AT^PREFMODE?");

    return rc;
}

s32 mobile_at_search_network(enum USB_MODEM modem, enum NETWORK_SERACH_TYPE s_mode) {

    s32 rc = -1;
    s8 atcmd[MAX_AT_CMD_LEN] = { 0 };

    if(modem == MODEM_HUAWEI)
        rc = at_send_command("AT^SYSCFGEX=\"030201\",3FFFFFFF,1,2,7FFFFFFFFFFFFFFF,,", NULL);
    else if(modem == MODEM_LONGSUNG)
        rc = at_send_command("AT+MODODREX=2", NULL);

    return rc;
}

/*信号实时获取,不采用同步获取的方式*/
s32 mobile_at_set_signal_notify(void) {

    s32 rc = -1;

    rc = at_send_command("AT+CSQ", NULL);
    if (rc != 0) {
        rc = at_send_command("AT+CCSQ", NULL);
        if (rc != 0) {
            return -1;
        }
    }
    return 0;
}

s32 mobile_at_get_signal(void) {
    s32 rc = -1;

    //8300c
    rc = at_send_command("AT+SIGNALIND=1", NULL);
    if (rc != 0) {
    }
    return 0;
    //"+SIGNALIND:99, rssi,ber " //"level:UNKNOWN"
    //"+SIGNALIND:0, rssi,ber" //"level:0"
}

//置支持热插拔功能
s32 mobile_at_set_wapen(void) {
    s32 rc = -1;

    //8300c
    //AT+ SIMSWAPEN?
    rc = at_send_command("AT+SIMSWAPEN=1", NULL);
    if (rc != 0) {
    }
    return 0;
}

//获取小区信息,信噪比等信息
s32 mobile_at_get_lscellinfo(void) {
    s32 rc = -1;

    //8300c
    rc = at_send_command("AT+LSCELLINFO", NULL);
    if (rc != 0) {
    }
    return 0;
    /*
    ====AT+LSCELLINFO
    LTE SERV CELL INFO:
            EARFCN:39148 GCELLID:190041608 TAC:22546 MCC:460 MNC:00 DLBW:5 ULBW:5
                    SINR:25.6 CAT:3 BAND:40 PCI:240  RSRP:-84  RSRQ:-3 RSSI:-60
    LTE INTRA INFO: 
            PCI[0]:240 RSRQ[0]:-3 RSRP[0]:-84 RSSI[0]:-60 RXLEV[0]:43
    LTE INTER INFO: 
            EARFCN[0]:38950 
                    PCI:44 RSRQ:-4 RSRP:-82 RSSI:-58
            EARFCN[1]:1250 

    OK
    ====
    */
}

s32 mobile_at_get_clccinfo(void) {
    s32 rc = -1;

    //8300c
    rc = at_send_command("AT+CLCC", NULL);
    if (rc != 0) {
    }
    return 0;
    /*
    ====
    +CLIP: "18969188036",161,,,,0
    ====
    ====
    RING
    +CLIP: "18969188036",161,,,,0
    ====
    ====ATA
    ====at+clcc
    +CLCC: 1,0,0,1,0,"",128
    +CLCC: 2,1,0,0,0,"18969188036",161
    OK
    ====
    ====
    +DISC: 2,1,0,16,"18969188036",161
    ====
    */
}


s32 mobile_at_set_dtmf(void) {
    s32 rc = -1;

    //8300c
    /*
    * DTMF 由高频群和低频群组成,高低频群各包含4个频率.
    * 一个高频信号和一个低频信号叠加组成一个组合信号,代表一个数字.
    * DTMF信令有16个编码, 对于设备来说是主叫输入，被叫输出.
    *  ====
    *	+DTMF: 0
    *	====
    */
    rc = at_send_command("AT+LSHTONEDET=%d", NULL);
    if (rc != 0) {
    }
    return 0;
}

s32 mobile_at_set_audio(void) {
    s32 rc = -1;


    /* 龙尚模块
     * 目前音频只支持PCM语音,MASTER mode,CLK 1024KHZ,SYNC 8KHZ,16bit linear.
     * 现在的9x15 平台开始,语音参数是通过一个类似于efs 的acdb 数据库配置的,
     * 如果需要修改修改这些参数，需要利用高通的QACT 工具,修改acdb 配置,
     * 利用adb 将acdb上传到ap 侧,重启生效.
    */

    //8300c
    //启动pcm并加载acdb,速度慢,需要18s 后才有pcm clock 输出,音质优化。
    rc = at_send_command("AT+SYSCMD=start_pcm acdb", NULL);
    if (rc != 0) {
    }
    rc = at_send_command("AT+SYSCMD=start_pcm volume 33", NULL);
    if (rc != 0) {
    }
    return 0;
}


s32 mobile_at_answer_call(void) {
    s32 rc = -1;

    //8300c
    rc = at_send_command("ATA", NULL);
    if (rc != 0) {
    }
    return 0;
}

s32 mobile_at_set_callinfo(void) {
    s32 rc = -1;

    //+CLIP:02150809688,129,,,,0   模块来电,号码是02150809688

    //8300c
    rc = at_send_command("AT+CLIP=%d", NULL);
    if (rc != 0) {
    }
    return 0;
}

s32 mobile_at_get_callinfo(void) {
    s32 rc = -1;

    //8300c
    rc = at_send_command("AT+CLIP", NULL);
    if (rc != 0) {
    }
    return 0;
}
s32 mobile_at_set_tts(void) {
    s32 rc = -1;

    //8300c
    /*
    U8300C U8300W
    at+lshtts=0,"start codehere"
    at+lshtts=2,"9F995C1A79D16280FF0C00770065006C0063006F006D0065"
    音量调节:
    AT+LSHTTSCONF=?
    AT+LSHTTSCONF=3  0-3 2默认音量

    TTS 停止播报
    AT+LSHTTSSTP 语音播报时，将停止语音播报
    */

    //HUAWEI
    //AT^TTSCFG=<op>,<value>
    //AT^TTS
    //^AUDEND

    return 0;
}


/* AT+COPS=3,0;+COPS?;+COPS=3,1;+COPS?;+COPS=3,2;+COPS?;
 * AT+COPS=3,0;+COPS?;+COPS=3,1;+COPS?;+COPS=3,2;+COPS?;
 * AT+QCIMI
 */
s32 mobile_at_get_operator(mobile_operator_match_f match_func) {

    s32 rc;
    s8 resp[16] = { 0 };
    s8 *line;
    ATResponse *p_response = NULL;

    rc = at_send_command_singleline("AT+CIMI", "+CIMI:", &p_response);
    if (rc != 0) {
        rc = at_send_command_singleline("AT+QCIMI", "+QCIMI:", &p_response);
        if (rc != 0) {
            goto done;
        }
    }

    if (rc < 0 || p_response->success == 0) {
       // assume radio is off
       goto done;
   }

   line = p_response->p_intermediates->line;
   rc = at_tok_start(&line);
   if (rc < 0) goto done;

   rc = at_tok_nextstr(&line, (s8 **)&resp);
   if (rc < 0) goto done;

   match_func(resp);
done:
    at_response_free(p_response);
    return rc;

    #if 0
    /*AT+COPS*/
    s32 s_mcc = 0;
    s32 s_mnc = 0;
    s32 i;
    s32 skip;
    ATLine *p_cur;
    s8* response[3];
    rc = at_send_command_multiline("AT+COPS=3,0;+COPS?;+COPS=3,1;+COPS?;+COPS=3,2;+COPS?", "+COPS:", &p_response);
    /* we expect 3 lines here:
     * +COPS: 0,0,"T - Mobile"
     * +COPS: 0,1,"TMO"
     * +COPS: 0,2,"310170"
     *  exp:
     *  +COPS: 0,0,"CHN-UNICOM",7
     *  +COPS: 0,1,"UNICOM",7
     *  +COPS: 0,2,"46001",7

     *  +COPS: 0,0,"CHINA MOBILE",7
     *  +COPS: 0,1,"CMCC",7
     *  +COPS: 0,2,"46000",7

     *  +COPS: 0,0,"CMCC",7
     *  +COPS: 0,1,"CMCC",7
     *  +COPS: 0,2,"46000",7
        
     *  +COPS: 0
     *  +COPS: 0
     *  +COPS: 0
     */
     
    for (i = 0, p_cur = p_response->p_intermediates
            ; p_cur != NULL
            ; p_cur = p_cur->p_next, i++) {

        s8 *line = p_cur->line;

        rc = at_tok_start(&line);
        if (rc < 0) goto error;

        rc = at_tok_nextint(&line, &skip);
        if (rc < 0) goto error;

        // If we're unregistered, we may just get
        // a "+COPS: 0" response
        if (!at_tok_hasmore(&line)) {
            response[i] = NULL;
            continue;
        }

        rc = at_tok_nextint(&line, &skip);
        if (rc < 0) goto error;


        // a "+COPS: 0, n" response is also possible
        if (!at_tok_hasmore(&line)) {
            response[i] = NULL;
            continue;
        }

        rc = at_tok_nextstr(&line, &(response[i]));
        if (rc < 0) goto error;

        //operator
        response[i];

        // Simple assumption that mcc and mnc are 3 digits each
        if (strlen(response[i]) == 6) {
            if (sscanf(response[i], "%3d%3d", &s_mcc, &s_mnc) != 2) {
                MSF_MOBILE_LOG(DBG_ERROR, "Expected mccmnc to be 6 decimal digits.");
            }
        }
    }

    if (i != 3) {
        /* expect 3 lines exactly */
        goto error;
    }
    #endif
}

s32 mobile_at_set_list_all_operators(void) {

    s32 rc;

    rc = at_send_command("AT+COPS=?", NULL);

    return rc;
}

s32 mobile_at_get_list_operator_by_numeric(void) {

    s32 rc;

    rc = at_send_command("AT+COPS=3,2", NULL);

    return rc;
}

s32 mobile_at_set_auto_register(void) {

    s32 rc;

    rc = at_send_command("AT+COPS=0,0", NULL);

    return rc;
}

s32 mobile_at_get_network_mode(void) {

    s32 rc;

    rc = at_send_command("AT+PSRAT", NULL);
    if (rc == 0) {
        goto done;
    }

    //AT^SYSINFO

    //huwei 4G
    rc = at_send_command("AT^SYSINFOEX", NULL);
    if (rc == 0) {
       goto done;
    }

    //Huawei 3G
    rc = at_send_command("AT^HCSQ?", NULL);
    if (rc == 0) {
       goto done;
    }

done:
    return rc;
}

s32 mobile_at_get_network_3gpp(void) {

    s32 rc;
    ATResponse *p_response = NULL;
    s32 response = 0;
    s8*line;

    //AT+COPS=?
    //+COPS: (2,"CHN-UNICOM","UNICOM","46001",7),(3,"CHN-CT","CT","46011",7),(3,"CHINA MOBILE","CMCC","46000",7),,(0,1,2,3,4),(0,1,2)
    //AT+COPS?
    //+COPS: 0,2,"46001",7
    rc = at_send_command_singleline("AT+COPS?", "+COPS:", &p_response);
    if (rc < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;

    rc = at_tok_start(&line);
    if (rc < 0) goto error;

    rc = at_tok_nextint(&line, &response);
    if (rc < 0) goto error;

    at_response_free(p_response);
    return 0;
error:
    at_response_free(p_response);
    return -1;
}

s32 mobile_at_get_2G_status(void) {
    s32 rc;

    rc = at_send_command("AT+CGREG?", NULL);

    return rc;
}

s32 mobile_at_get_3G_status(void) {
    s32 rc;

    rc = at_send_command("AT+CREG?", NULL);

    return rc;
}

s32 mobile_at_get_4G_status(void) {
    s32 rc;

    rc = at_send_command("AT+CEREG?", NULL);

    return rc;
}

s32 mobile_at_get_sms_center(void) {
    s32 rc;

    rc = at_send_command("AT+CSCA?", NULL);

    return rc;
}

s32 mobile_at_set_caller_id(void) {

    s32 rc;
    enum CALLER_ID_STATUS call_id = CALLER_ID_OPEN;

    s8 atcmd[MAX_AT_CMD_LEN] = { 0 };

    snprintf(atcmd, sizeof(atcmd)-1, "AT+CLIP=%d", call_id);

    rc = at_send_command(atcmd, NULL);

    return rc;
}

s32 mobile_at_set_TE_status(void) {

    s32 rc;

    rc = at_send_command("AT+CNMI=2,1,2,2,0", NULL);

    return rc;
}

s32 mobile_at_set_sms_storage_area(enum SMS_STORAGE_AREA area) {

    s32 rc = -1;

    if (ME_AREA == area)
        rc = at_send_command("AT+CPMS=\"ME\",\"ME\",\"ME\"", NULL);
    else if (SM_AREA == area)
        rc = at_send_command("AT+CPMS=\"SM\",\"SM\",\"SM\"", NULL);

    return rc;
}

s32 mobile_at_set_sms_format(enum SMS_FORMAT format) {

    s32 rc;
    s8 atcmd[MAX_AT_CMD_LEN] = { 0 };

    snprintf(atcmd, sizeof(atcmd), "AT+CMGF=%d", format);
    rc = at_send_command(atcmd, NULL);
    if (rc == 0) {
        goto done;
    }

    //evdo
    msf_memzero(atcmd, sizeof(atcmd));
    snprintf(atcmd, sizeof(atcmd), "AT$QCMGF=%d", format);
    rc = at_send_command(atcmd, NULL);
    if (rc == 0) {
       goto done;
    }
done:
    return rc;
}

s32 mobile_at_get_sms_exist(void) {

    s32 rc = -1;

    rc = at_send_command("AT+CMGD=?", NULL);
        if (rc != 0) {
        /*Telecom evdo*/
        rc = at_send_command("AT$QCMGD=?", NULL);
        if (rc != 0) {
            return -1;
        }
    }
    return 0;
}

s32 mobile_at_read_sms(u32 index) {

    s32 rc = -1;

    s8 atcmd[MAX_AT_CMD_LEN] = { 0 };

    snprintf(atcmd, sizeof(atcmd), "AT+CMGR=%d", index);
    snprintf(atcmd, sizeof(atcmd), "AT^HCMGR=%d", index);

    rc = at_send_command(atcmd, NULL);

    return 0;
}

s32 mobile_at_clear_sms(enum SMS_FORMAT sms_format, enum SMS_CLEAN_MODE sms_clean_mode) {

    s32 rc = -1;

    s8 atcmd[MAX_AT_CMD_LEN] = { 0 };

    snprintf(atcmd, sizeof(atcmd), "AT+CMGD=%d,%d", sms_format, sms_clean_mode);
    rc = at_send_command(atcmd, NULL);

    return 0;
}

s32 mobile_at_get_apns(void) {

    ATResponse *p_response;
    ATLine *p_cur;
    s32 rc;
    s32 n = 0;
    s8 *out;

    rc = at_send_command_multiline ("AT+CGDCONT?", "+CGDCONT:", &p_response);
    if (rc != 0 || p_response->success == 0) {

        return -1;
    }

    for (p_cur = p_response->p_intermediates; p_cur != NULL;
         p_cur = p_cur->p_next) {
        char *line = p_cur->line;
        int cid;

        rc = at_tok_start(&line);
        if (rc < 0) goto error;

        rc = at_tok_nextint(&line, &cid);
        if (rc < 0) goto error;


    //printf("cid = %d \n", cid);

    rc = at_tok_nextstr(&line, &out);
    if (rc < 0) goto error;

    //printf("type = %s \n", out);

     // APN ignored for v5
    rc = at_tok_nextstr(&line, &out);
    if (rc < 0) goto error;

    //printf("apn = %s \n", out);

    rc = at_tok_nextstr(&line, &out);
    if (rc < 0) goto error;

    //printf("addresses = %s \n", out);
       

    }

    at_response_free(p_response);
    return 0;

    error:
    at_response_free(p_response);
    return -1;
}

s32 mobile_at_clear_apns(void) {

    s32 rc;
    s8 atcmd[MAX_AT_CMD_LEN] = { 0 };

    snprintf(atcmd, sizeof(atcmd), "AT+cgdcont=1");
    rc = at_send_command(atcmd, NULL);

    snprintf(atcmd, sizeof(atcmd), "AT+cgdcont=2");
    rc = at_send_command(atcmd, NULL);

    return rc;
}

s32 mobile_at_set_apns(void) {

    s32 rc;
    s8 atcmd[MAX_AT_CMD_LEN] = { 0 };
    s8 apn1[16];
    s8 apn2[16];
    
    snprintf(atcmd, sizeof(atcmd),
        "AT+cgdcont=1,\"ip\",\"%s\"", apn1);
    snprintf(atcmd, sizeof(atcmd),
        "AT+cgdcont=2,\"ip\",\"%s\"", apn2);

    rc = at_send_command(atcmd, NULL);
    return rc;
}

s32 mobile_at_set_userinfo(void) {

    s32 rc;
    u8 verifyProto = 2;
    s8 szPassword[32];
    s8 szUsername[32];
    // LongSung: AT$QCPDPP=1,2,passwod, username
    //AT^PPPCFG=<user_name>,<password>
    s8 atcmd[MAX_AT_CMD_LEN] = { 0 };
    snprintf(atcmd, sizeof(atcmd),
        "AT$QCPDPP=1,%d,\"%s\",\"%s\"",
        verifyProto, 
        szPassword, 
        szUsername);

    //Huawei: AT^AUTHDATA=<cid><Auth_type><PLMN><passwd><username>
    snprintf(atcmd, sizeof(atcmd),
        "AT^AUTHDATA=1,%d,\"\",\"%s\",\"%s\"", 
        verifyProto, 
        szPassword, 
        szUsername);


    rc = at_send_command(atcmd, NULL);

    return rc;
}

s32 mobile_at_set_pdp_status(enum PDP_STATUS pdp_status) {

    s32 rc;
    s8 atcmd[MAX_AT_CMD_LEN] = { 0 };

    snprintf(atcmd, sizeof(atcmd), "AT+CGACT=%d,1", pdp_status);

    rc = at_send_command(atcmd, NULL);

    return rc;
}

s32 mobile_at_set_qos_request_briefing(void) {

    s32 rc;

    rc = at_send_command("AT+CGEQREQ=1,3,11520,11520,11520,11520,,,,,,,3", NULL);

    return rc;
}




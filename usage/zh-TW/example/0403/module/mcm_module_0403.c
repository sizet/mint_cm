// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../mcm_lib/mcm_lheader/mcm_type.h"
#include "../../mcm_lib/mcm_lheader/mcm_size.h"
#include "../../mcm_lib/mcm_lheader/mcm_control.h"
#include "../../mcm_lib/mcm_lheader/mcm_connect.h"
#include "../../mcm_lib/mcm_lheader/mcm_return.h"
#include "../../mcm_lib/mcm_lheader/mcm_debug.h"
#include "../../mcm_lib/mcm_lheader/mcm_data_exinfo_auto.h"
#include "../mcm_service_handle_extern.h"
#include "../mcm_config_handle_extern.h"


#define DMSG(msg_fmt, msgs...) printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msgs)


char *dump_status(
    MCM_DTYPE_DS_TD status_code)
{
    static char *status_type;


    switch(status_code)
    {
        case MCM_DSCHANGE_NONE:
            status_type = "none";
            break;
        case MCM_DSCHANGE_SET:
            status_type = "set";
            break;
        case MCM_DSCHANGE_ADD:
            status_type = "add";
            break;
        case MCM_DSCHANGE_DEL:
            status_type = "del";
            break;
    }

    return status_type;
}

int mcm_module_max_count(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1;
    MCM_DTYPE_EK_TD max_count;


    DMSG("max-count test 01 :");

    // device 的資料筆數上限.
    path1 = "device";
    if(mcm_config_get_max_count_by_path(this_session, path1, &max_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_max_count_by_path(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, max_count);

    // device.system 的資料筆數上限.
    path1 = "device.system";
    if(mcm_config_get_max_count_by_path(this_session, path1, &max_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_max_count_by_path(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, max_count);

    // device.vap.* 的資料筆數上限.
    path1 = "device.vap.*";
    if(mcm_config_get_max_count_by_path(this_session, path1, &max_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_max_count_by_path(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, max_count);

    // device.vap.*.extra 的資料筆數上限.
    path1 = "device.vap.*.extra";
    if(mcm_config_get_max_count_by_path(this_session, path1, &max_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_max_count_by_path(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, max_count);

    // device.vap.*.station.* 的資料筆數上限.
    path1 = "device.vap.*.station.*";
    if(mcm_config_get_max_count_by_path(this_session, path1, &max_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_max_count_by_path(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, max_count);

    // device.limit.* 的資料筆數上限.
    path1 = "device.limit.*";
    if(mcm_config_get_max_count_by_path(this_session, path1, &max_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_max_count_by_path(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, max_count);

    // device.client.* 的資料筆數上限.
    path1 = "device.client.*";
    if(mcm_config_get_max_count_by_path(this_session, path1, &max_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_max_count_by_path(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, max_count);

    DMSG("max-count test 02 :");

    // device 的資料筆數上限.
    path1 = "device";
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, MCM_MCOUNT_DEVICE_MAX_COUNT);

    // device.system 的資料筆數上限.
    path1 = "device.sustem";
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, MCM_MCOUNT_DEVICE_SYSTEM_MAX_COUNT);

    // device.vap.* 的資料筆數上限.
    path1 = "device.vap.*";
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, MCM_MCOUNT_DEVICE_VAP_MAX_COUNT);

    // device.vap.*.extra 的資料筆數上限.
    path1 = "device.vap.*.extra";
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, MCM_MCOUNT_DEVICE_VAP_EXTRA_MAX_COUNT);

    // device.vap.*.station.* 的資料筆數上限.
    path1 = "device.vap.*.station.*";
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, MCM_MCOUNT_DEVICE_VAP_STATION_MAX_COUNT);

    // device.limit.* 的資料筆數上限.
    path1 = "device.limit.*";
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, MCM_MCOUNT_DEVICE_LIMIT_MAX_COUNT);

    // device.client.* 的資料筆數上限.
    path1 = "device.client.*";
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, MCM_MCOUNT_DEVICE_CLIENT_MAX_COUNT);

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_count(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    MCM_DTYPE_EK_TD i, count1, count2;


    DMSG("count test :");

    // device 的資料筆數.
    path1 = "device";
    if(mcm_config_get_count_by_path(this_session, path1, &count1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_count_by_path(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count1);

    // device.system 的資料筆數.
    path1 = "device.system";
    if(mcm_config_get_count_by_path(this_session, path1, &count1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_count_by_path(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count1);

    // device.vap.* 的資料筆數.
    path1 = "device.vap.*";
    if(mcm_config_get_count_by_path(this_session, path1, &count1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_count_by_path(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count1);

    // device.vap.{i}.extra 的資料筆數.
    for(i = 0; i < count1; i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.@%u.extra", i + 1);
        if(mcm_config_get_count_by_path(this_session, path2, &count2) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_get_count_by_path(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[count] %s = " MCM_DTYPE_EK_PF, path2, count2);
    }

    // device.vap.{i}.station.* 的資料筆數.
    for(i = 0; i < count1; i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        if(mcm_config_get_count_by_path(this_session, path2, &count2) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_get_count_by_path(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[count] %s = " MCM_DTYPE_EK_PF, path2, count2);
    }

    // device.limit.* 的資料筆數.
    path1 = "device.limit.*";
    if(mcm_config_get_count_by_path(this_session, path1, &count1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_count_by_path(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count1);

    // device.client.* 的資料筆數.
    path1 = "device.client.*";
    if(mcm_config_get_count_by_path(this_session, path1, &count1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_count_by_path(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count1);

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_get_alone(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1;
    MCM_DTYPE_DS_TD tmp_status;
    MCM_DTYPE_S_TD date[MCM_BSIZE_DEVICE_SYSTEM_DATE];
    MCM_DTYPE_S_TD ip_addr[MCM_BSIZE_DEVICE_SYSTEM_IP_ADDR];
    MCM_DTYPE_IULL_TD uptime;
    MCM_DTYPE_FD_TD loading;


    DMSG("get-alone test :");

    // 目標 : device.system.date
    path1 = "device.system.date";
    // 取得狀態.
    if(mcm_config_get_alone_status_by_path(this_session, path1, &tmp_status) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_alone_status_by_path(%s) fail", path1);
        goto FREE_01;
    }
    tmp_status &= MCM_DSCHANGE_MASK;
    DMSG("[status] %s [%s]", path1, dump_status(tmp_status));
    // 取得資料 (SYS 模式).
    if(mcm_config_get_alone_by_path(this_session, path1, MCM_DACCESS_SYS, date) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_alone_by_path(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-alone][SYS] %s = " MCM_DTYPE_S_PF, path1, date);
    // 如果有設定, 取得資料 (NEW 模式).
    if(tmp_status == MCM_DSCHANGE_SET)
    {
        if(mcm_config_get_alone_by_path(this_session, path1, MCM_DACCESS_NEW, date)
                                        < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_get_alone_by_path(%s) fail", path1);
            goto FREE_01;
        }
        DMSG("[get-alone][NEW] %s = " MCM_DTYPE_S_PF, path1, date);
    }

    // 目標 : device.system.ip_addr
    path1 = "device.system.ip_addr";
    // 取得狀態.
    if(mcm_config_get_alone_status_by_path(this_session, path1, &tmp_status) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_alone_status_by_path(%s) fail", path1);
        goto FREE_01;
    }
    tmp_status &= MCM_DSCHANGE_MASK;
    DMSG("[status] %s [%s]", path1, dump_status(tmp_status));
    // 取得資料 (SYS 模式).
    if(mcm_config_get_alone_by_path(this_session, path1, MCM_DACCESS_SYS, ip_addr)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_alone_by_path(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-alone][SYS] %s = " MCM_DTYPE_S_PF, path1, ip_addr);
    // 如果有設定, 取得資料 (NEW 模式).
    if(tmp_status == MCM_DSCHANGE_SET)
    {
        if(mcm_config_get_alone_by_path(this_session, path1, MCM_DACCESS_NEW, ip_addr)
                                        < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_get_alone_by_path(%s) fail", path1);
            goto FREE_01;
        }
        DMSG("[get-alone][NEW] %s = " MCM_DTYPE_S_PF, path1, ip_addr);
    }

    // 目標 : device.system.uptime
    path1 = "device.system.uptime";
    // 取得狀態.
    if(mcm_config_get_alone_status_by_path(this_session, path1, &tmp_status) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_alone_status_by_path(%s) fail", path1);
        goto FREE_01;
    }
    tmp_status &= MCM_DSCHANGE_MASK;
    DMSG("[status] %s [%s]", path1, dump_status(tmp_status));
    // 取得資料 (SYS 模式).
    if(mcm_config_get_alone_by_path(this_session, path1, MCM_DACCESS_SYS, &uptime) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_alone_by_path(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-alone][SYS] %s = " MCM_DTYPE_IULL_PF, path1, uptime);
    // 如果有設定, 取得資料 (NEW 模式).
    if(tmp_status == MCM_DSCHANGE_SET)
    {
        if(mcm_config_get_alone_by_path(this_session, path1, MCM_DACCESS_NEW, &uptime)
                                        < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_get_alone_by_path(%s) fail", path1);
            goto FREE_01;
        }
        DMSG("[get-alone][NEW] %s = " MCM_DTYPE_IULL_PF, path1, uptime);
    }

    // 目標 : device.system.loading
    path1 = "device.system.loading";
    // 取得狀態.
    if(mcm_config_get_alone_status_by_path(this_session, path1, &tmp_status) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_alone_status_by_path(%s) fail", path1);
        goto FREE_01;
    }
    tmp_status &= MCM_DSCHANGE_MASK;
    DMSG("[status] %s [%s]", path1, dump_status(tmp_status));
    // 取得資料 (SYS 模式).
    if(mcm_config_get_alone_by_path(this_session, path1, MCM_DACCESS_SYS, &loading)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_alone_by_path(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-alone][SYS] %s = " MCM_DTYPE_FD_PF, path1, loading);
    // 如果有設定, 取得資料 (NEW 模式).
    if(tmp_status == MCM_DSCHANGE_SET)
    {
        if(mcm_config_get_alone_by_path(this_session, path1, MCM_DACCESS_NEW, &loading)
                                        < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_get_alone_by_path(%s) fail", path1);
            goto FREE_01;
        }
        DMSG("[get-alone][NEW] %s = " MCM_DTYPE_FD_PF, path1, loading);
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_get_entry(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    MCM_DTYPE_EK_TD count, i;
    MCM_DTYPE_DS_TD tmp_status;
    struct mcm_dv_device_vap_t vap_v;


    DMSG("get-entry test :");

    // device.vap.* 的資料筆數.
    path1 = "device.vap.*";
    if(mcm_config_get_count_by_path(this_session, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_count_by_path(%s) fail", path1);
        goto FREE_01;
    }

    // 目標 : device.vap.*
    for(i = 0; i < count; i++)
    {
        // 填充路徑.
        snprintf(path2, sizeof(path2), "device.vap.@%u", i + 1);

        // 取得狀態.
        if(mcm_config_get_entry_self_status_by_path(this_session, path2, &tmp_status)
                                                    < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_get_entry_self_status_by_path(%s) fail", path2);
            goto FREE_01;
        }
        tmp_status &= MCM_DSCHANGE_MASK;
        DMSG("[status] %s [%s]", path2, dump_status(tmp_status));

        // 沒做任何事.
        if(tmp_status == MCM_DSCHANGE_NONE)
        {
            // 取得資料 (SYS 模式).
            if(mcm_config_get_entry_by_path(this_session, path2, MCM_DACCESS_SYS, &vap_v)
                                            < MCM_RCODE_PASS)
            {
                DMSG("call mcm_config_get_entry_by_path(%s) fail", path2);
                goto FREE_01;
            }
            DMSG("[get-entry][SYS] %s.ekey = " MCM_DTYPE_EK_PF, path2, vap_v.ekey);
            DMSG("[get-entry][SYS] %s.ssid = " MCM_DTYPE_S_PF, path2, vap_v.ssid);
            DMSG("[get-entry][SYS] %s.channel = " MCM_DTYPE_IUI_PF, path2, vap_v.channel);
        }
        else
        // 做設定.
        if(tmp_status == MCM_DSCHANGE_SET)
        {
            // 取得資料 (SYS 模式).
            if(mcm_config_get_entry_by_path(this_session, path2, MCM_DACCESS_SYS, &vap_v)
                                            < MCM_RCODE_PASS)
            {
                DMSG("call mcm_config_get_entry_by_path(%s) fail", path2);
                goto FREE_01;
            }
            DMSG("[get-entry][SYS] %s.ekey = " MCM_DTYPE_EK_PF, path2, vap_v.ekey);
            DMSG("[get-entry][SYS] %s.ssid = " MCM_DTYPE_S_PF, path2, vap_v.ssid);
            DMSG("[get-entry][SYS] %s.channel = " MCM_DTYPE_IUI_PF, path2, vap_v.channel);
            // 取得資料 (NEW 模式).
            if(mcm_config_get_entry_by_path(this_session, path2, MCM_DACCESS_NEW, &vap_v)
                                            < MCM_RCODE_PASS)
            {
                DMSG("call mcm_config_get_entry_by_path(%s) fail", path2);
                goto FREE_01;
            }
            DMSG("[get-entry][NEW] %s.ekey = " MCM_DTYPE_EK_PF, path2, vap_v.ekey);
            DMSG("[get-entry][NEW] %s.ssid = " MCM_DTYPE_S_PF, path2, vap_v.ssid);
            DMSG("[get-entry][NEW] %s.channel = " MCM_DTYPE_IUI_PF, path2, vap_v.channel);
        }
        else
        // 做增加.
        if(tmp_status == MCM_DSCHANGE_ADD)
        {
            // 取得資料 (NEW 模式).
            if(mcm_config_get_entry_by_path(this_session, path2, MCM_DACCESS_NEW, &vap_v)
                                            < MCM_RCODE_PASS)
            {
                DMSG("call mcm_config_get_entry_by_path(%s) fail", path2);
                goto FREE_01;
            }
            DMSG("[get-entry][NEW] %s.ekey = " MCM_DTYPE_EK_PF, path2, vap_v.ekey);
            DMSG("[get-entry][NEW] %s.ssid = " MCM_DTYPE_S_PF, path2, vap_v.ssid);
            DMSG("[get-entry][NEW] %s.channel = " MCM_DTYPE_IUI_PF, path2, vap_v.channel);
        }
        else
        // 做刪除.
        if(tmp_status == MCM_DSCHANGE_DEL)
        {
            // 取得資料 (SYS 模式).
            if(mcm_config_get_entry_by_path(this_session, path2, MCM_DACCESS_SYS, &vap_v)
                                            < MCM_RCODE_PASS)
            {
                DMSG("call mcm_config_get_entry_by_path(%s) fail", path2);
                goto FREE_01;
            }
            DMSG("[get-entry][SYS] %s.ekey = " MCM_DTYPE_EK_PF, path2, vap_v.ekey);
            DMSG("[get-entry][SYS] %s.ssid = " MCM_DTYPE_S_PF, path2, vap_v.ssid);
            DMSG("[get-entry][SYS] %s.channel = " MCM_DTYPE_IUI_PF, path2, vap_v.channel);
        }
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_set_alone(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1;
    MCM_DTYPE_S_TD descript[MCM_BSIZE_DEVICE_DESCRIPT];
    MCM_DTYPE_B_TD serial_number[MCM_BSIZE_DEVICE_SERIAL_NUMBER];
    MCM_DTYPE_USIZE_TD i;


    DMSG("set-alone test :");

    srand(time(NULL));

    // 設定 device.descript
    path1 = "device.descript";
    snprintf(descript, sizeof(descript), "SOHO Router %u", rand());
    DMSG("[set-alone][SYS] %s = " MCM_DTYPE_S_PF, path1, descript);
    if(mcm_config_set_alone_by_path(this_session, path1, MCM_DACCESS_SYS, descript,
                                    strlen(descript)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_set_alone_by_path(%s) fail", path1);
        goto FREE_01;
    }

    // 設定 device.serial_number
    path1 = "device.serial_number";
    for(i = 0; i < sizeof(serial_number); i++)
        serial_number[i] = rand() % 256;
    DMSG("[set-alone][SYS] %s = "
         MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF
         MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF,
         path1,
         serial_number[0], serial_number[1], serial_number[2], serial_number[3], serial_number[4],
         serial_number[5], serial_number[6], serial_number[7], serial_number[8], serial_number[9]);
    if(mcm_config_set_alone_by_path(this_session, path1, MCM_DACCESS_SYS, serial_number,
                                    sizeof(serial_number)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_set_alone_by_path(%s) fail", path1);
        goto FREE_01;
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_set_entry(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1;
    struct mcm_dv_device_system_t system_v;


    DMSG("set-entry test :");

    srand(time(NULL));

    // 設定 device.system
    path1 = "device.system";
    memset(&system_v, 0, sizeof(system_v));
    snprintf(system_v.date, sizeof(system_v.date), "2134/12/%02u", rand() % 31);
    snprintf(system_v.ip_addr, sizeof(system_v.ip_addr), "192.168.20.%u", rand() % 255);
    system_v.uptime = rand();
    system_v.loading = ((MCM_DTYPE_FD_TD) 35000) / ((rand() % 100) + 1);
    DMSG("[set-entry][SYS] %s.date = " MCM_DTYPE_S_PF, path1, system_v.date);
    DMSG("[set-entry][SYS] %s.ip_addr = " MCM_DTYPE_S_PF, path1, system_v.ip_addr);
    DMSG("[set-entry][SYS] %s.uptime = " MCM_DTYPE_IULL_PF, path1, system_v.uptime);
    DMSG("[set-entry][SYS] %s.loading = " MCM_DTYPE_FD_PF, path1, system_v.loading);
    if(mcm_config_set_entry_by_path(this_session, path1, MCM_DACCESS_SYS, &system_v)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_set_entry_by_path(%s) fail", path1);
        goto FREE_01;
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_del_usablekey_add_entry(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    MCM_DTYPE_EK_TD count, key, i;
    struct mcm_dv_device_limit_t limit_v;


    DMSG("del-entry test :");
    DMSG("usable-key test :");
    DMSG("add-entry test :");

    srand(time(NULL));

    // device.limit.* 的資料筆數.
    path1 = "device.limit.*";
    if(mcm_config_get_count_by_path(this_session, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_count_by_path(%s) fail", path1);
        goto FREE_01;
    }

    // 刪除每一筆 device.limit.*
    for(i = 0; i < count; i++)
    {
        // 刪除第一筆, 刪除 count 次.
        path1 = "device.limit.@1";
        DMSG("[del-entry][SYS] %s", path1);
        if(mcm_config_del_entry_by_path(this_session, path1, MCM_DACCESS_SYS) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_del_entry_by_path(%s) fail", path1);
            goto FREE_01;
        }
    }

    // 增加 device.limit.*
    for(i = 0; i < MCM_MCOUNT_DEVICE_LIMIT_MAX_COUNT; i++)
    {
        // 取得可用的 key
        path1 = "device.limit.*";
        if(mcm_config_get_usable_key_by_path(this_session, path1, &key) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_get_usable_key_by_path(%s) fail", path1);
            goto FREE_01;
        }
        if(key == 0)
        {
            DMSG("[usable-key] %s = entry is full", path1);
            goto FREE_01;
        }
        else
        {
            DMSG("[usable-key] %s = " MCM_DTYPE_EK_PF, path1, key);
        }

        // 填充路徑.
        snprintf(path2, sizeof(path2), "device.limit.#%u", key);
        // 增加.
        memset(&limit_v, 0, sizeof(limit_v));
        snprintf(limit_v.name, sizeof(limit_v.name), "limit rule %u", rand());
        limit_v.priority = (rand() % 30000) * ((rand() % 3) - 1);
        DMSG("[add-entry][SYS] %s.name = " MCM_DTYPE_S_PF, path2, limit_v.name);
        DMSG("[add-entry][SYS] %s.priority = " MCM_DTYPE_ISI_PF, path2, limit_v.priority);
        if(mcm_config_add_entry_by_path(this_session, path2, MCM_DACCESS_SYS, &limit_v)
                                        < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_add_entry_by_path(%s) fail", path2);
            goto FREE_01;
        }
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_delall_add_entry(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    MCM_DTYPE_EK_TD i;
    struct mcm_dv_device_client_t client_v;


    DMSG("del-all-entry test :");
    DMSG("add-entry test :");

    srand(time(NULL));

    // 刪除所有的 device.client.*
    path1 = "device.client.*";
    DMSG("[del-all-entry][SYS] %s", path1);
    if(mcm_config_del_all_entry_by_path(this_session, path1, MCM_DACCESS_SYS) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_del_all_entry_by_path(%s) fail", path1);
        goto FREE_01;
    }

    // 增加 device.client.*
    for(i = 0; i < MCM_MCOUNT_DEVICE_CLIENT_MAX_COUNT; i++)
    {
        // 填充路徑.
        snprintf(path2, sizeof(path2), "device.client.#%u", i + 1);
        // 增加.
        memset(&client_v, 0, sizeof(client_v));
        snprintf(client_v.mac_addr, sizeof(client_v.mac_addr), "01:23:45:67:%02X", rand() % 255);
        client_v.location_x = ((MCM_DTYPE_FLD_TD) 45000) / ((rand() % 100) + 1);
        client_v.location_y = ((MCM_DTYPE_FLD_TD) 55000) / ((rand() % 100) + 1);
        DMSG("[add-entry][SYS] %s.mac_addr = " MCM_DTYPE_S_PF, path2, client_v.mac_addr);
        DMSG("[add-entry][SYS] %s.location_x = " MCM_DTYPE_FLD_PF, path2, client_v.location_x);
        DMSG("[add-entry][SYS] %s.location_y = " MCM_DTYPE_FLD_PF, path2, client_v.location_y);
        if(mcm_config_add_entry_by_path(this_session, path2, MCM_DACCESS_SYS, &client_v)
                                        < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_add_entry_by_path(%s) fail", path2);
            goto FREE_01;
        }
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

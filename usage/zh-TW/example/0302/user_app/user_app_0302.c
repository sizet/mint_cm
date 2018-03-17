// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mcm_lib/mcm_lheader/mcm_type.h"
#include "mcm_lib/mcm_lheader/mcm_size.h"
#include "mcm_lib/mcm_lheader/mcm_connect.h"
#include "mcm_lib/mcm_lheader/mcm_return.h"
#include "mcm_lib/mcm_lheader/mcm_data_exinfo_auto.h"
#include "mcm_lib/mcm_lulib/mcm_lulib_api.h"


#define DMSG(msg_fmt, msgs...) printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msgs)


int case_max_count(
    struct mcm_lulib_lib_t *this_lulib)
{
    int ret_val = -1;
    char *path1;
    MCM_DTYPE_EK_TD count;


    DMSG("max-count test 01 :");

    // device 的資料筆數上限.
    path1 = "device";
    if(mcm_lulib_get_max_count(this_lulib, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_max_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, count);

    // device.system 的資料筆數上限.
    path1 = "device.system";
    if(mcm_lulib_get_max_count(this_lulib, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_max_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, count);

    // device.vap.* 的資料筆數上限.
    path1 = "device.vap.*";
    if(mcm_lulib_get_max_count(this_lulib, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_max_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, count);

    // device.vap.*.extra 的資料筆數上限.
    path1 = "device.vap.*.extra";
    if(mcm_lulib_get_max_count(this_lulib, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_max_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, count);

    // device.vap.*.station.* 的資料筆數上限.
    path1 = "device.vap.*.station.*";
    if(mcm_lulib_get_max_count(this_lulib, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_max_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, count);

    // device.limit.* 的資料筆數上限.
    path1 = "device.limit.*";
    if(mcm_lulib_get_max_count(this_lulib, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_max_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, count);

    // device.client.* 的資料筆數上限.
    path1 = "device.client.*";
    if(mcm_lulib_get_max_count(this_lulib, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_max_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, count);

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

    ret_val = 0;
FREE_01:
    return ret_val;
}

int case_count(
    struct mcm_lulib_lib_t *this_lulib)
{
    int ret_val = -1;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    MCM_DTYPE_EK_TD i, count1, count2;


    DMSG("count test :");

    // device 的資料筆數.
    path1 = "device";
    if(mcm_lulib_get_count(this_lulib, path1, &count1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count1);

    // device.system 的資料筆數.
    path1 = "device.system";
    if(mcm_lulib_get_count(this_lulib, path1, &count1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count1);

    // device.vap.* 的資料筆數.
    path1 = "device.vap.*";
    if(mcm_lulib_get_count(this_lulib, path1, &count1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count1);

    // device.vap.{i}.extra 的資料筆數.
    for(i = 0; i < count1; i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.@%u.extra", i + 1);
        if(mcm_lulib_get_count(this_lulib, path2, &count2) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_get_count(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[count] %s = " MCM_DTYPE_EK_PF, path2, count2);
    }

    // device.vap.{i}.station.* 的資料筆數.
    for(i = 0; i < count1; i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        if(mcm_lulib_get_count(this_lulib, path2, &count2) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_get_count(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[count] %s = " MCM_DTYPE_EK_PF, path2, count2);
    }

    // device.limit.* 的資料筆數.
    path1 = "device.limit.*";
    if(mcm_lulib_get_count(this_lulib, path1, &count1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count1);

    // device.client.* 的資料筆數.
    path1 = "device.client.*";
    if(mcm_lulib_get_count(this_lulib, path1, &count1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count1);

    ret_val = 0;
FREE_01:
    return ret_val;
}

int case_get_alone(
    struct mcm_lulib_lib_t *this_lulib)
{
    int ret_val = -1;
    char *path1;
    MCM_DTYPE_S_TD descript[MCM_BSIZE_DEVICE_DESCRIPT];
    MCM_DTYPE_B_TD serial_number[MCM_BSIZE_DEVICE_SERIAL_NUMBER];
    MCM_DTYPE_FD_TD loading;
    MCM_DTYPE_IUI_TD channel;
    MCM_DTYPE_ISS_TD tx_power;
    MCM_DTYPE_RK_TD rule;
    MCM_DTYPE_S_TD name[MCM_BSIZE_DEVICE_LIMIT_NAME];


    DMSG("get alone test :");

    // 讀出 device.descript
    path1 = "device.descript";
    if(mcm_lulib_get_alone(this_lulib, path1, descript) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_alone(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-alone] %s = " MCM_DTYPE_S_PF, path1, descript);

    // 讀出 device.serial_number
    path1 = "device.serial_number";
    if(mcm_lulib_get_alone(this_lulib, path1, serial_number) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_alone(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-alone] %s = "
         MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF
         MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF,
         path1,
         serial_number[0], serial_number[1], serial_number[2], serial_number[3], serial_number[4],
         serial_number[5], serial_number[6], serial_number[7], serial_number[8], serial_number[9]);

    // 讀出 device.system.loading
    path1 = "device.system.loading";
    if(mcm_lulib_get_alone(this_lulib, path1, &loading) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_alone(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-alone] %s = " MCM_DTYPE_FD_PF, path1, loading);

    // 讀出 device.vap.#8.channel
    path1 = "device.vap.#8.channel";
    if(mcm_lulib_get_alone(this_lulib, path1, &channel) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_alone(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-alone] %s = " MCM_DTYPE_IUI_PF, path1, channel);

    // 讀出 device.vap.#23.extra.tx_power
    path1 = "device.vap.#23.extra.tx_power";
    if(mcm_lulib_get_alone(this_lulib, path1, &tx_power) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_alone(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-alone] %s = " MCM_DTYPE_ISS_PF, path1, tx_power);

    // 讀出 device.vap.@2.station.#33.rule
    path1 = "device.vap.@2.station.#33.rule";
    if(mcm_lulib_get_alone(this_lulib, path1, &rule) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_alone(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-alone] %s = " MCM_DTYPE_RK_PF, path1, rule);

    // 讀出 device.limit.@1.name
    path1 = "device.limit.@1.name";
    if(mcm_lulib_get_alone(this_lulib, path1, name) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_alone(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-alone] %s = " MCM_DTYPE_S_PF, path1, name);

    ret_val = 0;
FREE_01:
    return ret_val;
}

int case_set_alone(
    struct mcm_lulib_lib_t *this_lulib)
{
    int ret_val = -1;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    MCM_DTYPE_S_TD descript[MCM_BSIZE_DEVICE_DESCRIPT];
    MCM_DTYPE_B_TD serial_number[MCM_BSIZE_DEVICE_SERIAL_NUMBER];
    MCM_DTYPE_FD_TD loading;
    MCM_DTYPE_IUI_TD channel;
    MCM_DTYPE_ISC_TD hidden;
    MCM_DTYPE_RK_TD rule;
    MCM_DTYPE_S_TD name[MCM_BSIZE_DEVICE_LIMIT_NAME];
    MCM_DTYPE_EK_TD vap_count, vap_key, station_count, i, j;
    MCM_DTYPE_USIZE_TD k;


    DMSG("set alone test :");

    // 設定 device.descript
    path1 = "device.descript";
    snprintf(descript, sizeof(descript), "Network-Device-%u", rand() % 100);
    DMSG("[set-alone] %s = " MCM_DTYPE_S_PF, path1, descript);
    if(mcm_lulib_set_alone(this_lulib, path1, descript, strlen(descript)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_set_alone(%s) fail", path1);
        goto FREE_01;
    }

    // 設定 device.serial_number
    path1 = "device.serial_number";
    for(k = 0; k < sizeof(serial_number); k++)
        serial_number[k] = rand() % 256;
    DMSG("[set-alone] %s = "
         MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF
         MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF,
         path1,
         serial_number[0], serial_number[1], serial_number[2], serial_number[3], serial_number[4],
         serial_number[5], serial_number[6], serial_number[7], serial_number[8], serial_number[9]);
    if(mcm_lulib_set_alone(this_lulib, path1, serial_number, sizeof(serial_number))
                           < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_set_alone(%s) fail", path1);
        goto FREE_01;
    }

    // 設定 device.system.loading
    path1 = "device.system.loading";
    loading = ((MCM_DTYPE_FD_TD) 2500) / ((rand() % 100) + 1);
    DMSG("[set-alone] %s = " MCM_DTYPE_FD_PF, path1, loading);
    if(mcm_lulib_set_alone(this_lulib, path1, &loading, sizeof(loading)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_set_alone(%s) fail", path1);
        goto FREE_01;
    }

    // 設定每個 device.vap.*.channel
    // 取得 device.vap.* 的資料筆數.
    path1 = "device.vap.*";
    if(mcm_lulib_get_count(this_lulib, path1, &vap_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    for(i = 0; i < vap_count; i++)
    {
        // 設定 device.vap.{i}.channel
        snprintf(path2, sizeof(path2), "device.vap.@%u.channel", i + 1);
        channel = rand() % 200;
        DMSG("[set-alone] %s = " MCM_DTYPE_IUI_PF, path2, channel);
        if(mcm_lulib_set_alone(this_lulib, path2, &channel, sizeof(channel)) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_set_alone(%s) fail", path2);
            goto FREE_01;
        }
    }

    // 設定每個 device.vap.{i}.extra.hidden
    for(i = 0; i < vap_count; i++)
    {
        // 讀出 device.vap.{i}.ekey (使用 key 模式做設定).
        snprintf(path2, sizeof(path2), "device.vap.@%u.ekey", i + 1);
        if(mcm_lulib_get_alone(this_lulib, path2, &vap_key) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_get_alone(%s) fail", path2);
            goto FREE_01;
        }

        // 設定 device.vap.{vap_key}.extra.hidden
        snprintf(path2, sizeof(path2), "device.vap.#%u.extra.hidden", vap_key);
        hidden = rand() % 2;
        DMSG("[set-alone] %s = " MCM_DTYPE_ISC_PF, path2, hidden);
        if(mcm_lulib_set_alone(this_lulib, path2, &hidden, sizeof(hidden)) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_set_alone(%s) fail", path2);
            goto FREE_01;
        }
    }

    // 設定每個 device.vap.{i}.station.*.rule
    for(i = 0; i < vap_count; i++)
    {
        // 取得 device.vap.{i}.station.* 的資料筆數.
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        if(mcm_lulib_get_count(this_lulib, path2, &station_count) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_get_count(%s) fail", path2);
            goto FREE_01;
        }
        for(j = 0; j < station_count; j++)
        {
            // 設定 device.vap.{i}.station.{j}.rule
            snprintf(path2, sizeof(path2), "device.vap.@%u.station.@%u.rule", i + 1, j + 1);
            rule = rand() % 32;
            DMSG("[set-alone] %s = " MCM_DTYPE_RK_PF, path2, rule);
            if(mcm_lulib_set_alone(this_lulib, path2, &rule, sizeof(rule)) < MCM_RCODE_PASS)
            {
                DMSG("call mcm_lulib_set_alone(%s) fail", path2);
                goto FREE_01;
            }    
        }
    }

    // 設定 device.limit.#5.name
    path1 = "device.limit.#5.name";
    snprintf(name, sizeof(name), "level-%04u", rand() % 10000);
    DMSG("[set-alone] %s = " MCM_DTYPE_S_PF, path1, name);
    if(mcm_lulib_set_alone(this_lulib, path1, &name, strlen(name)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_set_alone(%s) fail", path1);
        goto FREE_01;
    }

    ret_val = 0;
FREE_01:
    return ret_val;
}

int case_get_entry(
    struct mcm_lulib_lib_t *this_lulib)
{
    int ret_val = -1;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_dv_device_t device_v;
    struct mcm_dv_device_system_t system_v;
    struct mcm_dv_device_vap_t vap_v;
    struct mcm_dv_device_vap_extra_t extra_v;
    struct mcm_dv_device_vap_station_t station_v;
    struct mcm_dv_device_limit_t limit_v;
    MCM_DTYPE_EK_TD vap_count, station_count, i, j;


    DMSG("get entry test :");

    // 讀出 device
    path1 = "device";
    if(mcm_lulib_get_entry(this_lulib, path1, &device_v) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_entry(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-entry] %s.descript = " MCM_DTYPE_S_PF,
         path1, device_v.descript);
    DMSG("[get-entry] %s.serial_number = "
         MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF
         MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF,
         path1,
         device_v.serial_number[0], device_v.serial_number[1], device_v.serial_number[2],
         device_v.serial_number[3], device_v.serial_number[4], device_v.serial_number[5],
         device_v.serial_number[6], device_v.serial_number[7], device_v.serial_number[8],
         device_v.serial_number[9]);

    // 讀出 device.system
    path1 = "device.system";
    if(mcm_lulib_get_entry(this_lulib, path1, &system_v) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_entry(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-entry] %s.date = " MCM_DTYPE_S_PF, path1, system_v.date);
    DMSG("[get-entry] %s.ip_addr = " MCM_DTYPE_S_PF, path1, system_v.ip_addr);
    DMSG("[get-entry] %s.uptime = " MCM_DTYPE_IULL_PF, path1, system_v.uptime);
    DMSG("[get-entry] %s.loading = " MCM_DTYPE_FD_PF, path1, system_v.loading);

    // 讀出每個 device.vap.*
    // 取得 device.vap.* 的資料筆數.
    path1 = "device.vap.*";
    if(mcm_lulib_get_count(this_lulib, path1, &vap_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    for(i = 0; i < vap_count; i++)
    {
        // 讀出 device.vap.{i}
        snprintf(path2, sizeof(path2), "device.vap.@%u", i + 1);
        if(mcm_lulib_get_entry(this_lulib, path2, &vap_v) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_get_entry(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[get-entry] %s.ssid = " MCM_DTYPE_S_PF, path2, vap_v.ssid);
        DMSG("[get-entry] %s.channel = " MCM_DTYPE_IUI_PF, path2, vap_v.channel);
    }

    // 讀出每個 device.vap.{i}.extra
    for(i = 0; i < vap_count; i++)
    {
        // 讀出 device.vap.{i}.extra
        snprintf(path2, sizeof(path2), "device.vap.@%u.extra", i + 1);
        if(mcm_lulib_get_entry(this_lulib, path2, &extra_v) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_get_entry(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[get-entry] %s.hidden = " MCM_DTYPE_ISC_PF, path2, extra_v.hidden);
        DMSG("[get-entry] %s.tx_power = " MCM_DTYPE_ISS_PF, path2, extra_v.tx_power);
    }

    // 讀出每個 device.vap.{i}.station.*
    for(i = 0; i < vap_count; i++)
    {
        // 取得 device.vap.{i} 的資料筆數.
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        if(mcm_lulib_get_count(this_lulib, path2, &station_count) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_get_count(%s) fail", path2);
            goto FREE_01;
        }
        for(j = 0; j < station_count; j++)
        {
            // 讀出 device.vap.{i}.station.{j}
            snprintf(path2, sizeof(path2), "device.vap.@%u.station.@%u", i + 1, j + 1);
            if(mcm_lulib_get_entry(this_lulib, path2, &station_v) < MCM_RCODE_PASS)
            {
                DMSG("call mcm_lulib_get_entry(%s) fail", path2);
                goto FREE_01;
            }
            DMSG("[get-entry] %s.mac_addr = " MCM_DTYPE_S_PF, path2, station_v.mac_addr);
            DMSG("[get-entry] %s.rule = " MCM_DTYPE_RK_PF, path2, station_v.rule);
        }
    }

    // 讀出 device.limit.#5
    path1 = "device.limit.#5";
    if(mcm_lulib_get_entry(this_lulib, path1, &limit_v) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_entry(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-entry] %s.name = " MCM_DTYPE_S_PF, path1, limit_v.name);
    DMSG("[get-entry] %s.priority = " MCM_DTYPE_ISI_PF, path1, limit_v.priority);

    ret_val = 0;
FREE_01:
    return ret_val;
}

int case_set_entry(
    struct mcm_lulib_lib_t *this_lulib)
{
    int ret_val = -1;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_dv_device_t device_v;
    struct mcm_dv_device_system_t system_v;
    struct mcm_dv_device_vap_t vap_v;
    struct mcm_dv_device_vap_extra_t extra_v;
    struct mcm_dv_device_vap_station_t station_v;
    struct mcm_dv_device_limit_t limit_v;
    MCM_DTYPE_EK_TD vap_count, vap_key, station_count, station_key, i, j;
    MCM_DTYPE_USIZE_TD k;


    DMSG("set entry test :");

    // 設定 device
    path1 = "device";
    memset(&device_v, 0, sizeof(device_v));
    snprintf(device_v.descript, sizeof(device_v.descript), "Wireless Device %u", rand() % 10000);
    for(k = 0; k < sizeof(device_v.serial_number); k++)
        device_v.serial_number[k] = rand() % 256;
    DMSG("[set-entry] %s.descript = " MCM_DTYPE_S_PF, path1, device_v.descript);
    DMSG("[set-entry] %s.serial_number = "
         MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF
         MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF,
         path1,
         device_v.serial_number[0], device_v.serial_number[1], device_v.serial_number[2],
         device_v.serial_number[3], device_v.serial_number[4], device_v.serial_number[5],
         device_v.serial_number[6], device_v.serial_number[7], device_v.serial_number[8],
         device_v.serial_number[9]);
    if(mcm_lulib_set_entry(this_lulib, path1, &device_v, sizeof(device_v)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_set_entry(%s) fail", path1);
        goto FREE_01;
    }

    // 設定 device.system
    path1 = "device.system";
    memset(&system_v, 0, sizeof(system_v));
    snprintf(system_v.date, sizeof(system_v.date), "2011/01/%02u", rand() % 30);
    snprintf(system_v.ip_addr, sizeof(system_v.ip_addr), "192.168.100.%u", (rand() % 253) + 1);
    system_v.uptime = rand();
    system_v.loading = ((MCM_DTYPE_FD_TD) -50000) / ((rand() % 30) + 1);
    DMSG("[set-entry] %s.date = " MCM_DTYPE_S_PF, path1, system_v.date);
    DMSG("[set-entry] %s.ip_addr = " MCM_DTYPE_S_PF, path1, system_v.ip_addr);
    DMSG("[set-entry] %s.uptime = " MCM_DTYPE_IULL_PF, path1, system_v.uptime);
    DMSG("[set-entry] %s.loading = " MCM_DTYPE_FD_PF, path1, system_v.loading);
    if(mcm_lulib_set_entry(this_lulib, path1, &system_v, sizeof(system_v)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_set_entry(%s) fail", path1);
        goto FREE_01;
    }

    // 設定每個 device.vap.*
    // 取得 device.vap.* 的資料筆數.
    path1 = "device.vap.*";
    if(mcm_lulib_get_count(this_lulib, path1, &vap_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    for(i = 0; i < vap_count; i++)
    {
        // 讀出 device.vap.{i} (使用 key 模式做設定).
        snprintf(path2, sizeof(path2), "device.vap.@%u", i + 1);
        if(mcm_lulib_get_entry(this_lulib, path2, &vap_v) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_get_entry(%s) fail", path2);
            goto FREE_01;
        }
        vap_key = vap_v.ekey;

        // 設定到 device.vap.{vap_key}
        snprintf(path2, sizeof(path2), "device.vap.#%u", vap_key);
        memset(&vap_v, 0, sizeof(vap_v));
        snprintf(vap_v.ssid, sizeof(vap_v.ssid), "open-%u", rand() % 100);
        vap_v.channel = rand() % 200;
        DMSG("[set-entry] %s.ssid = " MCM_DTYPE_S_PF, path2, vap_v.ssid);
        DMSG("[set-entry] %s.channel = " MCM_DTYPE_IUI_PF, path2, vap_v.channel);
        if(mcm_lulib_set_entry(this_lulib, path2, &vap_v, sizeof(vap_v)) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_set_entry(%s) fail", path2);
            goto FREE_01;
        }
    }

    // 設定每個 device.vap.{i}.extra
    for(i = 0; i < vap_count; i++)
    {
        // 設定到 device.vap.{vap_key}.extra
        snprintf(path2, sizeof(path2), "device.vap.@%u.extra", i + 1);
        memset(&extra_v, 0, sizeof(extra_v));
        extra_v.hidden = rand() % 2;
        extra_v.tx_power = (rand() % 100) - 50;
        DMSG("[set-entry] %s.hidden = " MCM_DTYPE_ISC_PF, path2, extra_v.hidden);
        DMSG("[set-entry] %s.tx_power = " MCM_DTYPE_ISS_PF, path2, extra_v.tx_power);
        if(mcm_lulib_set_entry(this_lulib, path2, &extra_v, sizeof(extra_v)) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_set_entry(%s) fail", path2);
            goto FREE_01;
        }
    }

    // 設定每個 device.vap.{i}.station.*
    for(i = 0; i < vap_count; i++)
    {
        // 取得 device.vap.{i}.station.* 的資料筆數.
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        if(mcm_lulib_get_count(this_lulib, path2, &station_count) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_get_count(%s) fail", path2);
            goto FREE_01;
        }
        for(j = 0; j < station_count; j++)
        {
            // 讀出 device.vap.{i}.station.{j} (使用 key 模式做設定).
            snprintf(path2, sizeof(path2), "device.vap.@%u.station.@%u", i + 1, j + 1);
            if(mcm_lulib_get_entry(this_lulib, path2, &station_v) < MCM_RCODE_PASS)
            {
                DMSG("call mcm_lulib_get_entry(%s) fail", path2);
                goto FREE_01;
            }
            station_key = station_v.ekey;

            // 設定到 device.vap.{i}.station.{station_key}
            snprintf(path2, sizeof(path2), "device.vap.@%u.station.#%u", i + 1, station_key);
            memset(&station_v, 0, sizeof(station_v));
            snprintf(station_v.mac_addr, sizeof(station_v.mac_addr), "11:AA:22:BB:3C:%02X",
                     rand() % 255);
            station_v.rule = rand() % 50;
            DMSG("[set-entry] %s.mac_addr = " MCM_DTYPE_S_PF, path2, station_v.mac_addr);
            DMSG("[set-entry] %s.rule = " MCM_DTYPE_RK_PF, path2, station_v.rule);
            if(mcm_lulib_set_entry(this_lulib, path2, &station_v, sizeof(station_v))
                                   < MCM_RCODE_PASS)
            {
                DMSG("call mcm_lulib_set_entry(%s) fail", path2);
                goto FREE_01;
            }
        }
    }

    // 設定 device.limit.@1
    path1 = "device.limit.@1";
    memset(&limit_v, 0, sizeof(limit_v));
    snprintf(limit_v.name, sizeof(limit_v.name), "level %u", rand() % 20);
    limit_v.priority = (rand() % 60) - 20;
    DMSG("[set-entry] %s.name = " MCM_DTYPE_S_PF, path1, limit_v.name);
    DMSG("[set-entry] %s.priority = " MCM_DTYPE_ISI_PF, path1, limit_v.priority);
    if(mcm_lulib_set_entry(this_lulib, path1, &limit_v, sizeof(limit_v)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_set_entry(%s) fail", path1);
        goto FREE_01;
    }

    ret_val = 0;
FREE_01:
    return ret_val;
}

int case_check_exist(
    struct mcm_lulib_lib_t *this_lulib)
{
    int ret_val = -1;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    MCM_DTYPE_BOOL_TD is_exist;
    MCM_DTYPE_EK_TD i;


    DMSG("check entry exist test :");

    // 讀出 device.system
    path1 = "device.system";
    if(mcm_lulib_check_exist(this_lulib, path1, &is_exist) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_check_exist(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[check-exist] %s = " MCM_DTYPE_BOOL_PF, path1, is_exist);

    // 檢查 device.vap.* 的第 1 到 9 筆的資料是否存在.
    for(i = 0; i < 9; i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.@%u", i + 1);
        if(mcm_lulib_check_exist(this_lulib, path2, &is_exist) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_check_exist(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[check-exist] %s = " MCM_DTYPE_BOOL_PF, path2, is_exist);
    }

    // 檢查 device.limit.* 中 key 為 1 到 9 的資料是否存在.
    for(i = 0; i < 9; i++)
    {
        snprintf(path2, sizeof(path2), "device.limit.#%u", i + 1);
        if(mcm_lulib_check_exist(this_lulib, path2, &is_exist) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_check_exist(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[check-exist] %s = " MCM_DTYPE_BOOL_PF, path2, is_exist);
    }

    ret_val = 0;
FREE_01:
    return ret_val;
}

int case_add_entry(
    struct mcm_lulib_lib_t *this_lulib)
{
    int fret = -1;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_dv_device_vap_t vap_v;
    struct mcm_dv_device_vap_extra_t extra_v;
    struct mcm_dv_device_vap_station_t station_v;
    MCM_DTYPE_EK_TD vap_count, vap_key, station_key, limit_count, limit_key, i;


    DMSG("add entry test :");

    // 在 device.vap.* 增加一筆資料.
    // 讀出 device.vap.* 中可用的 key (修改需要使用 key).
    path1 = "device.vap.*";
    if(mcm_lulib_get_usable_key(this_lulib, path1, &vap_key) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_usable_key(%s) fail", path1);
        goto FREE_01;
    }
    if(vap_key == 0)
    {
        DMSG("%s is full", path1);
        fret = 0;
        goto FREE_01;
    }
    // 增加 device.vap.{vap_key}
    snprintf(path2, sizeof(path2), "device.vap.#%u", vap_key);
    memset(&vap_v, 0, sizeof(vap_v));
    snprintf(vap_v.ssid, sizeof(vap_v.ssid), "wpa2-%u", rand() % 50);
    vap_v.channel = rand() % 200;
    DMSG("[add-entry] %s.ssid = " MCM_DTYPE_S_PF, path2, vap_v.ssid);
    DMSG("[add-entry] %s.channel = " MCM_DTYPE_IUI_PF, path2, vap_v.channel);
    if(mcm_lulib_add_entry(this_lulib, path2, NULL, &vap_v, sizeof(vap_v)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_add_entry(%s) fail", path2);
        goto FREE_01;
    }

    // device.vap.*.extra 是 gs 類型, 當 device.vap.* 增加時會自動同時建立,
    // 但是資料內容是 $(default), 替 device.vap.{vap_key}.extra 補上資料.
    snprintf(path2, sizeof(path2), "device.vap.#%u.extra", vap_key);
    memset(&extra_v, 0, sizeof(extra_v));
    extra_v.hidden = rand() % 2;
    extra_v.tx_power = (rand() % 90) - 40;
    DMSG("[set-entry] %s.hidden = " MCM_DTYPE_ISC_PF, path2, extra_v.hidden);
    DMSG("[set-entry] %s.tx_power = " MCM_DTYPE_ISS_PF, path2, extra_v.tx_power);
    if(mcm_lulib_set_entry(this_lulib, path2, &extra_v, sizeof(extra_v)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_set_entry(%s) fail", path2);
        goto FREE_01;
    }

    // 在每個 device.vap.*.station.* 增加一筆資料.
    // 讀出 device.vap.* 的資料筆數.
    path1 = "device.vap.*";
    if(mcm_lulib_get_count(this_lulib, path1, &vap_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    for(i = 0; i < vap_count; i++)
    {
        // 讀出 device.vap.{i}.station.* 中可用的 key (修改需要使用 key).
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        if(mcm_lulib_get_usable_key(this_lulib, path2, &station_key) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_get_usable_key(%s) fail", path2);
            goto FREE_01;
        }
        if(station_key == 0)
        {
            DMSG("%s is full", path2);
            continue;
        }

        // 增加 device.vap.{i}.station.{station_key}
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.#%u", i + 1, station_key);
        memset(&station_v, 0, sizeof(station_v));
        snprintf(station_v.mac_addr, sizeof(station_v.mac_addr), "a1:b2:c3:d4:e5:%02x",
                 rand() % 255);
        station_v.rule = rand() % 50;
        DMSG("[add-entry] %s.mac_addr = " MCM_DTYPE_S_PF, path2, station_v.mac_addr);
        DMSG("[add-entry] %s.rule = " MCM_DTYPE_RK_PF, path2, station_v.rule);
        if(mcm_lulib_add_entry(this_lulib, path2, "", &station_v, sizeof(station_v))
                               < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_add_entry(%s) fail", path2);
            goto FREE_01;
        }
    }

    // 增加 device.limit.* 的資料筆數到最大.
    // 讀出 device.limit.* 的資料筆數.
    path1 = "device.limit.*";
    if(mcm_lulib_get_count(this_lulib, path1, &limit_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    for(i = limit_count; i < MCM_MCOUNT_DEVICE_LIMIT_MAX_COUNT; i++)
    {
        // 讀出 device.limit.* 中可用的 key (修改需要使用 key).
        path1 = "device.limit.*";
        if(mcm_lulib_get_usable_key(this_lulib, path1, &limit_key) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_get_usable_key(%s) fail", path1);
            goto FREE_01;
        }
        if(limit_key == 0)
        {
            DMSG("%s is full", path2);
            fret = 0;
            goto FREE_01;
        }

        // 增加 device.limit.{limit_key}
        // 不填入資料, 使用預設值.
        // 每次增加都插入在第一筆之前.
        snprintf(path2, sizeof(path2), "device.limit.#%u", limit_key);
        DMSG("[add-entry] %s", path2);
        if(mcm_lulib_add_entry(this_lulib, path2, "@1", NULL, 0) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_add_entry([%s][%s]) fail", path2, "@1");
            goto FREE_01;
        }
    }

    fret = 0;
FREE_01:
    return fret;
}

int case_del_entry(
    struct mcm_lulib_lib_t *this_lulib)
{
    int fret = -1;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    MCM_DTYPE_EK_TD vap_count, station_count, i;
    MCM_DTYPE_EK_TD limit_count, limit_key;


    DMSG("del entry test :");

    // 刪除每個 device.vap.*.station.@1
    // 讀出 device.vap.* 的資料筆數.
    path1 = "device.vap.*";
    if(mcm_lulib_get_count(this_lulib, path1, &vap_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    for(i = 0; i < vap_count; i++)
    {
        // 讀出 device.vap.{i}.station.* 的資料筆數.
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        if(mcm_lulib_get_count(this_lulib, path2, &station_count) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_get_count(%s) fail", path2);
            goto FREE_01;
        }
        if(station_count > 0)
        {
            // 刪除 device.vap.{i}.station.@1
            snprintf(path2, sizeof(path2), "device.vap.@%u.station.@1", i + 1);
            DMSG("[del-entry] %s", path2);
            if(mcm_lulib_del_entry(this_lulib, path2) < MCM_RCODE_PASS)
            {
                DMSG("call mcm_lulib_del_entry(%s) fail", path2);
                goto FREE_01;
            }
        }
    }

    // 刪除所有的 device.limit.*
    // 讀出 device.limit.* 的資料筆數.
    path1 = "device.limit.*";
    if(mcm_lulib_get_count(this_lulib, path1, &limit_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    for(i = 0; i < limit_count; i++)
    {
        // 讀出 device.limit.{i}.ekey (使用 key 模式).
        snprintf(path2, sizeof(path2), "device.limit.@%u.ekey", i + 1);
        if(mcm_lulib_get_alone(this_lulib, path2, &limit_key) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_get_alone(%s) fail", path2);
            goto FREE_01;
        }
        // 刪除 device.limit.{limit_key}
        snprintf(path2, sizeof(path2), "device.limit.#%u", limit_key);
        DMSG("[del-entry] %s", path2);
        if(mcm_lulib_del_entry(this_lulib, path2) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_del_entry(%s) fail", path2);
            goto FREE_01;
        }
    }

    fret = 0;
FREE_01:
    return fret;
}

int case_get_all_key(
    struct mcm_lulib_lib_t *this_lulib)
{
    int fret = -1;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_dv_device_vap_t vap_v;
    struct mcm_dv_device_vap_extra_t extra_v;
    struct mcm_dv_device_vap_station_t station_v;
    struct mcm_dv_device_limit_t limit_v;
    MCM_DTYPE_EK_TD vap_count, station_count, limit_count, i, j,
        *vap_key_array, *station_key_array, *limit_key_array;


    DMSG("get all key test :");

    // 讀出 device.vap.*
    path1 = "device.vap.*";
    if(mcm_lulib_get_all_key(this_lulib, path1, (MCM_DTYPE_EK_TD **) &vap_key_array, &vap_count)
                             < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_all_key(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, vap_count);
    for(i = 0; i < vap_count; i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.#%u", vap_key_array[i]);
        if(mcm_lulib_get_entry(this_lulib, path2, &vap_v) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_get_entry(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[get-entry] %s.ssid = " MCM_DTYPE_S_PF, path2, vap_v.ssid);
        DMSG("[get-entry] %s.channel = " MCM_DTYPE_IUI_PF, path2, vap_v.channel);
    }

    // 讀出 device.vap.extra.*
    for(i = 0; i < vap_count; i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.#%u.extra", vap_key_array[i]);
        if(mcm_lulib_get_entry(this_lulib, path2, &extra_v) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_get_entry(%s) fail", path2);
            goto FREE_02;
        }
        DMSG("[get-entry] %s.hidden = " MCM_DTYPE_ISC_PF, path2, extra_v.hidden);
        DMSG("[get-entry] %s.tx_power = " MCM_DTYPE_ISS_PF, path2, extra_v.tx_power);
    }

    // 讀出 device.vap.*.station.*
    for(i = 0; i < vap_count; i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.#%u.station.*", vap_key_array[i]);
        if(mcm_lulib_get_all_key(this_lulib, path2, (MCM_DTYPE_EK_TD **) &station_key_array,
                                 &station_count) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_get_all_key(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[count] %s = " MCM_DTYPE_EK_PF, path2, station_count);
        for(j = 0; j < station_count; j++)
        {
            snprintf(path2, sizeof(path2), "device.vap.#%u.station.#%u",
                     vap_key_array[i], station_key_array[j]);
            if(mcm_lulib_get_entry(this_lulib, path2, &station_v) < MCM_RCODE_PASS)
            {
                DMSG("call mcm_lulib_get_entry(%s) fail", path2);
                goto FREE_01;
            }
            DMSG("[get-entry] %s.mac_addr = " MCM_DTYPE_S_PF, path2, station_v.mac_addr);
            DMSG("[get-entry] %s.rule = " MCM_DTYPE_RK_PF, path2, station_v.rule);
        }
        free(station_key_array);
    }

    // 讀出 device.limit.*
    path1 = "device.limit.*";
    if(mcm_lulib_get_all_key(this_lulib, path1, (MCM_DTYPE_EK_TD **) &limit_key_array,
                             &limit_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_all_key(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, limit_count);
    for(i = 0; i < limit_count; i++)
    {
        snprintf(path2, sizeof(path2), "device.limit.#%u", limit_key_array[i]);
        if(mcm_lulib_get_entry(this_lulib, path2, &limit_v) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_get_entry(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[get-entry] %s.name = " MCM_DTYPE_S_PF, path2, limit_v.name);
        DMSG("[get-entry] %s.priority = " MCM_DTYPE_ISI_PF, path2, limit_v.priority);
    }
    free(limit_key_array);

    fret = 0;
FREE_02:
    free(vap_key_array);
FREE_01:
    return fret;
}

int case_get_all_entry(
    struct mcm_lulib_lib_t *this_lulib)
{
    int fret = -1;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_dv_device_t *device_v;
    struct mcm_dv_device_system_t *system_v;
    struct mcm_dv_device_vap_t *vap_v;
    struct mcm_dv_device_vap_extra_t *extra_v;
    struct mcm_dv_device_vap_station_t *station_v;
    struct mcm_dv_device_limit_t *limit_v;
    MCM_DTYPE_EK_TD count, vap_count, station_count, i, j;


    DMSG("get all entry test :");

    // 讀出 device
    path1 = "device";
    if(mcm_lulib_get_all_entry(this_lulib, path1, (void **) &device_v, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_all_entry(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count);
    DMSG("[get-all-entry] %s.descript = " MCM_DTYPE_S_PF, path1, device_v[0].descript);
    DMSG("[get-all-entry] %s.serial_number = "
         MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF
         MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF,
         path1,
         device_v[0].serial_number[0], device_v[0].serial_number[1],
         device_v[0].serial_number[2], device_v[0].serial_number[3],
         device_v[0].serial_number[4], device_v[0].serial_number[5],
         device_v[0].serial_number[6], device_v[0].serial_number[7],
         device_v[0].serial_number[8], device_v[0].serial_number[9]);
    free(device_v);

    // 讀出 device.system
    path1 = "device.system";
    if(mcm_lulib_get_all_entry(this_lulib, path1, (void **) &system_v, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_all_entry(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count);
    DMSG("[get-all-entry] %s.date = " MCM_DTYPE_S_PF, path1, system_v[0].date);
    DMSG("[get-all-entry] %s.ip_addr = " MCM_DTYPE_S_PF, path1, system_v[0].ip_addr);
    DMSG("[get-all-entry] %s.uptime = " MCM_DTYPE_IULL_PF, path1, system_v[0].uptime);
    DMSG("[get-all-entry] %s.loading = " MCM_DTYPE_FD_PF, path1, system_v[0].loading);
    free(system_v);

    // 讀出 device.vap.*
    path1 = "device.vap.*";
    if(mcm_lulib_get_all_entry(this_lulib, path1, (void **) &vap_v, &vap_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_all_entry(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, vap_count);
    for(i = 0; i < vap_count; i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.@%u", i + 1);
        DMSG("[get-all-entry] %s.ekey = " MCM_DTYPE_EK_PF, path2, vap_v[i].ekey);
        DMSG("[get-all-entry] %s.ssid = " MCM_DTYPE_S_PF, path2, vap_v[i].ssid);
        DMSG("[get-all-entry] %s.channel = " MCM_DTYPE_IUI_PF, path2, vap_v[i].channel);
    }
    free(vap_v);

    // 讀出 device.vap.@1.extra
    path1 = "device.vap.@1.extra";
    if(mcm_lulib_get_all_entry(this_lulib, path1, (void **) &extra_v, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_all_entry(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count);
    DMSG("[get-all-entry] %s.hidden = " MCM_DTYPE_ISC_PF, path1, extra_v[0].hidden);
    DMSG("[get-all-entry] %s.tx_power = " MCM_DTYPE_ISS_PF, path1, extra_v[0].tx_power);
    free(extra_v);

    // 讀出所有的 device.vap.*.station.*
    for(i = 0; i < vap_count; i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        if(mcm_lulib_get_all_entry(this_lulib, path2, (void **) &station_v, &station_count)
                                   < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_get_all_entry(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[count] %s = " MCM_DTYPE_EK_PF, path2, station_count);
        for(j = 0; j < station_count; j++)
        {
            snprintf(path2, sizeof(path2), "device.vap.@%u.station.@%u", i + 1, j + 1);
            DMSG("[get-all-entry] %s.mac_addr = " MCM_DTYPE_S_PF, path2, station_v[j].mac_addr);
            DMSG("[get-all-entry] %s.rule = " MCM_DTYPE_RK_PF, path2, station_v[j].rule);
        }
        free(station_v);
    }

    // 讀出 device.limit.*
    path1 = "device.limit.*";
    if(mcm_lulib_get_all_entry(this_lulib, path1, (void **) &limit_v, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_all_entry(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count);
    for(i = 0; i < count; i++)
    {
        snprintf(path2, sizeof(path2), "device.limit.#%u", limit_v[i].ekey);
        DMSG("[get-all-entry] %s.name = " MCM_DTYPE_S_PF, path2, limit_v[i].name);
        DMSG("[get-all-entry] %s.priority = " MCM_DTYPE_ISI_PF, path2, limit_v[i].priority);
    }
    free(limit_v);

    fret = 0;
FREE_01:
    return fret;
}

int case_del_all_entry(
    struct mcm_lulib_lib_t *this_lulib)
{
    int fret = -1;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    MCM_DTYPE_EK_TD vap_count, i;


    DMSG("del all entry test :");

    // 刪除所有的 device.vap.*.station.*
    // 讀出 device.vap.* 的 entry 數目.
    path1 = "device.vap.*";
    if(mcm_lulib_get_count(this_lulib, path1, &vap_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    for(i = 0; i < vap_count; i++)
    {
        // 刪除 device.vap.{i}.station.*
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        DMSG("[del-all-entry] %s", path2);
        if(mcm_lulib_del_all_entry(this_lulib, path2) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lulib_del_all_entry(%s) fail", path2);
            goto FREE_01;
        }
    }

    // 刪除所有的 device.limit.*
    path1 = "device.limit.*";
    DMSG("[del-all-entry] %s", path1);
    if(mcm_lulib_del_all_entry(this_lulib, path1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_del_all_entry(%s) fail", path1);
        goto FREE_01;
    }

    fret = 0;
FREE_01:
    return fret;
}

int case_run(
    struct mcm_lulib_lib_t *this_lulib)
{
    int fret = -1;
    char *path1;


    DMSG("run test :");

    path1 = "mcm_module_user_test_01";
    DMSG("[run] %s", path1);
    if(mcm_lulib_run(this_lulib, path1, NULL, 0, NULL, NULL) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_run(%s) fail", path1);
        goto FREE_01;
    }

    path1 = "mcm_module_user_test_02";
    DMSG("[run] %s", path1);
    if(mcm_lulib_run(this_lulib, path1, NULL, 0, NULL, NULL) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_run(%s) fail", path1);
        goto FREE_01;
    }

    fret = 0;
FREE_01:
    return fret;
}

int case_update(
    struct mcm_lulib_lib_t *this_lulib)
{
    int fret = -1;
    char *path1;
    MCM_DTYPE_EK_TD count;


    DMSG("update store test :");

    // 1. 刪除 device.vap.* 所有資料.
    // 2. 取得 device.vap.* 的資料筆數, 應該要是 0, 但是資料庫尚未更新,
    //    這時還是讀的到資料.
    // 3. 更新資料庫.
    // 4. 取得 device.vap.* 的資料筆數, 這時就會是 0.

    // 1. 刪除 device.vap.* 所有資料.
    path1 = "device.vap.*";
    DMSG("[del-all-entry] %s", path1);
    if(mcm_lulib_del_all_entry(this_lulib, path1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_del_all_entry(%s) fail", path1);
        goto FREE_01;
    }

    // 2. 取得 device.vap.* 的資料筆數, 應該要是 0, 但是資料庫尚未更新,
    //    這時還是讀的到資料.
    path1 = "device.vap.*";
    if(mcm_lulib_get_count(this_lulib, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count (before)] %s = " MCM_DTYPE_EK_PF, path1, count);

    // 3. 更新資料庫.
    DMSG("[update]");
    if(mcm_lulib_update(this_lulib) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_update() fail");
        goto FREE_01;
    }

    // 4. 取得 device.vap.* 的資料筆數, 這時就會是 0.
    path1 = "device.vap.*";
    if(mcm_lulib_get_count(this_lulib, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count (after)] %s = " MCM_DTYPE_EK_PF, path1, count);

    fret = 0;
FREE_01:
    return fret;
}

int case_save(
    struct mcm_lulib_lib_t *this_lulib)
{
    int fret = -1;
    MCM_DTYPE_BOOL_TD force_save = 1;


    DMSG("save test :");

    DMSG("[save] %s", force_save == 0 ? "check" : "force");
    if(mcm_lulib_save(this_lulib, force_save) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_save() fail");
        goto FREE_01;
    }

    fret = 0;
FREE_01:
    return fret;
}

int case_shutdown(
    struct mcm_lulib_lib_t *this_lulib)
{
    int fret = -1;


    DMSG("shutdown test :");

    DMSG("[shutdown]");
    if(mcm_lulib_shutdown(this_lulib) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_shutdown() fail");
        goto FREE_01;
    }

    fret = 0;
FREE_01:
    return fret;
}

struct operate_cb_t
{
    char *opt_cmd;
    char *opt_help;
    int (*opt_cb)(struct mcm_lulib_lib_t *this_lulib);
    MCM_DTYPE_LIST_TD opt_permission;
    MCM_DTYPE_USIZE_TD opt_stack_size;
};
struct operate_cb_t operate_cb_list[] =
{
    {"max-count",     "max count test",     case_max_count,     MCM_SPERMISSION_RO, 0},
    {"count",         "count test",         case_count,         MCM_SPERMISSION_RO, 0},
    {"get-alone",     "get alone test",     case_get_alone,     MCM_SPERMISSION_RO, 0},
    {"set-alone",     "set alone test",     case_set_alone,     MCM_SPERMISSION_RW, 0},
    {"get-entry",     "get entry test",     case_get_entry,     MCM_SPERMISSION_RO, 0},
    {"set-entry",     "set entry test",     case_set_entry,     MCM_SPERMISSION_RW, 0},
    {"check-exist",   "check exist test",   case_check_exist,   MCM_SPERMISSION_RO, 0},
    {"add-entry",     "add entry test",     case_add_entry,     MCM_SPERMISSION_RW, 0},
    {"del-entry",     "del entry test",     case_del_entry,     MCM_SPERMISSION_RW, 0},
    {"get-all-key",   "get all key test",   case_get_all_key,   MCM_SPERMISSION_RO, 0},
    {"get-all-entry", "get all entry test", case_get_all_entry, MCM_SPERMISSION_RO, 0},
    {"del-all-entry", "del all entry test", case_del_all_entry, MCM_SPERMISSION_RW, 0},
    {"run",           "run test",           case_run,           MCM_SPERMISSION_RW, 1048576},
    {"update",        "update test",        case_update,        MCM_SPERMISSION_RW, 0},
    {"save",          "save test",          case_save,          MCM_SPERMISSION_RW, 0},
    {"shutdown",      "shutdown test",      case_shutdown,      MCM_SPERMISSION_RW, 0},
    {NULL, NULL, NULL, 0}
};

int main(
    int argc,
    char **argv)
{
    struct mcm_lulib_lib_t self_lulib;
    unsigned int i;


    if(argc != 2)
        goto FREE_HELP;

    srand(time(NULL));

    for(i = 0; operate_cb_list[i].opt_cmd != NULL; i++)
        if(strcmp(operate_cb_list[i].opt_cmd, argv[1]) == 0)
            break;
    if(operate_cb_list[i].opt_cb == NULL)
        goto FREE_HELP;

    self_lulib.socket_path = "@mintcm";
    self_lulib.call_from = MCM_CFROM_USER;
    self_lulib.session_permission = operate_cb_list[i].opt_permission;
    self_lulib.session_stack_size = operate_cb_list[i].opt_stack_size;
    if(mcm_lulib_init(&self_lulib) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_init() fail");
        goto FREE_01;
    }

    if(operate_cb_list[i].opt_cb(&self_lulib) < 0)
    {
        DMSG("call operate_cb_list[%s] fail", operate_cb_list[i].opt_cmd);
        goto FREE_02;
    }

FREE_02:
    mcm_lulib_exit(&self_lulib);
FREE_01:
    return 0;
FREE_HELP:
    printf("user_app_0302 <test_case>\n");
    printf("  <test_case> list :\n");
    for(i = 0; operate_cb_list[i].opt_cmd != NULL; i++)
        printf("    %s : %s\n", operate_cb_list[i].opt_cmd, operate_cb_list[i].opt_help);
    return 0;
}

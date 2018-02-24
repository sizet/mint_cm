// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "mcm_lib/mcm_lheader/mcm_type.h"
#include "mcm_lib/mcm_lheader/mcm_size.h"
#include "mcm_lib/mcm_lheader/mcm_connect.h"
#include "mcm_lib/mcm_lheader/mcm_return.h"
#include "mcm_lib/mcm_lheader/mcm_data_exinfo_auto.h"
#include "mcm_lib/mcm_lklib/mcm_lklib_api.h"


#define DMSG(msg_fmt, msgs...) \
    printk(KERN_INFO "%s(%04u): " msg_fmt "\n", \
           strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__, __LINE__, ##msgs)


static ssize_t node_read(
    struct file *file,
    char __user *buffer,
    size_t count,
    loff_t *pos);

static ssize_t node_write(
    struct file *file,
    const char __user *buffer,
    size_t count,
    loff_t *pos);


static char *node_name = "kernel_app_0303";
static struct proc_dir_entry *node_entry;
static struct file_operations node_fops =
{
    .read  = node_read,
    .write = node_write,
};


static unsigned int rand(
    void)
{
    unsigned int tmp_value;


    get_random_bytes(&tmp_value, sizeof(tmp_value));

    return tmp_value;
}

int case_max_count(
    struct mcm_lklib_lib_t *this_lklib)
{
    int ret_val = -1;
    char *path1;
    MCM_DTYPE_EK_TD count;


    DMSG("max-count test 01 :");

    // device 的資料筆數上限.
    path1 = "device";
    if(mcm_lklib_get_max_count(this_lklib, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_max_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, count);

    // device.system 的資料筆數上限.
    path1 = "device.system";
    if(mcm_lklib_get_max_count(this_lklib, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_max_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, count);

    // device.vap.* 的資料筆數上限.
    path1 = "device.vap.*";
    if(mcm_lklib_get_max_count(this_lklib, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_max_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, count);

    // device.vap.*.extra 的資料筆數上限.
    path1 = "device.vap.*.extra";
    if(mcm_lklib_get_max_count(this_lklib, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_max_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, count);

    // device.vap.*.station.* 的資料筆數上限.
    path1 = "device.vap.*.station.*";
    if(mcm_lklib_get_max_count(this_lklib, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_max_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, count);

    // device.limit.* 的資料筆數上限.
    path1 = "device.limit.*";
    if(mcm_lklib_get_max_count(this_lklib, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_max_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[max-count] %s = " MCM_DTYPE_EK_PF, path1, count);

    // device.client.* 的資料筆數上限.
    path1 = "device.client.*";
    if(mcm_lklib_get_max_count(this_lklib, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_max_count(%s) fail", path1);
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
    struct mcm_lklib_lib_t *this_lklib)
{
    int ret_val = -1;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    MCM_DTYPE_EK_TD i, count1, count2;


    DMSG("count test :");

    // device 的資料筆數.
    path1 = "device";
    if(mcm_lklib_get_count(this_lklib, path1, &count1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count1);

    // device.system 的資料筆數.
    path1 = "device.system";
    if(mcm_lklib_get_count(this_lklib, path1, &count1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count1);

    // device.vap.* 的資料筆數.
    path1 = "device.vap.*";
    if(mcm_lklib_get_count(this_lklib, path1, &count1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count1);

    // device.vap.{i}.extra 的資料筆數.
    for(i = 0; i < count1; i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.@%u.extra", i + 1);
        if(mcm_lklib_get_count(this_lklib, path2, &count2) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_get_count(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[count] %s = " MCM_DTYPE_EK_PF, path2, count2);
    }

    // device.vap.{i}.station.* 的資料筆數.
    for(i = 0; i < count1; i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        if(mcm_lklib_get_count(this_lklib, path2, &count2) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_get_count(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[count] %s = " MCM_DTYPE_EK_PF, path2, count2);
    }

    // device.limit.* 的資料筆數.
    path1 = "device.limit.*";
    if(mcm_lklib_get_count(this_lklib, path1, &count1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count1);

    // device.client.* 的資料筆數.
    path1 = "device.client.*";
    if(mcm_lklib_get_count(this_lklib, path1, &count1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count1);

    ret_val = 0;
FREE_01:
    return ret_val;
}

int case_get_alone(
    struct mcm_lklib_lib_t *this_lklib)
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
    if(mcm_lklib_get_alone(this_lklib, path1, descript) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_alone(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-alone] %s = " MCM_DTYPE_S_PF, path1, descript);

    // 讀出 device.serial_number
    path1 = "device.serial_number";
    if(mcm_lklib_get_alone(this_lklib, path1, serial_number) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_alone(%s) fail", path1);
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
    if(mcm_lklib_get_alone(this_lklib, path1, &loading) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_alone(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-alone] %s = " MCM_DTYPE_FD_PF, path1, loading);

    // 讀出 device.vap.#8.channel
    path1 = "device.vap.#8.channel";
    if(mcm_lklib_get_alone(this_lklib, path1, &channel) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_alone(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-alone] %s = " MCM_DTYPE_IUI_PF, path1, channel);

    // 讀出 device.vap.#23.extra.tx_power
    path1 = "device.vap.#23.extra.tx_power";
    if(mcm_lklib_get_alone(this_lklib, path1, &tx_power) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_alone(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-alone] %s = " MCM_DTYPE_ISS_PF, path1, tx_power);

    // 讀出 device.vap.@2.station.#33.rule
    path1 = "device.vap.@2.station.#33.rule";
    if(mcm_lklib_get_alone(this_lklib, path1, &rule) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_alone(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-alone] %s = " MCM_DTYPE_RK_PF, path1, rule);

    // 讀出 device.limit.@1.name
    path1 = "device.limit.@1.name";
    if(mcm_lklib_get_alone(this_lklib, path1, name) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_alone(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-alone] %s = " MCM_DTYPE_S_PF, path1, name);

    ret_val = 0;
FREE_01:
    return ret_val;
}

int case_set_alone(
    struct mcm_lklib_lib_t *this_lklib)
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
    if(mcm_lklib_set_alone(this_lklib, path1, descript, strlen(descript)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_set_alone(%s) fail", path1);
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
    if(mcm_lklib_set_alone(this_lklib, path1, serial_number, sizeof(serial_number))
                           < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_set_alone(%s) fail", path1);
        goto FREE_01;
    }

    // 設定 device.system.loading
    path1 = "device.system.loading";
    loading = 2500.6 / 35.1;
    DMSG("[set-alone] %s = " MCM_DTYPE_FD_PF, path1, loading);
    if(mcm_lklib_set_alone(this_lklib, path1, &loading, sizeof(loading)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_set_alone(%s) fail", path1);
        goto FREE_01;
    }

    // 設定每個 device.vap.*.channel
    // 取得 device.vap.* 的資料筆數.
    path1 = "device.vap.*";
    if(mcm_lklib_get_count(this_lklib, path1, &vap_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    for(i = 0; i < vap_count; i++)
    {
        // 設定 device.vap.{i}.channel
        snprintf(path2, sizeof(path2), "device.vap.@%u.channel", i + 1);
        channel = rand() % 200;
        DMSG("[set-alone] %s = " MCM_DTYPE_IUI_PF, path2, channel);
        if(mcm_lklib_set_alone(this_lklib, path2, &channel, sizeof(channel)) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_set_alone(%s) fail", path2);
            goto FREE_01;
        }
    }

    // 設定每個 device.vap.{i}.extra.hidden
    for(i = 0; i < vap_count; i++)
    {
        // 讀出 device.vap.{i}.ekey (使用 key 模式做設定).
        snprintf(path2, sizeof(path2), "device.vap.@%u.ekey", i + 1);
        if(mcm_lklib_get_alone(this_lklib, path2, &vap_key) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_get_alone(%s) fail", path2);
            goto FREE_01;
        }

        // 設定 device.vap.{vap_key}.extra.hidden
        snprintf(path2, sizeof(path2), "device.vap.#%u.extra.hidden", vap_key);
        hidden = rand() % 2;
        DMSG("[set-alone] %s = " MCM_DTYPE_ISC_PF, path2, hidden);
        if(mcm_lklib_set_alone(this_lklib, path2, &hidden, sizeof(hidden)) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_set_alone(%s) fail", path2);
            goto FREE_01;
        }
    }

    // 設定每個 device.vap.{i}.station.*.rule
    for(i = 0; i < vap_count; i++)
    {
        // 取得 device.vap.{i}.station.* 的資料筆數.
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        if(mcm_lklib_get_count(this_lklib, path2, &station_count) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_get_count(%s) fail", path2);
            goto FREE_01;
        }
        for(j = 0; j < station_count; j++)
        {
            // 設定 device.vap.{vap_key}.station.{station_key}.rule
            snprintf(path2, sizeof(path2), "device.vap.@%u.station.@%u.rule", i + 1, j + 1);
            rule = rand() % 32;
            DMSG("[set-alone] %s = " MCM_DTYPE_RK_PF, path2, rule);
            if(mcm_lklib_set_alone(this_lklib, path2, &rule, sizeof(rule)) < MCM_RCODE_PASS)
            {
                DMSG("call mcm_lklib_set_alone(%s) fail", path2);
                goto FREE_01;
            }    
        }
    }

    // 設定 device.limit.#5.name
    path1 = "device.limit.#5.name";
    snprintf(name, sizeof(name), "level-%04u", rand() % 10000);
    DMSG("[set-alone] %s = " MCM_DTYPE_S_PF, path1, name);
    if(mcm_lklib_set_alone(this_lklib, path1, &name, strlen(name)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_set_alone(%s) fail", path1);
        goto FREE_01;
    }

    ret_val = 0;
FREE_01:
    return ret_val;
}

int case_get_entry(
    struct mcm_lklib_lib_t *this_lklib)
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
    if(mcm_lklib_get_entry(this_lklib, path1, &device_v) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_entry(%s) fail", path1);
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
    if(mcm_lklib_get_entry(this_lklib, path1, &system_v) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_entry(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-entry] %s.date = " MCM_DTYPE_S_PF, path1, system_v.date);
    DMSG("[get-entry] %s.ip_addr = " MCM_DTYPE_S_PF, path1, system_v.ip_addr);
    DMSG("[get-entry] %s.uptime = " MCM_DTYPE_IULL_PF, path1, system_v.uptime);
    DMSG("[get-entry] %s.loading = " MCM_DTYPE_FD_PF, path1, system_v.loading);

    // 讀出每個 device.vap.*
    // 取得 device.vap.* 的資料筆數.
    path1 = "device.vap.*";
    if(mcm_lklib_get_count(this_lklib, path1, &vap_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    for(i = 0; i < vap_count; i++)
    {
        // 讀出 device.vap.{i}
        snprintf(path2, sizeof(path2), "device.vap.@%u", i + 1);
        if(mcm_lklib_get_entry(this_lklib, path2, &vap_v) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_get_entry(%s) fail", path2);
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
        if(mcm_lklib_get_entry(this_lklib, path2, &extra_v) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_get_entry(%s) fail", path2);
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
        if(mcm_lklib_get_count(this_lklib, path2, &station_count) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_get_count(%s) fail", path2);
            goto FREE_01;
        }
        for(j = 0; j < station_count; j++)
        {
            // 讀出 device.vap.{i}.station.{j}
            snprintf(path2, sizeof(path2), "device.vap.@%u.station.@%u", i + 1, j + 1);
            if(mcm_lklib_get_entry(this_lklib, path2, &station_v) < MCM_RCODE_PASS)
            {
                DMSG("call mcm_lklib_get_entry(%s) fail", path2);
                goto FREE_01;
            }
            DMSG("[get-entry] %s.mac_addr = " MCM_DTYPE_S_PF, path2, station_v.mac_addr);
            DMSG("[get-entry] %s.rule = " MCM_DTYPE_RK_PF, path2, station_v.rule);
        }
    }

    // 讀出 device.limit.#5
    path1 = "device.limit.#5";
    if(mcm_lklib_get_entry(this_lklib, path1, &limit_v) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_entry(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-entry] %s.name = " MCM_DTYPE_S_PF, path1, limit_v.name);
    DMSG("[get-entry] %s.priority = " MCM_DTYPE_ISI_PF, path1, limit_v.priority);

    ret_val = 0;
FREE_01:
    return ret_val;
}

int case_set_entry(
    struct mcm_lklib_lib_t *this_lklib)
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
    if(mcm_lklib_set_entry(this_lklib, path1, &device_v, sizeof(device_v)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_set_entry(%s) fail", path1);
        goto FREE_01;
    }

    // 設定 device.system
    path1 = "device.system";
    memset(&system_v, 0, sizeof(system_v));
    snprintf(system_v.date, sizeof(system_v.date), "2011/01/%02u", rand() % 30);
    snprintf(system_v.ip_addr, sizeof(system_v.ip_addr), "192.168.100.%u", (rand() % 253) + 1);
    system_v.uptime = rand();
    system_v.loading = -50000.9 / 72.5;
    DMSG("[set-entry] %s.date = " MCM_DTYPE_S_PF, path1, system_v.date);
    DMSG("[set-entry] %s.ip_addr = " MCM_DTYPE_S_PF, path1, system_v.ip_addr);
    DMSG("[set-entry] %s.uptime = " MCM_DTYPE_IULL_PF, path1, system_v.uptime);
    DMSG("[set-entry] %s.loading = " MCM_DTYPE_FD_PF, path1, system_v.loading);
    if(mcm_lklib_set_entry(this_lklib, path1, &system_v, sizeof(system_v)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_set_entry(%s) fail", path1);
        goto FREE_01;
    }

    // 設定每個 device.vap.*
    // 取得 device.vap.* 的資料筆數.
    path1 = "device.vap.*";
    if(mcm_lklib_get_count(this_lklib, path1, &vap_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    for(i = 0; i < vap_count; i++)
    {
        // 讀出 device.vap.{i} (使用 key 模式做設定).
        snprintf(path2, sizeof(path2), "device.vap.@%u", i + 1);
        if(mcm_lklib_get_entry(this_lklib, path2, &vap_v) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_get_entry(%s) fail", path2);
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
        if(mcm_lklib_set_entry(this_lklib, path2, &vap_v, sizeof(vap_v)) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_set_entry(%s) fail", path2);
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
        if(mcm_lklib_set_entry(this_lklib, path2, &extra_v, sizeof(extra_v)) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_set_entry(%s) fail", path2);
            goto FREE_01;
        }
    }

    // 設定每個 device.vap.{i}.station.*
    for(i = 0; i < vap_count; i++)
    {
        // 取得 device.vap.{i}.station.* 的資料筆數.
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        if(mcm_lklib_get_count(this_lklib, path2, &station_count) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_get_count(%s) fail", path2);
            goto FREE_01;
        }
        for(j = 0; j < station_count; j++)
        {
            // 讀出 device.vap.{i}.station.{j} (使用 key 模式做設定).
            snprintf(path2, sizeof(path2), "device.vap.@%u.station.@%u", i + 1, j + 1);
            if(mcm_lklib_get_entry(this_lklib, path2, &station_v) < MCM_RCODE_PASS)
            {
                DMSG("call mcm_lklib_get_entry(%s) fail", path2);
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
            if(mcm_lklib_set_entry(this_lklib, path2, &station_v, sizeof(station_v))
                                   < MCM_RCODE_PASS)
            {
                DMSG("call mcm_lklib_set_entry(%s) fail", path2);
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
    if(mcm_lklib_set_entry(this_lklib, path1, &limit_v, sizeof(limit_v)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_set_entry(%s) fail", path1);
        goto FREE_01;
    }

    ret_val = 0;
FREE_01:
    return ret_val;
}

int case_add_entry(
    struct mcm_lklib_lib_t *this_lklib)
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
    if(mcm_lklib_get_usable_key(this_lklib, path1, &vap_key) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_usable_key(%s) fail", path1);
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
    if(mcm_lklib_add_entry(this_lklib, path2, NULL, &vap_v, sizeof(vap_v)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_add_entry(%s) fail", path2);
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
    if(mcm_lklib_set_entry(this_lklib, path2, &extra_v, sizeof(extra_v)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_set_entry(%s) fail", path2);
        goto FREE_01;
    }

    // 在每個 device.vap.*.station.* 增加一筆資料.
    // 讀出 device.vap.* 的資料筆數.
    path1 = "device.vap.*";
    if(mcm_lklib_get_count(this_lklib, path1, &vap_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    for(i = 0; i < vap_count; i++)
    {
        // 讀出 device.vap.{i}.station.* 中可用的 key (修改需要使用 key).
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        if(mcm_lklib_get_usable_key(this_lklib, path2, &station_key) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_get_usable_key(%s) fail", path2);
            goto FREE_01;
        }
        if(station_key == 0)
        {
            DMSG("%s is full", path1);
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
        if(mcm_lklib_add_entry(this_lklib, path2, "", &station_v, sizeof(station_v))
                               < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_add_entry(%s) fail", path2);
            goto FREE_01;
        }
    }

    // 增加 device.limit.* 的資料筆數到最大.
    // 讀出 device.limit.* 的資料筆數.
    path1 = "device.limit.*";
    if(mcm_lklib_get_count(this_lklib, path1, &limit_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    for(i = limit_count; i < MCM_MCOUNT_DEVICE_LIMIT_MAX_COUNT; i++)
    {
        // 讀出 device.limit.* 中可用的 key (修改需要使用 key).
        path1 = "device.limit.*";
        if(mcm_lklib_get_usable_key(this_lklib, path1, &limit_key) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_get_usable_key(%s) fail", path1);
            goto FREE_01;
        }
        if(limit_key == 0)
        {
            DMSG("%s is full", path1);
            fret = 0;
            goto FREE_01;
        }

        // 增加 device.limit.{limit_key}
        // 不填入資料, 使用預設值.
        // 每次增加都插入在第一筆之前.
        snprintf(path2, sizeof(path2), "device.limit.#%u", limit_key);
        DMSG("[add-entry] %s", path2);
        if(mcm_lklib_add_entry(this_lklib, path2, "@1", NULL, 0) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_add_entry([%s][%s]) fail", path2, "@1");
            goto FREE_01;
        }
    }

    fret = 0;
FREE_01:
    return fret;
}

int case_del_entry(
    struct mcm_lklib_lib_t *this_lklib)
{
    int fret = -1;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    MCM_DTYPE_EK_TD vap_count, station_count, i;
    MCM_DTYPE_EK_TD limit_count, limit_key;


    DMSG("del entry test :");

    // 刪除每個 device.vap.*.station.@1
    // 讀出 device.vap.* 的資料筆數.
    path1 = "device.vap.*";
    if(mcm_lklib_get_count(this_lklib, path1, &vap_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    for(i = 0; i < vap_count; i++)
    {
        // 讀出 device.vap.{i}.station.* 的資料筆數.
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        if(mcm_lklib_get_count(this_lklib, path2, &station_count) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_get_count(%s) fail", path2);
            goto FREE_01;
        }
        if(station_count > 0)
        {
            // 刪除 device.vap.{i}.station.@1
            snprintf(path2, sizeof(path2), "device.vap.@%u.station.@1", i + 1);
            DMSG("[del-entry] %s", path2);
            if(mcm_lklib_del_entry(this_lklib, path2) < MCM_RCODE_PASS)
            {
                DMSG("call mcm_lklib_del_entry(%s) fail", path2);
                goto FREE_01;
            }
        }
    }

    // 刪除所有的 device.limit.*
    // 讀出 device.limit.* 的資料筆數.
    path1 = "device.limit.*";
    if(mcm_lklib_get_count(this_lklib, path1, &limit_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    for(i = 0; i < limit_count; i++)
    {
        // 讀出 device.limit.{i}.ekey (使用 key 模式).
        snprintf(path2, sizeof(path2), "device.limit.@%u.ekey", i + 1);
        if(mcm_lklib_get_alone(this_lklib, path2, &limit_key) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_get_alone(%s) fail", path2);
            goto FREE_01;
        }
        // 刪除 device.limit.{limit_key}
        snprintf(path2, sizeof(path2), "device.limit.#%u", limit_key);
        DMSG("[del-entry] %s", path2);
        if(mcm_lklib_del_entry(this_lklib, path2) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_del_entry(%s) fail", path2);
            goto FREE_01;
        }
    }

    fret = 0;
FREE_01:
    return fret;
}

int case_get_all_key(
    struct mcm_lklib_lib_t *this_lklib)
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
    if(mcm_lklib_get_all_key(this_lklib, path1, (MCM_DTYPE_EK_TD **) &vap_key_array, &vap_count)
                             < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_all_key(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, vap_count);
    for(i = 0; i < vap_count; i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.#%u", vap_key_array[i]);
        if(mcm_lklib_get_entry(this_lklib, path2, &vap_v) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_get_entry(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[get-entry] %s.ssid = " MCM_DTYPE_S_PF, path2, vap_v.ssid);
        DMSG("[get-entry] %s.channel = " MCM_DTYPE_IUI_PF, path2, vap_v.channel);
    }

    // 讀出 device.vap.extra.*
    for(i = 0; i < vap_count; i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.#%u.extra", vap_key_array[i]);
        if(mcm_lklib_get_entry(this_lklib, path2, &extra_v) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_get_entry(%s) fail", path2);
            goto FREE_02;
        }
        DMSG("[get-entry] %s.hidden = " MCM_DTYPE_ISC_PF, path2, extra_v.hidden);
        DMSG("[get-entry] %s.tx_power = " MCM_DTYPE_ISS_PF, path2, extra_v.tx_power);
    }

    // 讀出 device.vap.*.station.*
    for(i = 0; i < vap_count; i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.#%u.station.*", vap_key_array[i]);
        if(mcm_lklib_get_all_key(this_lklib, path2, (MCM_DTYPE_EK_TD **) &station_key_array,
                                 &station_count) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_get_all_key(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[count] %s = " MCM_DTYPE_EK_PF, path2, station_count);
        for(j = 0; j < station_count; j++)
        {
            snprintf(path2, sizeof(path2), "device.vap.#%u.station.#%u",
                     vap_key_array[i], station_key_array[j]);
            if(mcm_lklib_get_entry(this_lklib, path2, &station_v) < MCM_RCODE_PASS)
            {
                DMSG("call mcm_lklib_get_entry(%s) fail", path2);
                goto FREE_01;
            }
            DMSG("[get-entry] %s.mac_addr = " MCM_DTYPE_S_PF, path2, station_v.mac_addr);
            DMSG("[get-entry] %s.rule = " MCM_DTYPE_RK_PF, path2, station_v.rule);
        }
        kfree(station_key_array);
    }

    // 讀出 device.limit.*
    path1 = "device.limit.*";
    if(mcm_lklib_get_all_key(this_lklib, path1, (MCM_DTYPE_EK_TD **) &limit_key_array,
                             &limit_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_all_key(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, limit_count);
    for(i = 0; i < limit_count; i++)
    {
        snprintf(path2, sizeof(path2), "device.limit.#%u", limit_key_array[i]);
        if(mcm_lklib_get_entry(this_lklib, path2, &limit_v) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_get_entry(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[get-entry] %s.name = " MCM_DTYPE_S_PF, path2, limit_v.name);
        DMSG("[get-entry] %s.priority = " MCM_DTYPE_ISI_PF, path2, limit_v.priority);
    }
    kfree(limit_key_array);

    fret = 0;
FREE_02:
    kfree(vap_key_array);
FREE_01:
    return fret;
}

int case_get_all_entry(
    struct mcm_lklib_lib_t *this_lklib)
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
    if(mcm_lklib_get_all_entry(this_lklib, path1, (void **) &device_v, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_all_entry(%s) fail", path1);
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
    kfree(device_v);

    // 讀出 device.system
    path1 = "device.system";
    if(mcm_lklib_get_all_entry(this_lklib, path1, (void **) &system_v, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_all_entry(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count);
    DMSG("[get-all-entry] %s.date = " MCM_DTYPE_S_PF, path1, system_v[0].date);
    DMSG("[get-all-entry] %s.ip_addr = " MCM_DTYPE_S_PF, path1, system_v[0].ip_addr);
    DMSG("[get-all-entry] %s.uptime = " MCM_DTYPE_IULL_PF, path1, system_v[0].uptime);
    DMSG("[get-all-entry] %s.loading = " MCM_DTYPE_FD_PF, path1, system_v[0].loading);
    kfree(system_v);

    // 讀出 device.vap.*
    path1 = "device.vap.*";
    if(mcm_lklib_get_all_entry(this_lklib, path1, (void **) &vap_v, &vap_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_all_entry(%s) fail", path1);
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
    kfree(vap_v);

    // 讀出 device.vap.@1.extra
    path1 = "device.vap.@1.extra";
    if(mcm_lklib_get_all_entry(this_lklib, path1, (void **) &extra_v, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_all_entry(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count);
    DMSG("[get-all-entry] %s.hidden = " MCM_DTYPE_ISC_PF, path1, extra_v[0].hidden);
    DMSG("[get-all-entry] %s.tx_power = " MCM_DTYPE_ISS_PF, path1, extra_v[0].tx_power);
    kfree(extra_v);

    // 讀出所有的 device.vap.*.station.*
    for(i = 0; i < vap_count; i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        if(mcm_lklib_get_all_entry(this_lklib, path2, (void **) &station_v, &station_count)
                                         < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_get_all_entry(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[count] %s = " MCM_DTYPE_EK_PF, path2, station_count);
        for(j = 0; j < station_count; j++)
        {
            snprintf(path2, sizeof(path2), "device.vap.@%u.station.@%u", i + 1, j + 1);
            DMSG("[get-all-entry] %s.mac_addr = " MCM_DTYPE_S_PF, path2, station_v[j].mac_addr);
            DMSG("[get-all-entry] %s.rule = " MCM_DTYPE_RK_PF, path2, station_v[j].rule);
        }
        kfree(station_v);
    }

    // 讀出 device.limit.*
    path1 = "device.limit.*";
    if(mcm_lklib_get_all_entry(this_lklib, path1, (void **) &limit_v, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_all_entry(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, count);
    for(i = 0; i < count; i++)
    {
        snprintf(path2, sizeof(path2), "device.limit.#%u", limit_v[i].ekey);
        DMSG("[get-all-entry] %s.name = " MCM_DTYPE_S_PF, path2, limit_v[i].name);
        DMSG("[get-all-entry] %s.priority = " MCM_DTYPE_ISI_PF, path2, limit_v[i].priority);
    }
    kfree(limit_v);

    fret = 0;
FREE_01:
    return fret;
}

int case_del_all_entry(
    struct mcm_lklib_lib_t *this_lklib)
{
    int fret = -1;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    MCM_DTYPE_EK_TD vap_count, i;


    DMSG("del all entry test :");

    // 刪除所有的 device.vap.*.station.*
    // 讀出 device.vap.* 的 entry 數目.
    path1 = "device.vap.*";
    if(mcm_lklib_get_count(this_lklib, path1, &vap_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    for(i = 0; i < vap_count; i++)
    {
        // 刪除 device.vap.{i}.station.*
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        DMSG("[del-all-entry] %s", path2);
        if(mcm_lklib_del_all_entry(this_lklib, path2) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_lklib_del_all_entry(%s) fail", path2);
            goto FREE_01;
        }
    }

    // 刪除所有的 device.limit.*
    path1 = "device.limit.*";
    DMSG("[del-all-entry] %s", path1);
    if(mcm_lklib_del_all_entry(this_lklib, path1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_del_all_entry(%s) fail", path1);
        goto FREE_01;
    }

    fret = 0;
FREE_01:
    return fret;
}

int case_run(
    struct mcm_lklib_lib_t *this_lklib)
{
    int fret = -1;
    char *path1;


    DMSG("run test :");

    path1 = "mcm_module_kernel_test_01";
    DMSG("[run] %s", path1);
    if(mcm_lklib_run(this_lklib, path1, NULL, 0, NULL, NULL) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_run(%s) fail", path1);
        goto FREE_01;
    }

    path1 = "mcm_module_kernel_test_02";
    DMSG("[run] %s", path1);
    if(mcm_lklib_run(this_lklib, path1, NULL, 0, NULL, NULL) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_run(%s) fail", path1);
        goto FREE_01;
    }

    fret = 0;
FREE_01:
    return fret;
}

int case_update(
    struct mcm_lklib_lib_t *this_lklib)
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
    if(mcm_lklib_del_all_entry(this_lklib, path1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_del_all_entry(%s) fail", path1);
        goto FREE_01;
    }

    // 2. 取得 device.vap.* 的資料筆數, 應該要是 0, 但是資料庫尚未更新,
    //    這時還是讀的到資料.
    path1 = "device.vap.*";
    if(mcm_lklib_get_count(this_lklib, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count (before)] %s = " MCM_DTYPE_EK_PF, path1, count);

    // 3. 更新資料庫.
    DMSG("[update]");
    if(mcm_lklib_update(this_lklib) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_update() fail");
        goto FREE_01;
    }

    // 4. 取得 device.vap.* 的資料筆數, 這時就會是 0.
    path1 = "device.vap.*";
    if(mcm_lklib_get_count(this_lklib, path1, &count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_get_count(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[count (after)] %s = " MCM_DTYPE_EK_PF, path1, count);

    fret = 0;
FREE_01:
    return fret;
}

int case_save(
    struct mcm_lklib_lib_t *this_lklib)
{
    int fret = -1;
    MCM_DTYPE_BOOL_TD force_save = 1;


    DMSG("save test :");

    DMSG("[save] %s", force_save == 0 ? "check" : "force");
    if(mcm_lklib_save(this_lklib, force_save) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_save() fail");
        goto FREE_01;
    }

    fret = 0;
FREE_01:
    return fret;
}

int case_shutdown(
    struct mcm_lklib_lib_t *this_lklib)
{
    int fret = -1;


    DMSG("shutdown test :");

    DMSG("[shutdown]");
    if(mcm_lklib_shutdown(this_lklib) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_shutdown() fail");
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
    int (*opt_cb)(struct mcm_lklib_lib_t *this_lklib);
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

static void show_usage(
    void)
{
    unsigned int i;


    DMSG("kernel_app_0303 <test_case>");
    DMSG("  <test_case> list :");
    for(i = 0; operate_cb_list[i].opt_cmd != NULL; i++)
    {
        DMSG("    %s : %s", operate_cb_list[i].opt_cmd, operate_cb_list[i].opt_help);
    }
}

static ssize_t node_read(
    struct file *file,
    char __user *buffer,
    size_t count,
    loff_t *pos)
{
    show_usage();

	return 0;
}

static ssize_t node_write(
    struct file *file,
    const char __user *buffer,
    size_t count,
    loff_t *pos)
{
    struct mcm_lklib_lib_t self_lklib;
    char arg_buf[32];
    unsigned int arg_len, i;


    memset(arg_buf, 0, sizeof(arg_buf));
    arg_len = count >= (sizeof(arg_buf) - 1) ? sizeof(arg_buf) - 1 : count;
    copy_from_user(arg_buf, buffer, arg_len);

    if(arg_len > 1)
        if(arg_buf[arg_len - 1] == '\n')
            arg_buf[arg_len - 1] = '\0';

    for(i = 0; operate_cb_list[i].opt_cmd != NULL; i++)
        if(strcmp(operate_cb_list[i].opt_cmd, arg_buf) == 0)
            break;
    if(operate_cb_list[i].opt_cb == NULL)
        goto FREE_HELP;

    memset(&self_lklib, 0, sizeof(struct mcm_lklib_lib_t));
    self_lklib.socket_path = "@mintcm";
    self_lklib.call_from = MCM_CFROM_KERNEL;
    self_lklib.session_permission = operate_cb_list[i].opt_permission;
    self_lklib.session_stack_size = operate_cb_list[i].opt_stack_size;
    if(mcm_lklib_init(&self_lklib) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_init() fail");
        goto FREE_01;
    }

    if(operate_cb_list[i].opt_cb(&self_lklib) < 0)
    {
        DMSG("call operate_cb_list[%s] fail", operate_cb_list[i].opt_cmd);
        goto FREE_02;
    }

FREE_02:
    mcm_lklib_exit(&self_lklib);
FREE_01:
    return count;
FREE_HELP:
    show_usage();
    return count;
}

static int __init kernel_app_0303_init(
    void)
{
    if((node_entry = proc_create(node_name, S_IFREG | S_IRUGO | S_IWUGO, NULL, &node_fops)) == NULL)
    {
        DMSG("call proc_create(%s) fail", node_name);
        goto FREE_01;
    }

FREE_01:
    return 0;
}

static void __exit kernel_app_0303_exit(
    void)
{
    remove_proc_entry(node_name, NULL);

    return;
}

module_init(kernel_app_0303_init);
module_exit(kernel_app_0303_exit);

MODULE_LICENSE("GPL");

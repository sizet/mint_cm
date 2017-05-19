// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <stdio.h>
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


void dump_status(
    char *case_name,
    char *path_name,
    char *post_name,
    MCM_DTYPE_DS_TD status_code)
{
    char *status_type = NULL;


    switch(status_code)
    {
        case MCM_DSCHANGE_NONE:
            status_type = "non";
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

    DMSG("[%s] %s %s%s", case_name, status_type, path_name, post_name);
}

int mcm_module_status_test_get_device(
    struct mcm_service_session_t *this_session)
{
    char *path1;
    struct mcm_config_model_group_t *device_group;
    struct mcm_config_store_t *device_store;
    MCM_DTYPE_DS_TD tmp_status;
    struct mcm_ds_device_t device_s;


    // 取得 device 的狀態 (entry-self).
    path1 = "device";
    if(mcm_config_find_entry_by_full(this_session, path1, &device_group, &device_store)
                                     < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_find_entry_by_full(%s) fail", path1);
        goto FREE_01;
    }
    if(mcm_config_get_entry_self_status_by_info(this_session, device_group, device_store,
                                                &tmp_status) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_self_status_by_info(%s) fail", path1);
        goto FREE_01;
    }
    tmp_status &= MCM_DSCHANGE_MASK;
    dump_status("entry-self", path1, "", tmp_status);

    // 取得 device 的狀態 (entry-all).
    path1 = "device";
    if(mcm_config_get_entry_all_status_by_info(this_session, device_group, device_store,
                                               &device_s) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_self_status_by_info(%s) fail", path1);
        goto FREE_01;
    }
    device_s.ekey &= MCM_DSCHANGE_MASK;
    device_s.descript &= MCM_DSCHANGE_MASK;
    device_s.serial_number &= MCM_DSCHANGE_MASK;
    dump_status("entry-all", path1, "", device_s.ekey);
    dump_status("entry-all", path1, ".descript", device_s.descript);
    dump_status("entry-all", path1, ".serial_number", device_s.serial_number);

FREE_01:
    return MCM_RCODE_PASS;
}

int mcm_module_status_test_get_system(
    struct mcm_service_session_t *this_session)
{
    char *path1;
    struct mcm_config_model_group_t *system_group;
    struct mcm_config_store_t *system_store;
    MCM_DTYPE_DS_TD tmp_status;
    struct mcm_ds_device_system_t system_s;


    // 取得 device.system 的狀態 (entry-self).
    path1 = "device.system";
    if(mcm_config_find_entry_by_full(this_session, path1, &system_group, &system_store)
                                     < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_find_entry_by_full(%s) fail", path1);
        goto FREE_01;
    }
    if(mcm_config_get_entry_self_status_by_info(this_session, system_group, system_store,
                                                &tmp_status) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_self_status_by_info(%s) fail", path1);
        goto FREE_01;
    }
    tmp_status &= MCM_DSCHANGE_MASK;
    dump_status("entry-self", path1, "", tmp_status);

    // 取得 device.system 的狀態 (entry-all).
    path1 = "device.system";
    if(mcm_config_get_entry_all_status_by_info(this_session, system_group, system_store,
                                               &system_s) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_self_status_by_info(%s) fail", path1);
        goto FREE_01;
    }
    system_s.ekey &= MCM_DSCHANGE_MASK;
    system_s.date &= MCM_DSCHANGE_MASK;
    system_s.ip_addr &= MCM_DSCHANGE_MASK;
    system_s.uptime &= MCM_DSCHANGE_MASK;
    system_s.loading &= MCM_DSCHANGE_MASK;
    dump_status("entry-all", path1, "", system_s.ekey);
    dump_status("entry-all", path1, ".date", system_s.date);
    dump_status("entry-all", path1, ".ip_addr", system_s.ip_addr);
    dump_status("entry-all", path1, ".uptime", system_s.uptime);
    dump_status("entry-all", path1, ".loading", system_s.loading);

FREE_01:
    return MCM_RCODE_PASS;
}

int mcm_module_status_test_get_vap(
    struct mcm_service_session_t *this_session)
{
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_config_model_group_t *vap_group;
    struct mcm_config_store_t *vap_store, *vap_each_store;
    MCM_DTYPE_DS_TD tmp_status;
    MCM_DTYPE_EK_TD i;
    struct mcm_ds_device_vap_t vap_s;


    // 找到 device.vap.* 的開頭 entry.
    path1 = "device.vap.*";
    if(mcm_config_find_entry_by_mix(this_session, path1, &vap_group, &vap_store, NULL, NULL)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_find_entry_by_full(%s) fail", path1);
        goto FREE_01;
    }

    // 取得 device.vap.* 的狀態 (entry-self).
    for(vap_each_store = vap_store, i = 0;
        vap_each_store != NULL; vap_each_store = vap_each_store->next_store, i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.@%u", i + 1);
        if(mcm_config_get_entry_self_status_by_info(this_session, vap_group, vap_each_store,
                                                    &tmp_status) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_get_entry_self_status_by_info(%s) fail", path1);
            goto FREE_01;
        }
        tmp_status &= MCM_DSCHANGE_MASK;
        dump_status("entry-self", path1, "", tmp_status);
    }

    // 取得 device.vap.* 的狀態 (entry-all).
    for(vap_each_store = vap_store, i = 0;
        vap_each_store != NULL; vap_each_store = vap_each_store->next_store, i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.@%u", i + 1);
        if(mcm_config_get_entry_all_status_by_info(this_session, vap_group, vap_each_store,
                                                   &vap_s) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_get_entry_self_status_by_info(%s) fail", path2);
            goto FREE_01;
        }
        vap_s.ekey &= MCM_DSCHANGE_MASK;
        vap_s.ssid &= MCM_DSCHANGE_MASK;
        vap_s.channel &= MCM_DSCHANGE_MASK;
        dump_status("entry-all", path2, "", vap_s.ekey);
        dump_status("entry-all", path2, ".ssid", vap_s.ssid);
        dump_status("entry-all", path2, ".channel", vap_s.channel);
    }

FREE_01:
    return MCM_RCODE_PASS;
}

int mcm_module_status_test_get_station(
    struct mcm_service_session_t *this_session)
{
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_config_model_group_t *station_group;
    struct mcm_config_store_t *station_store;
    MCM_DTYPE_DS_TD tmp_status;
    MCM_DTYPE_EK_TD vap_count, i, j;
    struct mcm_ds_device_vap_station_t station_s;


    // 取得 device.vap.* 的資料筆數.
    path1 = "device.vap.*";
    if(mcm_config_get_count_by_path(this_session, path1, &vap_count) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_count_by_path(%s) fail", path1);
        goto FREE_01;
    }

    // 取得 device.vap.*.station 的狀態 (entry-self).
    for(i = 0; i < vap_count; i++)
    {
        // 找到 device.vap.{i}.station.* 的開頭 entry.
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        if(mcm_config_find_entry_by_mix(this_session, path2, &station_group, &station_store, NULL,
                                        NULL) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_find_entry_by_full(%s) fail", path2);
            goto FREE_01;
        }
        // 取得狀態.
        for(j = 0; station_store != NULL; station_store = station_store->next_store)
        {
            snprintf(path2, sizeof(path2), "device.vap.@%u.station.@%u", i + 1, j + 1);
            if(mcm_config_get_entry_self_status_by_info(this_session, station_group,
                                                        station_store, &tmp_status)
                                                        < MCM_RCODE_PASS)
            {
                DMSG("call mcm_config_get_entry_self_status_by_info(%s) fail", path2);
                goto FREE_01;
            }
            tmp_status &= MCM_DSCHANGE_MASK;
            dump_status("entry-self", path2, "", tmp_status);
        }
    }

    // 取得 device.vap.*.station 的狀態 (entry-all).
    for(i = 0; i < vap_count; i++)
    {
        // 找到 device.vap.{i}.station.* 的開頭 entry.
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        if(mcm_config_find_entry_by_mix(this_session, path2, &station_group, &station_store, NULL,
                                        NULL) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_find_entry_by_full(%s) fail", path2);
            goto FREE_01;
        }
        // 取得狀態.
        for(j = 0; station_store != NULL; station_store = station_store->next_store)
        {
            snprintf(path2, sizeof(path2), "device.vap.@%u.station.@%u", i + 1, j + 1);
            if(mcm_config_get_entry_all_status_by_info(this_session, station_group, station_store,
                                                       &station_s) < MCM_RCODE_PASS)
            {
                DMSG("call mcm_config_get_entry_self_status_by_info(%s) fail", path2);
                goto FREE_01;
            }
            station_s.ekey &= MCM_DSCHANGE_MASK;
            station_s.mac_addr &= MCM_DSCHANGE_MASK;
            station_s.rule &= MCM_DSCHANGE_MASK;
            dump_status("entry-all", path2, "", station_s.ekey);
            dump_status("entry-all", path2, ".mac_addr", station_s.mac_addr);
            dump_status("entry-all", path2, ".rule", station_s.rule);
        }
    }

FREE_01:
    return MCM_RCODE_PASS;
}

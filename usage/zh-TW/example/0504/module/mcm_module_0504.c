// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mcm_lib/mcm_lheader/mcm_type.h"
#include "mcm_lib/mcm_lheader/mcm_size.h"
#include "mcm_lib/mcm_lheader/mcm_control.h"
#include "mcm_lib/mcm_lheader/mcm_connect.h"
#include "mcm_lib/mcm_lheader/mcm_return.h"
#include "mcm_lib/mcm_lheader/mcm_debug.h"
#include "mcm_lib/mcm_lheader/mcm_data_exinfo_auto.h"
#include "../mcm_service_handle_define.h"
#include "../mcm_config_handle_extern.h"


#define DMSG(msg_fmt, msgs...) printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msgs)


int mcm_module_obtain_error_test(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    MCM_DTYPE_USIZE_TD tmp_len;
    char *tmp_buf;


    DMSG("obtain error test :");

    tmp_len = 128;
    tmp_buf = (char *) malloc(tmp_len);
    if(tmp_buf == NULL)
    {
        DMSG("call malloc() fail [%s]", strerror(errno));
        goto FREE_01;
    }

    snprintf(tmp_buf, tmp_len, "alert(\"custom error\");");
    tmp_len = strlen(tmp_buf) + 1;

    this_session->rep_data_buf = tmp_buf;
    this_session->rep_data_len = tmp_len;

FREE_01:
    return fret;
}

int mcm_module_obtain_multiple_test_01(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    MCM_DTYPE_USIZE_TD tmp_len;
    char *tmp_buf;


    DMSG("obtain multiple test 01 :");

    tmp_len = 128;
    tmp_buf = (char *) malloc(tmp_len);
    if(tmp_buf == NULL)
    {
        DMSG("call malloc() fail [%s]", strerror(errno));
        goto FREE_01;
    }

    snprintf(tmp_buf, tmp_len, "alert(\"custom error 01\");");
    tmp_len = strlen(tmp_buf) + 1;

    this_session->rep_data_buf = tmp_buf;
    this_session->rep_data_len = tmp_len;

FREE_01:
    return fret;
}

int mcm_module_obtain_multiple_test_02(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    MCM_DTYPE_USIZE_TD tmp_len;
    char *tmp_buf;


    DMSG("obtain multiple test 02 :");

    tmp_len = 128;
    tmp_buf = (char *) malloc(tmp_len);
    if(tmp_buf == NULL)
    {
        DMSG("call malloc() fail [%s]", strerror(errno));
        goto FREE_01;
    }

    snprintf(tmp_buf, tmp_len, "alert(\"custom error 02\");");
    tmp_len = strlen(tmp_buf) + 1;

    this_session->rep_data_buf = tmp_buf;
    this_session->rep_data_len = tmp_len;

FREE_01:
    return fret;
}

int process_station_vap_part(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_EK_TD *part_key,
    MCM_DTYPE_EK_TD **key_list_buf,
    MCM_DTYPE_EK_TD *key_count_buf,
    MCM_DTYPE_ISI_TD rule_rule)
{
    int fret;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    MCM_DTYPE_EK_TD vap_count, *rep_list = NULL, rep_count = 0;
    MCM_DTYPE_BOOL_TD is_match;
    struct mcm_config_model_group_t *vap_group, *station_group;
    struct mcm_config_store_t *device_store, *vap_store, *station_store;
    struct mcm_dv_device_vap_t vap_v;
    struct mcm_dv_device_vap_station_t station_v;


    // 使用進階模式讀取資料, 先取出 device.vap.* 的開頭 store_info.
    path1 = "device.vap.*";
    fret = mcm_config_find_entry_by_mix(this_session, path1, &vap_group, &vap_store, NULL,
                                        NULL, NULL, &device_store);
    if(fret < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_find_entry_by_mix(%s) fail", path1);
        goto FREE_01;
    }

    // 取出 vap 的資料數目, 之後配置空間記錄符合的資料的 key.
    fret = mcm_config_get_count_by_info(this_session, vap_group, device_store, &vap_count);
    if(fret < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_count_by_info(%s) fail", path1);
        goto FREE_01;
    }

    if(vap_count > 0)
    {
        // 配置空間.
        rep_list = malloc(sizeof(MCM_DTYPE_EK_TD) * vap_count);
        if(rep_list == NULL)
        {
            DMSG("call malloc() fail [%s]", strerror(errno));
            goto FREE_01;
        }

        // 處理每一個 device.vap.*
        for(; vap_store != NULL; vap_store = vap_store->next_store)
        {
            // 讀出 vap 的 key 以便組合 station 的路徑來取出資料.
            fret = mcm_config_get_entry_by_info(this_session, vap_group, vap_store,
                                                MCM_DACCESS_SYS, &vap_v);
            if(fret < MCM_RCODE_PASS)
            {
                DMSG("call mcm_config_get_entry_by_info() fail");
                goto FREE_02;
            }

            // 使用進階模式讀取資料, 取出 device.vap.${vap_v.ekey}.station.* 的開頭 store_info.
            snprintf(path2, sizeof(path2), "device.vap.#%u.station.*", vap_v.ekey);
            fret = mcm_config_find_entry_by_mix(this_session, path2, &station_group,
                                                &station_store, NULL, NULL, NULL, NULL);
            if(fret < MCM_RCODE_PASS)
            {
                DMSG("call mcm_config_find_entry_by_mix(%s) fail", path2);
                goto FREE_02;
            }

            is_match = 0;

            // 處理每一個 device.vap.${vap_v.ekey}.station.*
            for(; station_store != NULL; station_store = station_store->next_store)
            {
                // 讀出 station 資料.
                fret = mcm_config_get_entry_by_info(this_session, station_group, station_store,
                                                    MCM_DACCESS_SYS, &station_v);
                if(fret < MCM_RCODE_PASS)
                {
                    DMSG("call mcm_config_get_entry_by_info() fail");
                    goto FREE_02;
                }

                // 檢查是否符合.
                if(rule_rule == station_v.rule)
                {
                    is_match = 1;
                    break;
                }
            }

            // 把符合的 key 加入到表中.
            if(is_match != 0)
            {
                rep_list[rep_count] = vap_v.ekey;
                rep_count++;
            }
        }
    }

    *key_list_buf = rep_list;
    *key_count_buf = rep_count;

    return fret;
FREE_02:
    free(rep_list);
FREE_01:
    return fret;
}

int process_station_station_part(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_EK_TD *part_key,
    MCM_DTYPE_EK_TD **key_list_buf,
    MCM_DTYPE_EK_TD *key_count_buf,
    MCM_DTYPE_ISI_TD rule_rule)
{
    int fret;
    char path2[MCM_PATH_MAX_LENGTH];
    MCM_DTYPE_EK_TD target_vap, station_count, *rep_list = NULL, rep_count = 0;
    struct mcm_config_model_group_t *station_group;
    struct mcm_config_store_t *vap_store, *station_store;
    struct mcm_dv_device_vap_station_t station_v;


    // 使用進階模式讀取資料, 取出 device.vap.${target_vap}.station.* 的開頭 store_info.
    target_vap = part_key[1];
    snprintf(path2, sizeof(path2), "device.vap.#%u.station.*", target_vap);
    fret = mcm_config_find_entry_by_mix(this_session, path2, &station_group,
                                        &station_store, NULL, NULL, NULL, &vap_store);
    if(fret < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_find_entry_by_mix(%s) fail", path2);
        goto FREE_01;
    }

    // 取出 station 的資料數目, 之後配置空間記錄符合的資料的 key.
    fret = mcm_config_get_count_by_info(this_session, station_group, vap_store, &station_count);
    if(fret < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_count_by_info(%s) fail", path2);
        goto FREE_01;
    }

    if(station_count > 0)
    {
        // 配置空間.
        rep_list = malloc(sizeof(MCM_DTYPE_EK_TD) * station_count);
        if(rep_list == NULL)
        {
            DMSG("call malloc() fail [%s]", strerror(errno));
            goto FREE_01;
        }

        // 處理每一個 device.vap.${target_vap}.station.*
        for(; station_store != NULL; station_store = station_store->next_store)
        {
            // 讀出 station 資料.
            fret = mcm_config_get_entry_by_info(this_session, station_group, station_store,
                                                MCM_DACCESS_SYS, &station_v);
            if(fret < MCM_RCODE_PASS)
            {
                DMSG("call mcm_config_get_entry_by_info() fail");
                goto FREE_02;
            }

            // 檢查是否符合.
            if(rule_rule == station_v.rule)
            {
                rep_list[rep_count] = station_v.ekey;
                rep_count++;
            }
        }
    }

    *key_list_buf = rep_list;
    *key_count_buf = rep_count;

    return fret;
FREE_02:
    free(rep_list);
FREE_01:
    return fret;
}

int mcm_module_obtain_match_rule_station(
    struct mcm_service_session_t *this_session)
{
    int fret;
    char *path1;
    MCM_DTYPE_USIZE_TD part_level;
    MCM_DTYPE_EK_TD *part_key, *rep_list = NULL, rep_count = 0;
    MCM_DTYPE_ISI_TD rule_rule;


    DMSG("obtain match rule station test (advance get) :");

    // 取出 CGI 模組傳送的資料,
    // 格式 : | part_key[0] | part_key[1] | ... | part_key[part_level - 1] |.
    part_level = this_session->req_data_len / sizeof(MCM_DTYPE_EK_TD);
    part_key = (MCM_DTYPE_EK_TD *) this_session->req_data_con;

    DMSG("part_level = %u", part_level);

    // 讀出規則.
    path1 = "device.filter.rule1";
    fret = mcm_config_get_alone_by_path(this_session, path1, MCM_DACCESS_AUTO, &rule_rule);
    if(fret < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_get_alone(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("find station (rule = " MCM_DTYPE_ISI_PF ")", rule_rule);

    // 處理 device.vap.*.station.* 的 vap 部分,
    // 回報符合規則的 station 所屬的 vap.
    if(part_level == 2)
    {
        DMSG("process vap part");

        fret = process_station_vap_part(this_session, part_key, &rep_list, &rep_count,
                                        rule_rule);
        if(fret < MCM_RCODE_PASS)
        {
            DMSG("call process_station_vap_part() fail");
            goto FREE_01;
        }
    }
    else
    // 處理 device.vap.*.station.* 的 station 部分,
    // 回報符合規則的 station.
    if(part_level == 3)
    {
        DMSG("process station part");
        DMSG("device.vap.#%u.station.*", part_key[1]);

        fret = process_station_station_part(this_session, part_key, &rep_list, &rep_count,
                                            rule_rule);
        if(fret < MCM_RCODE_PASS)
        {
            DMSG("call process_station_station_part() fail");
            goto FREE_01;
        }
    }

    // 回傳符合的資料, 格式 : | key1 | key2 | ... | keyN |.
    this_session->rep_data_buf = rep_list;
    this_session->rep_data_len = sizeof(MCM_DTYPE_EK_TD) * rep_count;

FREE_01:
    return fret;
}

int mcm_module_submit_error_test(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    MCM_DTYPE_USIZE_TD tmp_len;
    char *tmp_buf;


    DMSG("submit error test :");

    tmp_len = 128;
    tmp_buf = (char *) malloc(tmp_len);
    if(tmp_buf == NULL)
    {
        DMSG("call malloc() fail [%s]", strerror(errno));
        goto FREE_01;
    }

    snprintf(tmp_buf, tmp_len, "alert(\"custom error\");");
    tmp_len = strlen(tmp_buf) + 1;

    this_session->rep_data_buf = tmp_buf;
    this_session->rep_data_len = tmp_len;

FREE_01:
    return fret;
}

int mcm_module_submit_text_test(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1, *tmp_buf;
    struct mcm_dv_device_t device_v;
    MCM_DTYPE_USIZE_TD tmp_len;


    DMSG("submit text test :");

    path1 = "device";
    if(mcm_config_get_entry_by_path(this_session, path1, MCM_DACCESS_AUTO, &device_v)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_by_path(%s) fail", path1);
        goto FREE_01;
    }

    tmp_len = 512;
    tmp_buf = (char *) malloc(tmp_len);
    if(tmp_buf == NULL)
    {
        DMSG("call malloc() fail [%s]", strerror(errno));
        goto FREE_01;
    }

    snprintf(tmp_buf, tmp_len, " %s", device_v.descript);
    tmp_len = strlen(tmp_buf) + 1;

    this_session->rep_data_buf = tmp_buf;
    this_session->rep_data_len = tmp_len;

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_submit_json_test(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1, *tmp_buf;
    struct mcm_dv_device_t device_v;
    MCM_DTYPE_USIZE_TD tmp_len;


    DMSG("submit json test :");

    path1 = "device";
    if(mcm_config_get_entry_by_path(this_session, path1, MCM_DACCESS_AUTO, &device_v)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_by_path(%s) fail", path1);
        goto FREE_01;
    }

    tmp_len = 512;
    tmp_buf = (char *) malloc(tmp_len);
    if(tmp_buf == NULL)
    {
        DMSG("call malloc() fail [%s]", strerror(errno));
        goto FREE_01;
    }

    snprintf(tmp_buf, tmp_len, "{\"descript\":\"%s\"}", device_v.descript);
    tmp_len = strlen(tmp_buf) + 1;

    this_session->rep_data_buf = tmp_buf;
    this_session->rep_data_len = tmp_len;

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_javascript_control_element_test(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1, *tmp_buf;
    struct mcm_dv_device_system_t system_v;
    MCM_DTYPE_USIZE_TD tmp_len;


    DMSG("submit javascript control element test :");

    path1 = "device.system";
    if(mcm_config_get_entry_by_path(this_session, path1, MCM_DACCESS_AUTO, &system_v)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_by_path(%s) fail", path1);
        goto FREE_01;
    }

    tmp_len = 512;
    tmp_buf = (char *) malloc(tmp_len);
    if(tmp_buf == NULL)
    {
        DMSG("call malloc() fail [%s]", strerror(errno));
        goto FREE_01;
    }

    snprintf(tmp_buf, tmp_len,
             "$(\"#number_div\").html(\""
             "<font color=\\\"#%s\\\">%s number</font>"
             "\");",
             system_v.uptime % 2 == 0 ? "FF0000" : "0000FF",
             system_v.uptime % 2 == 0 ? "even" : "odd");
    tmp_len = strlen(tmp_buf) + 1;

    this_session->rep_data_buf = tmp_buf;
    this_session->rep_data_len = tmp_len;

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}


int mcm_module_submit_javascript_redirect_page_test(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1, *tmp_buf;
    struct mcm_dv_device_t device_v;
    MCM_DTYPE_USIZE_TD tmp_len;


    DMSG("submit javascript redirect page test :");

    path1 = "device";
    if(mcm_config_get_entry_by_path(this_session, path1, MCM_DACCESS_AUTO, &device_v)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_by_path(%s) fail", path1);
        goto FREE_01;
    }

    tmp_len = 512;
    tmp_buf = (char *) malloc(tmp_len);
    if(tmp_buf == NULL)
    {
        DMSG("call malloc() fail [%s]", strerror(errno));
        goto FREE_01;
    }

    snprintf(tmp_buf, tmp_len, "window.location.href = \"%s\";", device_v.descript);
    tmp_len = strlen(tmp_buf) + 1;

    this_session->rep_data_buf = tmp_buf;
    this_session->rep_data_len = tmp_len;

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_submit_multiple_test_01(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *tmp_buf;
    MCM_DTYPE_USIZE_TD tmp_len;


    DMSG("submit multiple test 01 :");

    tmp_len = 128;
    tmp_buf = (char *) malloc(tmp_len);
    if(tmp_buf == NULL)
    {
        DMSG("call malloc() fail [%s]", strerror(errno));
        goto FREE_01;
    }

    snprintf(tmp_buf, tmp_len, "custom message 01");
    tmp_len = strlen(tmp_buf) + 1;

    this_session->rep_data_buf = tmp_buf;
    this_session->rep_data_len = tmp_len;

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_submit_multiple_test_02(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *tmp_buf;
    MCM_DTYPE_USIZE_TD tmp_len;


    DMSG("submit multiple test 02 :");

    tmp_len = 128;
    tmp_buf = (char *) malloc(tmp_len);
    if(tmp_buf == NULL)
    {
        DMSG("call malloc() fail [%s]", strerror(errno));
        goto FREE_01;
    }

    snprintf(tmp_buf, tmp_len, "custom message 02");
    tmp_len = strlen(tmp_buf) + 1;

    this_session->rep_data_buf = tmp_buf;
    this_session->rep_data_len = tmp_len;

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

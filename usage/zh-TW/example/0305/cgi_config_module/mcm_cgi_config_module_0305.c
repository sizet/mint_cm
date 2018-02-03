// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <errno.h>
#include <stdlib.h>
#include "../../mcm_lib/mcm_lheader/mcm_type.h"
#include "../../mcm_lib/mcm_lheader/mcm_size.h"
#include "../../mcm_lib/mcm_lheader/mcm_connect.h"
#include "../../mcm_lib/mcm_lheader/mcm_return.h"
#include "../../mcm_lib/mcm_lheader/mcm_data_exinfo_auto.h"
#include "../../mcm_lib/mcm_lulib/mcm_lulib_api.h"
#include "mcm_cgi_module_debug.h"


int find_5g_vap(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_USIZE_TD part_level,
    MCM_DTYPE_EK_TD *part_key,
    MCM_DTYPE_EK_TD **key_list_buf,
    MCM_DTYPE_EK_TD *key_count_buf)
{
    int fret;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_dv_device_vap_t *vap_v;
    MCM_DTYPE_EK_TD vap_idx, vap_count, *rep_list = NULL, rep_count = 0;


#if MCM_CCMEMODE | MCM_CCMDMODE
    dbg_console_fd = open(MCM_DBG_CONSOLE, O_WRONLY);
    if(dbg_console_fd == -1)
        return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
#endif

    MCM_CCMDMSG("part_level = %u", part_level);

    //  讀出所有的 device.vap.*
    path1 = "device.vap.*";
    fret = mcm_lulib_get_all_entry(this_lulib, path1, (void **) &vap_v, &vap_count);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CCMEMSG("call mcm_lulib_get_all_entry(%s) fail", path1);
        goto FREE_01;
    }

    if(vap_count > 0)
    {
        rep_list = (MCM_DTYPE_EK_TD *) malloc(sizeof(MCM_DTYPE_EK_TD) * vap_count);
        if(rep_list == NULL)
        {
            MCM_CCMEMSG("call malloc() fail [%s]", strerror(errno));
            goto FREE_02;
        }

        for(vap_idx = 0; vap_idx < vap_count; vap_idx++)
        {
            snprintf(path2, sizeof(path2), "device.vap.#%u", vap_v[vap_idx].ekey);
            MCM_CCMDMSG("%s.ssid = " MCM_DTYPE_S_PF, path2, vap_v[vap_idx].ssid);
            MCM_CCMDMSG("%s.channel = " MCM_DTYPE_IUI_PF, path2, vap_v[vap_idx].channel);

            if(vap_v[vap_idx].channel >= 36)
            {
                MCM_CCMDMSG("this vap is match");
                rep_list[rep_count] = vap_v[vap_idx].ekey;
                rep_count++;
            }
            else
            {
                MCM_CCMDMSG("this vap is not match");
            }
        }
    }

    *key_list_buf = rep_list;
    *key_count_buf = rep_count;

FREE_02:
    free(vap_v);
FREE_01:
#if MCM_CCMEMODE | MCM_CCMDMODE
    close(dbg_console_fd);
#endif
    return fret;
}

int find_hidden_vap(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_USIZE_TD part_level,
    MCM_DTYPE_EK_TD *part_key,
    MCM_DTYPE_EK_TD **key_list_buf,
    MCM_DTYPE_EK_TD *key_count_buf)
{
    int fret;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_dv_device_vap_extra_t extra_v;
    MCM_DTYPE_EK_TD vap_idx, vap_count, *vap_key, *rep_list = NULL, rep_count = 0;


#if MCM_CCMEMODE | MCM_CCMDMODE
    dbg_console_fd = open(MCM_DBG_CONSOLE, O_WRONLY);
    if(dbg_console_fd == -1)
        return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
#endif

    MCM_CCMDMSG("part_level = %u", part_level);

    //  讀出 device.vap.* 所有的 key, 再逐一讀出 device.vap.*.extra
    path1 = "device.vap.*";
    fret = mcm_lulib_get_all_key(this_lulib, path1, (MCM_DTYPE_EK_TD **) &vap_key, &vap_count);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CCMEMSG("call mcm_lulib_get_all_key(%s) fail", path1);
        goto FREE_01;
    }

    if(vap_count > 0)
    {
        rep_list = (MCM_DTYPE_EK_TD *) malloc(sizeof(MCM_DTYPE_EK_TD) * vap_count);
        if(rep_list == NULL)
        {
            MCM_CCMEMSG("call malloc() fail [%s]", strerror(errno));
            goto FREE_02;
        }

        for(vap_idx = 0; vap_idx < vap_count; vap_idx++)
        {
            snprintf(path2, sizeof(path2), "device.vap.#%u.extra", vap_key[vap_idx]);
            fret = mcm_lulib_get_entry(this_lulib, path2, &extra_v);
            if(fret < MCM_RCODE_PASS)
            {
                MCM_CCMEMSG("call mcm_lulib_get_entry(%s) fail", path2);
                goto FREE_02;
            }

            MCM_CCMDMSG("%s.hidden = " MCM_DTYPE_ISC_PF, path2, extra_v.hidden);
            MCM_CCMDMSG("%s.tx_power = " MCM_DTYPE_ISS_PF, path2, extra_v.tx_power);

            if(extra_v.hidden != 1)
            {
                MCM_CCMDMSG("this vap is match");
                rep_list[rep_count] = vap_key[vap_idx];
                rep_count++;
            }
            else
            {
                MCM_CCMDMSG("this vap is not match");
            }
        }
    }

    *key_list_buf = rep_list;
    *key_count_buf = rep_count;

FREE_02:
    free(vap_key);
FREE_01:
#if MCM_CCMEMODE | MCM_CCMDMODE
    close(dbg_console_fd);
#endif
    return fret;
}

int find_limit_by_priority(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_USIZE_TD part_level,
    MCM_DTYPE_EK_TD *part_key,
    MCM_DTYPE_EK_TD **key_list_buf,
    MCM_DTYPE_EK_TD *key_count_buf)
{
    int fret;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_dv_device_limit_t *limit_v;
    MCM_DTYPE_ISI_TD priority_rule, priority_range[2];
    MCM_DTYPE_EK_TD limit_idx, limit_count, *rep_list = NULL, rep_count = 0;


#if MCM_CCMEMODE | MCM_CCMDMODE
    dbg_console_fd = open(MCM_DBG_CONSOLE, O_WRONLY);
    if(dbg_console_fd == -1)
        return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
#endif

    MCM_CCMDMSG("part_level = %u", part_level);

    // 讀出規則.
    path1 = "device.filter.rule1";
    fret = mcm_lulib_get_alone(this_lulib, path1, &priority_rule);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CCMEMSG("call mcm_lulib_get_alone(%s) fail", path1);
        goto FREE_01;
    }
    priority_range[0] = priority_range[1] = 0;
    if(priority_rule == 0)
    {
        priority_range[0] = 1;
        priority_range[1] = 10;
    }
    else
    if(priority_rule == 1)
    {
        priority_range[0] = 11;
        priority_range[1] = 20;
    }
    MCM_CCMDMSG("find limit (priority = " MCM_DTYPE_ISI_PF " ~ " MCM_DTYPE_ISI_PF ")",
                priority_range[0], priority_range[1]);

    //  讀出所有的 device.limit.*
    path1 = "device.limit.*";
    fret = mcm_lulib_get_all_entry(this_lulib, path1, (void **) &limit_v, &limit_count);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CCMEMSG("call mcm_lulib_get_all_entry(%s) fail", path1);
        goto FREE_01;
    }

    if(limit_count > 0)
    {
        rep_list = (MCM_DTYPE_EK_TD *) malloc(sizeof(MCM_DTYPE_EK_TD) * limit_count);
        if(rep_list == NULL)
        {
            MCM_CCMEMSG("call malloc() fail [%s]", strerror(errno));
            goto FREE_02;
        }

        for(limit_idx = 0; limit_idx < limit_count; limit_idx++)
        {
            snprintf(path2, sizeof(path2), "device.limit.#%u", limit_v[limit_idx].ekey);
            MCM_CCMDMSG("%s.name = %s", path2, limit_v[limit_idx].name);
            MCM_CCMDMSG("%s.priority = " MCM_DTYPE_ISI_PF, path2, limit_v[limit_idx].priority);

            if((priority_range[0] <= limit_v[limit_idx].priority) &&
               (limit_v[limit_idx].priority <= priority_range[1]))
            {
                MCM_CCMDMSG("this limit is match");
                rep_list[rep_count] = limit_v[limit_idx].ekey;
                rep_count++;
            }
            else
            {
                MCM_CCMDMSG("this limit is not match");
            }
        }
    }

    *key_list_buf = rep_list;
    *key_count_buf = rep_count;

FREE_02:
    free(limit_v);
FREE_01:
#if MCM_CCMEMODE | MCM_CCMDMODE
    close(dbg_console_fd);
#endif
    return fret;
}

int process_user_vap_part(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_EK_TD *part_key,
    MCM_DTYPE_EK_TD **key_list_buf,
    MCM_DTYPE_EK_TD *key_count_buf,
    MCM_DTYPE_ISI_TD gender_rule)
{
    int fret;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_dv_device_vap_station_user_t *user_v;
    MCM_DTYPE_EK_TD vap_idx, vap_count, *vap_key, station_idx, station_count, *station_key,
        user_idx, user_count, rep_idx, *rep_list = NULL, rep_count = 0;


    // 讀出 device.vap.* 所有的 key.
    path1 = "device.vap.*";
    fret = mcm_lulib_get_all_key(this_lulib, path1, (MCM_DTYPE_EK_TD **) &vap_key, &vap_count);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CCMEMSG("call mcm_lulib_get_all_key(%s) fail", path1);
        goto FREE_01;
    }

    if(vap_count > 0)
    {
        // 配置空間放置 key.
        rep_list = (MCM_DTYPE_EK_TD *) malloc(sizeof(MCM_DTYPE_EK_TD) * vap_count);
        if(rep_list == NULL)
        {
            MCM_CCMEMSG("call malloc() fail [%s]", strerror(errno));
            goto FREE_02;
        }

        for(vap_idx = 0; vap_idx < vap_count; vap_idx++)
        {
            // 讀出 device.vap.#{vap_key[vap_idx]}.station.* 所有的 key.
            snprintf(path2, sizeof(path2), "device.vap.#%u.station.*", vap_key[vap_idx]);
            fret = mcm_lulib_get_all_key(this_lulib, path2, (MCM_DTYPE_EK_TD **) &station_key,
                                         &station_count);
            if(fret < MCM_RCODE_PASS)
            {
                MCM_CCMEMSG("call mcm_lulib_get_all_key(%s) fail", path2);
                goto FREE_03;
            }

            for(station_idx = 0; station_idx < station_count; station_idx++)
            {
                // 讀 device.vap.#{vap_key[vap_idx]}.station.#{station_key[station_idx]}.user.*
                snprintf(path2, sizeof(path2), "device.vap.#%u.station.#%u.user.*",
                         vap_key[vap_idx], station_key[station_idx]);
                fret = mcm_lulib_get_all_entry(this_lulib, path2, (void **) &user_v, &user_count);
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_CCMEMSG("call mcm_lulib_get_all_entry(%s) fail", path2);
                    goto FREE_04;
                }

                for(user_idx = 0; user_idx < user_count; user_idx++)
                {
                    snprintf(path2, sizeof(path2), "device.vap.#%u.station.#%u.user.#%u",
                             vap_key[vap_idx], station_key[station_idx], user_v[user_idx].ekey);
                    MCM_CCMDMSG("%s.name = " MCM_DTYPE_S_PF, path2, user_v[user_idx].name);
                    MCM_CCMDMSG("%s.gender = " MCM_DTYPE_IUC_PF, path2, user_v[user_idx].gender);

                    //檢查是否符合.
                    if(gender_rule == user_v[user_idx].gender)
                    {
                        MCM_CCMDMSG("this vap is match");
                        // 檢查是否已經在表內, 沒有才加入.
                        for(rep_idx = 0; rep_idx < rep_count; rep_idx++)
                            if(rep_list[rep_idx] == vap_key[vap_idx])
                                break;
                        if(rep_idx == rep_count)
                        {
                            rep_list[rep_count] = vap_key[vap_idx];
                            rep_count++;
                        }
                    }
                    else
                    {
                        MCM_CCMDMSG("this vap is not match");
                    }
                }

                free(user_v);
            }

            free(station_key);
        }
    }

    *key_list_buf = rep_list;
    *key_count_buf = rep_count;

FREE_04:
    if(fret < MCM_RCODE_PASS)
        free(station_key);
FREE_03:
    if(fret < MCM_RCODE_PASS)
        free(rep_list);
FREE_02:
    free(vap_key);
FREE_01:
    return fret;
}

int process_user_station_part(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_EK_TD *part_key,
    MCM_DTYPE_EK_TD **key_list_buf,
    MCM_DTYPE_EK_TD *key_count_buf,
    MCM_DTYPE_ISI_TD gender_rule,
    MCM_DTYPE_EK_TD target_vap)
{
    int fret;
    char path2[MCM_PATH_MAX_LENGTH];
    struct mcm_dv_device_vap_station_user_t *user_v;
    MCM_DTYPE_EK_TD station_idx, station_count, *station_key, user_idx, user_count, rep_idx,
        *rep_list = NULL, rep_count = 0;


    // 讀出 device.vap.#{target_vap}.station.* 所有的 key.
    snprintf(path2, sizeof(path2), "device.vap.#%u.station.*", target_vap);
    fret = mcm_lulib_get_all_key(this_lulib, path2, (MCM_DTYPE_EK_TD **) &station_key,
                                 &station_count);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CCMEMSG("call mcm_lulib_get_all_key(%s) fail", path2);
        goto FREE_01;
    }

    if(station_count > 0)
    {
        // 配置空間放置 key.
        rep_list = (MCM_DTYPE_EK_TD *) malloc(sizeof(MCM_DTYPE_EK_TD) * station_count);
        if(rep_list == NULL)
        {
            MCM_CCMEMSG("call malloc() fail [%s]", strerror(errno));
            goto FREE_02;
        }

        for(station_idx = 0; station_idx < station_count; station_idx++)
        {
            // 讀出 device.vap.#{target_vap}.station.#{station_key[station_idx]}.user.*
            snprintf(path2, sizeof(path2), "device.vap.#%u.station.#%u.user.*",
                     target_vap, station_key[station_idx]);
            fret = mcm_lulib_get_all_entry(this_lulib, path2, (void **) &user_v, &user_count);
            if(fret < MCM_RCODE_PASS)
            {
                MCM_CCMEMSG("call mcm_lulib_get_all_entry(%s) fail", path2);
                goto FREE_03;
            }

            for(user_idx = 0; user_idx < user_count; user_idx++)
            {
                snprintf(path2, sizeof(path2), "device.vap.#%u.station.#%u.user.#%u",
                         target_vap, station_key[station_idx], user_v[user_idx].ekey);
                MCM_CCMDMSG("%s.name = " MCM_DTYPE_S_PF, path2, user_v[user_idx].name);
                MCM_CCMDMSG("%s.gender = " MCM_DTYPE_IUC_PF, path2, user_v[user_idx].gender);

                //檢查是否符合.
                if(gender_rule == user_v[user_idx].gender)
                {
                    MCM_CCMDMSG("this station is match");
                    // 檢查是否已經在表內, 沒有才加入.
                    for(rep_idx = 0; rep_idx < rep_count; rep_idx++)
                        if(rep_list[rep_idx] == station_key[station_idx])
                            break;
                    if(rep_idx == rep_count)
                    {
                        rep_list[rep_count] = station_key[station_idx];
                        rep_count++;
                    }
                }
                else
                {
                    MCM_CCMDMSG("this station is not match");
                }
            }

            free(user_v);
        }
    }

    *key_list_buf = rep_list;
    *key_count_buf = rep_count;

FREE_03:
    if(fret < MCM_RCODE_PASS)
        free(rep_list);
FREE_02:
    free(station_key);
FREE_01:
    return fret;
}

int process_user_user_part(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_EK_TD *part_key,
    MCM_DTYPE_EK_TD **key_list_buf,
    MCM_DTYPE_EK_TD *key_count_buf,
    MCM_DTYPE_ISI_TD gender_rule,
    MCM_DTYPE_EK_TD target_vap,
    MCM_DTYPE_EK_TD target_station)
{
    int fret;
    char path2[MCM_PATH_MAX_LENGTH];
    struct mcm_dv_device_vap_station_user_t *user_v;
    MCM_DTYPE_EK_TD user_idx, user_count, *rep_list = NULL, rep_count = 0;


    // 讀出 device.vap.#{target_vap}.station.#{target_station}.user.*
    snprintf(path2, sizeof(path2), "device.vap.#%u.station.#%u.user.*",
             target_vap, target_station);
    fret = mcm_lulib_get_all_entry(this_lulib, path2, (void **) &user_v, &user_count);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CCMEMSG("call mcm_lulib_get_all_entry(%s) fail", path2);
        goto FREE_01;
    }

    if(user_count > 0)
    {
        // 配置空間放置 key.
        rep_list = (MCM_DTYPE_EK_TD *) malloc(sizeof(MCM_DTYPE_EK_TD) * user_count);
        if(rep_list == NULL)
        {
            MCM_CCMEMSG("call malloc() fail [%s]", strerror(errno));
            goto FREE_02;
        }

        for(user_idx = 0; user_idx < user_count; user_idx++)
        {
            snprintf(path2, sizeof(path2), "device.vap.#%u.station.#%u.user.#%u",
                     target_vap, target_station, user_v[user_idx].ekey);
            MCM_CCMDMSG("%s.name = " MCM_DTYPE_S_PF, path2, user_v[user_idx].name);
            MCM_CCMDMSG("%s.gender = " MCM_DTYPE_IUC_PF, path2, user_v[user_idx].gender);

            //檢查是否符合.
            if(gender_rule == user_v[user_idx].gender)
            {
                MCM_CCMDMSG("this user is match");
                rep_list[rep_count] = user_v[user_idx].ekey;
                rep_count++;
            }
            else
            {
                MCM_CCMDMSG("this user is not match");
            }
        }
    }

    *key_list_buf = rep_list;
    *key_count_buf = rep_count;

FREE_02:
    free(user_v);
FREE_01:
    return fret;
}

int find_user_by_gender(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_USIZE_TD part_level,
    MCM_DTYPE_EK_TD *part_key,
    MCM_DTYPE_EK_TD **key_list_buf,
    MCM_DTYPE_EK_TD *key_count_buf)
{
    int fret;
    char *path1;
    MCM_DTYPE_ISI_TD gender_rule;
    MCM_DTYPE_EK_TD *rep_list = NULL, rep_count = 0;


#if MCM_CCMEMODE | MCM_CCMDMODE
    dbg_console_fd = open(MCM_DBG_CONSOLE, O_WRONLY);
    if(dbg_console_fd == -1)
        return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
#endif

    MCM_CCMDMSG("part_level = %u", part_level);

    // 讀出規則.
    path1 = "device.filter.rule2";
    fret = mcm_lulib_get_alone(this_lulib, path1, &gender_rule);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CCMEMSG("call mcm_lulib_get_alone(%s) fail", path1);
        goto FREE_01;
    }
    MCM_CCMDMSG("find user (gender = " MCM_DTYPE_ISI_PF ")", gender_rule);

    // 處理 device.vap.*.station.*.user.* 的 vap 部分,
    // 回報符合規則的 user 所屬的 vap.
    if(part_level == 2)
    {
        MCM_CCMDMSG("process vap part");

        fret = process_user_vap_part(this_lulib, part_key, &rep_list, &rep_count, gender_rule);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_CCMEMSG("call process_user_vap_part() fail");
            goto FREE_01;
        }
    }
    else
    // 處理 device.vap.*.station.*.user.* 的 station 部分,
    // 回報符合規則的 user 所屬的 station.
    if(part_level == 3)
    {
        MCM_CCMDMSG("process station part");
        MCM_CCMDMSG("device.vap.#%u.station.*", part_key[1]);

        fret = process_user_station_part(this_lulib, part_key, &rep_list, &rep_count, gender_rule,
                                         part_key[1]);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_CCMEMSG("call process_user_station_part() fail");
            goto FREE_01;
        }
    }
    else
    // 處理 device.vap.*.station.*.user.* 的 user 部分,
    // 回報符合規則的 user.
    if(part_level == 4)
    {
        MCM_CCMDMSG("process user part");
        MCM_CCMDMSG("device.vap.#%u.station.#%u.user.*", part_key[1], part_key[2]);

        fret = process_user_user_part(this_lulib, part_key, &rep_list, &rep_count, gender_rule,
                                      part_key[1], part_key[2]);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_CCMEMSG("call process_user_user_part() fail");
            goto FREE_01;
        }
    }

    *key_list_buf = rep_list;
    *key_count_buf = rep_count;

FREE_01:
#if MCM_CCMEMODE | MCM_CCMDMODE
    close(dbg_console_fd);
#endif
    return fret;
}

int find_2g_or_5g_vap(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_USIZE_TD part_level,
    MCM_DTYPE_EK_TD *part_key,
    MCM_DTYPE_EK_TD **key_list_buf,
    MCM_DTYPE_EK_TD *key_count_buf)
{
    int fret;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    MCM_DTYPE_ISI_TD radio_rule, is_match;
    struct mcm_dv_device_vap_t *vap_v;
    MCM_DTYPE_EK_TD vap_idx, vap_count, *rep_list = NULL, rep_count = 0;


#if MCM_CCMEMODE | MCM_CCMDMODE
    dbg_console_fd = open(MCM_DBG_CONSOLE, O_WRONLY);
    if(dbg_console_fd == -1)
        return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
#endif

    MCM_CCMDMSG("part_level = %u", part_level);

    // 讀出規則.
    path1 = "device.filter.rule1";
    fret = mcm_lulib_get_alone(this_lulib, path1, &radio_rule);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CCMEMSG("call mcm_lulib_get_alone(%s) fail", path1);
        goto FREE_01;
    }
    MCM_CCMDMSG("find vap (radio = %s)", radio_rule == 0 ? "2.4G" : "5G");

    //  讀出所有的 device.vap.*
    path1 = "device.vap.*";
    fret = mcm_lulib_get_all_entry(this_lulib, path1, (void **) &vap_v, &vap_count);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CCMEMSG("call mcm_lulib_get_all_entry(%s) fail", path1);
        goto FREE_01;
    }

    if(vap_count > 0)
    {
        rep_list = (MCM_DTYPE_EK_TD *) malloc(sizeof(MCM_DTYPE_EK_TD) * vap_count);
        if(rep_list == NULL)
        {
            MCM_CCMEMSG("call malloc() fail [%s]", strerror(errno));
            goto FREE_02;
        }

        for(vap_idx = 0; vap_idx < vap_count; vap_idx++)
        {
            snprintf(path2, sizeof(path2), "device.vap.#%u", vap_v[vap_idx].ekey);
            MCM_CCMDMSG("%s.ssid = " MCM_DTYPE_S_PF, path2, vap_v[vap_idx].ssid);
            MCM_CCMDMSG("%s.channel = " MCM_DTYPE_IUI_PF, path2, vap_v[vap_idx].channel);

            //檢查是否符合.
            if(radio_rule == 0)
                is_match = vap_v[vap_idx].channel < 36 ? 1 : 0;
            else
                is_match = vap_v[vap_idx].channel < 36 ? 0 : 1;

            if(is_match != 0)
            {
                MCM_CCMDMSG("this vap is match");
                rep_list[rep_count] = vap_v[vap_idx].ekey;
                rep_count++;
            }
            else
            {
                MCM_CCMDMSG("this vap is not match");
            }
        }
    }

    *key_list_buf = rep_list;
    *key_count_buf = rep_count;

FREE_02:
    free(vap_v);
FREE_01:
#if MCM_CCMEMODE | MCM_CCMDMODE
    close(dbg_console_fd);
#endif
    return fret;
}

int find_station_by_rule(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_USIZE_TD part_level,
    MCM_DTYPE_EK_TD *part_key,
    MCM_DTYPE_EK_TD **key_list_buf,
    MCM_DTYPE_EK_TD *key_count_buf)
{
    int fret;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    MCM_DTYPE_ISI_TD rule_rule;
    struct mcm_dv_device_vap_station_t *station_v;
    MCM_DTYPE_EK_TD target_vap, station_idx, station_count, *rep_list = NULL, rep_count = 0;


#if MCM_CCMEMODE | MCM_CCMDMODE
    dbg_console_fd = open(MCM_DBG_CONSOLE, O_WRONLY);
    if(dbg_console_fd == -1)
        return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
#endif

    MCM_CCMDMSG("part_level = %u", part_level);

    // 讀出規則.
    path1 = "device.filter.rule2";
    fret = mcm_lulib_get_alone(this_lulib, path1, &rule_rule);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CCMEMSG("call mcm_lulib_get_alone(%s) fail", path1);
        goto FREE_01;
    }
    MCM_CCMDMSG("find station (rule = " MCM_DTYPE_ISI_PF ")", rule_rule);

    // 處理 device.vap.*.station.* 的 vap 部分,
    // 回報符合規則的 station 所屬的 vap.
    if(part_level == 2)
    {
        MCM_CCMDMSG("process vap part");

        // 在此範例下此部分不用處理, 因為執行 "&get.device.vap.*=find_2g_or_5g_vap" 之後,
        // 已經決定好哪些 vap 要回傳,
        // 因此在執行 "&get.device.vap.*.station.*=find_station_by_rule" 時,
        // vap 的部分不會被呼叫.
    }
    else
    // 處理 device.vap.*.station.* 的 station 部分,
    // 回報符合規則的 station.
    if(part_level == 3)
    {
        MCM_CCMDMSG("process station part");
        MCM_CCMDMSG("device.vap.#%u.station.*", part_key[1]);

        target_vap = part_key[1];

        // 讀出 device.vap.#{target_vap}.station.*
        snprintf(path2, sizeof(path2), "device.vap.#%u.station.*", target_vap);
        fret = mcm_lulib_get_all_entry(this_lulib, path2, (void **) &station_v, &station_count);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_CCMEMSG("call mcm_lulib_get_all_entry(%s) fail", path2);
            goto FREE_01;
        }

        if(station_count > 0)
        {
            rep_list = (MCM_DTYPE_EK_TD *) malloc(sizeof(MCM_DTYPE_EK_TD) * station_count);
            if(rep_list == NULL)
            {
                MCM_CCMEMSG("call malloc() fail [%s]", strerror(errno));
                goto FREE_02;
            }

            for(station_idx = 0; station_idx < station_count; station_idx++)
            {
                snprintf(path2, sizeof(path2), "device.vap.#%u.station.#%u",
                         target_vap, station_v[station_idx].ekey);
                MCM_CCMDMSG("%s.mac_addr = " MCM_DTYPE_S_PF,
                            path2, station_v[station_idx].mac_addr);
                MCM_CCMDMSG("%s.rule = " MCM_DTYPE_RK_PF,
                            path2, station_v[station_idx].rule);

                //檢查是否符合.
                if(rule_rule == station_v[station_idx].rule)
                {
                    MCM_CCMDMSG("this station is match");
                    rep_list[rep_count] = station_v[station_idx].ekey;
                    rep_count++;
                }
                else
                {
                    MCM_CCMDMSG("this station is not match");
                }
            }
        }
    }

    *key_list_buf = rep_list;
    *key_count_buf = rep_count;

FREE_02:
    free(station_v);
FREE_01:
#if MCM_CCMEMODE | MCM_CCMDMODE
    close(dbg_console_fd);
#endif
    return fret;
}

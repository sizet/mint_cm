// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include "../../mcm_lib/mcm_lheader/mcm_type.h"
#include "../../mcm_lib/mcm_lheader/mcm_size.h"
#include "../../mcm_lib/mcm_lheader/mcm_control.h"
#include "../../mcm_lib/mcm_lheader/mcm_connect.h"
#include "../../mcm_lib/mcm_lheader/mcm_return.h"
#include "../../mcm_lib/mcm_lheader/mcm_debug.h"
#include "../../mcm_lib/mcm_lheader/mcm_data_exinfo_auto.h"
#include "../mcm_service_handle_define.h"
#include "../mcm_config_handle_extern.h"


#define DMSG(msg_fmt, msgs...) printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msgs)


int mcm_module_check_version(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;


    DMSG("check version test :");

    // 取得資料模型的版本.
    DMSG("profile version = [%s]", MCM_PROFILE_VERSION);
    // 取得資料現在值檔案的版本.
    DMSG("current version = [%s]", mcm_config_base_data.profile_current_version);

    // 如果資料現在值檔案的版本為空, 表示版本資訊遺失, 還原到預設值.
    if(mcm_config_base_data.profile_current_version[0] == '\0')
    {
        DMSG("lose version, do reset to default");
        goto FREE_01;
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int initial_device_value(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_group,
    struct mcm_config_store_t *this_store,
    char *this_path,
    char *this_member)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    struct mcm_dv_device_t device_v;


    if(mcm_config_get_entry_by_info(this_session, this_group, this_store, MCM_DACCESS_SYS,
                                    &device_v) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_by_info(%s) fail", this_path);
        goto FREE_01;
    }
    if((this_member == NULL) || (strcmp(this_member, "descript") == 0))
    {
       DMSG("[get] %s.descript = " MCM_DTYPE_S_PF, this_path, device_v.descript);
    }
    if((this_member == NULL) || (strcmp(this_member, "serial_number") == 0))
    {
        DMSG("[get] %s.serial_number = "
             MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF
             MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF,
             this_path,
             device_v.serial_number[0], device_v.serial_number[1], device_v.serial_number[2],
             device_v.serial_number[3], device_v.serial_number[4], device_v.serial_number[5],
             device_v.serial_number[6], device_v.serial_number[7], device_v.serial_number[8],
             device_v.serial_number[9]);
    }

    if((this_member == NULL) || (strcmp(this_member, "descript") == 0))
    {
        snprintf(device_v.descript, sizeof(device_v.descript), "Network-Device");
        DMSG("[set] %s.descript = " MCM_DTYPE_S_PF, this_path, device_v.descript);
    }
    if((this_member == NULL) || (strcmp(this_member, "serial_number") == 0))
    {
        device_v.serial_number[0] = 0x01;
        device_v.serial_number[1] = 0x02;
        device_v.serial_number[2] = 0x04;
        device_v.serial_number[3] = 0x05;
        device_v.serial_number[4] = 0xA0;
        device_v.serial_number[5] = 0xB0;
        device_v.serial_number[6] = 0xC0;
        device_v.serial_number[7] = 0xD0;
        device_v.serial_number[8] = 0xE0;
        device_v.serial_number[9] = 0xF0;
        DMSG("[set] %s.serial_number = "
             MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF
             MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF,
             this_path,
             device_v.serial_number[0], device_v.serial_number[1], device_v.serial_number[2],
             device_v.serial_number[3], device_v.serial_number[4], device_v.serial_number[5],
             device_v.serial_number[6], device_v.serial_number[7], device_v.serial_number[8],
             device_v.serial_number[9]);
    }
    if(mcm_config_set_entry_by_info(this_session, this_group, this_store, MCM_DACCESS_SYS,
                                    &device_v) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_set_entry_by_info(%s) fail", this_path);
        goto FREE_01;
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_check_device(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1;
    struct mcm_config_model_group_t *tmp_group;
    struct mcm_config_store_t *tmp_store;
    struct mcm_ds_device_t device_s;


    DMSG("check device test :");

    // 取得 device 資料.
    path1 = "device";
    // 使用進階模式存取, 先使用路徑取得資訊.
    if(mcm_config_find_entry_by_full(this_session, path1, &tmp_group, &tmp_store) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_find_entry_by_full(%s) fail", path1);
        goto FREE_01;
    }

    // 取得狀態.
    if(mcm_config_get_entry_all_status_by_info(this_session, tmp_group, tmp_store, &device_s)
                                               < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_self_status_by_info(%s) fail", path1);
        goto FREE_01;
    }
    device_s.ekey &= MCM_DSERROR_MASK;
    device_s.descript &= MCM_DSERROR_MASK;

    // 檢查錯誤.
    if(device_s.ekey != 0)
    {
        DMSG("%s detect error", path1);

        // 遺失 parent 錯誤.
        if((device_s.ekey & MCM_DSERROR_LOSE_PARENT) != 0)
        {
            DMSG("%s = MCM_DSERROR_LOSE_PARENT", path1);
        }

        // 遺失 entry 錯誤.
        if((device_s.ekey & MCM_DSERROR_LOSE_ENTRY) != 0)
        {
            DMSG("%s = MCM_DSERROR_LOSE_ENTRY", path1);
            // 重設所有 member 的資料數值.
            if(initial_device_value(this_session, tmp_group, tmp_store, path1, NULL)
                                    < MCM_RCODE_PASS)
            {
                DMSG("call initial_device_value fail");
                goto FREE_01;
            }
        }

        // 重複 entry 錯誤.
        if((device_s.ekey & MCM_DSERROR_DUPLIC_ENTRY) != 0)
        {
            DMSG("%s = MCM_DSERROR_DUPLIC_ENTRY", path1);
        }

        // 未知參數錯誤.
        if((device_s.ekey & MCM_DSERROR_UNKNOWN_PARAMETER) != 0)
        {
            DMSG("%s = MCM_DSERROR_UNKNOWN_PARAMETER", path1);
        }

        // 未知 member 錯誤.
        if((device_s.ekey & MCM_DSERROR_UNKNOWN_MEMBER) != 0)
        {
            DMSG("%s = MCM_DSERROR_UNKNOWN_MEMBER", path1);
        }

        // 遺失 member 錯誤.
        if((device_s.ekey & MCM_DSERROR_LOSE_MEMBER) != 0)
        {
            DMSG("%s = MCM_DSERROR_LOSE_MEMBER", path1);
            // 遺失 device.descript 錯誤, 重設資料數值.
            if((device_s.descript & MCM_DSERROR_LOSE_MEMBER) != 0)
            {
                DMSG("%s.descript = MCM_DSERROR_LOSE_MEMBER", path1);
                if((device_s.ekey & MCM_DSERROR_LOSE_ENTRY) == 0)
                    if(initial_device_value(this_session, tmp_group, tmp_store, path1, "descript")
                                            < MCM_RCODE_PASS)
                    {
                        DMSG("call initial_device_value fail");
                        goto FREE_01;
                    }
            }
            // 遺失 device.serial_number 錯誤, 重設資料數值.
            if((device_s.serial_number & MCM_DSERROR_LOSE_MEMBER) != 0)
            {
                DMSG("%s.serial_number = MCM_DSERROR_LOSE_MEMBER", path1);
                if((device_s.ekey & MCM_DSERROR_LOSE_ENTRY) == 0)
                    if(initial_device_value(this_session, tmp_group, tmp_store, path1,
                                            "serial_number") < MCM_RCODE_PASS)
                    {
                        DMSG("call initial_device_value fail");
                        goto FREE_01;
                    }
            }
        }

        // 重複 member 錯誤.
        if((device_s.ekey & MCM_DSERROR_DUPLIC_MEMBER) != 0)
        {
            DMSG("%s = MCM_DSERROR_DUPLIC_MEMBER", path1);
            // 重複 device.descript 錯誤.
            if((device_s.descript & MCM_DSERROR_DUPLIC_MEMBER) != 0)
            {
                DMSG("%s.descript = MCM_DSERROR_DUPLIC_MEMBER", path1);
            }
            // 重複 device.serial_number 錯誤.
            if((device_s.serial_number & MCM_DSERROR_DUPLIC_MEMBER) != 0)
            {
                DMSG("%s.serial_number = MCM_DSERROR_DUPLIC_MEMBER", path1);
            }
        }

        // 數值無效錯誤.
        if((device_s.ekey & MCM_DSERROR_INVALID_VALUE) != 0)
        {
            DMSG("%s = MCM_DSERROR_INVALID_VALUE", path1);
            // device.descript 數值無效錯誤, 重設資料數值.
            if((device_s.descript & MCM_DSERROR_INVALID_VALUE) != 0)
            {
                DMSG("%s.descript = MCM_DSERROR_INVALID_VALUE", path1);
                if(initial_device_value(this_session, tmp_group, tmp_store, path1, "descript")
                                        < MCM_RCODE_PASS)
                {
                    DMSG("call initial_device_value fail");
                    goto FREE_01;
                }
            }
            // device.serial_number 數值無效錯誤, 重設資料數值.
            if((device_s.serial_number & MCM_DSERROR_INVALID_VALUE) != 0)
            {
                DMSG("%s.serial_number = MCM_DSERROR_INVALID_VALUE", path1);
                if(initial_device_value(this_session, tmp_group, tmp_store, path1, "serial_number")
                                        < MCM_RCODE_PASS)
                {
                    DMSG("call initial_device_value fail");
                    goto FREE_01;
                }
            }
        }
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int initial_system_value(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_group,
    struct mcm_config_store_t *this_store,
    char *this_path,
    char *this_member)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    struct mcm_dv_device_system_t system_v;


    if(mcm_config_get_entry_by_info(this_session, this_group, this_store, MCM_DACCESS_SYS,
                                    &system_v) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_by_info(%s) fail", this_path);
        goto FREE_01;
    }
    if((this_member == NULL) || (strcmp(this_member, "date") == 0))
    {
       DMSG("[get] %s.date = " MCM_DTYPE_S_PF, this_path, system_v.date);
    }
    if((this_member == NULL) || (strcmp(this_member, "ip_addr") == 0))
    {
       DMSG("[get] %s.ip_addr = " MCM_DTYPE_S_PF, this_path, system_v.ip_addr);
    }
    if((this_member == NULL) || (strcmp(this_member, "uptime") == 0))
    {
       DMSG("[get] %s.uptime = " MCM_DTYPE_IULL_PF, this_path, system_v.uptime);
    }
    if((this_member == NULL) || (strcmp(this_member, "loading") == 0))
    {
       DMSG("[get] %s.loading = " MCM_DTYPE_FD_PF, this_path, system_v.loading);
    }

    if((this_member == NULL) || (strcmp(this_member, "date") == 0))
    {
        snprintf(system_v.date, sizeof(system_v.date), "2001/01/01");
        DMSG("[set] %s.date = " MCM_DTYPE_S_PF, this_path, system_v.date);
    }
    if((this_member == NULL) || (strcmp(this_member, "ip_addr") == 0))
    {
        snprintf(system_v.ip_addr, sizeof(system_v.ip_addr), "10.0.0.254");
        DMSG("[set] %s.ip_addr = " MCM_DTYPE_S_PF, this_path, system_v.ip_addr);
    }
    if((this_member == NULL) || (strcmp(this_member, "uptime") == 0))
    {
        system_v.uptime = 1;
        DMSG("[set] %s.uptime = " MCM_DTYPE_IULL_PF, this_path, system_v.uptime);
    }
    if((this_member == NULL) || (strcmp(this_member, "loading") == 0))
    {
        system_v.loading = 1.0;
        DMSG("[set] %s.loading = " MCM_DTYPE_FD_PF, this_path, system_v.loading);
    }
    if(mcm_config_set_entry_by_info(this_session, this_group, this_store, MCM_DACCESS_SYS,
                                    &system_v) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_set_entry_by_info(%s) fail", this_path);
        goto FREE_01;
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_check_system(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1;
    struct mcm_config_model_group_t *tmp_group;
    struct mcm_config_store_t *tmp_store;
    struct mcm_ds_device_system_t system_s;


    DMSG("check system test :");

    // 取得 device.system 資料.
    path1 = "device.system";
    // 使用進階模式存取, 先使用路徑取得資訊.
    if(mcm_config_find_entry_by_full(this_session, path1, &tmp_group, &tmp_store) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_find_entry_by_full(%s) fail", path1);
        goto FREE_01;
    }

    // 取得狀態.
    if(mcm_config_get_entry_all_status_by_info(this_session, tmp_group, tmp_store, &system_s)
                                               < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_self_status_by_info(%s) fail", path1);
        goto FREE_01;
    }
    system_s.ekey &= MCM_DSERROR_MASK;
    system_s.date &= MCM_DSERROR_MASK;
    system_s.ip_addr &= MCM_DSERROR_MASK;
    system_s.uptime &= MCM_DSERROR_MASK;
    system_s.loading &= MCM_DSERROR_MASK;

    // 檢查錯誤.
    if(system_s.ekey != 0)
    {
        DMSG("%s detect error", path1);

        // 遺失 parent 錯誤.
        if((system_s.ekey & MCM_DSERROR_LOSE_PARENT) != 0)
        {
            DMSG("%s = MCM_DSERROR_LOSE_PARENT", path1);
        }

        // 遺失 entry 錯誤.
        if((system_s.ekey & MCM_DSERROR_LOSE_ENTRY) != 0)
        {
            DMSG("%s = MCM_DSERROR_LOSE_ENTRY", path1);
            // 重設所有 member 的資料數值.
            if(initial_system_value(this_session, tmp_group, tmp_store, path1, NULL)
                                    < MCM_RCODE_PASS)
            {
                DMSG("call initial_system_value fail");
                goto FREE_01;
            }
        }

        // 重複 entry 錯誤.
        if((system_s.ekey & MCM_DSERROR_DUPLIC_ENTRY) != 0)
        {
            DMSG("%s = MCM_DSERROR_DUPLIC_ENTRY", path1);
        }

        // 未知參數錯誤.
        if((system_s.ekey & MCM_DSERROR_UNKNOWN_PARAMETER) != 0)
        {
            DMSG("%s = MCM_DSERROR_UNKNOWN_PARAMETER", path1);
        }

        // 未知 member 錯誤.
        if((system_s.ekey & MCM_DSERROR_UNKNOWN_MEMBER) != 0)
        {
            DMSG("%s = MCM_DSERROR_UNKNOWN_MEMBER", path1);
        }

        // 遺失 member 錯誤.
        if((system_s.ekey & MCM_DSERROR_LOSE_MEMBER) != 0)
        {
            DMSG("%s = MCM_DSERROR_LOSE_MEMBER", path1);
            // 遺失 system.date 錯誤, 重設資料數值.
            if((system_s.date & MCM_DSERROR_LOSE_MEMBER) != 0)
            {
                DMSG("%s.date = MCM_DSERROR_LOSE_MEMBER", path1);
                if((system_s.ekey & MCM_DSERROR_LOSE_ENTRY) == 0)
                    if(initial_system_value(this_session, tmp_group, tmp_store, path1, "date")
                                            < MCM_RCODE_PASS)
                    {
                        DMSG("call initial_system_value fail");
                        goto FREE_01;
                    }
            }
            // 遺失 system.date 錯誤, 重設資料數值.
            if((system_s.ip_addr & MCM_DSERROR_LOSE_MEMBER) != 0)
            {
                DMSG("%s.ip_addr = MCM_DSERROR_LOSE_MEMBER", path1);
                if((system_s.ekey & MCM_DSERROR_LOSE_ENTRY) == 0)
                    if(initial_system_value(this_session, tmp_group, tmp_store, path1, "ip_addr")
                                            < MCM_RCODE_PASS)
                    {
                        DMSG("call initial_system_value fail");
                        goto FREE_01;
                    }
            }
            // 遺失 system.uptime 錯誤, 重設資料數值.
            if((system_s.uptime & MCM_DSERROR_LOSE_MEMBER) != 0)
            {
                DMSG("%s.uptime = MCM_DSERROR_LOSE_MEMBER", path1);
                if((system_s.ekey & MCM_DSERROR_LOSE_ENTRY) == 0)
                    if(initial_system_value(this_session, tmp_group, tmp_store, path1, "uptime")
                                            < MCM_RCODE_PASS)
                    {
                        DMSG("call initial_system_value fail");
                        goto FREE_01;
                    }
            }
            // 遺失 system.loading 錯誤, 重設資料數值.
            if((system_s.loading & MCM_DSERROR_LOSE_MEMBER) != 0)
            {
                DMSG("%s.loading = MCM_DSERROR_LOSE_MEMBER", path1);
                if((system_s.ekey & MCM_DSERROR_LOSE_ENTRY) == 0)
                    if(initial_system_value(this_session, tmp_group, tmp_store, path1, "loading")
                                            < MCM_RCODE_PASS)
                    {
                        DMSG("call initial_system_value fail");
                        goto FREE_01;
                    }
            }
        }

        // 重複 member 錯誤.
        if((system_s.ekey & MCM_DSERROR_DUPLIC_MEMBER) != 0)
        {
            DMSG("%s = MCM_DSERROR_DUPLIC_MEMBER", path1);
            // 重複 system.date 錯誤.
            if((system_s.date & MCM_DSERROR_DUPLIC_MEMBER) != 0)
            {
                DMSG("%s.date = MCM_DSERROR_DUPLIC_MEMBER", path1);
            }
            // 重複 system.ip_addr 錯誤.
            if((system_s.ip_addr & MCM_DSERROR_DUPLIC_MEMBER) != 0)
            {
                DMSG("%s.ip_addr = MCM_DSERROR_DUPLIC_MEMBER", path1);
            }
            // 重複 system.uptime 錯誤.
            if((system_s.uptime & MCM_DSERROR_DUPLIC_MEMBER) != 0)
            {
                DMSG("%s.uptime = MCM_DSERROR_DUPLIC_MEMBER", path1);
            }
            // 重複 system.loading 錯誤.
            if((system_s.loading & MCM_DSERROR_DUPLIC_MEMBER) != 0)
            {
                DMSG("%s.loading = MCM_DSERROR_DUPLIC_MEMBER", path1);
            }
        }

        // 數值無效錯誤.
        if((system_s.ekey & MCM_DSERROR_INVALID_VALUE) != 0)
        {
            DMSG("%s = MCM_DSERROR_INVALID_VALUE", path1);
            // system.date 數值無效錯誤, 重設資料數值.
            if((system_s.date & MCM_DSERROR_INVALID_VALUE) != 0)
            {
                DMSG("%s.date = MCM_DSERROR_INVALID_VALUE", path1);
                if(initial_system_value(this_session, tmp_group, tmp_store, path1, "date")
                                        < MCM_RCODE_PASS)
                {
                    DMSG("call initial_system_value fail");
                    goto FREE_01;
                }
            }
            // system.ip_addr 數值無效錯誤, 重設資料數值.
            if((system_s.ip_addr & MCM_DSERROR_INVALID_VALUE) != 0)
            {
                DMSG("%s.ip_addr = MCM_DSERROR_INVALID_VALUE", path1);
                if(initial_system_value(this_session, tmp_group, tmp_store, path1, "ip_addr")
                                        < MCM_RCODE_PASS)
                {
                    DMSG("call initial_system_value fail");
                    goto FREE_01;
                }
            }
            // system.uptime 數值無效錯誤, 重設資料數值.
            if((system_s.uptime & MCM_DSERROR_INVALID_VALUE) != 0)
            {
                DMSG("%s.uptime = MCM_DSERROR_INVALID_VALUE", path1);
                if(initial_system_value(this_session, tmp_group, tmp_store, path1, "uptime")
                                        < MCM_RCODE_PASS)
                {
                    DMSG("call initial_system_value fail");
                    goto FREE_01;
                }
            }
            // system.loading 數值無效錯誤, 重設資料數值.
            if((system_s.loading & MCM_DSERROR_INVALID_VALUE) != 0)
            {
                DMSG("%s.loading = MCM_DSERROR_INVALID_VALUE", path1);
                if(initial_system_value(this_session, tmp_group, tmp_store, path1, "loading")
                                        < MCM_RCODE_PASS)
                {
                    DMSG("call initial_system_value fail");
                    goto FREE_01;
                }
            }
        }
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int initial_vap_value(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_group,
    struct mcm_config_store_t *this_store,
    char *this_path,
    char *this_member)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    struct mcm_dv_device_vap_t vap_v;


    if(mcm_config_get_entry_by_info(this_session, this_group, this_store, MCM_DACCESS_SYS, &vap_v)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_by_info(%s) fail", this_path);
        goto FREE_01;
    }
    DMSG("[get] %s.ekey = " MCM_DTYPE_EK_PF, this_path, vap_v.ekey);
    if((this_member == NULL) || (strcmp(this_member, "ssid") == 0))
    {
       DMSG("[get] %s.ssid = " MCM_DTYPE_S_PF, this_path, vap_v.ssid);
    }
    if((this_member == NULL) || (strcmp(this_member, "channel") == 0))
    {
       DMSG("[get] %s.channel = " MCM_DTYPE_IUI_PF, this_path, vap_v.channel);
    }

    if((this_member == NULL) || (strcmp(this_member, "ssid") == 0))
    {
        snprintf(vap_v.ssid, sizeof(vap_v.ssid), "public-%u", vap_v.ekey);
        DMSG("[set] %s.ssid = " MCM_DTYPE_S_PF, this_path, vap_v.ssid);
    }
    if((this_member == NULL) || (strcmp(this_member, "channel") == 0))
    {
        vap_v.channel = 1;
        DMSG("[set] %s.channel = " MCM_DTYPE_IUI_PF, this_path, vap_v.channel);
    }
    if(mcm_config_set_entry_by_info(this_session, this_group, this_store, MCM_DACCESS_SYS, &vap_v)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_set_entry_by_info(%s) fail", this_path);
        goto FREE_01;
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_check_vap(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_config_model_group_t *tmp_group;
    struct mcm_config_store_t *tmp_store;
    struct mcm_ds_device_vap_t vap_s;
    MCM_DTYPE_EK_TD i;


    DMSG("check vap test :");

    // 取得 device.vap.* 資料.
    path1 = "device.vap.*";
    // 使用進階模式存取, 先使用路徑取得資訊.
    if(mcm_config_find_entry_by_mix(this_session, path1, &tmp_group, &tmp_store, NULL, NULL, NULL,
                                    NULL) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_find_entry_by_mix(%s) fail", path1);
        goto FREE_01;
    }

    // 處理 device.vap.*
    for(i = 0; tmp_store != NULL; tmp_store = tmp_store->next_store, i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.@%u", i + 1);

        // 取得狀態.
        if(mcm_config_get_entry_all_status_by_info(this_session, tmp_group, tmp_store, &vap_s)
                                                   < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_get_entry_self_status_by_info(%s) fail", path2);
            goto FREE_01;
        }
        vap_s.ekey &= MCM_DSERROR_MASK;
        vap_s.ssid &= MCM_DSERROR_MASK;
        vap_s.channel &= MCM_DSERROR_MASK;

        // 檢查錯誤.
        if(vap_s.ekey != 0)
        {
            DMSG("%s detect error", path2);

            // 遺失 parent 錯誤.
            if((vap_s.ekey & MCM_DSERROR_LOSE_PARENT) != 0)
            {
                DMSG("%s = MCM_DSERROR_LOSE_PARENT", path2);
            }

            // 遺失 entry 錯誤.
            if((vap_s.ekey & MCM_DSERROR_LOSE_ENTRY) != 0)
            {
                DMSG("%s = MCM_DSERROR_LOSE_ENTRY", path2);
                // 重設所有 member 的資料數值.
                if(initial_vap_value(this_session, tmp_group, tmp_store, path2, NULL)
                                     < MCM_RCODE_PASS)
                {
                    DMSG("call initial_vap_value fail");
                    goto FREE_01;
                }
            }

            // 重複 entry 錯誤.
            if((vap_s.ekey & MCM_DSERROR_DUPLIC_ENTRY) != 0)
            {
                DMSG("%s = MCM_DSERROR_DUPLIC_ENTRY", path2);
            }

            // 未知參數錯誤.
            if((vap_s.ekey & MCM_DSERROR_UNKNOWN_PARAMETER) != 0)
            {
                DMSG("%s = MCM_DSERROR_UNKNOWN_PARAMETER", path2);
            }

            // 未知 member 錯誤.
            if((vap_s.ekey & MCM_DSERROR_UNKNOWN_MEMBER) != 0)
            {
                DMSG("%s = MCM_DSERROR_UNKNOWN_MEMBER", path2);
            }

            // 遺失 member 錯誤.
            if((vap_s.ekey & MCM_DSERROR_LOSE_MEMBER) != 0)
            {
                DMSG("%s = MCM_DSERROR_LOSE_MEMBER", path2);
                // 遺失 vap.ssid 錯誤, 重設資料數值.
                if((vap_s.ssid & MCM_DSERROR_LOSE_MEMBER) != 0)
                {
                    DMSG("%s.ssid = MCM_DSERROR_LOSE_MEMBER", path2);
                    if((vap_s.ekey & MCM_DSERROR_LOSE_ENTRY) == 0)
                        if(initial_vap_value(this_session, tmp_group, tmp_store, path2, "ssid")
                                             < MCM_RCODE_PASS)
                        {
                            DMSG("call initial_vap_value fail");
                            goto FREE_01;
                        }
                }
                // 遺失 vap.channel 錯誤, 重設資料數值.
                if((vap_s.channel & MCM_DSERROR_LOSE_MEMBER) != 0)
                {
                    DMSG("%s.channel = MCM_DSERROR_LOSE_MEMBER", path2);
                    if((vap_s.ekey & MCM_DSERROR_LOSE_ENTRY) == 0)
                        if(initial_vap_value(this_session, tmp_group, tmp_store, path2, "channel")
                                             < MCM_RCODE_PASS)
                        {
                            DMSG("call initial_vap_value fail");
                            goto FREE_01;
                        }
                }
            }

            // 重複 member 錯誤.
            if((vap_s.ekey & MCM_DSERROR_DUPLIC_MEMBER) != 0)
            {
                DMSG("%s = MCM_DSERROR_DUPLIC_MEMBER", path2);
                // 重複 vap.ssid 錯誤.
                if((vap_s.ssid & MCM_DSERROR_DUPLIC_MEMBER) != 0)
                {
                    DMSG("%s.ssid = MCM_DSERROR_DUPLIC_MEMBER", path2);
                }
                // 重複 vap.channel 錯誤.
                if((vap_s.channel & MCM_DSERROR_DUPLIC_MEMBER) != 0)
                {
                    DMSG("%s.channel = MCM_DSERROR_DUPLIC_MEMBER", path2);
                }
            }

            // 數值無效錯誤.
            if((vap_s.ekey & MCM_DSERROR_INVALID_VALUE) != 0)
            {
                DMSG("%s = MCM_DSERROR_INVALID_VALUE", path2);
                // vap.ssid 數值無效錯誤, 重設資料數值.
                if((vap_s.ssid & MCM_DSERROR_INVALID_VALUE) != 0)
                {
                    DMSG("%s.ssid = MCM_DSERROR_INVALID_VALUE", path2);
                    if(initial_vap_value(this_session, tmp_group, tmp_store, path2, "ssid")
                                         < MCM_RCODE_PASS)
                    {
                        DMSG("call initial_vap_value fail");
                        goto FREE_01;
                    }
                }
                // vap.channel 數值無效錯誤, 重設資料數值.
                if((vap_s.channel & MCM_DSERROR_INVALID_VALUE) != 0)
                {
                    DMSG("%s.channel = MCM_DSERROR_INVALID_VALUE", path2);
                    if(initial_vap_value(this_session, tmp_group, tmp_store, path2, "channel")
                                         < MCM_RCODE_PASS)
                    {
                        DMSG("call initial_vap_value fail");
                        goto FREE_01;
                    }
                }
            }
        }
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int initial_extra_value(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_group,
    struct mcm_config_store_t *this_store,
    char *this_path,
    char *this_member)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    struct mcm_dv_device_vap_extra_t extra_v;


    if(mcm_config_get_entry_by_info(this_session, this_group, this_store, MCM_DACCESS_SYS,
                                    &extra_v) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_by_info(%s) fail", this_path);
        goto FREE_01;
    }
    if((this_member == NULL) || (strcmp(this_member, "hidden") == 0))
    {
       DMSG("[get] %s.hidden = " MCM_DTYPE_ISC_PF, this_path, extra_v.hidden);
    }
    if((this_member == NULL) || (strcmp(this_member, "tx_power") == 0))
    {
       DMSG("[get] %s.tx_power = " MCM_DTYPE_ISS_PF, this_path, extra_v.tx_power);
    }

    if((this_member == NULL) || (strcmp(this_member, "hidden") == 0))
    {
        extra_v.hidden = 0;
        DMSG("[set] %s.hidden = " MCM_DTYPE_ISC_PF, this_path, extra_v.hidden);
    }
    if((this_member == NULL) || (strcmp(this_member, "tx_power") == 0))
    {
        extra_v.tx_power = 5;
        DMSG("[set] %s.tx_power = " MCM_DTYPE_ISS_PF, this_path, extra_v.tx_power);
    }
    if(mcm_config_set_entry_by_info(this_session, this_group, this_store, MCM_DACCESS_SYS,
                                    &extra_v) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_set_entry_by_info(%s) fail", this_path);
        goto FREE_01;
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_check_extra(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_config_model_group_t *tmp_group;
    struct mcm_config_store_t *tmp_store;
    struct mcm_ds_device_vap_extra_t extra_s;
    MCM_DTYPE_EK_TD count1, i;


    DMSG("check extra test :");

    // 取得 device.vap.* 的筆數.
    path1 = "device.vap.*";
    if(mcm_config_get_count_by_path(this_session, path1, &count1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_count_by_path(%s) fail", path1);
        goto FREE_01;
    }

    // 處理 device.vap.{i}.extra
    for(i = 0; i < count1; i++)
    {
        // 取得 device.vap.{i}.extra 資料.
        snprintf(path2, sizeof(path2), "device.vap.@%u.extra", i + 1);
        // 使用進階模式存取, 先使用路徑取得資訊.
        if(mcm_config_find_entry_by_full(this_session, path2, &tmp_group, &tmp_store)
                                         < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_find_entry_by_full(%s) fail", path2);
            goto FREE_01;
        }

        // 取得狀態.
        if(mcm_config_get_entry_all_status_by_info(this_session, tmp_group, tmp_store, &extra_s)
                                                   < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_get_entry_self_status_by_info(%s) fail", path2);
            goto FREE_01;
        }
        extra_s.ekey &= MCM_DSERROR_MASK;
        extra_s.hidden &= MCM_DSERROR_MASK;
        extra_s.tx_power &= MCM_DSERROR_MASK;

        // 檢查錯誤.
        if(extra_s.ekey != 0)
        {
            DMSG("%s detect error", path2);

            // 遺失 parent 錯誤.
            if((extra_s.ekey & MCM_DSERROR_LOSE_PARENT) != 0)
            {
                DMSG("%s = MCM_DSERROR_LOSE_PARENT", path2);
            }

            // 遺失 entry 錯誤.
            if((extra_s.ekey & MCM_DSERROR_LOSE_ENTRY) != 0)
            {
                DMSG("%s = MCM_DSERROR_LOSE_ENTRY", path2);
                // 重設所有 member 的資料數值.
                if(initial_extra_value(this_session, tmp_group, tmp_store, path2, NULL)
                                       < MCM_RCODE_PASS)
                {
                    DMSG("call initial_extra_value fail");
                    goto FREE_01;
                }
            }

            // 重複 entry 錯誤.
            if((extra_s.ekey & MCM_DSERROR_DUPLIC_ENTRY) != 0)
            {
                DMSG("%s = MCM_DSERROR_DUPLIC_ENTRY", path2);
            }

            // 未知參數錯誤.
            if((extra_s.ekey & MCM_DSERROR_UNKNOWN_PARAMETER) != 0)
            {
                DMSG("%s = MCM_DSERROR_UNKNOWN_PARAMETER", path2);
            }

            // 未知 member 錯誤.
            if((extra_s.ekey & MCM_DSERROR_UNKNOWN_MEMBER) != 0)
            {
                DMSG("%s = MCM_DSERROR_UNKNOWN_MEMBER", path2);
            }

            // 遺失 member 錯誤.
            if((extra_s.ekey & MCM_DSERROR_LOSE_MEMBER) != 0)
            {
                DMSG("%s = MCM_DSERROR_LOSE_MEMBER", path2);
                // 遺失 extra.hidden 錯誤, 重設資料數值.
                if((extra_s.hidden & MCM_DSERROR_LOSE_MEMBER) != 0)
                {
                    DMSG("%s.hidden = MCM_DSERROR_LOSE_MEMBER", path2);
                    if((extra_s.ekey & MCM_DSERROR_LOSE_ENTRY) == 0)
                        if(initial_extra_value(this_session, tmp_group, tmp_store, path2,
                                               "hidden") < MCM_RCODE_PASS)
                        {
                            DMSG("call initial_extra_value fail");
                            goto FREE_01;
                        }
                }
                // 遺失 extra.tx_power 錯誤, 重設資料數值.
                if((extra_s.tx_power & MCM_DSERROR_LOSE_MEMBER) != 0)
                {
                    DMSG("%s.tx_power = MCM_DSERROR_LOSE_MEMBER", path2);
                    if((extra_s.ekey & MCM_DSERROR_LOSE_ENTRY) == 0)
                        if(initial_extra_value(this_session, tmp_group, tmp_store, path2,
                                               "tx_power") < MCM_RCODE_PASS)
                        {
                            DMSG("call initial_extra_value fail");
                            goto FREE_01;
                        }
                }
            }

            // 重複 member 錯誤.
            if((extra_s.ekey & MCM_DSERROR_DUPLIC_MEMBER) != 0)
            {
                DMSG("%s = MCM_DSERROR_DUPLIC_MEMBER", path2);
                // 重複 extra.hidden 錯誤.
                if((extra_s.hidden & MCM_DSERROR_DUPLIC_MEMBER) != 0)
                {
                    DMSG("%s.hidden = MCM_DSERROR_DUPLIC_MEMBER", path2);
                }
                // 重複 extra.tx_power 錯誤.
                if((extra_s.tx_power & MCM_DSERROR_DUPLIC_MEMBER) != 0)
                {
                    DMSG("%s.tx_power = MCM_DSERROR_DUPLIC_MEMBER", path2);
                }
            }

            // 數值無效錯誤.
            if((extra_s.ekey & MCM_DSERROR_INVALID_VALUE) != 0)
            {
                DMSG("%s = MCM_DSERROR_INVALID_VALUE", path2);
                // extra.hidden 數值無效錯誤, 重設資料數值.
                if((extra_s.hidden & MCM_DSERROR_INVALID_VALUE) != 0)
                {
                    DMSG("%s.hidden = MCM_DSERROR_INVALID_VALUE", path2);
                    if(initial_extra_value(this_session, tmp_group, tmp_store, path2, "hidden")
                                           < MCM_RCODE_PASS)
                    {
                        DMSG("call initial_extra_value fail");
                        goto FREE_01;
                    }
                }
                // extra.tx_power 數值無效錯誤, 重設資料數值.
                if((extra_s.tx_power & MCM_DSERROR_INVALID_VALUE) != 0)
                {
                    DMSG("%s.tx_power = MCM_DSERROR_INVALID_VALUE", path2);
                    if(initial_extra_value(this_session, tmp_group, tmp_store, path2, "tx_power")
                                           < MCM_RCODE_PASS)
                    {
                        DMSG("call initial_extra_value fail");
                        goto FREE_01;
                    }
                }
            }
        }
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int initial_station_value(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_group,
    struct mcm_config_store_t *this_store,
    char *this_path,
    char *this_member)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    struct mcm_dv_device_vap_station_t station_v;


    if(mcm_config_get_entry_by_info(this_session, this_group, this_store, MCM_DACCESS_SYS,
                                    &station_v) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_by_info(%s) fail", this_path);
        goto FREE_01;
    }
    DMSG("[get] %s.ekey = " MCM_DTYPE_EK_PF, this_path, station_v.ekey);
    if((this_member == NULL) || (strcmp(this_member, "mac_addr") == 0))
    {
       DMSG("[get] %s.mac_addr = " MCM_DTYPE_S_PF, this_path, station_v.mac_addr);
    }
    if((this_member == NULL) || (strcmp(this_member, "rule") == 0))
    {
       DMSG("[get] %s.rule = " MCM_DTYPE_RK_PF, this_path, station_v.rule);
    }

    if((this_member == NULL) || (strcmp(this_member, "mac_addr") == 0))
    {
        snprintf(station_v.mac_addr, sizeof(station_v.mac_addr), "FF:FF:FF:FF:FF:FF");
        DMSG("[set] %s.mac_addr = " MCM_DTYPE_S_PF, this_path, station_v.mac_addr);
    }
    if((this_member == NULL) || (strcmp(this_member, "rule") == 0))
    {
        station_v.rule = 1;
        DMSG("[set] %s.rule = " MCM_DTYPE_RK_PF, this_path, station_v.rule);
    }
    if(mcm_config_set_entry_by_info(this_session, this_group, this_store, MCM_DACCESS_SYS,
                                    &station_v) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_set_entry_by_info(%s) fail", this_path);
        goto FREE_01;
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_check_station(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_config_model_group_t *tmp_group;
    struct mcm_config_store_t *tmp_store;
    struct mcm_ds_device_vap_station_t station_s;
    MCM_DTYPE_EK_TD count1, i, j;


    DMSG("check station test :");

    // 取得 device.vap.* 的筆數.
    path1 = "device.vap.*";
    if(mcm_config_get_count_by_path(this_session, path1, &count1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_count_by_path(%s) fail", path1);
        goto FREE_01;
    }

    // 處理 device.vap.{i}.station.*
    for(i = 0; i < count1; i++)
    {
        // 取得 device.vap.* 資料.
        snprintf(path2, sizeof(path2), "device.vap.@%u.station.*", i + 1);
        // 使用進階模式存取, 先使用路徑取得資訊.
        if(mcm_config_find_entry_by_mix(this_session, path2, &tmp_group, &tmp_store, NULL, NULL,
                                        NULL, NULL) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_find_entry_by_mix(%s) fail", path1);
            goto FREE_01;
        }

        // 處理 device.vap.{i}.station.{j}
        for(j = 0; tmp_store != NULL; tmp_store = tmp_store->next_store, j++)
        {
            snprintf(path2, sizeof(path2), "device.vap.@%u.station.@%u", i + 1, j + 1);

            // 取得狀態.
            if(mcm_config_get_entry_all_status_by_info(this_session, tmp_group, tmp_store,
                                                       &station_s) < MCM_RCODE_PASS)
            {
                DMSG("call mcm_config_get_entry_self_status_by_info(%s) fail", path2);
                goto FREE_01;
            }
            station_s.ekey &= MCM_DSERROR_MASK;
            station_s.mac_addr &= MCM_DSERROR_MASK;
            station_s.rule &= MCM_DSERROR_MASK;

            // 檢查錯誤.
            if(station_s.ekey != 0)
            {
                DMSG("%s detect error", path2);

                // 遺失 parent 錯誤.
                if((station_s.ekey & MCM_DSERROR_LOSE_PARENT) != 0)
                {
                    DMSG("%s = MCM_DSERROR_LOSE_PARENT", path2);
                }

                // 遺失 entry 錯誤.
                if((station_s.ekey & MCM_DSERROR_LOSE_ENTRY) != 0)
                {
                    DMSG("%s = MCM_DSERROR_LOSE_ENTRY", path2);
                    // 重設所有 member 的資料數值.
                    if(initial_station_value(this_session, tmp_group, tmp_store, path2, NULL)
                                             < MCM_RCODE_PASS)
                    {
                        DMSG("call initial_station_value fail");
                        goto FREE_01;
                    }
                }

                // 重複 entry 錯誤.
                if((station_s.ekey & MCM_DSERROR_DUPLIC_ENTRY) != 0)
                {
                    DMSG("%s = MCM_DSERROR_DUPLIC_ENTRY", path2);
                }

                // 未知參數錯誤.
                if((station_s.ekey & MCM_DSERROR_UNKNOWN_PARAMETER) != 0)
                {
                    DMSG("%s = MCM_DSERROR_UNKNOWN_PARAMETER", path2);
                }

                // 未知 member 錯誤.
                if((station_s.ekey & MCM_DSERROR_UNKNOWN_MEMBER) != 0)
                {
                    DMSG("%s = MCM_DSERROR_UNKNOWN_MEMBER", path2);
                }

                // 遺失 member 錯誤.
                if((station_s.ekey & MCM_DSERROR_LOSE_MEMBER) != 0)
                {
                    DMSG("%s = MCM_DSERROR_LOSE_MEMBER", path2);
                    // 遺失 station.mac_addr 錯誤, 重設資料數值.
                    if((station_s.mac_addr & MCM_DSERROR_LOSE_MEMBER) != 0)
                    {
                        DMSG("%s.mac_addr = MCM_DSERROR_LOSE_MEMBER", path2);
                        if((station_s.ekey & MCM_DSERROR_LOSE_ENTRY) == 0)
                            if(initial_station_value(this_session, tmp_group, tmp_store, path2,
                                                     "mac_addr") < MCM_RCODE_PASS)
                            {
                                DMSG("call initial_station_value fail");
                                goto FREE_01;
                            }
                    }
                    // 遺失 station.rule 錯誤, 重設資料數值.
                    if((station_s.rule & MCM_DSERROR_LOSE_MEMBER) != 0)
                    {
                        DMSG("%s.rule = MCM_DSERROR_LOSE_MEMBER", path2);
                        if((station_s.ekey & MCM_DSERROR_LOSE_ENTRY) == 0)
                            if(initial_station_value(this_session, tmp_group, tmp_store, path2,
                                                     "rule") < MCM_RCODE_PASS)
                            {
                                DMSG("call initial_station_value fail");
                                goto FREE_01;
                            }
                    }
                }

                // 重複 member 錯誤.
                if((station_s.ekey & MCM_DSERROR_DUPLIC_MEMBER) != 0)
                {
                    DMSG("%s = MCM_DSERROR_DUPLIC_MEMBER", path2);
                    // 重複 station.mac_addr 錯誤.
                    if((station_s.mac_addr & MCM_DSERROR_DUPLIC_MEMBER) != 0)
                    {
                        DMSG("%s.mac_addr = MCM_DSERROR_DUPLIC_MEMBER", path2);
                    }
                    // 重複 station.rule 錯誤.
                    if((station_s.rule & MCM_DSERROR_DUPLIC_MEMBER) != 0)
                    {
                        DMSG("%s.rule = MCM_DSERROR_DUPLIC_MEMBER", path2);
                    }
                }

                // 數值無效錯誤.
                if((station_s.ekey & MCM_DSERROR_INVALID_VALUE) != 0)
                {
                    DMSG("%s = MCM_DSERROR_INVALID_VALUE", path2);
                    // station.mac_addr 數值無效錯誤, 重設資料數值.
                    if((station_s.mac_addr & MCM_DSERROR_INVALID_VALUE) != 0)
                    {
                        DMSG("%s.mac_addr = MCM_DSERROR_INVALID_VALUE", path2);
                        if(initial_station_value(this_session, tmp_group, tmp_store, path2,
                                                 "mac_addr") < MCM_RCODE_PASS)
                        {
                            DMSG("call initial_station_value fail");
                            goto FREE_01;
                        }
                    }
                    // station.rule 數值無效錯誤, 重設資料數值.
                    if((station_s.rule & MCM_DSERROR_INVALID_VALUE) != 0)
                    {
                        DMSG("%s.rule = MCM_DSERROR_INVALID_VALUE", path2);
                        if(initial_station_value(this_session, tmp_group, tmp_store, path2,
                                                 "rule") < MCM_RCODE_PASS)
                        {
                            DMSG("call initial_station_value fail");
                            goto FREE_01;
                        }
                    }
                }
            }
        }
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int initial_limit_value(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_group,
    struct mcm_config_store_t *this_store,
    char *this_path,
    char *this_member)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    struct mcm_dv_device_limit_t limit_v;


    if(mcm_config_get_entry_by_info(this_session, this_group, this_store, MCM_DACCESS_SYS,
                                    &limit_v) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_by_info(%s) fail", this_path);
        goto FREE_01;
    }
    DMSG("[get] %s.ekey = " MCM_DTYPE_EK_PF, this_path, limit_v.ekey);
    if((this_member == NULL) || (strcmp(this_member, "name") == 0))
    {
       DMSG("[get] %s.name = " MCM_DTYPE_S_PF, this_path, limit_v.name);
    }
    if((this_member == NULL) || (strcmp(this_member, "priority") == 0))
    {
       DMSG("[get] %s.priority = " MCM_DTYPE_ISI_PF, this_path, limit_v.priority);
    }

    if((this_member == NULL) || (strcmp(this_member, "name") == 0))
    {
        snprintf(limit_v.name, sizeof(limit_v.name), "default-%u", limit_v.ekey);
        DMSG("[set] %s.name = " MCM_DTYPE_S_PF, this_path, limit_v.name);
    }
    if((this_member == NULL) || (strcmp(this_member, "priority") == 0))
    {
        limit_v.priority = -1;
        DMSG("[set] %s.priority = " MCM_DTYPE_ISI_PF, this_path, limit_v.priority);
    }
    if(mcm_config_set_entry_by_info(this_session, this_group, this_store, MCM_DACCESS_SYS,
                                    &limit_v) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_set_entry_by_info(%s) fail", this_path);
        goto FREE_01;
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_check_limit(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_config_model_group_t *tmp_group;
    struct mcm_config_store_t *tmp_store;
    struct mcm_ds_device_limit_t limit_s;
    MCM_DTYPE_EK_TD i;


    DMSG("check limit test :");

    // 取得 device.limit.* 資料.
    path1 = "device.limit.*";
    // 使用進階模式存取, 先使用路徑取得資訊.
    if(mcm_config_find_entry_by_mix(this_session, path1, &tmp_group, &tmp_store, NULL, NULL, NULL,
                                    NULL) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_find_entry_by_mix(%s) fail", path1);
        goto FREE_01;
    }

    // 處理 device.limit.*
    for(i = 0; tmp_store != NULL; tmp_store = tmp_store->next_store, i++)
    {
        snprintf(path2, sizeof(path2), "device.limit.@%u", i + 1);

        // 取得狀態.
        if(mcm_config_get_entry_all_status_by_info(this_session, tmp_group, tmp_store, &limit_s)
                                                   < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_get_entry_self_status_by_info(%s) fail", path2);
            goto FREE_01;
        }
        limit_s.ekey &= MCM_DSERROR_MASK;
        limit_s.name &= MCM_DSERROR_MASK;
        limit_s.priority &= MCM_DSERROR_MASK;

        // 檢查錯誤.
        if(limit_s.ekey != 0)
        {
            DMSG("%s detect error", path2);

            // 遺失 parent 錯誤.
            if((limit_s.ekey & MCM_DSERROR_LOSE_PARENT) != 0)
            {
                DMSG("%s = MCM_DSERROR_LOSE_PARENT", path2);
            }

            // 遺失 entry 錯誤.
            if((limit_s.ekey & MCM_DSERROR_LOSE_ENTRY) != 0)
            {
                DMSG("%s = MCM_DSERROR_LOSE_ENTRY", path2);
                // 重設所有 member 的資料數值.
                if(initial_limit_value(this_session, tmp_group, tmp_store, path2, NULL)
                                       < MCM_RCODE_PASS)
                {
                    DMSG("call initial_limit_value fail");
                    goto FREE_01;
                }
            }

            // 重複 entry 錯誤.
            if((limit_s.ekey & MCM_DSERROR_DUPLIC_ENTRY) != 0)
            {
                DMSG("%s = MCM_DSERROR_DUPLIC_ENTRY", path2);
            }

            // 未知參數錯誤.
            if((limit_s.ekey & MCM_DSERROR_UNKNOWN_PARAMETER) != 0)
            {
                DMSG("%s = MCM_DSERROR_UNKNOWN_PARAMETER", path2);
            }

            // 未知 member 錯誤.
            if((limit_s.ekey & MCM_DSERROR_UNKNOWN_MEMBER) != 0)
            {
                DMSG("%s = MCM_DSERROR_UNKNOWN_MEMBER", path2);
            }

            // 遺失 member 錯誤.
            if((limit_s.ekey & MCM_DSERROR_LOSE_MEMBER) != 0)
            {
                DMSG("%s = MCM_DSERROR_LOSE_MEMBER", path2);
                // 遺失 limit.name 錯誤, 重設資料數值.
                if((limit_s.name & MCM_DSERROR_LOSE_MEMBER) != 0)
                {
                    DMSG("%s.name = MCM_DSERROR_LOSE_MEMBER", path2);
                    if((limit_s.ekey & MCM_DSERROR_LOSE_ENTRY) == 0)
                        if(initial_limit_value(this_session, tmp_group, tmp_store, path2, "name")
                                               < MCM_RCODE_PASS)
                        {
                            DMSG("call initial_limit_value fail");
                            goto FREE_01;
                        }
                }
                // 遺失 limit.priority 錯誤, 重設資料數值.
                if((limit_s.priority & MCM_DSERROR_LOSE_MEMBER) != 0)
                {
                    DMSG("%s.priority = MCM_DSERROR_LOSE_MEMBER", path2);
                    if((limit_s.ekey & MCM_DSERROR_LOSE_ENTRY) == 0)
                        if(initial_limit_value(this_session, tmp_group, tmp_store, path2,
                                               "priority") < MCM_RCODE_PASS)
                        {
                            DMSG("call initial_limit_value fail");
                            goto FREE_01;
                        }
                }
            }

            // 重複 member 錯誤.
            if((limit_s.ekey & MCM_DSERROR_DUPLIC_MEMBER) != 0)
            {
                DMSG("%s = MCM_DSERROR_DUPLIC_MEMBER", path2);
                // 重複 limit.name 錯誤.
                if((limit_s.name & MCM_DSERROR_DUPLIC_MEMBER) != 0)
                {
                    DMSG("%s.name = MCM_DSERROR_DUPLIC_MEMBER", path2);
                }
                // 重複 limit.priority 錯誤.
                if((limit_s.priority & MCM_DSERROR_DUPLIC_MEMBER) != 0)
                {
                    DMSG("%s.priority = MCM_DSERROR_DUPLIC_MEMBER", path2);
                }
            }

            // 數值無效錯誤.
            if((limit_s.ekey & MCM_DSERROR_INVALID_VALUE) != 0)
            {
                DMSG("%s = MCM_DSERROR_INVALID_VALUE", path2);
                // limit.name 數值無效錯誤, 重設資料數值.
                if((limit_s.name & MCM_DSERROR_INVALID_VALUE) != 0)
                {
                    DMSG("%s.name = MCM_DSERROR_INVALID_VALUE", path2);
                    if(initial_limit_value(this_session, tmp_group, tmp_store, path2, "name")
                                           < MCM_RCODE_PASS)
                    {
                        DMSG("call initial_limit_value fail");
                        goto FREE_01;
                    }
                }
                // limit.priority 數值無效錯誤, 重設資料數值.
                if((limit_s.priority & MCM_DSERROR_INVALID_VALUE) != 0)
                {
                    DMSG("%s.priority = MCM_DSERROR_INVALID_VALUE", path2);
                    if(initial_limit_value(this_session, tmp_group, tmp_store, path2, "priority")
                                           < MCM_RCODE_PASS)
                    {
                        DMSG("call initial_limit_value fail");
                        goto FREE_01;
                    }
                }
            }
        }
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

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


int mcm_module_boot_config_device(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1;
    struct mcm_dv_device_t device_v;


    DMSG("boot config device test :");

    srand(time(NULL));

    if(this_session->call_from == MCM_CFROM_BOOT)
    {
        DMSG("[boot] device");
    }

    path1 = "device";
    if(mcm_config_get_entry_by_path(this_session, path1, MCM_DACCESS_SYS, &device_v)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_by_path(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-entry] %s.descript = " MCM_DTYPE_S_PF, path1, device_v.descript);
    DMSG("[get-entry] %s.serial_number = "
         MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF
         MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF MCM_DTYPE_B_PF,
         path1,
         device_v.serial_number[0], device_v.serial_number[1], device_v.serial_number[2],
         device_v.serial_number[3], device_v.serial_number[4], device_v.serial_number[5],
         device_v.serial_number[6], device_v.serial_number[7], device_v.serial_number[8],
         device_v.serial_number[9]);

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_boot_config_system(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1;
    struct mcm_dv_device_system_t system_v;


    DMSG("boot config system test :");

    srand(time(NULL));

    if(this_session->call_from == MCM_CFROM_BOOT)
    {
        DMSG("[boot] system");
    }

    path1 = "device.system";
    if(mcm_config_get_entry_by_path(this_session, path1, MCM_DACCESS_AUTO, &system_v)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_by_path(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[get-entry] %s.date = " MCM_DTYPE_S_PF, path1, system_v.date);
    DMSG("[get-entry] %s.ip_addr = " MCM_DTYPE_S_PF, path1, system_v.ip_addr);
    DMSG("[get-entry] %s.uptime = " MCM_DTYPE_IULL_PF, path1, system_v.uptime);
    DMSG("[get-entry] %s.loading = " MCM_DTYPE_FD_PF, path1, system_v.loading);

    snprintf(system_v.date, sizeof(system_v.date), "2005/11/%02u", rand() % 31);
    snprintf(system_v.ip_addr, sizeof(system_v.ip_addr), "10.0.0.%03u", rand() % 255);
    system_v.uptime = rand();
    system_v.loading = 56000.0 / (rand() % 3000);
    DMSG("[set-entry] %s.date = " MCM_DTYPE_S_PF, path1, system_v.date);
    DMSG("[set-entry] %s.ip_addr = " MCM_DTYPE_S_PF, path1, system_v.ip_addr);
    DMSG("[set-entry] %s.uptime = " MCM_DTYPE_IULL_PF, path1, system_v.uptime);
    DMSG("[set-entry] %s.loading = " MCM_DTYPE_FD_PF, path1, system_v.loading);
    if(mcm_config_set_entry_by_path(this_session, path1, MCM_DACCESS_AUTO, &system_v)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_set_entry_by_path(%s) fail", path1);
        goto FREE_01;
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_boot_config_vap(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_config_model_group_t *tmp_group;
    struct mcm_config_store_t *tmp_store;
    MCM_DTYPE_EK_TD i;
    struct mcm_dv_device_vap_t vap_v;


    DMSG("boot config vap test :");

    srand(time(NULL));

    if(this_session->call_from == MCM_CFROM_BOOT)
    {
        DMSG("[boot] vap");
    }

    path1 = "device.vap.*";
    if(mcm_config_find_entry_by_mix(this_session, path1, &tmp_group, &tmp_store, NULL, NULL)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_find_entry_by_mix(%s) fail", path1);
        goto FREE_01;
    }

    for(i = 0; tmp_store != NULL; tmp_store = tmp_store->next_store, i++)
    {
        snprintf(path2, sizeof(path2), "device.vap.@%u", i + 1);

        if(mcm_config_get_entry_by_info(this_session, tmp_group, tmp_store, MCM_DACCESS_SYS,
                                        &vap_v) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_get_entry_by_info(%s) fail", path2);
            goto FREE_01;
        }
        DMSG("[get-entry] %s.ekey = " MCM_DTYPE_EK_PF, path2, vap_v.ekey);
        DMSG("[get-entry] %s.date = " MCM_DTYPE_S_PF, path2, vap_v.ssid);
        DMSG("[get-entry] %s.date = " MCM_DTYPE_IUI_PF, path2, vap_v.channel);
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

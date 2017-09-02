// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../../mcm_lib/mcm_lheader/mcm_type.h"
#include "../../mcm_lib/mcm_lheader/mcm_size.h"
#include "../../mcm_lib/mcm_lheader/mcm_control.h"
#include "../../mcm_lib/mcm_lheader/mcm_connect.h"
#include "../../mcm_lib/mcm_lheader/mcm_return.h"
#include "../../mcm_lib/mcm_lheader/mcm_debug.h"
#include "../../mcm_lib/mcm_lheader/mcm_data_exinfo_auto.h"
#include "../mcm_service_handle_extern.h"
#include "../mcm_config_handle_extern.h"
#include "maam_lulib/maam_lulib_api.h"


#define DMSG(msg_fmt, msgs...) printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msgs)


int mcm_module_get_session(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    struct maam_lulib_t self_maam_lulib;
    struct maam_session_t *each_session;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_config_model_group_t *tmp_group;
    struct mcm_config_store_t *tmp_parent_store;
    struct mcm_dv_device_web_session_t session_v;
    MCM_DTYPE_EK_TD i = 0;


    DMSG("get-session test:");

    memset(&self_maam_lulib, 0, sizeof(struct maam_lulib_t));
    self_maam_lulib.sm_key = MAAM_SHARE_MEMORY_KEY;
    self_maam_lulib.sm_mutex_path = MAAM_MUTEX_PATH;
    if(maam_lulib_init(&self_maam_lulib) < MAAM_RCODE_PASS)
    {
        DMSG("call maam_lulib_init() fail");
        goto FREE_01;
    }

    // 先使用路徑取得資訊.
    path1 = "device.web.session.*";
    if(mcm_config_find_entry_by_mix(this_session, path1, &tmp_group, NULL, NULL, &tmp_parent_store)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_find_entry_by_mix(%s) fail", path1);
        goto FREE_02;
    }

    // 刪除所有的 device.web.session.*
    DMSG("[del-all-entry][SYS] %s", path1);
    if(mcm_config_del_all_entry_by_info(this_session, tmp_group, tmp_parent_store,
                                        MCM_DACCESS_SYS) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_del_all_entry_by_info(%s) fail", path1);
        goto FREE_02;
    }

    // 加入 session 資料.
    for(each_session = MAAM_LULIB_HEAD_SESSION(self_maam_lulib.auth_sys); each_session != NULL;
        each_session = MAAM_LULIB_NEXT_SESSION(self_maam_lulib.auth_sys, each_session))
    {
        i++;
        if(i <= MCM_MCOUNT_DEVICE_WEB_SESSION_MAX_COUNT)
        {
            snprintf(path2, sizeof(path2), "device.web.session.#" MCM_DTYPE_EK_PF, i);

            memset(&session_v, 0, sizeof(session_v));

            MAAM_LULIB_IP_NTOH(each_session->client_addr,
                               session_v.client_addr, sizeof(session_v.client_addr));
            DMSG("%s.client_addr = " MCM_DTYPE_S_PF, path2, session_v.client_addr);

            snprintf(session_v.account_name, sizeof(session_v.account_name), "%s",
                     each_session->account_name);
            DMSG("%s.account_name = " MCM_DTYPE_S_PF, path2, session_v.account_name);

            session_v.account_permission = each_session->account_permission;
            DMSG("%s.account_permission = " MCM_DTYPE_ISI_PF, path2, session_v.account_permission);

            snprintf(session_v.session_key, sizeof(session_v.session_key), "%s",
                     each_session->session_key);
            DMSG("%s.session_key = " MCM_DTYPE_S_PF, path2, session_v.session_key);

            session_v.idle_timeout = each_session->idle_timeout;
            DMSG("%s.idle_timeout = " MCM_DTYPE_ISI_PF, path2, session_v.idle_timeout);

            session_v.create_uptime = each_session->create_uptime;
            DMSG("%s.create_uptime = " MCM_DTYPE_ISI_PF, path2, session_v.create_uptime);

            session_v.last_access_uptime = each_session->last_access_uptime;
            DMSG("%s.last_access_uptime = " MCM_DTYPE_ISI_PF, path2, session_v.last_access_uptime);

            DMSG("[add-entry][SYS] %s", path2);
            if(mcm_config_add_entry_by_info(this_session, tmp_group, tmp_parent_store, i, NULL,
                                            MCM_DACCESS_SYS, &session_v, NULL) < MCM_RCODE_PASS)
            {
                DMSG("call mcm_config_add_entry_by_info(%s) fail", path2);
                goto FREE_02;
            }
        }
    }

    fret = MCM_RCODE_PASS;
FREE_02:
    maam_lulib_exit(&self_maam_lulib);
FREE_01:
    return fret;
}

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

int kick_session(
    struct maam_auth_sys_t *auth_sys_info,
    char *session_key)
{
    struct maam_session_t *each_session, *next_session;

    for(each_session = MAAM_LULIB_HEAD_SESSION(auth_sys_info), next_session = NULL;
        each_session != NULL; each_session = next_session)
    {
        next_session = MAAM_LULIB_NEXT_SESSION(auth_sys_info, each_session);

        if(strcmp(each_session->session_key, session_key) == 0)
        {
            DMSG("kick [%s][%s]", each_session->account_name, each_session->session_key);
            maam_lulib_kick_session(auth_sys_info, each_session);
        }
    }

    return 0;
}

int mcm_module_kick_session(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    struct maam_lulib_t self_maam_lulib;
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_config_model_group_t *tmp_group;
    struct mcm_config_store_t *tmp_store;
    MCM_DTYPE_EK_TD i;
    MCM_DTYPE_DS_TD tmp_status;
    struct mcm_dv_device_web_session_t session_v;


    DMSG("kick-session test:");

    memset(&self_maam_lulib, 0, sizeof(struct maam_lulib_t));
    self_maam_lulib.sm_key = MAAM_SHARE_MEMORY_KEY;
    self_maam_lulib.sm_mutex_path = MAAM_MUTEX_PATH;
    if(maam_lulib_init(&self_maam_lulib) < MAAM_RCODE_PASS)
    {
        DMSG("call maam_lulib_init() fail");
        goto FREE_01;
    }

    // device.web.session.* 的開頭 store_info.
    path1 = "device.web.session.*";
    if(mcm_config_find_entry_by_mix(this_session, path1, &tmp_group, &tmp_store, NULL, NULL)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_find_entry_by_mix(%s) fail", path1);
        goto FREE_02;
    }

    // 目標 : device.web.session.*
    for(i = 0; tmp_store != NULL; tmp_store = tmp_store->next_store, i++)
    {
        snprintf(path2, sizeof(path2), "device.web.session.@%u", i + 1);

        // 取得狀態.
        if(mcm_config_get_entry_self_status_by_info(this_session, tmp_group, tmp_store,
                                                    &tmp_status) < MCM_RCODE_PASS)
        {
            DMSG("call mcm_config_get_entry_self_status_by_info(%s) fail", path2);
            goto FREE_02;
        }
        tmp_status &= MCM_DSCHANGE_MASK;
        DMSG("[status] %s [%s]", path2, dump_status(tmp_status));

        // 刪除 session.
        if(tmp_status == MCM_DSCHANGE_DEL)
        {
            // 取得資料 (SYS 模式).
            if(mcm_config_get_entry_by_info(this_session, tmp_group, tmp_store, MCM_DACCESS_SYS,
                                            &session_v) < MCM_RCODE_PASS)
            {
                DMSG("call mcm_config_get_entry_by_info(%s) fail", path2);
                goto FREE_02;
            }

            kick_session(self_maam_lulib.auth_sys, session_v.session_key);
        }
    }

    fret = MCM_RCODE_PASS;
FREE_02:
    maam_lulib_exit(&self_maam_lulib);
FREE_01:
    return fret;
}

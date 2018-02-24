// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "mcm_lib/mcm_lheader/mcm_type.h"
#include "mcm_lib/mcm_lheader/mcm_control.h"
#include "mcm_lib/mcm_lheader/mcm_connect.h"
#include "mcm_lib/mcm_lheader/mcm_return.h"
#include "mcm_lib/mcm_lheader/mcm_debug.h"
#include "mcm_service_handle_extern.h"
#include "mcm_config_handle_extern.h"
#include "mcm_action_handle_extern.h"




#define NAME_TO_CHAR(name_str) #name_str
#define NAME_TO_STR(name_str)  NAME_TO_CHAR(name_str)




int mcm_action_boot_profile_run(
    void)
{
    int fret;
    struct mcm_service_session_t self_session;
    struct mcm_action_module_t *cb_list;
    MCM_DTYPE_USIZE_TD midx;
    char *dl_err;
    int (*module_cb)(struct mcm_service_session_t *this_session);


    MCM_ATDMSG("=> %s", __FUNCTION__);

    memset(&self_session, 0, sizeof(struct mcm_service_session_t));
    self_session.call_from = MCM_CFROM_BOOT;
    self_session.session_permission = MCM_SPERMISSION_RW;

    cb_list = dlsym(mcm_config_module_fp,
                    NAME_TO_STR(MCM_ACTION_CUSTOM_MODULE_BOOT_PROFILE_LIST_NAME));
    dl_err = dlerror();
    if(dl_err != NULL)
    {
        MCM_ATDMSG("not find boot profile list variable (%s) [%s]",
                   NAME_TO_STR(MCM_ACTION_CUSTOM_MODULE_BOOT_PROFILE_LIST_NAME), dl_err);
    }

    if(cb_list != NULL)
        for(midx = 0; cb_list[midx].cb_name[0] != '\0'; midx++)
        {
            MCM_ATDMSG("boot_profile[%s]", cb_list[midx].cb_name);
            module_cb = dlsym(mcm_config_module_fp, cb_list[midx].cb_name);
            dl_err = dlerror();
            if(dl_err != NULL)
            {
                MCM_EMSG("call dlsym(%s) fail [%s]", cb_list[midx].cb_name, dl_err);
                return MCM_RCODE_ACTION_INTERNAL_ERROR;
            }

            self_session.rep_data_buf = NULL;
            self_session.rep_data_len = 0;

            fret = module_cb(&self_session);

            if(self_session.rep_data_buf != NULL)
                free(self_session.rep_data_buf);

            if(fret < MCM_RCODE_PASS)
                return fret;
        }

    // 開機執行順序 : mcm_action_boot_profile_run() -> mcm_action_boot_other_run().
    // 這邊只做資料同步, 執行 mcm_action_boot_other_run() 才做儲存.
    fret = mcm_config_update(&self_session, MCM_DUPDATE_SYNC);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_update() fail");
        return fret;
    }

    return MCM_RCODE_PASS;
}

int mcm_action_boot_other_run(
    void)
{
    int fret;
    struct mcm_service_session_t self_session;
    struct mcm_action_module_t *cb_list;
    MCM_DTYPE_USIZE_TD midx;
    char *dl_err;
    int (*module_cb)(struct mcm_service_session_t *this_session);


    MCM_ATDMSG("=> %s", __FUNCTION__);

    memset(&self_session, 0, sizeof(struct mcm_service_session_t));
    self_session.call_from = MCM_CFROM_BOOT;
    self_session.session_permission = MCM_SPERMISSION_RW;

    cb_list = dlsym(mcm_config_module_fp,
                    NAME_TO_STR(MCM_ACTION_CUSTOM_MODULE_BOOT_OTHER_LIST_NAME));
    dl_err = dlerror();
    if(dl_err != NULL)
    {
        MCM_ATDMSG("not find boot other list variable (%s) [%s]",
                   NAME_TO_STR(MCM_ACTION_CUSTOM_MODULE_BOOT_OTHER_LIST_NAME), dl_err);
    }

    if(cb_list != NULL)
        for(midx = 0; cb_list[midx].cb_name[0] != '\0'; midx++)
        {
            MCM_ATDMSG("boot_other[%s]", cb_list[midx].cb_name);
            module_cb = dlsym(mcm_config_module_fp, cb_list[midx].cb_name);
            dl_err = dlerror();
            if(dl_err != NULL)
            {
                MCM_EMSG("call dlsym(%s) fail [%s]", cb_list[midx].cb_name, dl_err);
                return MCM_RCODE_ACTION_INTERNAL_ERROR;
            }

            self_session.rep_data_buf = NULL;
            self_session.rep_data_len = 0;

            module_cb(&self_session);

            if(self_session.rep_data_buf != NULL)
                free(self_session.rep_data_buf);
        }

    fret = mcm_config_save(&self_session, MCM_DUPDATE_SYNC, 1, 0);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_save() fail");
        return fret;
    }

    return MCM_RCODE_PASS;
}

int mcm_action_reset_default_run(
    void)
{
    int fret;
    char *dl_err;
    int (*module_cb)(void);


    MCM_ATDMSG("=> %s", __FUNCTION__);

    module_cb = dlsym(mcm_config_module_fp, NAME_TO_STR(MCM_ACTION_CUSTOM_RESET_DEFAULT_NAME));
    dl_err = dlerror();
    if(dl_err != NULL)
    {
        MCM_ATDMSG("not find reset to default function (%s) [%s]",
                   NAME_TO_STR(MCM_ACTION_CUSTOM_RESET_DEFAULT_NAME), dl_err);
        return MCM_RCODE_PASS;
    }

    fret = module_cb();
    if(fret < MCM_RCODE_PASS)
        return fret;

    return MCM_RCODE_PASS;
}

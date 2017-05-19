// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include "../../mcm_lib/mcm_lheader/mcm_type.h"
#include "../../mcm_lib/mcm_lheader/mcm_return.h"
#include "../../mcm_lib/mcm_lheader/mcm_debug.h"
#include "../mcm_action_handle_define.h"


struct mcm_action_module_t MCM_ACTION_CUSTOM_MODULE_BOOT_PROFILE_LIST_NAME[] =
{
    {""}
};

struct mcm_action_module_t MCM_ACTION_CUSTOM_MODULE_BOOT_OTHER_LIST_NAME[] =
{
    {"mcm_module_boot_config_device"},
    {"mcm_module_boot_config_system"},
    {"mcm_module_boot_config_vap"},
    {""}
};


int MCM_ACTION_CUSTOM_RESET_DEFAULT_NAME(
    void)
{
    return MCM_RCODE_PASS;
}

// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include "../../mcm_lib/mcm_lheader/mcm_type.h"
#include "../../mcm_lib/mcm_lheader/mcm_return.h"
#include "../../mcm_lib/mcm_lheader/mcm_debug.h"
#include "../mcm_action_handle_define.h"


struct mcm_action_module_t MCM_ACTION_CUSTOM_MODULE_BOOT_PROFILE_LIST_NAME[] =
{
    {"mcm_module_check_version"},
    {"mcm_module_check_device"},
    {"mcm_module_check_system"},
    {"mcm_module_check_vap"},
    {"mcm_module_check_extra"},
    {"mcm_module_check_station"},
    {"mcm_module_check_limit"},
    {""}
};

struct mcm_action_module_t MCM_ACTION_CUSTOM_MODULE_BOOT_OTHER_LIST_NAME[] =
{
    {""}
};

int MCM_ACTION_CUSTOM_RESET_DEFAULT_NAME(
    void)
{
    return MCM_RCODE_PASS;
}

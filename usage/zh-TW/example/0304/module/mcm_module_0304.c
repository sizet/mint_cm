// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <time.h>
#include <stdio.h>
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


int mcm_module_web_test_01(
    struct mcm_service_session_t *this_session)
{
    srand(time(NULL));

    DMSG("module-%u", rand());

    return MCM_RCODE_PASS;
}

int mcm_module_web_test_02(
    struct mcm_service_session_t *this_session)
{
    srand(time(NULL));

    DMSG("module-%u", rand());

    return MCM_RCODE_PASS;
}

int mcm_module_save_test(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    MCM_DTYPE_BOOL_TD force_save = 1;

    DMSG("[save] %s", force_save == 0 ? "check" : "force");
    if(mcm_config_save(this_session, MCM_DUPDATE_SYNC, 0, force_save) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_save() fail");
        goto FREE_01;
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_shutdown_test(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;

    DMSG("[shutdown]");
    if(mcm_config_shutdown(this_session) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_shutdown() fail");
        goto FREE_01;
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

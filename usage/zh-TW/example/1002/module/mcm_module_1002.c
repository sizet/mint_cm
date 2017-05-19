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


int mcm_module_get_loading(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1;
    MCM_DTYPE_FD_TD loading;


    DMSG("post-for-anyone test:");

    srand(time(NULL));

    path1 = "device.system.loading";
    loading = (rand() % 10000) / ((MCM_DTYPE_FD_TD) 100.0);
    DMSG("loading = " MCM_DTYPE_FD_PF, loading);
    if(mcm_config_set_alone_by_path(this_session, path1, MCM_DACCESS_SYS, &loading,
                                    sizeof(loading)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_set_alone_by_path(%s) fail", path1);
        goto FREE_01;
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

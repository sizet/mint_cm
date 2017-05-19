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


int mcm_module_return_test_01(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;


    DMSG("return test 01 :");

    srand(time(NULL));

    if(mcm_service_response_init(this_session, 256) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_service_response_init() fail");
        goto FREE_01;
    }
    DMSG("buffer [%p][" MCM_DTYPE_USIZE_PF "]",
         this_session->rep_msg_buf, this_session->rep_msg_size);

    snprintf(this_session->rep_msg_buf, this_session->rep_msg_size,
             "call from %d, rand = %u",
             this_session->call_from, rand());

    fret = 12345;

FREE_01:
    return fret;
}

int mcm_module_return_test_02(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    MCM_DTYPE_USIZE_TD tmp_len = 0;


    DMSG("return test 02 :");

    srand(time(NULL));

    if(mcm_service_response_init(this_session, 1024) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_service_response_init() fail");
        goto FREE_01;
    }
    DMSG("buffer [%p][" MCM_DTYPE_USIZE_PF "]",
         this_session->rep_msg_buf, this_session->rep_msg_size);

    tmp_len += snprintf(this_session->rep_msg_buf + tmp_len,
                        this_session->rep_msg_size - tmp_len,
                        "call from %d, return test 2\n",
                        this_session->call_from);
    tmp_len += snprintf(this_session->rep_msg_buf + tmp_len,
                        this_session->rep_msg_size - tmp_len,
                        "rand = %u\n",
                        rand());

    fret = 2468;

FREE_01:
    return fret;
}

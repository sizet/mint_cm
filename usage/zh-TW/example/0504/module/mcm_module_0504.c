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


int mcm_module_obtain_error_test(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;


    DMSG("obtain error test :");

    if(mcm_service_response_init(this_session, 1024) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_service_response_init() fail");
        goto FREE_01;
    }
    DMSG("buffer [%p][" MCM_DTYPE_USIZE_PF "]",
         this_session->rep_msg_buf, this_session->rep_msg_size);

    snprintf(this_session->rep_msg_buf, this_session->rep_msg_size,
             "alert(\"custom error\");");

FREE_01:
    return fret;
}

int mcm_module_obtain_multiple_test_01(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;


    DMSG("obtain multiple test 01 :");

    if(mcm_service_response_init(this_session, 1024) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_service_response_init() fail");
        goto FREE_01;
    }
    DMSG("buffer [%p][" MCM_DTYPE_USIZE_PF "]",
         this_session->rep_msg_buf, this_session->rep_msg_size);

    snprintf(this_session->rep_msg_buf, this_session->rep_msg_size,
             "alert(\"custom error 01\");");

FREE_01:
    return fret;
}

int mcm_module_obtain_multiple_test_02(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;


    DMSG("obtain multiple test 02 :");

    if(mcm_service_response_init(this_session, 1024) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_service_response_init() fail");
        goto FREE_01;
    }
    DMSG("buffer [%p][" MCM_DTYPE_USIZE_PF "]",
         this_session->rep_msg_buf, this_session->rep_msg_size);

    snprintf(this_session->rep_msg_buf, this_session->rep_msg_size,
             "alert(\"custom error 02\");");

FREE_01:
    return fret;
}

int mcm_module_submit_error_test(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;


    DMSG("submit error test :");

    if(mcm_service_response_init(this_session, 1024) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_service_response_init() fail");
        goto FREE_01;
    }
    DMSG("buffer [%p][" MCM_DTYPE_USIZE_PF "]",
         this_session->rep_msg_buf, this_session->rep_msg_size);

    snprintf(this_session->rep_msg_buf, this_session->rep_msg_size,
             "alert(\"custom error\");");

FREE_01:
    return fret;
}

int mcm_module_submit_text_test(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1;
    struct mcm_dv_device_t device_v;


    DMSG("submit text test :");

    if(mcm_service_response_init(this_session, 1024) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_service_response_init() fail");
        goto FREE_01;
    }
    DMSG("buffer [%p][" MCM_DTYPE_USIZE_PF "]",
         this_session->rep_msg_buf, this_session->rep_msg_size);

    path1 = "device";
    if(mcm_config_get_entry_by_path(this_session, path1, MCM_DACCESS_AUTO, &device_v)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_by_path(%s) fail", path1);
        goto FREE_01;
    }

    snprintf(this_session->rep_msg_buf, this_session->rep_msg_size,
             " %s",
             device_v.descript);

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_submit_json_test(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1;
    struct mcm_dv_device_t device_v;


    DMSG("submit json test :");

    if(mcm_service_response_init(this_session, 1024) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_service_response_init() fail");
        goto FREE_01;
    }
    DMSG("buffer [%p][" MCM_DTYPE_USIZE_PF "]",
         this_session->rep_msg_buf, this_session->rep_msg_size);

    path1 = "device";
    if(mcm_config_get_entry_by_path(this_session, path1, MCM_DACCESS_AUTO, &device_v)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_by_path(%s) fail", path1);
        goto FREE_01;
    }

    snprintf(this_session->rep_msg_buf, this_session->rep_msg_size,
             "{\"descript\":\"%s\"}",
             device_v.descript);

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_javascript_control_element_test(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1;
    struct mcm_dv_device_system_t system_v;


    DMSG("submit javascript control element test :");

    if(mcm_service_response_init(this_session, 1024) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_service_response_init() fail");
        goto FREE_01;
    }
    DMSG("buffer [%p][" MCM_DTYPE_USIZE_PF "]",
         this_session->rep_msg_buf, this_session->rep_msg_size);

    path1 = "device.system";
    if(mcm_config_get_entry_by_path(this_session, path1, MCM_DACCESS_AUTO, &system_v)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_by_path(%s) fail", path1);
        goto FREE_01;
    }

    snprintf(this_session->rep_msg_buf, this_session->rep_msg_size,
             "$(\"#number_div\").html(\""
             "<font color=\\\"#%s\\\">%s number</font>"
             "\");",
             system_v.uptime % 2 == 0 ? "FF0000" : "0000FF",
             system_v.uptime % 2 == 0 ? "even" : "odd");

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}


int mcm_module_submit_javascript_redirect_page_test(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1;
    struct mcm_dv_device_t device_v;


    DMSG("submit javascript redirect page test :");

    if(mcm_service_response_init(this_session, 1024) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_service_response_init() fail");
        goto FREE_01;
    }
    DMSG("buffer [%p][" MCM_DTYPE_USIZE_PF "]",
         this_session->rep_msg_buf, this_session->rep_msg_size);

    path1 = "device";
    if(mcm_config_get_entry_by_path(this_session, path1, MCM_DACCESS_AUTO, &device_v)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_by_path(%s) fail", path1);
        goto FREE_01;
    }

    snprintf(this_session->rep_msg_buf, this_session->rep_msg_size,
             "window.location.href = \"%s\";",
             device_v.descript);

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_submit_multiple_test_01(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;


    DMSG("submit multiple test 01 :");

    if(mcm_service_response_init(this_session, 1024) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_service_response_init() fail");
        goto FREE_01;
    }
    DMSG("buffer [%p][" MCM_DTYPE_USIZE_PF "]",
         this_session->rep_msg_buf, this_session->rep_msg_size);

    snprintf(this_session->rep_msg_buf, this_session->rep_msg_size,
             "custom message 01");

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_submit_multiple_test_02(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;


    DMSG("submit multiple test 02 :");

    if(mcm_service_response_init(this_session, 1024) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_service_response_init() fail");
        goto FREE_01;
    }
    DMSG("buffer [%p][" MCM_DTYPE_USIZE_PF "]",
         this_session->rep_msg_buf, this_session->rep_msg_size);

    snprintf(this_session->rep_msg_buf, this_session->rep_msg_size,
             "custom message 02");

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

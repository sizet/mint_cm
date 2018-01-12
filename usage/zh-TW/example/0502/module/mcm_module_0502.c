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


int mcm_module_user_data_test_01(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    MCM_DTYPE_USIZE_TD tmp_len;
    char *tmp_buf;


    DMSG("string data test :");

    srand(time(NULL));

    // 外部程式傳來的資料.

    DMSG("recv [" MCM_DTYPE_USIZE_PF "][%s]",
         this_session->req_data_len, (char *) this_session->req_data_con);

    // 內部模組要回傳的資料.

    tmp_len = 256;
    tmp_buf = (char *) malloc(tmp_len);
    if(tmp_buf == NULL)
    {
        DMSG("call malloc() fail [%s]", strerror(errno));
        goto FREE_01;
    }

    snprintf(tmp_buf, tmp_len, "internal data %u", rand());
    tmp_len = strlen(tmp_buf) + 1;
    DMSG("send [" MCM_DTYPE_USIZE_PF "][%s]", tmp_len, tmp_buf);

    this_session->rep_data_buf = tmp_buf;
    this_session->rep_data_len = tmp_len;

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

int mcm_module_user_data_test_02(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    MCM_DTYPE_USIZE_TD tmp_len, idx, count;
    int *tmp_buf;


    DMSG("bytes data test :");

    srand(time(NULL));

    // 外部程式傳來的資料.

    tmp_buf = (int *) this_session->req_data_con;
    count = this_session->req_data_len / sizeof(int);
    DMSG("recv [" MCM_DTYPE_USIZE_PF "] (" MCM_DTYPE_USIZE_PF ") -",
         this_session->req_data_len, count);
    for(idx = 0; idx < count; idx++)
    {
        DMSG("%d", tmp_buf[idx]);
    }

    // 內部模組要回傳的資料.

    count = 6;
    tmp_buf = calloc(count, sizeof(int));
    if(tmp_buf == NULL)
    {
        DMSG("call calloc() fail [%s]", strerror(errno));
        goto FREE_01;
    }
    tmp_len = sizeof(int) * count;

    for(idx = 0; idx < count; idx++)
        tmp_buf[idx] = rand() % 10;

    DMSG("send [" MCM_DTYPE_USIZE_PF "] (" MCM_DTYPE_USIZE_PF ") -", tmp_len, count);
    for(idx = 0; idx < count; idx++)
    {
        DMSG("%d", tmp_buf[idx]);
    }

    this_session->rep_data_buf = tmp_buf;
    this_session->rep_data_len = tmp_len;

    fret = MCM_RCODE_PASS;

FREE_01:
    return fret;
}

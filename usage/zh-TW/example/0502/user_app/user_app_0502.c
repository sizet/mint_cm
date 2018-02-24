// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mcm_lib/mcm_lheader/mcm_type.h"
#include "mcm_lib/mcm_lheader/mcm_size.h"
#include "mcm_lib/mcm_lheader/mcm_connect.h"
#include "mcm_lib/mcm_lheader/mcm_return.h"
#include "mcm_lib/mcm_lheader/mcm_data_exinfo_auto.h"
#include "mcm_lib/mcm_lulib/mcm_lulib_api.h"


#define DMSG(msg_fmt, msgs...) printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msgs)


int main(
    int argc,
    char **argv)
{
    char *path1;
    struct mcm_lulib_lib_t self_lulib;
    char req_data1[128], *rep_data1;
    MCM_DTYPE_USIZE_TD req_len1, rep_len1;
    int req_data2[4], *rep_data2;
    MCM_DTYPE_USIZE_TD req_len2, rep_len2, idx, count;


    srand(time(NULL));

    memset(&self_lulib, 0, sizeof(struct mcm_lulib_lib_t));
    self_lulib.socket_path = "@mintcm";
    self_lulib.call_from = MCM_CFROM_USER;
    self_lulib.session_permission = MCM_SPERMISSION_RO;
    self_lulib.session_stack_size = 0;
    if(mcm_lulib_init(&self_lulib) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_init() fail");
        goto FREE_01;
    }

    // 測試-01 : 字串類型資料.

    path1 = "mcm_module_user_data_test_01";
    DMSG("[run] %s", path1);

    snprintf(req_data1, sizeof(req_data1), "user-data-%u", rand());
    req_len1 = strlen(req_data1) + 1;
    DMSG("send [" MCM_DTYPE_USIZE_PF "][%s]", req_len1, req_data1);

    if(mcm_lulib_run(&self_lulib, path1, &req_data1, req_len1, (void **) &rep_data1, &rep_len1)
                     < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_run(%s) fail", path1);
        goto FREE_02;
    }

    DMSG("recv [" MCM_DTYPE_USIZE_PF "][%s]", rep_len1, rep_data1);
    free(rep_data1);

    // 測試-02 : 字節類型資料.

    path1 = "mcm_module_user_data_test_02";
    DMSG("[run] %s", path1);

    count = sizeof(req_data2) / sizeof(int);
    for(idx = 0; idx < count; idx++)
        req_data2[idx] = rand() % 100;

    req_len2 = sizeof(req_data2);

    DMSG("send [" MCM_DTYPE_USIZE_PF "] (" MCM_DTYPE_USIZE_PF ") -", req_len2, count);
    for(idx = 0; idx < count; idx++)
    {
        DMSG("%d", req_data2[idx]);
    }

    if(mcm_lulib_run(&self_lulib, path1, &req_data2, req_len2, (void **) &rep_data2, &rep_len2)
                     < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_run(%s) fail", path1);
        goto FREE_02;
    }

    count = rep_len2 / sizeof(int);
    DMSG("recv [" MCM_DTYPE_USIZE_PF "] (" MCM_DTYPE_USIZE_PF ") -", rep_len2, count);
    for(idx = 0; idx < count; idx++)
    {
        DMSG("%d", rep_data2[idx]);
    }
    free(rep_data2);

FREE_02:
    mcm_lulib_exit(&self_lulib);
FREE_01:
    return 0;
}

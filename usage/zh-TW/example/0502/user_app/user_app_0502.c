// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mcm_lheader/mcm_type.h"
#include "mcm_lheader/mcm_size.h"
#include "mcm_lheader/mcm_connect.h"
#include "mcm_lheader/mcm_return.h"
#include "mcm_lheader/mcm_data_exinfo_auto.h"
#include "mcm_lulib/mcm_lulib_api.h"


#define DMSG(msg_fmt, msgs...) printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msgs)


int main(int argc, char **argv)
{
    char *path1;
    struct mcm_lulib_lib_t self_lulib;


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

    // 呼叫內部模組.
    path1 = "mcm_module_return_test_01";
    DMSG("[run] %s", path1);
    if(mcm_lulib_run(&self_lulib, path1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_run(%s) fail", path1);
        goto FREE_02;
    }
    DMSG("[return] rep_code [" MCM_DTYPE_LIST_PF "]", self_lulib.rep_code);
    DMSG("[return] rep_msg_len [" MCM_DTYPE_USIZE_PF "]", self_lulib.rep_msg_len);
    DMSG("[return] rep_msg_con [%s]", self_lulib.rep_msg_con);

    // 呼叫內部模組.
    path1 = "mcm_module_return_test_02";
    DMSG("[run] %s", path1);
    if(mcm_lulib_run(&self_lulib, path1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_run(%s) fail", path1);
        goto FREE_02;
    }
    DMSG("[return] rep_code [" MCM_DTYPE_LIST_PF "]", self_lulib.rep_code);
    DMSG("[return] rep_msg_len [" MCM_DTYPE_USIZE_PF "]", self_lulib.rep_msg_len);
    DMSG("[return] rep_msg_con\n%s", self_lulib.rep_msg_con);

FREE_02:
    mcm_lulib_exit(&self_lulib);
FREE_01:
    return 0;
}

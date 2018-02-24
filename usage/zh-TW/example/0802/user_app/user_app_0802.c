// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

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


int case_check_store_file(
    struct mcm_lulib_lib_t *this_lulib)
{
    char *path1, store_version[MCM_BASE_VERSION_BUFFER_SIZE];
    MCM_DTYPE_LIST_TD store_result;

    path1 = "mcm_store_upload.txt";
    if(mcm_lulib_check_store_file(this_lulib, path1, &store_result,
                                  store_version, sizeof(store_version)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_check_store_file(%s) fail", path1);
        goto FREE_01;
    }
    DMSG("[%s][%s][%s]", path1, store_version, store_result < MCM_RCODE_PASS ? "fail" : "pass");

FREE_01:
    return 0;
}

struct operate_cb_t
{
    char *opt_cmd;
    char *opt_help;
    int (*opt_cb)(struct mcm_lulib_lib_t *this_lulib);
    MCM_DTYPE_LIST_TD opt_permission;
    MCM_DTYPE_USIZE_TD opt_stack_size;
};
struct operate_cb_t operate_cb_list[] =
{
    {"check-store-file", "check store file test", case_check_store_file, MCM_SPERMISSION_RO, 0},
    {NULL, NULL, NULL}
};

int main(
    int argc,
    char **argv)
{
    struct mcm_lulib_lib_t self_lulib;
    unsigned int i;


    if(argc != 2)
        goto FREE_HELP;

    for(i = 0; operate_cb_list[i].opt_cmd != NULL; i++)
        if(strcmp(operate_cb_list[i].opt_cmd, argv[1]) == 0)
            break;
    if(operate_cb_list[i].opt_cb == NULL)
        goto FREE_HELP;

    memset(&self_lulib, 0, sizeof(struct mcm_lulib_lib_t));
    self_lulib.socket_path = "@mintcm";
    self_lulib.call_from = MCM_CFROM_USER;
    self_lulib.session_permission = operate_cb_list[i].opt_permission;
    self_lulib.session_stack_size = operate_cb_list[i].opt_stack_size;
    if(mcm_lulib_init(&self_lulib) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_init() fail");
        goto FREE_01;
    }

    if(operate_cb_list[i].opt_cb(&self_lulib) < 0)
    {
        DMSG("call operate_cb_list[%s] fail", operate_cb_list[i].opt_cmd);
        goto FREE_02;
    }

FREE_02:
    mcm_lulib_exit(&self_lulib);
FREE_01:
    return 0;
FREE_HELP:
    printf("user_app_0802 <test_case>\n");
    printf("  <test_case> list :\n");
    for(i = 0; operate_cb_list[i].opt_cmd != NULL; i++)
        printf("    %s : %s\n", operate_cb_list[i].opt_cmd, operate_cb_list[i].opt_help);
    return 0;
}

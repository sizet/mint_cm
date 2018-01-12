// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/random.h>
#include "mcm_lheader/mcm_type.h"
#include "mcm_lheader/mcm_size.h"
#include "mcm_lheader/mcm_connect.h"
#include "mcm_lheader/mcm_return.h"
#include "mcm_lheader/mcm_data_exinfo_auto.h"
#include "mcm_lklib/mcm_lklib_api.h"


#define DMSG(msg_fmt, msgs...) \
    printk(KERN_INFO "%s(%04u): " msg_fmt "\n", \
           strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__, __LINE__, ##msgs)


static unsigned int rand(
    void)
{
    unsigned int tmp_value;


    get_random_bytes(&tmp_value, sizeof(tmp_value));

    return tmp_value;
}

static int __init kernel_app_0503_init(
    void)
{
    char *path1;
    struct mcm_lklib_lib_t self_lklib;
    char req_data1[128], *rep_data1;
    MCM_DTYPE_USIZE_TD req_len1, rep_len1;
    int req_data2[4], *rep_data2;
    MCM_DTYPE_USIZE_TD req_len2, rep_len2, idx, count;


    memset(&self_lklib, 0, sizeof(struct mcm_lklib_lib_t));
    self_lklib.socket_path = "@mintcm";
    self_lklib.call_from = MCM_CFROM_KERNEL;
    self_lklib.session_permission = MCM_SPERMISSION_RO;
    self_lklib.session_stack_size = 0;
    if(mcm_lklib_init(&self_lklib) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_init() fail");
        goto FREE_01;
    }

    // 測試-01 : 字串類型資料.

    path1 = "mcm_module_kernel_data_test_01";
    DMSG("[run] %s", path1);

    snprintf(req_data1, sizeof(req_data1), "kernel-data-%u", rand());
    req_len1 = strlen(req_data1) + 1;
    DMSG("send [" MCM_DTYPE_USIZE_PF "][%s]", req_len1, req_data1);

    if(mcm_lklib_run(&self_lklib, path1, &req_data1, req_len1, (void **) &rep_data1, &rep_len1)
                     < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_run(%s) fail", path1);
        goto FREE_02;
    }

    DMSG("recv [" MCM_DTYPE_USIZE_PF "][%s]", rep_len1, rep_data1);
    kfree(rep_data1);

    // 測試-02 : 字節類型資料.

    path1 = "mcm_module_kernel_data_test_02";
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

    if(mcm_lklib_run(&self_lklib, path1, &req_data2, req_len2, (void **) &rep_data2, &rep_len2)
                     < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_run(%s) fail", path1);
        goto FREE_02;
    }

    count = rep_len2 / sizeof(int);
    DMSG("recv [" MCM_DTYPE_USIZE_PF "] (" MCM_DTYPE_USIZE_PF ") -", rep_len2, count);
    for(idx = 0; idx < count; idx++)
    {
        DMSG("%d", rep_data2[idx]);
    }
    kfree(rep_data2);

FREE_02:
    mcm_lklib_exit(&self_lklib);
FREE_01:
    return 0;
}

static void __exit kernel_app_0503_exit(
    void)
{
    return;
}

module_init(kernel_app_0503_init);
module_exit(kernel_app_0503_exit);

MODULE_LICENSE("GPL");

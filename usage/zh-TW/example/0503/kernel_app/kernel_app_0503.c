// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include "mcm_lheader/mcm_type.h"
#include "mcm_lheader/mcm_size.h"
#include "mcm_lheader/mcm_connect.h"
#include "mcm_lheader/mcm_return.h"
#include "mcm_lheader/mcm_data_exinfo_auto.h"
#include "mcm_lklib/mcm_lklib_api.h"


#define DMSG(msg_fmt, msgs...) \
    printk(KERN_INFO "%s(%04u): " msg_fmt "\n", \
           strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__, __LINE__, ##msgs)


static int __init kernel_app_0503_init(void)
{
    char *path1;
    struct mcm_lklib_lib_t self_lklib;


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

    // 呼叫內部模組.
    path1 = "mcm_module_return_test_01";
    DMSG("[run] %s", path1);
    if(mcm_lklib_run(&self_lklib, path1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_run(%s) fail", path1);
        goto FREE_02;
    }
    DMSG("[return] rep_code [" MCM_DTYPE_LIST_PF "]", self_lklib.rep_code);
    DMSG("[return] rep_msg_len [" MCM_DTYPE_USIZE_PF "]", self_lklib.rep_msg_len);
    DMSG("[return] rep_msg_con [%s]", self_lklib.rep_msg_con);

    // 呼叫內部模組.
    path1 = "mcm_module_return_test_02";
    DMSG("[run] %s", path1);
    if(mcm_lklib_run(&self_lklib, path1) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lulib_run(%s) fail", path1);
        goto FREE_02;
    }
    DMSG("[return] rep_code [" MCM_DTYPE_LIST_PF "]", self_lklib.rep_code);
    DMSG("[return] rep_msg_len [" MCM_DTYPE_USIZE_PF "]", self_lklib.rep_msg_len);
    DMSG("[return] rep_msg_con\n%s", self_lklib.rep_msg_con);

FREE_02:
    mcm_lklib_exit(&self_lklib);
FREE_01:
    return 0;
}

static void __exit kernel_app_0503_exit(void)
{
    return;
}

module_init(kernel_app_0503_init);
module_exit(kernel_app_0503_exit);

MODULE_LICENSE("GPL");

// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "mcm_lib/mcm_lheader/mcm_type.h"
#include "mcm_lib/mcm_lheader/mcm_size.h"
#include "mcm_lib/mcm_lheader/mcm_connect.h"
#include "mcm_lib/mcm_lheader/mcm_return.h"
#include "mcm_lib/mcm_lheader/mcm_data_exinfo_auto.h"
#include "mcm_lib/mcm_lklib/mcm_lklib_api.h"


#define DMSG(msg_fmt, msgs...) \
    printk(KERN_INFO "%s(%04u): " msg_fmt "\n", \
           strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__, __LINE__, ##msgs)


static ssize_t node_read(
    struct file *file,
    char __user *buffer,
    size_t count,
    loff_t *pos);

static ssize_t node_write(
    struct file *file,
    const char __user *buffer,
    size_t count,
    loff_t *pos);


static char *node_name = "kernel_app_0803";
static struct proc_dir_entry *node_entry;
static struct file_operations node_fops =
{
    .read  = node_read,
    .write = node_write,
};


int case_check_store_file(
    struct mcm_lklib_lib_t *this_lklib)
{
    char *path1, store_version[MCM_BASE_VERSION_BUFFER_SIZE];
    MCM_DTYPE_LIST_TD store_result;

    path1 = "mcm_store_upload.txt";
    if(mcm_lklib_check_store_file(this_lklib, path1, &store_result,
                                  store_version, sizeof(store_version)) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_check_store_file(%s) fail", path1);
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
    int (*opt_cb)(struct mcm_lklib_lib_t *this_lklib);
    MCM_DTYPE_LIST_TD opt_permission;
    MCM_DTYPE_USIZE_TD opt_stack_size;
};
struct operate_cb_t operate_cb_list[] =
{
    {"check-store-file", "check store file test", case_check_store_file, MCM_SPERMISSION_RO, 0},
    {NULL, NULL, NULL}
};

static void show_usage(
    void)
{
    unsigned int i;


    DMSG("kernel_app_0803 <test_case>");
    DMSG("  <test_case> list :");
    for(i = 0; operate_cb_list[i].opt_cmd != NULL; i++)
    {
        DMSG("    %s : %s", operate_cb_list[i].opt_cmd, operate_cb_list[i].opt_help);
    }
}

static ssize_t node_read(
    struct file *file,
    char __user *buffer,
    size_t count,
    loff_t *pos)
{
    show_usage();

	return 0;
}

static ssize_t node_write(
    struct file *file,
    const char __user *buffer,
    size_t count,
    loff_t *pos)
{
    struct mcm_lklib_lib_t self_lklib;
    char arg_buf[32];
    unsigned int arg_len, i;


    memset(arg_buf, 0, sizeof(arg_buf));
    arg_len = count >= (sizeof(arg_buf) - 1) ? sizeof(arg_buf) - 1 : count;
    copy_from_user(arg_buf, buffer, arg_len);

    if(arg_len > 1)
        if(arg_buf[arg_len - 1] == '\n')
            arg_buf[arg_len - 1] = '\0';

    for(i = 0; operate_cb_list[i].opt_cmd != NULL; i++)
        if(strcmp(operate_cb_list[i].opt_cmd, arg_buf) == 0)
            break;
    if(operate_cb_list[i].opt_cb == NULL)
        goto FREE_HELP;

    self_lklib.socket_path = "@mintcm";
    self_lklib.call_from = MCM_CFROM_KERNEL;
    self_lklib.session_permission = operate_cb_list[i].opt_permission;
    self_lklib.session_stack_size = operate_cb_list[i].opt_stack_size;
    if(mcm_lklib_init(&self_lklib) < MCM_RCODE_PASS)
    {
        DMSG("call mcm_lklib_init() fail");
        goto FREE_01;
    }

    if(operate_cb_list[i].opt_cb(&self_lklib) < 0)
    {
        DMSG("call operate_cb_list[%s] fail", operate_cb_list[i].opt_cmd);
        goto FREE_02;
    }

FREE_02:
    mcm_lklib_exit(&self_lklib);
FREE_01:
    return count;
FREE_HELP:
    show_usage();
    return count;
}

static int __init kernel_app_0803_init(
    void)
{
    if((node_entry = proc_create(node_name, S_IFREG | S_IRUGO | S_IWUGO, NULL, &node_fops))
                                 == NULL)
    {
        DMSG("call proc_create(%s) fail", node_name);
        goto FREE_01;
    }

FREE_01:
    return 0;
}

static void __exit kernel_app_0803_exit(
    void)
{
    remove_proc_entry(node_name, NULL);

    return;
}

module_init(kernel_app_0803_init);
module_exit(kernel_app_0803_exit);

MODULE_LICENSE("GPL");

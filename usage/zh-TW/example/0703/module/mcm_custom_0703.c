// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../../mcm_lib/mcm_lheader/mcm_type.h"
#include "../../mcm_lib/mcm_lheader/mcm_return.h"
#include "../../mcm_lib/mcm_lheader/mcm_debug.h"
#include "../mcm_action_handle_define.h"


#define DMSG(msg_fmt, msgs...) printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msgs)


struct mcm_action_module_t MCM_ACTION_CUSTOM_MODULE_BOOT_PROFILE_LIST_NAME[] =
{
    {""}
};

struct mcm_action_module_t MCM_ACTION_CUSTOM_MODULE_BOOT_OTHER_LIST_NAME[] =
{
    {""}
};

int MCM_ACTION_CUSTOM_RESET_DEFAULT_NAME(
    void)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *script_path = "reset_default.sh", cmd_buf[32];
    FILE *file_fp;


    DMSG("do reset to default");

    // 建立外部 shell script, 使用外部 shell script 處理.
    if((file_fp = fopen(script_path, "w")) == NULL)
    {
        MCM_EMSG("call fopen(%s) fail [%s]", script_path, strerror(errno));
        goto FREE_01;
    }

    fprintf(file_fp, "#!/bin/sh\n");

    // 等待一段時間, 等目前的程式關閉.
    fprintf(file_fp, "sleep 3\n");

    // 重新執行.
    fprintf(file_fp,
            "./mcm_daemon "
            "-t 0 "
            "-a @mintcm "
            "-l ./mcm_module.lib "
            "-m mcm_model_profile.txt "
            "-d mcm_store_profile_default.txt "
            "-c mcm_store_profile_current.txt "
            "-s 0 "
            "-e 1 "
            "-p /var/run/mcm_daemon.pid &\n");

    fprintf(file_fp, "rm -f $0\n");

    fclose(file_fp);
    chmod(script_path, 0777);

    // 執行 shell script.
    snprintf(cmd_buf, sizeof(cmd_buf), "./%s &", script_path);
    DMSG("[%s]", cmd_buf);
    if(system(cmd_buf) == -1)
    {
        MCM_EMSG("call system(%s) fail", cmd_buf);
        goto FREE_01;
    }

    fret =  MCM_RCODE_PASS;
FREE_01:
    return fret;
}

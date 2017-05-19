// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
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


int mcm_module_upload_test_02(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR;
    char *path1;
    struct mcm_dv_device_web_upload_t web_upload_v;
    FILE *file_fp;
    unsigned char file_buf[1024];
    size_t rlen, file_len = 0;


    DMSG("upload test 02 :");

    // 讀出檔案的資訊.
    path1 = "device.web_upload";
    if(mcm_config_get_entry_by_path(this_session, path1, MCM_DACCESS_NEW, &web_upload_v)
                                    < MCM_RCODE_PASS)
    {
        DMSG("call mcm_config_get_entry_by_path(%s) fail", path1);
        goto FREE_01;
    }

    file_fp = fopen(web_upload_v.save_path, "rb");
    if(file_fp == NULL)
    {
        DMSG("call fopen(%s) fail [%s]", web_upload_v.save_path, strerror(errno));
        goto FREE_02;
    }

    do
    {
        rlen = fread(file_buf, 1, sizeof(file_buf), file_fp);
        file_len += rlen;
    }
    while(rlen > 0);

    if(file_len != web_upload_v.file_size)
    {
        DMSG("fail, file size not match [%zu/" MCM_DTYPE_IUI_PF "]",
             file_len, web_upload_v.file_size);
        goto FREE_03;
    }

    DMSG("prcess pass, [" MCM_DTYPE_S_PF "][" MCM_DTYPE_IUI_PF "] save to [" MCM_DTYPE_S_PF "]",
         web_upload_v.file_name, web_upload_v.file_size, web_upload_v.save_path);

    fret = MCM_RCODE_PASS;
FREE_03:
    fclose(file_fp);
FREE_02:
    DMSG("unlink upload file [%s]", web_upload_v.save_path);
    unlink(web_upload_v.save_path);
FREE_01:
    return fret;
}

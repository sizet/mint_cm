// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "mcm_lib/mcm_lheader/mcm_type.h"
#include "mcm_lib/mcm_lheader/mcm_size.h"
#include "mcm_lib/mcm_lheader/mcm_connect.h"
#include "mcm_lib/mcm_lheader/mcm_return.h"
#include "mcm_lib/mcm_lheader/mcm_data_exinfo_auto.h"
#include "mcm_lib/mcm_lulib/mcm_lulib_api.h"
#include "../mcm_cgi_common_extern.h"
#include "mcm_cgi_module_debug.h"


void upload_handle_01(
    struct part_info_t *part_list)
{
    struct part_info_t *tmp_part;


#if MCM_CUMEMODE | MCM_CUMDMODE
    dbg_console_fd = open(MCM_DBG_CONSOLE, O_WRONLY);
    if(dbg_console_fd == -1)
        return;
#endif

#if MCM_CUMDMODE
    for(tmp_part = part_list; tmp_part != NULL; tmp_part = tmp_part->next_part)
    {
        MCM_CUMDMSG("name = %s", tmp_part->name_tag);
        MCM_CUMDMSG("filename = %s", tmp_part->filename_tag);
        if(tmp_part->filename_tag != NULL)
        {
            MCM_CUMDMSG("data = [file][" MCM_DTYPE_USIZE_PF "][%p]",
                        tmp_part->data_len, tmp_part->data_con);
        }
        else
        {
            MCM_CUMDMSG("data = [string][" MCM_DTYPE_USIZE_PF "][%s]",
                        tmp_part->data_len, tmp_part->data_con);
        }
    }
#endif

    mcm_cgi_fill_response_header(1, 1, MCM_RCODE_PASS);

    printf("var tmp_html = \"\";");
    for(tmp_part = part_list; tmp_part != NULL; tmp_part = tmp_part->next_part)
    {
        printf("tmp_html += \"<br>\";");
        printf("tmp_html += \"name = %s<br>\";", tmp_part->name_tag);
        if(tmp_part->filename_tag != NULL)
        {
            printf("tmp_html += \"filename = %s<br>\";", tmp_part->filename_tag);
            printf("tmp_html += \"data = [file][" MCM_DTYPE_USIZE_PF "][%p]<br>\";",
                   tmp_part->data_len, tmp_part->data_con);
        }
        else
        {
            printf("tmp_html += \"data = [string][" MCM_DTYPE_USIZE_PF "][%s]<br>\";",
                   tmp_part->data_len, tmp_part->data_con);
        }
    }
    printf("$(\"#show_box\").html(tmp_html);");

#if MCM_CUMEMODE | MCM_CUMDMODE
    close(dbg_console_fd);
#endif
    return;
}

void upload_handle_02(
    struct part_info_t *part_list)
{
    int fret = MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
    struct part_info_t *tmp_part, *file_part = NULL, *text_part = NULL;
    char msg_buf[1024];
    MCM_DTYPE_USIZE_TD msg_len = 0;
    char *file_path = "/tmp/upload_file";
    FILE *file_fp;
    size_t wlen;
    char *path1;
    struct mcm_lulib_lib_t self_lulib;
    struct mcm_dv_device_web_upload_t web_upload_v;


#define FILL_MSG(msg_fmt, msg_args...) \
    msg_len += snprintf(msg_buf + msg_len, sizeof(msg_buf) - msg_len, msg_fmt, ##msg_args)


#if MCM_CUMEMODE | MCM_CUMDMODE
    dbg_console_fd = open(MCM_DBG_CONSOLE, O_WRONLY);
    if(dbg_console_fd == -1)
        goto FREE_01;
#endif

    msg_buf[0] = '\0';

    for(tmp_part = part_list; tmp_part != NULL; tmp_part = tmp_part->next_part)
        if(strcmp(tmp_part->name_tag, "input_file") == 0)
            file_part = tmp_part;
        else
        if(strcmp(tmp_part->name_tag, "input_text") == 0)
            text_part = tmp_part;

    FILL_MSG("var tmp_html = \"\";");

    if(file_part->filename_tag == NULL)
    {
        MCM_CUMEMSG("no select any file");
        FILL_MSG("tmp_html += \"process fail, no select any file<br>\";");
        goto FREE_02;
    }

    // 將上傳的檔案寫入到檔案系統.
    MCM_CUMDMSG("save upload file to [%s]", file_path);
    file_fp = fopen(file_path, "wb");
    if(file_fp == NULL)
    {
        MCM_CUMEMSG("call fopen(%s) fail [%s]", file_path, strerror(errno));
        FILL_MSG("tmp_html += \"process fail, call fopen(%s) fail [%s]<br>\";",
                 file_path, strerror(errno));
        goto FREE_02;
    }
    wlen = fwrite(file_part->data_con, 1, file_part->data_len, file_fp);
    if(wlen != file_part->data_len)
    {
        MCM_CUMEMSG("call fwrite() fail [%s]", strerror(errno));
        FILL_MSG("tmp_html += \"process fail, call fwrite() fail [%s]<br>\";", strerror(errno));
        goto FREE_02;
    }
    fclose(file_fp);

    // 傳送資料.

    self_lulib.socket_path = "@mintcm";
    self_lulib.call_from = MCM_CFROM_USER;
    self_lulib.session_permission = MCM_SPERMISSION_RW;
    self_lulib.session_stack_size = 0;
    if(mcm_lulib_init(&self_lulib) < MCM_RCODE_PASS)
    {
        MCM_CUMEMSG("call mcm_lulib_init() fail");
        FILL_MSG("tmp_html += \"process fail, call mcm_lulib_init() fail<br>\";");
        goto FREE_03;
    }

    // 紀錄檔案的資訊.

    path1 = "device.web_upload";

    memset(&web_upload_v, 0, sizeof(web_upload_v));

    snprintf(web_upload_v.file_name, sizeof(web_upload_v.file_name),
             "%s", file_part->filename_tag);
    MCM_CUMDMSG("[set-entry] %s.file_name = " MCM_DTYPE_S_PF, path1, web_upload_v.file_name);

    web_upload_v.file_size = file_part->data_len;
    MCM_CUMDMSG("[set-entry] %s.file_size = " MCM_DTYPE_IUI_PF, path1, web_upload_v.file_size);

    snprintf(web_upload_v.save_path, sizeof(web_upload_v.save_path), "%s", file_path);
    MCM_CUMDMSG("[set-entry] %s.save_path = " MCM_DTYPE_S_PF, path1, web_upload_v.save_path);

    snprintf(web_upload_v.addition_data, sizeof(web_upload_v.addition_data), "%s",
             text_part->data_con);
    MCM_CUMDMSG("[set-entry] %s.addition_data = " MCM_DTYPE_S_PF,
                path1, web_upload_v.addition_data);

    if(mcm_lulib_set_entry(&self_lulib, path1, &web_upload_v, sizeof(web_upload_v))
                                 < MCM_RCODE_PASS)
    {
        MCM_CUMEMSG("call mcm_lulib_set_entry(%s) fail", path1);
        FILL_MSG("tmp_html += \"process fail, call mcm_lulib_set_entry(%s) fail<br>\";",
                 path1);
        goto FREE_04;
    }

    // 執行處理的內部模組.
    path1 = "mcm_module_upload_test_02";
    MCM_CUMDMSG("[run] %s", path1);
    if(mcm_lulib_run(&self_lulib, path1, NULL, 0, NULL, NULL) < MCM_RCODE_PASS)
    {
        MCM_CUMEMSG("call mcm_lulib_run(%s) fail", path1);
        FILL_MSG("tmp_html += \"process fail, call mcm_lulib_run(%s) fail<br>\";", path1);
        goto FREE_04;
    }

    FILL_MSG("tmp_html += \"process pass<br>\";");
    FILL_MSG("tmp_html += \"file name [%s]<br>\";", file_part->filename_tag);
    FILL_MSG("tmp_html += \"file size [" MCM_DTYPE_USIZE_PF "]<br>\";", file_part->data_len);
    FILL_MSG("tmp_html += \"file save [%s]<br>\";", file_path);
    FILL_MSG("tmp_html += \"addition [%s]<br>\";", text_part->data_con);

    fret = MCM_RCODE_PASS;
FREE_04:
    mcm_lulib_exit(&self_lulib);
FREE_03:
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CUMDMSG("unlink upload file [%s]", file_path);
        unlink(file_path);
    }
FREE_02:
    mcm_cgi_fill_response_header(1, 1, fret);
    printf("%s", msg_buf);
    printf("$(\"#show_box\").html(tmp_html);");
#if MCM_CUMEMODE | MCM_CUMDMODE
    close(dbg_console_fd);
FREE_01:
#endif
    return;
}

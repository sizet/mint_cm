// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../mcm_lib/mcm_lheader/mcm_type.h"
#include "../mcm_lib/mcm_lheader/mcm_size.h"
#include "../mcm_lib/mcm_lheader/mcm_connect.h"
#include "../mcm_lib/mcm_lheader/mcm_return.h"
#include "../mcm_lib/mcm_lheader/mcm_debug.h"
#include "mcm_cgi_common.h"
#include "mcm_cgi_upload_handle.h"
#include "mcm_cgi_upload_custom.h"




#if MCM_CGIEMODE | MCM_CUCDMODE
    int dbg_tty_fd_c;
    char dbg_msg_buf_c[MCM_DBG_BUFFER_SIZE];
#endif

#if MCM_CGIEMODE
    #define MCM_CEMSG(msg_fmt, msg_args...) \
        MCM_CGI_TTY_MSG(dbg_tty_fd_c, dbg_msg_buf_c, msg_fmt, ##msg_args)
#else
    #define MCM_CEMSG(msg_fmt, msg_args...)
#endif

#if MCM_CUCDMODE
    #define MCM_CUCDMSG(msg_fmt, msg_args...) \
        MCM_CGI_TTY_MSG(dbg_tty_fd_c, dbg_msg_buf_c, msg_fmt, ##msg_args)
#else
    #define MCM_CUCDMSG(msg_fmt, msg_args...)
#endif




void custom_handle_01(
    struct part_info_t *part_list)
{
    struct part_info_t *tmp_part;


#if MCM_CGIEMODE | MCM_CUCDMODE
    dbg_tty_fd_c = open(MCM_DBG_DEV_TTY, O_WRONLY);
    if(dbg_tty_fd_c == -1)
        goto FREE_01;
#endif

#if MCM_CUCDMODE
    for(tmp_part = part_list; tmp_part != NULL; tmp_part = tmp_part->next_part)
    {
        MCM_CUCDMSG("name = %s", tmp_part->name_tag);
        MCM_CUCDMSG("filename = %s", tmp_part->filename_tag);
        if(tmp_part->filename_tag != NULL)
        {
            MCM_CUCDMSG("data = [file][" MCM_DTYPE_USIZE_PF "][%p]",
                        tmp_part->data_len, tmp_part->data_con);
        }
        else
        {
            MCM_CUCDMSG("data = [string][" MCM_DTYPE_USIZE_PF "][%s]",
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

#if MCM_CGIEMODE | MCM_CUCDMODE
    close(dbg_tty_fd_c);
FREE_01:
#endif
    return;
}




struct mcm_cgi_upload_callback_t mcm_upload_callback_list[] =
{
    {"custom_upload_01", custom_handle_01},
    {"", NULL}
};

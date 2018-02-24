// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "mcm_lib/mcm_lheader/mcm_return.h"
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

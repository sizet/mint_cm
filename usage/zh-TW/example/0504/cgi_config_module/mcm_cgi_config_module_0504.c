// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <stdlib.h>
#include "../../mcm_lib/mcm_lheader/mcm_type.h"
#include "../../mcm_lib/mcm_lheader/mcm_size.h"
#include "../../mcm_lib/mcm_lheader/mcm_connect.h"
#include "../../mcm_lib/mcm_lheader/mcm_return.h"
#include "../../mcm_lib/mcm_lheader/mcm_data_exinfo_auto.h"
#include "../../mcm_lib/mcm_lulib/mcm_lulib_api.h"
#include "mcm_cgi_module_debug.h"


int find_match_rule_station(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_USIZE_TD part_level,
    MCM_DTYPE_EK_TD *part_key,
    MCM_DTYPE_EK_TD **key_list_buf,
    MCM_DTYPE_EK_TD *key_count_buf)
{
    int fret;
    char *path1;
    MCM_DTYPE_USIZE_TD pidx, req_len, rep_len;
    MCM_DTYPE_EK_TD req_data[MCM_PATH_MAX_LEVEL], *rep_data;


#if MCM_CCMEMODE | MCM_CCMDMODE
    dbg_tty_fd = open(MCM_DBG_DEV_TTY, O_WRONLY);
    if(dbg_tty_fd == -1)
        return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
#endif

    MCM_CCMDMSG("part_level = %u", part_level);

    // 依照 | part_key[0] | part_key[1] | ... | part_key[part_level - 1] |,
    // 的順序將資料傳給內部模組.
    req_len = (sizeof(MCM_DTYPE_EK_TD) * part_level);
    for(pidx = 0; pidx < part_level; pidx++)
        req_data[pidx] = part_key[pidx];

    path1 = "mcm_module_obtain_match_rule_station";
    MCM_CCMDMSG("[run] %s", path1);
    fret = mcm_lulib_run(this_lulib, path1, req_data, req_len, (void **) &rep_data, &rep_len);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CCMDMSG("call mcm_lulib_run(%s) fail", path1);
        goto FREE_01;
    }

    // 內部模組回傳的資料, 格式 : | key1 | key2 | ... | keyN |.
    *key_list_buf = rep_data;
    *key_count_buf = rep_len / sizeof(MCM_DTYPE_EK_TD);

FREE_01:
#if MCM_CCMEMODE | MCM_CCMDMODE
    close(dbg_tty_fd);
#endif
    return fret;
}

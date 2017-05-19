// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <stdio.h>
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




struct mcm_cgi_upload_callback_t mcm_upload_callback_list[] =
{
    {"", NULL}
};

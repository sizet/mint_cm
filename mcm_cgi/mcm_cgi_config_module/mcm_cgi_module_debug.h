// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_CGI_MODULE_DEBUG_H_
#define _MCM_CGI_MODULE_DEBUG_H_




#include "../mcm_cgi_common_define.h"

#if MCM_CCMEMODE | MCM_CCMDMODE
    #include <fcntl.h>
    #include <stdio.h>
    #include <string.h>
    #include <unistd.h>
    #include <sys/stat.h>
#endif




#if MCM_CCMEMODE | MCM_CCMDMODE
extern int dbg_console_fd;
extern char dbg_msg_buf[MCM_DBG_BUFFER_SIZE];
#endif

#if MCM_CCMEMODE
    #define MCM_CCMEMSG(msg_fmt, msg_args...) \
        MCM_CGI_CONSOLE_MSG(dbg_console_fd, dbg_msg_buf, msg_fmt, ##msg_args)
#else
    #define MCM_CCMEMSG(msg_fmt, msg_args...)
#endif

#if MCM_CCMDMODE
    #define MCM_CCMDMSG(msg_fmt, msg_args...) \
        MCM_CGI_CONSOLE_MSG(dbg_console_fd, dbg_msg_buf, msg_fmt, ##msg_args)
#else
    #define MCM_CCMDMSG(msg_fmt, msg_args...)
#endif




#endif

// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_CGI_MODULE_DEBUG_H_
#define _MCM_CGI_MODULE_DEBUG_H_




#include "../mcm_cgi_common_define.h"

#if MCM_CUMEMODE | MCM_CUMDMODE
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/stat.h>
#endif




#if MCM_CUMEMODE | MCM_CUMDMODE
extern int dbg_tty_fd;
extern char dbg_msg_buf[MCM_DBG_BUFFER_SIZE];
#endif

#if MCM_CUMEMODE
    #define MCM_CUMEMSG(msg_fmt, msg_args...) \
        MCM_CGI_TTY_MSG(dbg_tty_fd, dbg_msg_buf, msg_fmt, ##msg_args)
#else
    #define MCM_CUMEMSG(msg_fmt, msg_args...)
#endif

#if MCM_CUMDMODE
    #define MCM_CUMDMSG(msg_fmt, msg_args...) \
        MCM_CGI_TTY_MSG(dbg_tty_fd, dbg_msg_buf, msg_fmt, ##msg_args)
#else
    #define MCM_CUMDMSG(msg_fmt, msg_args...)
#endif




#endif

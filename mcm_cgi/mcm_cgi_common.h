// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_CGI_COMMON_H_
#define _MCM_CGI_COMMON_H_




#include "../mcm_lib/mcm_lheader/mcm_type.h"
#include "../mcm_lib/mcm_lheader/mcm_debug.h"




#if MCM_CGIEMODE | MCM_CCDMODE | MCM_CUHDMODE | MCM_CUCDMODE
    #define MCM_CGI_TTY_MSG(tty_fd, msg_buf, msg_fmt, msg_args...) \
        do                                                                \
        {                                                                 \
            snprintf(msg_buf, sizeof(msg_buf), "%s(%04u): " msg_fmt "\n", \
                     __FILE__, __LINE__, ##msg_args);                     \
            write(tty_fd, msg_buf, strlen(msg_buf));                      \
        }                                                                 \
        while(0)
#endif

#define MCM_CGI_AEMSG(error_code, reload_page, msg_fmt, msg_args...) \
    mcm_cgi_alert_error_msg(error_code, reload_page, msg_fmt, __FILE__, __LINE__, ##msg_args)

#define MCM_CGI_REQUEST_METHOD_KEY "REQUEST_METHOD"
#define MCM_CGI_QUERY_STRING_KEY   "QUERY_STRING"
#define MCM_CGI_POST_KEY           "POST"
#define MCM_CGI_CONTENT_TYPE_KEY   "CONTENT_TYPE"
#define MCM_CGI_CONTENT_LENGTH_KRY "CONTENT_LENGTH"




int mcm_cgi_fill_response_header(
    MCM_DTYPE_BOOL_TD fill_content_text,
    MCM_DTYPE_BOOL_TD fill_connection_close,
    int call_ret);

int mcm_cgi_alert_error_msg(
    int error_code,
    MCM_DTYPE_BOOL_TD reload_page,
    char *msg_fmt,
    char *msg_file,
    MCM_DTYPE_USIZE_TD msg_line,
    ...);




#endif

// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_CGI_COMMON_EXTERN_H_
#define _MCM_CGI_COMMON_EXTERN_H_




#include "mcm_cgi_common_define.h"




#define MCM_CGI_AEMSG(error_code, reload_page, msg_fmt, msg_args...) \
    mcm_cgi_alert_error_msg(error_code, reload_page, msg_fmt, __FILE__, __LINE__, ##msg_args)




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

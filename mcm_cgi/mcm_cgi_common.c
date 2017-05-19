// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <stdio.h>
#include <stdarg.h>
#include "../mcm_lib/mcm_lheader/mcm_type.h"
#include "../mcm_lib/mcm_lheader/mcm_return.h"
#include "mcm_cgi_common.h"



#define MCM_CRLF_KEY             "\r\n"
#define MCM_CONTENT_TEXT_KEY     "Content-Type: text/plain" MCM_CRLF_KEY
#define MCM_CONNECTION_CLOSE_KEY "Connection: close" MCM_CRLF_KEY
#define MCM_SPLIT_KEY            ':'




int mcm_cgi_fill_response_header(
    MCM_DTYPE_BOOL_TD fill_content_text,
    MCM_DTYPE_BOOL_TD fill_connection_close,
    int ret_code)
{
    if(fill_content_text != 0)
        printf(MCM_CONTENT_TEXT_KEY);
    if(fill_connection_close != 0)
        printf(MCM_CONNECTION_CLOSE_KEY);
    printf(MCM_CRLF_KEY);

    printf("%d%c", ret_code, MCM_SPLIT_KEY);

    return 0;
}

int mcm_cgi_alert_error_msg(
    int error_code,
    MCM_DTYPE_BOOL_TD reload_page,
    char *msg_fmt,
    char *msg_file,
    MCM_DTYPE_USIZE_TD msg_line,
    ...)
{
    va_list arg_list;


    mcm_cgi_fill_response_header(1, 1, error_code);

    printf("alert(\"%s(%04u):\\n", msg_file, msg_line);
    va_start(arg_list, msg_line);
    vprintf(msg_fmt, arg_list);
    va_end(arg_list);
    printf("\");\n");

    if(reload_page != 0)
        printf("window.location.reload();\n");

    return MCM_RCODE_PASS;
}

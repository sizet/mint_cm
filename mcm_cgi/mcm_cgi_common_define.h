// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_CGI_COMMON_DEFINE_H_
#define _MCM_CGI_COMMON_DEFINE_H_




#include "../mcm_lib/mcm_lheader/mcm_type.h"
#include "../mcm_lib/mcm_lheader/mcm_debug.h"




#if MCM_CGIEMODE | MCM_CGIECTMODE | \
    MCM_CCDMODE | \
    MCM_CUDMODE | MCM_CUMEMODE | MCM_CUMDMODE
    #define MCM_CGI_TTY_MSG(tty_fd, msg_buf, msg_fmt, msg_args...) \
        do                                                                \
        {                                                                 \
            snprintf(msg_buf, sizeof(msg_buf), "%s(%04u): " msg_fmt "\n", \
                     __FILE__, __LINE__, ##msg_args);                     \
            write(tty_fd, msg_buf, strlen(msg_buf));                      \
        }                                                                 \
        while(0)
#endif

#define MCM_CGI_REQUEST_METHOD_KEY "REQUEST_METHOD"
#define MCM_CGI_QUERY_STRING_KEY   "QUERY_STRING"
#define MCM_CGI_POST_KEY           "POST"
#define MCM_CGI_CONTENT_TYPE_KEY   "CONTENT_TYPE"
#define MCM_CGI_CONTENT_LENGTH_KRY "CONTENT_LENGTH"




struct part_info_t
{
    char *name_tag;
    char *filename_tag;
    char *data_con;
    MCM_DTYPE_USIZE_TD data_len;
    struct part_info_t *prev_part;
    struct part_info_t *next_part;
};




#endif

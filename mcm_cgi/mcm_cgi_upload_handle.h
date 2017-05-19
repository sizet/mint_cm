// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_CGI_UPLOAD_HANDLE_H_
#define _MCM_CGI_UPLOAD_HANDLE_H_




#include "../mcm_lib/mcm_lheader/mcm_type.h"




struct part_info_t
{
    char *name_tag;
    char *filename_tag;
    char *data_con;
    MCM_DTYPE_USIZE_TD data_len;
    struct part_info_t *prev_part;
    struct part_info_t *next_part;
};

struct mcm_cgi_upload_callback_t
{
    char *cb_name;
    void (*cb_function)(struct part_info_t *part_list);
};




#endif

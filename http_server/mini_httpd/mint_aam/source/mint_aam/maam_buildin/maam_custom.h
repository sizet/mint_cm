/*
Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
This file is part of the MintAAM.
Some rights reserved. See README.
*/

#ifndef _MAAM_CUSTOM_H_
#define _MAAM_CUSTOM_H_




#include "maam_local.h"




extern char *maam_html_key;
extern size_t maam_html_len;
extern char *maam_page_login;
extern char *maam_page_index;
extern struct maam_access_t maam_access_allow_get_html_list[];
extern struct maam_access_t maam_access_deny_get_other_list[];
extern struct maam_access_t maam_access_allow_post_list[];




int maam_verify_account(
    struct maam_login_t *login_info,
    int *verify_result_buf);




#endif

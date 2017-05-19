/*
Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
This file is part of the MintAAM.
Some rights reserved. See README.
*/

#ifndef _MAAM_HANDLE_H_
#define _MAAM_HANDLE_H_




#include "maam_local.h"




int maam_init(
    int sm_id,
    char *sm_mutex_path);

int maam_action(
    char *req_path);

int maam_handle(
    int sm_id,
    char *sm_mutex_path,
    struct maam_req_data_t *req_data_info,
    struct maam_rep_data_t *rep_data_info);




#endif

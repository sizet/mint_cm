// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_SERVICE_HANDLE_EXTERN_H_
#define _MCM_SERVICE_HANDLE_EXTERN_H_




#include "mcm_service_handle_define.h"




int mcm_service_init(
    char *socket_path,
    MCM_DTYPE_USIZE_TD max_session_count);

int mcm_service_exit(
    void);

int mcm_service_shutdown(
    void);

int mcm_service_run_wait(
    void);

int mcm_service_run_post(
    void);

int mcm_service_response_init(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_USIZE_TD buf_size);

int mcm_service_response_exit(
    struct mcm_service_session_t *this_session);




#endif

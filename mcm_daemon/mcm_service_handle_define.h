// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_SERVICE_HANDLE_DEFINE_H_
#define _MCM_SERVICE_HANDLE_DEFINE_H_




#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include "mcm_lib/mcm_lheader/mcm_type.h"
#include "mcm_lib/mcm_lheader/mcm_connect.h"




#define MCM_SERVICE_SHUTDOWN_SIGNAL SIGUSR1




struct mcm_service_session_t
{
    int socket_fd;
    pthread_t thread_id;
    sem_t access_mutex;
    MCM_DTYPE_LIST_TD call_from;
    MCM_DTYPE_LIST_TD session_permission;
    MCM_DTYPE_USIZE_TD session_stack_size;
    MCM_DTYPE_BOOL_TD need_shutdown;
    void *pkt_buf;
    void *pkt_offset;
    MCM_DTYPE_USIZE_TD pkt_len;
    MCM_DTYPE_USIZE_TD pkt_size;
    MCM_DTYPE_LIST_TD req_type;
    char *req_path;
    char *req_other_path;
    void *req_data_con;
    MCM_DTYPE_USIZE_TD req_data_len;
    void *cache_buf;
    MCM_DTYPE_USIZE_TD cache_size;
    void *rep_data_buf;
    MCM_DTYPE_USIZE_TD rep_data_len;
    struct mcm_service_session_t *prev_session;
    struct mcm_service_session_t *next_session;
};




#endif

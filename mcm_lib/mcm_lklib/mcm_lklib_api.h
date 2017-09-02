// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_LKLIB_API_H_
#define _MCM_LKLIB_API_H_




#include "../mcm_lheader/mcm_type.h"




struct mcm_lklib_lib_t
{
    char *socket_path;
    MCM_DTYPE_LIST_TD call_from;
    MCM_DTYPE_LIST_TD session_permission;
    MCM_DTYPE_USIZE_TD session_stack_size;
    struct socket *sock_fp;
    void *pkt_buf;
    void *pkt_offset;
    MCM_DTYPE_USIZE_TD pkt_len;
    MCM_DTYPE_USIZE_TD pkt_size;
    MCM_DTYPE_LIST_TD rep_code;
    char *rep_msg_con;
    MCM_DTYPE_USIZE_TD rep_msg_len;
};




int mcm_lklib_init(
    struct mcm_lklib_lib_t *this_lklib);

int mcm_lklib_exit(
    struct mcm_lklib_lib_t *this_lklib);

int mcm_lklib_get_alone(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path,
    void *data_buf);

int mcm_lklib_set_alone(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len);

int mcm_lklib_get_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path,
    void *data_buf);

int mcm_lklib_set_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len);

int mcm_lklib_add_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path,
    char *insert_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len);

int mcm_lklib_del_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path);

int mcm_lklib_get_all_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *mix_path,
    MCM_DTYPE_EK_TD *count_buf,
    void **data_buf);

int mcm_lklib_del_all_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *mix_path);

int mcm_lklib_get_max_count(
    struct mcm_lklib_lib_t *this_lklib,
    char *mask_path,
    MCM_DTYPE_EK_TD *count_buf);

int mcm_lklib_get_count(
    struct mcm_lklib_lib_t *this_lklib,
    char *mix_path,
    MCM_DTYPE_EK_TD *count_buf);

int mcm_lklib_get_usable_key(
    struct mcm_lklib_lib_t *this_lklib,
    char *mix_path,
    MCM_DTYPE_EK_TD *key_buf);

int mcm_lklib_update(
    struct mcm_lklib_lib_t *this_lklib);

int mcm_lklib_save(
    struct mcm_lklib_lib_t *this_lklib,
    MCM_DTYPE_BOOL_TD force_save);

int mcm_lklib_run(
    struct mcm_lklib_lib_t *this_lklib,
    char *module_function);

int mcm_lklib_shutdown(
    struct mcm_lklib_lib_t *this_lklib);

int mcm_lklib_check_store_file(
    struct mcm_lklib_lib_t *this_lklib,
    char *file_path,
    MCM_DTYPE_LIST_TD *store_result_buf,
    char *store_version_buf,
    MCM_DTYPE_USIZE_TD store_version_size);




#endif

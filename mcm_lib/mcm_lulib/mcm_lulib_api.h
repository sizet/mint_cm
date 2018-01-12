// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_LULIB_API_H_
#define _MCM_LULIB_API_H_




#include "../mcm_lheader/mcm_type.h"




struct mcm_lulib_lib_t
{
    char *socket_path;
    MCM_DTYPE_LIST_TD call_from;
    MCM_DTYPE_LIST_TD session_permission;
    MCM_DTYPE_USIZE_TD session_stack_size;
    int sock_fd;
    void *pkt_buf;
    MCM_DTYPE_USIZE_TD pkt_len;
    MCM_DTYPE_USIZE_TD pkt_size;
};




extern MCM_DTYPE_BOOL_TD mcm_lulib_show_msg;




int mcm_lulib_init(
    struct mcm_lulib_lib_t *this_lulib);

int mcm_lulib_exit(
    struct mcm_lulib_lib_t *this_lulib);

int mcm_lulib_get_alone(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void *data_buf);

int mcm_lulib_set_alone(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len);

int mcm_lulib_get_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void *data_buf);

int mcm_lulib_set_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len);

int mcm_lulib_add_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    char *insert_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len);

int mcm_lulib_del_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path);

int mcm_lulib_get_all_key(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path,
    MCM_DTYPE_EK_TD **key_buf,
    MCM_DTYPE_EK_TD *count_buf);

int mcm_lulib_get_all_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path,
    void **data_buf,
    MCM_DTYPE_EK_TD *count_buf);

int mcm_lulib_del_all_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path);

int mcm_lulib_get_max_count(
    struct mcm_lulib_lib_t *this_lulib,
    char *mask_path,
    MCM_DTYPE_EK_TD *count_buf);

int mcm_lulib_get_count(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path,
    MCM_DTYPE_EK_TD *count_buf);

int mcm_lulib_get_usable_key(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path,
    MCM_DTYPE_EK_TD *key_buf);

int mcm_lulib_update(
    struct mcm_lulib_lib_t *this_lulib);

int mcm_lulib_save(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_BOOL_TD force_save);

int mcm_lulib_run(
    struct mcm_lulib_lib_t *this_lulib,
    char *module_function,
    void *req_data_con,
    MCM_DTYPE_USIZE_TD req_data_len,
    void **rep_data_buf,
    MCM_DTYPE_USIZE_TD *rep_data_len_buf);

int mcm_lulib_shutdown(
    struct mcm_lulib_lib_t *this_lulib);

int mcm_lulib_check_store_file(
    struct mcm_lulib_lib_t *this_lulib,
    char *file_path,
    MCM_DTYPE_LIST_TD *store_result_buf,
    char *store_version_buf,
    MCM_DTYPE_USIZE_TD store_version_size);

int mcm_lulib_check_mask_path(
    struct mcm_lulib_lib_t *this_lulib,
    char *mask_path);

int mcm_lulib_get_path_max_length(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_USIZE_TD *max_len_buf);

int mcm_lulib_get_list_name(
    struct mcm_lulib_lib_t *this_lulib,
    char *mask_path,
    void **data_buf_buf,
    MCM_DTYPE_USIZE_TD *data_len_buf);

int mcm_lulib_get_list_type(
    struct mcm_lulib_lib_t *this_lulib,
    char *mask_path,
    void **data_buf_buf,
    MCM_DTYPE_USIZE_TD *data_len_buf);

int mcm_lulib_get_list_value(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void **data_buf_buf,
    MCM_DTYPE_USIZE_TD *data_len_buf);

int mcm_lulib_set_any_type_alone(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    char *data_con);

int mcm_lulib_get_with_type_alone(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    MCM_DTYPE_LIST_TD *type_buf,
    void **data_buf_buf,
    MCM_DTYPE_USIZE_TD *data_len_buf);

int mcm_lulib_do_get_alone(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void *data_buf);

int mcm_lulib_do_set_alone(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len);

int mcm_lulib_do_get_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void *data_buf);

int mcm_lulib_do_set_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len);

int mcm_lulib_do_add_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    char *insert_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len);

int mcm_lulib_do_del_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path);

int mcm_lulib_do_get_all_key(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path,
    MCM_DTYPE_EK_TD **key_buf,
    MCM_DTYPE_EK_TD *count_buf);

int mcm_lulib_do_get_all_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path,
    void **data_buf,
    MCM_DTYPE_EK_TD *count_buf);

int mcm_lulib_do_del_all_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path);

int mcm_lulib_do_get_max_count(
    struct mcm_lulib_lib_t *this_lulib,
    char *mask_path,
    MCM_DTYPE_EK_TD *count_buf);

int mcm_lulib_do_get_count(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path,
    MCM_DTYPE_EK_TD *count_buf);

int mcm_lulib_do_get_usable_key(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path,
    MCM_DTYPE_EK_TD *key_buf);

int mcm_lulib_do_update(
    struct mcm_lulib_lib_t *this_lulib);

int mcm_lulib_do_save(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_BOOL_TD force_save);

int mcm_lulib_do_run(
    struct mcm_lulib_lib_t *this_lulib,
    char *module_function,
    void *req_data_con,
    MCM_DTYPE_USIZE_TD req_data_len,
    void **rep_data_buf,
    MCM_DTYPE_USIZE_TD *rep_data_len_buf);

int mcm_lulib_do_shutdown(
    struct mcm_lulib_lib_t *this_lulib);

int mcm_lulib_do_check_store_file(
    struct mcm_lulib_lib_t *this_lulib,
    char *file_path,
    MCM_DTYPE_LIST_TD *store_result_buf,
    char *store_version_buf,
    MCM_DTYPE_USIZE_TD store_version_size);




#endif

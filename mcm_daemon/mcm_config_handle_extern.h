// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_CONFIG_HANDLE_EXTERN_H_
#define _MCM_CONFIG_HANDLE_EXTERN_H_




#include "mcm_service_handle_define.h"
#include "mcm_config_handle_define.h"




extern char *mcm_config_store_default_profile_path;
extern char *mcm_config_store_current_profile_path;
extern MCM_DTYPE_LIST_TD mcm_config_store_profile_error_handle;
extern MCM_DTYPE_LIST_TD mcm_config_store_profile_save_mode;
extern MCM_DTYPE_LIST_TD mcm_config_store_profile_source;
extern struct mcm_config_base_t mcm_config_base_data;
extern struct mcm_config_model_group_t *mcm_config_root_model_group;
extern struct mcm_config_store_t *mcm_config_root_store;
extern MCM_DTYPE_BOOL_TD mcm_config_data_error;
extern void *mcm_config_module_fp;




int mcm_config_load_model(
    char *file_path);

int mcm_config_free_model(
    void);

int mcm_config_load_store(
    void);

int mcm_config_save_store(
    void);

int mcm_config_free_store(
    void);

int mcm_config_load_module(
    char *file_path);

int mcm_config_free_module(
    void);

int mcm_config_store_profile_error_process(
    MCM_DTYPE_BOOL_TD *exit_buf);

int mcm_config_update(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD update_type);

int mcm_config_save(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD update_type,
    MCM_DTYPE_BOOL_TD check_save_mode,
    MCM_DTYPE_BOOL_TD force_save);

int mcm_config_shutdown(
    struct mcm_service_session_t *this_session);

int mcm_config_remove_store_current_profile(
    struct mcm_service_session_t *this_session);

int mcm_config_find_group_by_mask(
    struct mcm_service_session_t *this_session,
    char *mask_path,
    struct mcm_config_model_group_t **self_model_group_buf);

int mcm_config_find_entry_use_mix(
    struct mcm_service_session_t *this_session,
    char *mix_path,
    MCM_DTYPE_LIST_TD check_number,
    struct mcm_config_model_group_t **self_model_group_buf,
    struct mcm_config_store_t **self_store_list_buf,
    struct mcm_config_store_t **self_store_tree_buf,
    struct mcm_config_store_t **parent_store_buf);

int mcm_config_find_entry_by_mix(
    struct mcm_service_session_t *this_session,
    char *mix_path,
    struct mcm_config_model_group_t **self_model_group_buf,
    struct mcm_config_store_t **self_store_list_buf,
    struct mcm_config_store_t **self_store_tree_buf,
    struct mcm_config_store_t **parent_store_buf);

int mcm_config_find_alone_use_full(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_LIST_TD check_number,
    struct mcm_config_model_group_t **self_model_group_buf,
    struct mcm_config_model_member_t **self_model_member_buf,
    struct mcm_config_store_t **self_store_buf);

int mcm_config_find_alone_by_full(
    struct mcm_service_session_t *this_session,
    char *full_path,
    struct mcm_config_model_group_t **self_model_group_buf,
    struct mcm_config_model_member_t **self_model_member_buf,
    struct mcm_config_store_t **self_store_buf);

int mcm_config_find_entry_use_full(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_LIST_TD check_non_last_number,
    MCM_DTYPE_LIST_TD check_last_number,
    MCM_DTYPE_BOOL_TD check_last_exist,
    struct mcm_config_model_group_t **self_model_group_buf,
    struct mcm_config_store_t **self_store_buf,
    struct mcm_config_store_t **parent_store_buf,
    MCM_DTYPE_EK_TD *self_ik_buf);

int mcm_config_find_entry_by_full(
    struct mcm_service_session_t *this_session,
    char *full_path,
    struct mcm_config_model_group_t **self_model_group_buf,
    struct mcm_config_store_t **self_store_buf);

int mcm_config_get_alone_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_model_member_t *this_model_member,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_FLAG_TD data_access,
    void *data_buf);

int mcm_config_get_alone_by_path(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_FLAG_TD data_access,
    void *data_buf);

int mcm_config_set_alone_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_model_member_t *this_model_member,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_FLAG_TD data_access,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len);

int mcm_config_set_alone_by_path(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_FLAG_TD data_access,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len);

int mcm_config_get_entry_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_FLAG_TD data_access,
    void *data_buf);

int mcm_config_get_entry_by_path(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_FLAG_TD data_access,
    void *data_buf);

int mcm_config_set_entry_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_FLAG_TD data_access,
    void *data_con);

int mcm_config_set_entry_by_path(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_FLAG_TD data_access,
    void *data_con);

int mcm_config_add_entry_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *parent_store,
    MCM_DTYPE_FLAG_TD data_access,
    MCM_DTYPE_EK_TD this_key,
    void *data_con,
    struct mcm_config_store_t **new_store_buf);

int mcm_config_add_entry_by_path(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_FLAG_TD data_access,
    void *data_con);

int mcm_config_del_entry_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_FLAG_TD data_access);

int mcm_config_del_entry_by_path(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_FLAG_TD data_access);

int mcm_config_get_all_entry_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *parent_store,
    MCM_DTYPE_FLAG_TD data_access,
    MCM_DTYPE_EK_TD *count_buf,
    void **data_buf);

int mcm_config_get_all_entry_by_path(
    struct mcm_service_session_t *this_session,
    char *mix_path,
    MCM_DTYPE_FLAG_TD data_access,
    MCM_DTYPE_EK_TD *count_buf,
    void **data_buf);

int mcm_config_del_all_entry_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *parent_store,
    MCM_DTYPE_FLAG_TD data_access);

int mcm_config_del_all_entry_by_path(
    struct mcm_service_session_t *this_session,
    char *mix_path,
    MCM_DTYPE_FLAG_TD data_access);

int mcm_config_get_max_count_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    MCM_DTYPE_EK_TD *count_buf);

int mcm_config_get_max_count_by_path(
    struct mcm_service_session_t *this_session,
    char *mask_path,
    MCM_DTYPE_EK_TD *count_buf);

int mcm_config_get_count_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *parent_store,
    MCM_DTYPE_EK_TD *count_buf);

int mcm_config_get_count_by_path(
    struct mcm_service_session_t *this_session,
    char *mix_path,
    MCM_DTYPE_EK_TD *count_buf);

int mcm_config_get_usable_key_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *parent_store,
    MCM_DTYPE_EK_TD *key_buf);

int mcm_config_get_usable_key_by_path(
    struct mcm_service_session_t *this_session,
    char *mix_path,
    MCM_DTYPE_EK_TD *key_buf);

int mcm_config_get_alone_status_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_model_member_t *this_model_member,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_DS_TD *data_buf);

int mcm_config_get_alone_status_by_path(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_DS_TD *data_buf);

int mcm_config_get_entry_self_status_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_DS_TD *data_buf);

int mcm_config_get_entry_self_status_by_path(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_DS_TD *data_buf);

int mcm_config_get_entry_all_status_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    void *data_buf);

int mcm_config_get_entry_all_status_by_path(
    struct mcm_service_session_t *this_session,
    char *full_path,
    void *data_buf);

int mcm_config_set_alone_status(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_model_member_t *this_model_member,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_LIST_TD assign_type,
    MCM_DTYPE_DS_TD data_con);

int mcm_config_set_entry_self_status(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_LIST_TD assign_type,
    MCM_DTYPE_DS_TD data_con);

int mcm_config_set_entry_all_status(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_LIST_TD assign_type,
    MCM_DTYPE_DS_TD data_con,
    MCM_DTYPE_BOOL_TD skip_key);

int mcm_config_check_store_file(
    struct mcm_service_session_t *this_session,
    char *file_path,
    MCM_DTYPE_LIST_TD *store_result_buf,
    char *store_version_buf,
    MCM_DTYPE_USIZE_TD store_version_size);

int mcm_config_get_path_max_length(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_USIZE_TD *max_len_buf);

int mcm_config_get_list_name_size(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    MCM_DTYPE_USIZE_TD *size_buf);

int mcm_config_get_list_name_data(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    void *data_buf);

int mcm_config_get_list_type_size(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    MCM_DTYPE_USIZE_TD *size_buf);

int mcm_config_get_list_type_data(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    void *data_buf);

int mcm_config_get_list_value_size(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_USIZE_TD *size_buf);

int mcm_config_get_list_value_data(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    void *data_buf);

int mcm_config_set_any_type_alone_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_model_member_t *this_model_member,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_FLAG_TD data_access,
    char *data_con,
    MCM_DTYPE_USIZE_TD data_len);




#endif

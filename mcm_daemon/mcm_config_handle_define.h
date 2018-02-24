// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_CONFIG_HANDLE_DEFINE_H_
#define _MCM_CONFIG_HANDLE_DEFINE_H_




#include "mcm_lib/mcm_lheader/mcm_type.h"
#include "mcm_lib/mcm_lheader/mcm_size.h"




// 紀錄 member 資訊.
struct mcm_config_model_member_t
{
    // member 名稱.
    char *member_name;
    // member 名稱長度.
    MCM_DTYPE_USIZE_TD member_nlen;
    // member 類型 (MCM_DATA_TYPE_INDEX).
    MCM_DTYPE_LIST_TD member_type;
    // member 的儲存大小.
    MCM_DTYPE_USIZE_TD member_size;
    // member 的預設值內容.
    void *member_default_con;
    // member 的預設值長度.
    MCM_DTYPE_USIZE_TD member_default_len;
    // member 的資料狀態在 mcm_lib/mcm_lheader/mcm_config_data_define_info_auto.h 中 -
    // struct mcm_ds_xxxx 的偏移植.
    MCM_DTYPE_USIZE_TD offset_in_status;
    // member 的資料數值在 mcm_lib/mcm_lheader/mcm_config_data_define_info_auto.h 中 -
    // struct mcm_dv_xxxx 的偏移植.
    MCM_DTYPE_USIZE_TD offset_in_value;
    // 鍊結下一個 member (link-list).
    struct mcm_config_model_member_t *next_model_member;
    // avl-tree : 樹高.
    MCM_DTYPE_LIST_TD tree_height_member;
    // avl-tree : 鍊結左 member.
    struct mcm_config_model_member_t *ltree_model_member;
    // avl-tree : 鍊結右 member.
    struct mcm_config_model_member_t *rtree_model_member;
};

// 紀錄 group 資訊.
struct mcm_config_model_group_t
{
    // group 的名稱.
    char *group_name;
    // group 的名稱的長度.
    MCM_DTYPE_USIZE_TD group_nlen;
    // group 的類型 (MCM_DATA_TYPE_INDEX).
    MCM_DTYPE_LIST_TD group_type;
    // group 的最大 entry 數目.
    MCM_DTYPE_EK_TD group_max;
    // group 資料是否要記錄到檔案.
    MCM_DTYPE_BOOL_TD group_save;
    // group 的 member 數目 (不含 ek 類型的 member).
    MCM_DTYPE_USIZE_TD member_real_count;
    // 鍊結 head member (link-list).
    struct mcm_config_model_member_t *model_member_list;
    // 鍊結 root member (avl-tree).
    struct mcm_config_model_member_t *model_member_tree;
    // group 的 member 中 ek 類型的 member 在 -
    // mcm_lib/mcm_lheader/mcm_config_data_define_info_auto.h 中 struct mcm_ds_xxxx 的偏移植.
    MCM_DTYPE_USIZE_TD entry_key_offset_status;
    // group 的 member 中 ek 類型的 member 在 -
    // mcm_lib/mcm_lheader/mcm_config_data_define_info_auto.h 中 struct mcm_dv_xxxx 的偏移植.
    MCM_DTYPE_USIZE_TD entry_key_offset_value;
    // mcm_lib/mcm_lheader/mcm_config_data_define_info_auto.h 中 struct mcm_ds_xxxx 的大小.
    MCM_DTYPE_USIZE_TD data_status_size;
    // mcm_lib/mcm_lheader/mcm_config_data_define_info_auto.h 中 struct mcm_dv_xxxx 的大小.
    MCM_DTYPE_USIZE_TD data_value_size;
    // 在 parent-gorup 中的 child-group 的順序.
    MCM_DTYPE_USIZE_TD store_index_in_parent;
    // 有幾個 child-group.
    MCM_DTYPE_USIZE_TD store_child_count;
    // 鍊結 parent-group.
    struct mcm_config_model_group_t *parent_model_group;
    // 鍊結 head child-group (link-list).
    struct mcm_config_model_group_t *child_model_group_list;
    // 鍊結 root child-group (avl-tree).
    struct mcm_config_model_group_t *child_model_group_tree;
    // 鍊結下一個 group.
    struct mcm_config_model_group_t *next_model_group;
    // avl-tree : 樹高.
    MCM_DTYPE_LIST_TD tree_height_group;
    // avl-tree : 鍊結左 group.
    struct mcm_config_model_group_t *ltree_model_group;
    // avl-tree : 鍊結右 group.
    struct mcm_config_model_group_t *rtree_model_group;
};

// 紀錄 store 資訊.
struct mcm_config_store_t
{
    // 使用哪個 model.
    struct mcm_config_model_group_t *link_model_group;
    // 資料的狀態的緩衝.
    void *data_status;
    // 資料庫的系統資料的緩衝.
    void *data_value_sys;
    // 資料庫的現在資料的緩衝.
    void *data_value_new;
    // 紀錄每個 child-store 的 head (link-list) 的緩衝.
    struct mcm_config_store_t **child_store_list_head_array;
    // 紀錄每個 child-store 的 tail (link-list) 的緩衝.
    struct mcm_config_store_t **child_store_list_tail_array;
    // 紀錄每個 child-store 的 root (rb-tree) 的緩衝.
    struct mcm_config_store_t **child_store_tree_array;
    // 紀錄每個 child-store 的 數目.
    MCM_DTYPE_EK_TD *child_store_count_array;
    // 鍊結 parent-store.
    struct mcm_config_store_t *parent_store;
    // 鍊結上一個 store (link-list).
    struct mcm_config_store_t *prev_store;
    // 鍊結下一個 store (link-list).
    struct mcm_config_store_t *next_store;
    // rb-tree : 節點顏色.
    MCM_DTYPE_LIST_TD tree_color_store;
    // rb-tree : 鍊結 parent-store.
    struct mcm_config_store_t *ptree_store;
    // rb-tree : 鍊結左 store.
    struct mcm_config_store_t *ltree_store;
    // rb-tree : 鍊結右 store.
    struct mcm_config_store_t *rtree_store;
    // 紀錄此 store 是否需要更新 (被使用 MCM_DACCESS_NEW 修改資料).
    MCM_DTYPE_BOOL_TD need_update;
    // 鍊結上一個 update_store (link-list).
    struct mcm_config_store_t *prev_update_store;
    // 鍊結下一個 update_store (link-list).
    struct mcm_config_store_t *next_update_store;
};

// 紀錄額外資料.
struct mcm_config_base_t
{
    // 資料檔內的版本.
    char profile_current_version[MCM_BASE_VERSION_BUFFER_SIZE];
};




#endif

// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../mcm_lib/mcm_lheader/mcm_type.h"
#include "../mcm_lib/mcm_lheader/mcm_keyword.h"
#include "../mcm_lib/mcm_lheader/mcm_control.h"
#include "../mcm_lib/mcm_lheader/mcm_connect.h"
#include "../mcm_lib/mcm_lheader/mcm_return.h"
#include "../mcm_lib/mcm_lheader/mcm_limit.h"
#include "../mcm_lib/mcm_lheader/mcm_debug.h"
#include "mcm_data_ininfo_auto.h"
#include "mcm_daemon_extern.h"
#include "mcm_service_handle_define.h"
#include "mcm_action_handle_extern.h"
#include "mcm_config_handle_extern.h"




#define MCM_READ_BUFFER_UNIT_SIZE 1024

#define MCM_TREE_MAX(lh, rh) ((lh) > (rh) ? lh : rh)
#define MCM_TREE_HEIGHT_GROUP(node) ((node == NULL) ? -1 : (node)->tree_height_group)
#define MCM_TREE_HEIGHT_MEMBER(node) ((node == NULL) ? -1 : (node)->tree_height_member)

#define MCM_SFILE_DEFAULT_KEY "default"
#define MCM_SFILE_CURRENT_KEY "current"
#define MCM_SFILE_VERIFY_KEY  "verify"

#define MCM_SSOURCE(source_type) \
    (source_type == MCM_FSOURCE_DEFAULT ? MCM_SFILE_DEFAULT_KEY :                          \
        source_type == MCM_FSOURCE_CURRENT ? MCM_SFILE_CURRENT_KEY : MCM_SFILE_VERIFY_KEY)
#define MCM_SPROFILE_ERROR_PREFIX_MSG \
    "\nstore profile error [%s], line " MCM_DTYPE_USIZE_PF ", code %s :\n"

#define MCM_GET_ENTRY_SELF_STATUS(this_model_group, this_store, status_buf) \
    do                                                                              \
    {                                                                               \
        status_buf = *((MCM_DTYPE_DS_TD *)                                          \
            (this_store->data_status + this_model_group->entry_key_offset_status)); \
    }                                                                               \
    while(0)

#define MCM_GET_ALONE_STATUS(self_model_member, this_store, status_buf) \
    do                                                                        \
    {                                                                         \
        status_buf = *((MCM_DTYPE_DS_TD *)                                    \
            (this_store->data_status + self_model_member->offset_in_status)); \
    }                                                                         \
    while(0)

enum MCM_TREE_COLOR
{
    MCM_TCOLOR_RED = 0,
    MCM_TCOLOR_BLACK
};

enum MCM_MODEL_NODE_TYPE
{
    MCM_MNODE_GROUP = 0,
    MCM_MNODE_MEMBER
};

enum MCM_CHECK_PART_PATH_METHOD
{
    MCM_CPPATH_NAME = 0,
    MCM_CPPATH_IK,
    MCM_CPPATH_MASK
};

enum MCM_MODIFY_OBJECT
{
    MCM_MOBJECT_ALONE = 0,
    MCM_MOBJECT_ENTRY
};

enum MCM_DATA_MODIFY_METHOD
{
    MCM_DMODIFY_SET_NEW = 0,
    MCM_DMODIFY_ADD_NEW,
    MCM_DMODIFY_DEL_NEW
};




struct mcm_load_store_t
{
    struct mcm_config_model_group_t *link_model_group;
    struct mcm_config_store_t *link_store;
    MCM_DTYPE_EK_TD target_key;
};

#if MCM_CFDMODE
struct mcm_dbg_status_msg_t
{
    MCM_DTYPE_DS_TD status_int;
    char *status_str;
};
struct mcm_dbg_status_msg_t mcm_dbg_status_msg_list[] =
{
    {MCM_DSASSIGN_SET, "set"},
    {MCM_DSASSIGN_ADD, "add"},
    {MCM_DSASSIGN_DEL, "del"},
    {0, ""},
    {MCM_DSCHANGE_SET,              "CHANGE_SET"},
    {MCM_DSCHANGE_ADD,              "CHANGE_ADD"},
    {MCM_DSCHANGE_DEL,              "CHANGE_DEL"},
    {MCM_DSERROR_LOSE_ENTRY,        "ERROR_LOSE_ENTRY"},
    {MCM_DSERROR_DUPLIC_ENTRY,      "ERROR_DUPLIC_ENTRY"},
    {MCM_DSERROR_LOSE_PARENT,       "ERROR_LOSE_PARENT"},
    {MCM_DSERROR_UNKNOWN_PARAMETER, "ERROR_UNKNOWN_PARAMETER"},
    {MCM_DSERROR_UNKNOWN_MEMBER,    "ERROR_UNKNOWN_MEMBER"},
    {MCM_DSERROR_LOSE_MEMBER,       "ERROR_LOSE_MEMBER"},
    {MCM_DSERROR_DUPLIC_MEMBER,     "ERROR_DUPLIC_MEMBER"},
    {MCM_DSERROR_INVALID_VALUE,     "ERROR_INVALID_VALUE"},
    {0, ""}
};
#endif




// 儲存預設資料的檔案的路徑.
char *mcm_config_store_default_profile_path = NULL;
// 儲存目前資料的檔案的路徑.
char *mcm_config_store_current_profile_path = NULL;
// 錯誤處理的方式.
MCM_DTYPE_LIST_TD mcm_config_store_profile_error_handle = -1;
// 修改資料後是否自動儲存.
MCM_DTYPE_LIST_TD mcm_config_store_profile_save_mode = -1;
// 紀錄資料是從哪個檔案讀取的 (MCM_FILE_SOURCE_TABLE).
MCM_DTYPE_LIST_TD mcm_config_store_profile_source = -1;
// 紀錄額外資料.
struct mcm_config_base_t mcm_config_base_data;
// struct mcm_config_model_group_t 的根節點.
struct mcm_config_model_group_t *mcm_config_root_model_group = NULL;
// struct mcm_config_store_t 的根節點.
struct mcm_config_store_t *mcm_config_root_store = NULL;
// 檔案資料是否錯誤.
MCM_DTYPE_BOOL_TD mcm_config_data_error = 0;
// 是否有使用 MCM_DACCESS_NEW 修改資料, 使用 MCM_DACCESS_NEW 修改後, 
// 須要做同步資料或放棄資料的處理, 紀錄被修改的 store 的串列.
struct mcm_config_store_t *mcm_update_store_head = NULL, *mcm_update_store_tail = NULL;
// 是否有使用 MCM_DACCESS_SYS 修改資料 (包括使用 MCM_DACCESS_NEW 修改資料之後同步資料),
// 需要將資料儲存回檔案, 使用此變數判定是否需要做處理.
MCM_DTYPE_BOOL_TD mcm_do_save = 0;
// 內部模組的檔案符號.
void *mcm_config_module_fp = NULL;




// model group (avl-tree) 旋轉調整 (left-left).
struct mcm_config_model_group_t *mcm_tree_rotate_ll_group(
    struct mcm_config_model_group_t *this_model_group)
{
    struct mcm_config_model_group_t *base_model_group;


    base_model_group = this_model_group->ltree_model_group;
    this_model_group->ltree_model_group = base_model_group->rtree_model_group;
    base_model_group->rtree_model_group = this_model_group;

    this_model_group->tree_height_group =
        MCM_TREE_MAX(MCM_TREE_HEIGHT_GROUP(this_model_group->ltree_model_group),
                     MCM_TREE_HEIGHT_GROUP(this_model_group->rtree_model_group)) + 1;
    base_model_group->tree_height_group =
        MCM_TREE_MAX(MCM_TREE_HEIGHT_GROUP(base_model_group->ltree_model_group),
                     this_model_group->tree_height_group) + 1;

    return base_model_group;
}

// model group (avl-tree) 旋轉調整 (right-right).
struct mcm_config_model_group_t *mcm_tree_rotate_rr_group(
    struct mcm_config_model_group_t *this_model_group)
{
    struct mcm_config_model_group_t *base_model_group;


    base_model_group = this_model_group->rtree_model_group;
    this_model_group->rtree_model_group = base_model_group->ltree_model_group;
    base_model_group->ltree_model_group = this_model_group;

    this_model_group->tree_height_group =
        MCM_TREE_MAX(MCM_TREE_HEIGHT_GROUP(this_model_group->ltree_model_group),
                     MCM_TREE_HEIGHT_GROUP(this_model_group->rtree_model_group)) + 1;
    base_model_group->tree_height_group =
        MCM_TREE_MAX(MCM_TREE_HEIGHT_GROUP(base_model_group->rtree_model_group),
                     this_model_group->tree_height_group) + 1;

    return base_model_group;
}

// model group (avl-tree) 旋轉調整 (left-right).
struct mcm_config_model_group_t *mcm_tree_rotate_lr_group(
    struct mcm_config_model_group_t *this_model_group)
{
    this_model_group->ltree_model_group =
        mcm_tree_rotate_rr_group(this_model_group->ltree_model_group);

    return mcm_tree_rotate_ll_group(this_model_group);
}

// model group (avl-tree) 旋轉調整 (right-left).
struct mcm_config_model_group_t *mcm_tree_rotate_rl_group(
    struct mcm_config_model_group_t *this_model_group)
{
    this_model_group->rtree_model_group =
        mcm_tree_rotate_ll_group(this_model_group->rtree_model_group);

    return mcm_tree_rotate_rr_group(this_model_group);
}

// model group (avl-tree) 增加節點.
struct mcm_config_model_group_t *mcm_tree_add_group(
    struct mcm_config_model_group_t *base_model_group,
    struct mcm_config_model_group_t *this_model_group)
{
    if(base_model_group == NULL)
    {
        base_model_group = this_model_group;
    }
    else
    {
        if(strcmp(this_model_group->group_name, base_model_group->group_name) < 0)
        {
            base_model_group->ltree_model_group =
                mcm_tree_add_group(base_model_group->ltree_model_group, this_model_group);

            if((MCM_TREE_HEIGHT_GROUP(base_model_group->ltree_model_group) -
                MCM_TREE_HEIGHT_GROUP(base_model_group->rtree_model_group)) >= 2)
            {
                if(strcmp(this_model_group->group_name,
                          base_model_group->ltree_model_group->group_name) < 0)
                {
                    base_model_group = mcm_tree_rotate_ll_group(base_model_group);
                }
                else
                {
                    base_model_group = mcm_tree_rotate_lr_group(base_model_group);
                }
            }
        }
        else
        {
            base_model_group->rtree_model_group =
                mcm_tree_add_group(base_model_group->rtree_model_group, this_model_group);

            if((MCM_TREE_HEIGHT_GROUP(base_model_group->rtree_model_group) -
                MCM_TREE_HEIGHT_GROUP(base_model_group->ltree_model_group)) >= 2)
            {
                if(strcmp(this_model_group->group_name,
                          base_model_group->rtree_model_group->group_name) > 0)
                {
                    base_model_group = mcm_tree_rotate_rr_group(base_model_group);
                }
                else
                {
                    base_model_group = mcm_tree_rotate_rl_group(base_model_group);
                }
            }
        }
    }

    base_model_group->tree_height_group =
        MCM_TREE_MAX(MCM_TREE_HEIGHT_GROUP(base_model_group->ltree_model_group),
                     MCM_TREE_HEIGHT_GROUP(base_model_group->rtree_model_group)) + 1;

    return base_model_group;
}

// model group (avl-tree) 搜尋 (使用 group_name).
struct mcm_config_model_group_t *mcm_tree_search_group(
    struct mcm_config_model_group_t *this_model_group,
    char *name_str,
    MCM_DTYPE_USIZE_TD name_len)
{
    int fret;
    MCM_DTYPE_USIZE_TD nidx;


    while(this_model_group != NULL)
    {
        for(nidx = 0; nidx < name_len; nidx++)
        {
            fret = name_str[nidx] - this_model_group->group_name[nidx];
            if(fret != 0)
            {
                this_model_group = fret < 0 ?
                                   this_model_group->ltree_model_group :
                                   this_model_group->rtree_model_group;
                break;
            }
        }
        if(nidx == name_len)
        {
            if(name_len < this_model_group->group_nlen)
                this_model_group = this_model_group->ltree_model_group;
            else
                break;
        }
    }

    return this_model_group;
}

// model member (avl-tree) 旋轉調整 (left-left).
struct mcm_config_model_member_t *mcm_tree_rotate_ll_member(
    struct mcm_config_model_member_t *this_model_member)
{
    struct mcm_config_model_member_t *base_model_member;


    base_model_member = this_model_member->ltree_model_member;
    this_model_member->ltree_model_member = base_model_member->rtree_model_member;
    base_model_member->rtree_model_member = this_model_member;

    this_model_member->tree_height_member =
        MCM_TREE_MAX(MCM_TREE_HEIGHT_MEMBER(this_model_member->ltree_model_member),
                     MCM_TREE_HEIGHT_MEMBER(this_model_member->rtree_model_member)) + 1;
    base_model_member->tree_height_member =
        MCM_TREE_MAX(MCM_TREE_HEIGHT_MEMBER(base_model_member->ltree_model_member),
                     this_model_member->tree_height_member) + 1;

    return base_model_member;
}

// model member (avl-tree) 旋轉調整 (right-right).
struct mcm_config_model_member_t *mcm_tree_rotate_rr_member(
    struct mcm_config_model_member_t *this_model_member)
{
    struct mcm_config_model_member_t *base_model_member;


    base_model_member = this_model_member->rtree_model_member;
    this_model_member->rtree_model_member = base_model_member->ltree_model_member;
    base_model_member->ltree_model_member = this_model_member;

    this_model_member->tree_height_member =
        MCM_TREE_MAX(MCM_TREE_HEIGHT_MEMBER(this_model_member->ltree_model_member),
                     MCM_TREE_HEIGHT_MEMBER(this_model_member->rtree_model_member)) + 1;
    base_model_member->tree_height_member =
        MCM_TREE_MAX(MCM_TREE_HEIGHT_MEMBER(base_model_member->rtree_model_member),
                     this_model_member->tree_height_member) + 1;

    return base_model_member;
}

// model member (avl-tree) 旋轉調整 (left-right).
struct mcm_config_model_member_t *mcm_tree_rotate_lr_member(
    struct mcm_config_model_member_t *this_model_member)
{
    this_model_member->ltree_model_member =
        mcm_tree_rotate_rr_member(this_model_member->ltree_model_member);

    return mcm_tree_rotate_ll_member(this_model_member);
}

// model member (avl-tree) 旋轉調整 (right-left).
struct mcm_config_model_member_t *mcm_tree_rotate_rl_member(
    struct mcm_config_model_member_t *this_model_member)
{
    this_model_member->rtree_model_member =
        mcm_tree_rotate_ll_member(this_model_member->rtree_model_member);

    return mcm_tree_rotate_rr_member(this_model_member);
}

// model member (avl-tree) 增加節點.
struct mcm_config_model_member_t *mcm_tree_add_member(
    struct mcm_config_model_member_t *base_model_member,
    struct mcm_config_model_member_t *this_model_member)
{
    if(base_model_member == NULL)
    {
        base_model_member = this_model_member;
    }
    else
    {
        if(strcmp(this_model_member->member_name, base_model_member->member_name) < 0)
        {
            base_model_member->ltree_model_member =
                mcm_tree_add_member(base_model_member->ltree_model_member, this_model_member);

            if((MCM_TREE_HEIGHT_MEMBER(base_model_member->ltree_model_member) -
                MCM_TREE_HEIGHT_MEMBER(base_model_member->rtree_model_member)) >= 2)
            {
                if(strcmp(this_model_member->member_name,
                          base_model_member->ltree_model_member->member_name) < 0)
                {
                    base_model_member = mcm_tree_rotate_ll_member(base_model_member);
                }
                else
                {
                    base_model_member = mcm_tree_rotate_lr_member(base_model_member);
                }
            }
        }
        else
        {
            base_model_member->rtree_model_member =
                mcm_tree_add_member(base_model_member->rtree_model_member, this_model_member);

            if((MCM_TREE_HEIGHT_MEMBER(base_model_member->rtree_model_member) -
                MCM_TREE_HEIGHT_MEMBER(base_model_member->ltree_model_member)) >= 2)
            {
                if(strcmp(this_model_member->member_name,
                          base_model_member->rtree_model_member->member_name) > 0)
                {
                    base_model_member = mcm_tree_rotate_rr_member(base_model_member);
                }
                else
                {
                    base_model_member = mcm_tree_rotate_rl_member(base_model_member);
                }
            }
        }
    }

    base_model_member->tree_height_member =
        MCM_TREE_MAX(MCM_TREE_HEIGHT_MEMBER(base_model_member->ltree_model_member),
                     MCM_TREE_HEIGHT_MEMBER(base_model_member->rtree_model_member)) + 1;

    return base_model_member;
}

// model member (avl-tree) 搜尋 (使用 member_name).
struct mcm_config_model_member_t *mcm_tree_search_member(
    struct mcm_config_model_member_t *this_model_member,
    char *name_str,
    MCM_DTYPE_USIZE_TD name_len)
{
    int fret;
    MCM_DTYPE_USIZE_TD nidx;


    while(this_model_member != NULL)
    {
        for(nidx = 0; nidx < name_len; nidx++)
        {
            fret = name_str[nidx] - this_model_member->member_name[nidx];
            if(fret != 0)
            {
                this_model_member = fret < 0 ?
                                    this_model_member->ltree_model_member :
                                    this_model_member->rtree_model_member;
                break;
            }
        }
        if(nidx == name_len)
        {
            if(name_len < this_model_member->member_nlen)
                this_model_member = this_model_member->ltree_model_member;
            else
                break;
        }
    }

    return this_model_member;
}

// store (rb-tree) 旋轉調整 (left).
void mcm_tree_rotate_l_store(
    struct mcm_config_store_t *this_store,
    struct mcm_config_store_t **base_store)
{
    struct mcm_config_store_t *head_store;


    head_store = this_store->rtree_store;

    if((this_store->rtree_store = head_store->ltree_store) != NULL)
        head_store->ltree_store->ptree_store = this_store;

    if((head_store->ptree_store = this_store->ptree_store) == NULL)
    {
        *base_store = head_store;
    }
    else
    {
        if(this_store->ptree_store->ltree_store == this_store)
            this_store->ptree_store->ltree_store = head_store;
        else
            this_store->ptree_store->rtree_store = head_store;
    }

    head_store->ltree_store = this_store;
    this_store->ptree_store = head_store;
}

// store (rb-tree) 旋轉調整 (right).
void mcm_tree_rotate_r_store(
    struct mcm_config_store_t *this_store,
    struct mcm_config_store_t **base_store)
{
    struct mcm_config_store_t *head_store;


    head_store = this_store->ltree_store;

    if((this_store->ltree_store = head_store->rtree_store) != NULL)
        head_store->rtree_store->ptree_store = this_store;

    if((head_store->ptree_store = this_store->ptree_store) == NULL)
    {
        *base_store = head_store;
    }
    else
    {
        if(this_store->ptree_store->rtree_store == this_store)
            this_store->ptree_store->rtree_store = head_store;
        else
            this_store->ptree_store->ltree_store = head_store;
    }

    head_store->rtree_store = this_store;
    this_store->ptree_store = head_store;
}

// store (rb-tree) 新增節點後的調整.
void mcm_tree_adjust_add_store(
    struct mcm_config_store_t *this_store,
    struct mcm_config_store_t **base_store)
{
    struct mcm_config_store_t *parent_store, *grand_store, *temp_store;


    while(((parent_store = this_store->ptree_store) != NULL) &&
          (parent_store->tree_color_store == MCM_TCOLOR_RED))
    {
        grand_store = parent_store->ptree_store;

        if(parent_store == grand_store->ltree_store)
        {
            if((temp_store = grand_store->rtree_store) != NULL)
                if(temp_store->tree_color_store == MCM_TCOLOR_RED)
                {
                    temp_store->tree_color_store = MCM_TCOLOR_BLACK;
                    parent_store->tree_color_store = MCM_TCOLOR_BLACK;
                    grand_store->tree_color_store = MCM_TCOLOR_RED;
                    this_store = grand_store;
                    continue;
                }

            if(this_store == parent_store->rtree_store)
            {
                mcm_tree_rotate_l_store(parent_store, base_store);
                temp_store = parent_store;
                parent_store = this_store;
                this_store = temp_store;
            }

            parent_store->tree_color_store = MCM_TCOLOR_BLACK;
            grand_store->tree_color_store = MCM_TCOLOR_RED;
            mcm_tree_rotate_r_store(grand_store, base_store);
        }
        else
        {
            if((temp_store = grand_store->ltree_store) != NULL)
                if(temp_store->tree_color_store == MCM_TCOLOR_RED)
                {
                    temp_store->tree_color_store = MCM_TCOLOR_BLACK;
                    parent_store->tree_color_store = MCM_TCOLOR_BLACK;
                    grand_store->tree_color_store = MCM_TCOLOR_RED;
                    this_store = grand_store;
                    continue;
                }

            if(parent_store->ltree_store == this_store)
            {
                mcm_tree_rotate_r_store(parent_store, base_store);
                temp_store = parent_store;
                parent_store = this_store;
                this_store = temp_store;
            }

            parent_store->tree_color_store = MCM_TCOLOR_BLACK;
            grand_store->tree_color_store = MCM_TCOLOR_RED;
            mcm_tree_rotate_l_store(grand_store, base_store);
        }
    }

    (*base_store)->tree_color_store = MCM_TCOLOR_BLACK;
}

// store (rb-tree) 增加節點.
void mcm_tree_add_store(
    struct mcm_config_store_t *this_store,
    struct mcm_config_store_t **base_store)
{
    struct mcm_config_store_t *temp_store, *parent_store = NULL;
    void *data_loc;
    MCM_DTYPE_EK_TD this_key, temp_key, parent_key;


    data_loc = this_store->data_value_new != NULL ?
               this_store->data_value_new : this_store->data_value_sys;
    data_loc += this_store->link_model_group->entry_key_offset_value;
    this_key = *((MCM_DTYPE_EK_TD *) data_loc);

    for(temp_store = *base_store; temp_store != NULL;)
    {
        parent_store = temp_store;
        data_loc = temp_store->data_value_new != NULL ?
                   temp_store->data_value_new : temp_store->data_value_sys;
        data_loc += temp_store->link_model_group->entry_key_offset_value;
        temp_key = *((MCM_DTYPE_EK_TD *) data_loc);
        temp_store = this_key < temp_key ? temp_store->ltree_store : temp_store->rtree_store;
    }

    if((this_store->ptree_store = parent_store) != NULL)
    {
        data_loc = parent_store->data_value_new != NULL ?
                   parent_store->data_value_new : parent_store->data_value_sys;
        data_loc += parent_store->link_model_group->entry_key_offset_value;
        parent_key = *((MCM_DTYPE_EK_TD *) data_loc);
        if(this_key < parent_key)
            parent_store->ltree_store = this_store;
        else
            parent_store->rtree_store = this_store;
    }
    else
    {
        *base_store = this_store;
    }

    this_store->tree_color_store = MCM_TCOLOR_RED;

    mcm_tree_adjust_add_store(this_store, base_store);
}

// store (rb-tree) 移除節點後的調整.
void mcm_tree_adjust_del_store(
    struct mcm_config_store_t *this_store,
    struct mcm_config_store_t *parent_store,
    struct mcm_config_store_t **base_store)
{
    struct mcm_config_store_t *temp_store;
    MCM_DTYPE_BOOL_TD l_color, r_color;


    while(((this_store == NULL) || (this_store->tree_color_store == MCM_TCOLOR_BLACK)) &&
          (this_store != *base_store))
    {
        if(parent_store->ltree_store == this_store)
        {
            temp_store = parent_store->rtree_store;
            if(temp_store->tree_color_store == MCM_TCOLOR_RED)
            {
                temp_store->tree_color_store = MCM_TCOLOR_BLACK;
                parent_store->tree_color_store = MCM_TCOLOR_RED;
                mcm_tree_rotate_l_store(parent_store, base_store);
                temp_store = parent_store->rtree_store;
            }
            l_color = temp_store->ltree_store == NULL ?
                      1 : temp_store->ltree_store->tree_color_store == MCM_TCOLOR_BLACK ? 1 : 0;
            r_color = temp_store->rtree_store == NULL ?
                      1 : temp_store->rtree_store->tree_color_store == MCM_TCOLOR_BLACK ? 1 : 0;
            if((l_color == 1) && (r_color == 1))
            {
                temp_store->tree_color_store = MCM_TCOLOR_RED;
                this_store = parent_store;
                parent_store = this_store->ptree_store;
            }
            else
            {
                if(r_color == 1)
                {
                    temp_store->ltree_store->tree_color_store = MCM_TCOLOR_BLACK;
                    temp_store->tree_color_store = MCM_TCOLOR_RED;
                    mcm_tree_rotate_r_store(temp_store, base_store);
                    temp_store = parent_store->rtree_store;
                }
                temp_store->tree_color_store = parent_store->tree_color_store;
                parent_store->tree_color_store = MCM_TCOLOR_BLACK;
                temp_store->rtree_store->tree_color_store = MCM_TCOLOR_BLACK;
                mcm_tree_rotate_l_store(parent_store, base_store);
                this_store = *base_store;
                break;
            }
        }
        else
        {
            temp_store = parent_store->ltree_store;
            if(temp_store->tree_color_store == MCM_TCOLOR_RED)
            {
                temp_store->tree_color_store = MCM_TCOLOR_BLACK;
                parent_store->tree_color_store = MCM_TCOLOR_RED;
                mcm_tree_rotate_r_store(parent_store, base_store);
                temp_store = parent_store->ltree_store;
            }
            l_color = temp_store->ltree_store == NULL ?
                      1 : temp_store->ltree_store->tree_color_store == MCM_TCOLOR_BLACK ? 1 : 0;
            r_color = temp_store->rtree_store == NULL ?
                      1 : temp_store->rtree_store->tree_color_store == MCM_TCOLOR_BLACK ? 1 : 0;
            if((l_color == 1) && (r_color == 1))
            {
                temp_store->tree_color_store = MCM_TCOLOR_RED;
                this_store = parent_store;
                parent_store = this_store->ptree_store;
            }
            else
            {
                if(l_color == 1)
                {
                    temp_store->rtree_store->tree_color_store = MCM_TCOLOR_BLACK;
                    temp_store->tree_color_store = MCM_TCOLOR_RED;
                    mcm_tree_rotate_l_store(temp_store, base_store);
                    temp_store = parent_store->ltree_store;
                }
                temp_store->tree_color_store = parent_store->tree_color_store;
                parent_store->tree_color_store = MCM_TCOLOR_BLACK;
                temp_store->ltree_store->tree_color_store = MCM_TCOLOR_BLACK;
                mcm_tree_rotate_r_store(parent_store, base_store);
                this_store = *base_store;
                break;
            }
        }
    }
    if(this_store != NULL)
        this_store->tree_color_store = MCM_TCOLOR_BLACK;
}

// store (rb-tree) 刪除節點.
void mcm_tree_del_store(
    struct mcm_config_store_t *this_store,
    struct mcm_config_store_t **base_store)
{
    struct mcm_config_store_t *temp_store, *child_store, *parent_store;
    MCM_DTYPE_LIST_TD self_color;


    if((this_store->ltree_store != NULL) && (this_store->rtree_store != NULL))
    {
        for(temp_store = this_store->rtree_store; temp_store->ltree_store != NULL;
            temp_store = temp_store->ltree_store);

        if(this_store->ptree_store != NULL)
        {
            if(this_store->ptree_store->ltree_store == this_store)
                this_store->ptree_store->ltree_store = temp_store;
            else
                this_store->ptree_store->rtree_store = temp_store;
        }
        else
        {
            *base_store = temp_store;
        }

        child_store = temp_store->rtree_store;
        parent_store = temp_store->ptree_store;
        self_color = temp_store->tree_color_store;

        if(parent_store == this_store)
        {
            parent_store = temp_store;
        }
        else
        {
            if(child_store != NULL)
                child_store->ptree_store = parent_store;
            parent_store->ltree_store = child_store;

            temp_store->rtree_store = this_store->rtree_store;
            this_store->rtree_store->ptree_store = temp_store;
        }

        temp_store->ptree_store = this_store->ptree_store;
        temp_store->tree_color_store = this_store->tree_color_store;
        temp_store->ltree_store = this_store->ltree_store;
        this_store->ltree_store->ptree_store = temp_store;

        if(self_color == MCM_TCOLOR_BLACK)
            mcm_tree_adjust_del_store(child_store, parent_store, base_store);
    }
    else
    {
        child_store = this_store->ltree_store != NULL ?
                      this_store->ltree_store : this_store->rtree_store;
        parent_store = this_store->ptree_store;
        self_color = this_store->tree_color_store;

        if(child_store != NULL)
            child_store->ptree_store = parent_store;

        if(parent_store != NULL)
        {
            if(parent_store->ltree_store == this_store)
                parent_store->ltree_store = child_store;
            else
                parent_store->rtree_store = child_store;
        }
        else
        {
            *base_store = child_store;
        }

        if(self_color == MCM_TCOLOR_BLACK)
            mcm_tree_adjust_del_store(child_store, parent_store, base_store);
    }
}

// store (rb-tree) 搜尋 (使用 key).
struct mcm_config_store_t *mcm_tree_search_store(
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_EK_TD this_key)
{
    void *data_loc;
    MCM_DTYPE_EK_TD self_key;


    while(this_store != NULL)
    {
        data_loc = this_store->data_value_new != NULL ?
                   this_store->data_value_new : this_store->data_value_sys;
        data_loc += this_store->link_model_group->entry_key_offset_value;
        self_key = *((MCM_DTYPE_EK_TD *) data_loc);
        if(this_key != self_key)
            this_store = this_key < self_key ? this_store->ltree_store : this_store->rtree_store;
        else
            break;
    }

    return this_store;
}

// 將 hex 字串轉為字元.
// data_con (I) :
//   要轉換的 hex 字串 (2 byte).
// char_buf (O) :
//   儲存轉換後的字元.
// return :
//   MCM_RCODE_PASS.
#if MCM_SUPPORT_DTYPE_S
int mcm_hex_to_char(
    char *data_con,
    char *char_buf)
{
    MCM_DTYPE_USIZE_TD didx, tmp_dec, tmp_sum = 0;


    for(didx = 0; didx < 2; didx++)
    {
        MCM_CONVERT_HEX_TO_DEC(data_con[didx], tmp_dec);
        tmp_dec *= didx == 0 ? 16 : 1;
        tmp_sum += tmp_dec;
    }

    *char_buf = tmp_sum;

    return MCM_RCODE_PASS;
}
#endif

// 將 hex 字串轉為數值.
// data_con (I) :
//   要轉換的 hex 字串 (2 byte).
// hex_buf (O) :
//   儲存轉換後的數值.
// return :
//   MCM_RCODE_PASS.
#if MCM_SUPPORT_DTYPE_B
int mcm_hex_to_hex(
    char *data_con,
    unsigned char *hex_buf)
{
    MCM_DTYPE_USIZE_TD didx, tmp_dec, tmp_sum = 0;


    for(didx = 0; didx < 2; didx++)
    {
        MCM_CONVERT_HEX_TO_DEC(data_con[didx], tmp_dec);
        tmp_dec *= didx == 0 ? 16 : 1;
        tmp_sum += tmp_dec;
    }

    *hex_buf = tmp_sum;

    return MCM_RCODE_PASS;
}
#endif

// 增加緩衝的大小.
// data_buf_buf (O) :
//   要增加緩衝大小的緩衝.
// data_size_buf (I/O) :
//   I : 目前的緩衝大小.
//   O : 儲存增加後的緩衝的大小.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_realloc_buf_config(
    char **data_buf_buf,
    MCM_DTYPE_USIZE_TD *data_size_buf)
{
    void *tmp_buf;
    MCM_DTYPE_USIZE_TD tmp_size = *data_size_buf;


    tmp_buf = realloc(*data_buf_buf, tmp_size + MCM_READ_BUFFER_UNIT_SIZE);
    if(tmp_buf == NULL)
    {
        MCM_EMSG("call realloc() fail [%s]", strerror(errno));
        return MCM_RCODE_CONFIG_ALLOC_FAIL;
    }
    MCM_CFDMSG("realloc [" MCM_DTYPE_USIZE_PF "][%p] -> [" MCM_DTYPE_USIZE_PF "][%p]",
               tmp_size, *data_buf_buf, tmp_size + MCM_READ_BUFFER_UNIT_SIZE, tmp_buf);

    memset(tmp_buf + tmp_size, 0, MCM_READ_BUFFER_UNIT_SIZE);
    *data_buf_buf = (char *) tmp_buf;
    *data_size_buf += MCM_READ_BUFFER_UNIT_SIZE;

    return MCM_RCODE_PASS;
}

// 從文字檔中讀取一行資料.
// file_fp (I) :
//   要讀取的檔案.
// data_buf_buf (O) :
//   讀取緩衝.
// data_size_buf (I/O) :
//   I : 讀取緩衝的大小.
//   O : 儲存讀取緩衝不足時, 重新調整後的大小.
// data_len_buf (O) :
//   讀取的資料的長度.
// file_eof_buf (O) :
//   是否讀取到尾端.
//     0 : 否.
//     1 : 是.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_read_file(
    FILE *file_fp,
    char **data_buf_buf,
    MCM_DTYPE_USIZE_TD *data_size_buf,
    MCM_DTYPE_USIZE_TD *data_len_buf,
    MCM_DTYPE_BOOL_TD *file_eof_buf)
{
    int fret;
    char *tmp_buf = *data_buf_buf;
    MCM_DTYPE_USIZE_TD tmp_size = *data_size_buf, tmp_len;


    if(fgets(tmp_buf, tmp_size, file_fp) == NULL)
    {
        *file_eof_buf = 1;
        return MCM_RCODE_PASS;
    }
    tmp_len = strlen(tmp_buf);

    // 資料長度 + 1 >= 緩衝的大小, 表示讀取緩衝有可能太小.
    if((tmp_len + 1) >= tmp_size)
    {
        // 最後一個字元不是 '\n',
        // 1. 可能讀取緩衝不足所以資料沒讀完整.
        // 2. 或讀取到最後一行.
        while(tmp_buf[tmp_len - 1] != '\n')
        {
            fret = mcm_realloc_buf_config(&tmp_buf, &tmp_size);
            if(fret < MCM_RCODE_PASS)
            {
                MCM_EMSG("call mcm_realloc_buf_config() fail");
                return fret;
            }
            *data_buf_buf = tmp_buf;
            *data_size_buf = tmp_size;

            // != NULL, 讀取剩下的,
            // == NULL, 讀取到最後一行.
            if(fgets(tmp_buf + tmp_len, tmp_size - tmp_len, file_fp) != NULL)
                tmp_len += strlen(tmp_buf + tmp_len);
            else
                break;
        }
    }

    if(tmp_buf[tmp_len - 1] == '\n')
    {
        tmp_len--;
        tmp_buf[tmp_len] = '\0';
    }

    *data_len_buf = tmp_len;
    *file_eof_buf = 0;

    return MCM_RCODE_PASS;
}

// 設定 model group 中 member 的預設值.
// this_model_member (I) :
//   要處理的 model member.
// default_con (I) :
//   預設值資料.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_assign_model_default(
    struct mcm_config_model_member_t *this_model_member,
    char *default_con)
{
#if MCM_SUPPORT_DTYPE_S || MCM_SUPPORT_DTYPE_B
    MCM_DTYPE_USIZE_TD diidx, doidx;
#endif
#if MCM_CFDMODE
    MCM_DTYPE_USIZE_TD dbg_tidx, dbg_tlen;
    char dbg_buf[MCM_DBG_BUFFER_SIZE];
#endif


#define MCM_MODEL_INT_VALUE(type_def, stob_api, type_fmt) \
    do                                                                          \
    {                                                                           \
        *((type_def *) this_model_member->member_default_con) =                 \
            stob_api(default_con, NULL, 10);                                    \
        this_model_member->member_default_len = this_model_member->member_size; \
        MCM_CFDMSG("assign member_default_con[" type_fmt "]",                   \
                   *((type_def *) this_model_member->member_default_con));      \
    }                                                                           \
    while(0)

#define MCM_MODEL_FLO_VALUE(type_def, stob_api, type_fmt) \
    do                                                                          \
    {                                                                           \
        *((type_def *) this_model_member->member_default_con) =                 \
            stob_api(default_con, NULL);                                        \
        this_model_member->member_default_len = this_model_member->member_size; \
        MCM_CFDMSG("assign member_default_con[" type_fmt "]",                   \
                   *((type_def *) this_model_member->member_default_con));      \
    }                                                                           \
    while(0)


    MCM_CFDMSG("=> %s", __FUNCTION__);

    this_model_member->member_default_con = (void *) calloc(1, this_model_member->member_size);
    if(this_model_member->member_default_con == NULL)
    {
        MCM_EMSG("call calloc() fail [%s]", strerror(errno));
        return MCM_RCODE_CONFIG_ALLOC_FAIL;
    }
    MCM_CFDMSG("alloc member_default_con[" MCM_DTYPE_USIZE_PF "][%p]",
               this_model_member->member_size, this_model_member->member_default_con);

    switch(this_model_member->member_type)
    {
        case MCM_DTYPE_EK_INDEX:
            MCM_MODEL_INT_VALUE(MCM_DTYPE_EK_TD, MCM_DTYPE_EK_SB, MCM_DTYPE_EK_PF);
            break;
#if MCM_SUPPORT_DTYPE_RK
        case MCM_DTYPE_RK_INDEX:
            MCM_MODEL_INT_VALUE(MCM_DTYPE_RK_TD, MCM_DTYPE_RK_SB, MCM_DTYPE_RK_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISC
        case MCM_DTYPE_ISC_INDEX:
            MCM_MODEL_INT_VALUE(MCM_DTYPE_ISC_TD, MCM_DTYPE_ISC_SB, MCM_DTYPE_ISC_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IUC
        case MCM_DTYPE_IUC_INDEX:
            MCM_MODEL_INT_VALUE(MCM_DTYPE_IUC_TD, MCM_DTYPE_IUC_SB, MCM_DTYPE_IUC_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISS
        case MCM_DTYPE_ISS_INDEX:
            MCM_MODEL_INT_VALUE(MCM_DTYPE_ISS_TD, MCM_DTYPE_ISS_SB, MCM_DTYPE_ISS_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IUS
        case MCM_DTYPE_IUS_INDEX:
            MCM_MODEL_INT_VALUE(MCM_DTYPE_IUS_TD, MCM_DTYPE_IUS_SB, MCM_DTYPE_IUS_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISI
        case MCM_DTYPE_ISI_INDEX:
            MCM_MODEL_INT_VALUE(MCM_DTYPE_ISI_TD, MCM_DTYPE_ISI_SB, MCM_DTYPE_ISI_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IUI
        case MCM_DTYPE_IUI_INDEX:
            MCM_MODEL_INT_VALUE(MCM_DTYPE_IUI_TD, MCM_DTYPE_IUI_SB, MCM_DTYPE_IUI_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISLL
        case MCM_DTYPE_ISLL_INDEX:
            MCM_MODEL_INT_VALUE(MCM_DTYPE_ISLL_TD, MCM_DTYPE_ISLL_SB, MCM_DTYPE_ISLL_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IULL
        case MCM_DTYPE_IULL_INDEX:
            MCM_MODEL_INT_VALUE(MCM_DTYPE_IULL_TD, MCM_DTYPE_IULL_SB, MCM_DTYPE_IULL_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_FF
        case MCM_DTYPE_FF_INDEX:
            MCM_MODEL_FLO_VALUE(MCM_DTYPE_FF_TD, MCM_DTYPE_FF_SB, MCM_DTYPE_FF_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_FD
        case MCM_DTYPE_FD_INDEX:
            MCM_MODEL_FLO_VALUE(MCM_DTYPE_FD_TD, MCM_DTYPE_FD_SB, MCM_DTYPE_FD_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_FLD
        case MCM_DTYPE_FLD_INDEX:
            MCM_MODEL_FLO_VALUE(MCM_DTYPE_FLD_TD, MCM_DTYPE_FLD_SB, MCM_DTYPE_FLD_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_S
        case MCM_DTYPE_S_INDEX:
            for(diidx = doidx = 0; default_con[diidx] != '\0'; doidx++)
                if(default_con[diidx] != MCM_CSTR_SPECIAL_KEY)
                {
                    *(((MCM_DTYPE_S_TD *) this_model_member->member_default_con) + doidx) =
                        default_con[diidx];
                    diidx++;
                }
                else
                {
                    // 將特殊字元表示式轉為一般字元.
                    mcm_hex_to_char(default_con + diidx + 1,
                                    ((MCM_DTYPE_S_TD *) this_model_member->member_default_con) +
                                    doidx);
                    diidx += 3;
                }
            // 字串長度 + '\0'.
            this_model_member->member_default_len = doidx + 1;
#if MCM_CFDMODE
            MCM_DBG_FORMAT_CONFIG_S_VALUE(dbg_buf, this_model_member->member_default_con, doidx,
                                          dbg_tidx, dbg_tlen);
            MCM_CFDMSG("assign member_default_con[%s]", dbg_buf);
#endif
            break;
#endif
#if MCM_SUPPORT_DTYPE_B
        case MCM_DTYPE_B_INDEX:
            for(diidx = doidx = 0; default_con[diidx] != '\0'; diidx += 2, doidx++)
            {
                mcm_hex_to_hex(default_con + diidx,
                               ((MCM_DTYPE_B_TD *) this_model_member->member_default_con) + doidx);
            }
            this_model_member->member_default_len = this_model_member->member_size;
#if MCM_CFDMODE
            MCM_DBG_FORMAT_CONFIG_B_VALUE(dbg_buf, this_model_member->member_default_con, doidx,
                                          dbg_tidx, dbg_tlen);
            MCM_CFDMSG("assign member_default_con[%s]", dbg_buf);
#endif
            break;
#endif
    }

    return MCM_RCODE_PASS;
}

// 釋放 model.
// this_model_group (I) :
//   要釋放的 model group.
// return :
//   MCM_RCODE_PASS.
int mcm_destory_model(
    struct mcm_config_model_group_t *this_model_group)
{
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_model_member_t *self_model_member;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    while(this_model_group != NULL)
    {
        // 從子層往上層處理.
        if(this_model_group->child_model_group_list != NULL)
            mcm_destory_model(this_model_group->child_model_group_list);

        self_model_group = this_model_group;
        this_model_group = this_model_group->next_model_group;

        MCM_CFDMSG("destory[%s]", self_model_group->group_name);

        MCM_CFDMSG("free member_list[%p]", self_model_group->member_list);
        for(self_model_member = self_model_group->member_list; self_model_member != NULL;
            self_model_member = self_model_group->member_list)
        {
            self_model_group->member_list = self_model_member->next_model_member;

            MCM_CFDMSG("destory member[%s]", self_model_member->member_name);

            MCM_CFDMSG("free member_name[%p]", self_model_member->member_name);
            if(self_model_member->member_name != NULL)
                free(self_model_member->member_name);

            MCM_CFDMSG("free member_default_con[%p]", self_model_member->member_default_con);
            if(self_model_member->member_default_con != NULL)
                free(self_model_member->member_default_con);

            MCM_CFDMSG("free model_member[%p]", self_model_member);
            free(self_model_member);
        }

        MCM_CFDMSG("free group_name[%p]", self_model_group->group_name);
        if(self_model_group->group_name != NULL)
            free(self_model_group->group_name);

        MCM_CFDMSG("free model_group[%p]", self_model_group);
        free(self_model_group);
    }

    return MCM_RCODE_PASS;
}

// 建立 model.
// file_fp (I) :
//   來源檔案.
// read_buf_buf (I) :
//   讀取緩衝.
// read_size_buf (I) :
//   讀取緩衝的大小.
// node_type (I) :
//   節點類型.
//     MCM_MNODE_GROUP  : group 類型.
//     MCM_MNODE_MEMBER : member 類型.
// parent_model_group (I) :
//   對 group 而言  : 表示 parent model group.
//   對 member 而言 : 表示所屬的 model group.
// last_model_group (I) :
//   同一層 model group 串列上的最後一個.
// last_model_member (I) :
//   同一個 model group 下的 model member 串列上的最後一個.
// new_model_group_buf (O) :
//   儲存建立的 model group.
// new_model_member_buf (O) :
//   儲存建立的 model member.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_create_model(
    FILE *file_fp,
    char **read_buf_buf,
    MCM_DTYPE_USIZE_TD *read_size_buf,
    MCM_DTYPE_LIST_TD node_type,
    struct mcm_config_model_group_t *parent_model_group,
    struct mcm_config_model_group_t *last_model_group,
    struct mcm_config_model_member_t *last_model_member,
    struct mcm_config_model_group_t **new_model_group_buf,
    struct mcm_config_model_member_t **new_model_member_buf)
{
    int fret;
    struct mcm_config_model_group_t *self_model_group = NULL, *sub_model_group = NULL;
    struct mcm_config_model_member_t *self_model_member = NULL, *sub_model_member = NULL;
    char *read_con;
    MCM_DTYPE_USIZE_TD read_len, struct_index;
    MCM_DTYPE_BOOL_TD read_eof, has_link = 0;
    MCM_DTYPE_LIST_TD read_step = 0, next_node;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("create[%s]", node_type == MCM_MNODE_GROUP ? "GROUP" : "MEMBER");
    // 處理 group 類型.
    if(node_type == MCM_MNODE_GROUP)
    {
        self_model_group = (struct mcm_config_model_group_t *)
            calloc(1, sizeof(struct mcm_config_model_group_t));
        if(self_model_group == NULL)
        {
            MCM_EMSG("call calloc() fail [%s]", strerror(errno));
            fret = MCM_RCODE_CONFIG_ALLOC_FAIL;
            goto FREE_01;
        }
        MCM_CFDMSG("alloc model_group[%zu][%p]",
                   sizeof(struct mcm_config_model_group_t), self_model_group);
    }
    // 處理 member 類型.
    else
    {
        self_model_member = (struct mcm_config_model_member_t *)
            calloc(1, sizeof(struct mcm_config_model_member_t));
        if(self_model_member == NULL)
        {
            MCM_EMSG("call calloc() fail [%s]", strerror(errno));
            fret = MCM_RCODE_CONFIG_ALLOC_FAIL;
            goto FREE_01;
        }
        MCM_CFDMSG("alloc model_member[%zu][%p]",
                   sizeof(struct mcm_config_model_member_t), self_model_member);
    }

    while(1)
    {
        fret = mcm_read_file(file_fp, read_buf_buf, read_size_buf, &read_len, &read_eof);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_EMSG("call mcm_read_file() fail");
            goto FREE_01;
        }
        else
        if(read_eof == 1)
            break;

        read_con = *read_buf_buf;

        if(node_type == MCM_MNODE_GROUP)
        {
            // 逐一讀取對應的資料, 參考 mcm_build/mcm_build.c:output_sub_model_profile().

            if(read_step == 0)
            {
                self_model_group->group_name = (char *) malloc(read_len + 1);
                if(self_model_group->group_name == NULL)
                {
                    MCM_EMSG("call malloc() fail [%s]", strerror(errno));
                    fret = MCM_RCODE_CONFIG_ALLOC_FAIL;
                    goto FREE_01;
                }
                MCM_CFDMSG("alloc group_name[" MCM_DTYPE_USIZE_PF "][%p]",
                          read_len + 1, self_model_group->group_name);
                memcpy(self_model_group->group_name, read_con, read_len + 1);
                self_model_group->group_nlen = read_len;
                MCM_CFDMSG("assign group_name[" MCM_DTYPE_USIZE_PF "][%s]",
                           self_model_group->group_nlen, self_model_group->group_name);
            }
            else
            if(read_step == 1)
            {
                self_model_group->group_type = MCM_DTYPE_LIST_SB(read_con, NULL, 10);
                MCM_CFDMSG("assign group_type[" MCM_DTYPE_LIST_PF "]",
                           self_model_group->group_type);
            }
            else
            if(read_step == 2)
            {
                self_model_group->group_max = MCM_DTYPE_EK_SB(read_con, NULL, 10);
                MCM_CFDMSG("assign group_max[" MCM_DTYPE_EK_PF "]",
                           self_model_group->group_max);
            }
            else
            if(read_step == 3)
            {
                self_model_group->group_save = MCM_DTYPE_BOOL_SB(read_con, NULL, 10);
                MCM_CFDMSG("assign group_save[" MCM_DTYPE_BOOL_PF "]",
                           self_model_group->group_save);
            }
            else
            if(read_step == 4)
            {
                self_model_group->member_real_count = MCM_DTYPE_USIZE_SB(read_con, NULL, 10);
                MCM_CFDMSG("assign member_real_count[" MCM_DTYPE_USIZE_PF "]",
                           self_model_group->member_real_count);
            }
            else
            if(read_step == 5)
            {
                struct_index = MCM_DTYPE_USIZE_SB(read_con, NULL, 10);
                MCM_CFDMSG("read data_size_offset[" MCM_DTYPE_USIZE_PF "]", struct_index);

                self_model_group->data_status_size =
                    mcm_config_data_size_offset_list[struct_index].data_status;
                MCM_CFDMSG("assign data_status_size[" MCM_DTYPE_USIZE_PF "]",
                           self_model_group->data_status_size);
                self_model_group->data_value_size =
                    mcm_config_data_size_offset_list[struct_index].data_value;
                MCM_CFDMSG("assign data_value_size[" MCM_DTYPE_USIZE_PF "]",
                           self_model_group->data_value_size);
            }
            else
            if(read_step == 6)
            {
                self_model_group->store_index_in_parent = MCM_DTYPE_USIZE_SB(read_con, NULL, 10);
                MCM_CFDMSG("assign store_index_in_parent[" MCM_DTYPE_USIZE_PF "]",
                           self_model_group->store_index_in_parent);
            }
            else
            if(read_step == 7)
            {
                self_model_group->store_child_count = MCM_DTYPE_USIZE_SB(read_con, NULL, 10);
                MCM_CFDMSG("assign store_child_count[" MCM_DTYPE_USIZE_PF "]",
                           self_model_group->store_child_count);
            }
            else
            if(read_step == 8)
            {
                if(has_link == 0)
                {
                    if(parent_model_group != NULL)
                    {
                        self_model_group->parent_model_group = parent_model_group;
                        MCM_CFDMSG("link parent_model_group[%p]",
                                   self_model_group->parent_model_group);

                        if(parent_model_group->child_model_group_list == NULL)
                        {
                            parent_model_group->child_model_group_list = self_model_group;
                            MCM_CFDMSG("link parent->child_model_group_list[%p]",
                                       parent_model_group->child_model_group_list);
                        }
                    }
                    if(last_model_group != NULL)
                    {
                        last_model_group->next_model_group = self_model_group;
                        MCM_CFDMSG("link neighbor->next_model_group[%p -> %p]",
                                   last_model_group, last_model_group->next_model_group);
                    }
                    if(parent_model_group != NULL)
                    {
                        parent_model_group->child_model_group_tree =
                            mcm_tree_add_group(parent_model_group->child_model_group_tree,
                                               self_model_group);
                        MCM_CFDMSG("link parent->child_model_group_tree[%s][%s]",
                                   parent_model_group->child_model_group_tree->group_name,
                                   self_model_group->group_name);
                    }
                    has_link = 1;
                }

                if(read_con[0] != MCM_MPROFILE_END_KEY)
                {
                    next_node = read_con[0] == MCM_MPROFILE_GROUP_KEY ?
                                MCM_MNODE_GROUP : MCM_MNODE_MEMBER;

                    fret = mcm_create_model(file_fp, read_buf_buf, read_size_buf, next_node,
                                            self_model_group, sub_model_group, sub_model_member,
                                            &sub_model_group, &sub_model_member);
                    if(fret < MCM_RCODE_PASS)
                    {
                        MCM_EMSG("call mcm_create_model()");
                        goto FREE_01;
                    }
                }
                else
                {
                    break;
                }
            }

            read_step = read_step < 8 ? read_step + 1 : read_step;
        }
        else
        {
            // 逐一讀取對應的資料, 參考 mcm_build/mcm_build.c:output_sub_model_profile().

            if(read_step == 0)
            {
                self_model_member->member_name = (char *) malloc(read_len + 1);
                if(self_model_member->member_name == NULL)
                {
                    MCM_EMSG("call malloc() fail [%s]", strerror(errno));
                    fret = MCM_RCODE_CONFIG_ALLOC_FAIL;
                    goto FREE_01;
                }
                MCM_CFDMSG("alloc member_name[" MCM_DTYPE_USIZE_PF "][%p]",
                           read_len + 1, self_model_member->member_name);
                memcpy(self_model_member->member_name, read_con, read_len + 1);
                self_model_member->member_nlen = read_len;
                MCM_CFDMSG("assign member_name[" MCM_DTYPE_USIZE_PF "][%s]",
                           self_model_member->member_nlen, self_model_member->member_name);
            }
            else
            if(read_step == 1)
            {
                self_model_member->member_type = MCM_DTYPE_LIST_SB(read_con, NULL, 10);
                MCM_CFDMSG("assign member_type[" MCM_DTYPE_LIST_PF "]",
                           self_model_member->member_type);
            }
            else
            if(read_step == 2)
            {
                self_model_member->member_size = MCM_DTYPE_USIZE_SB(read_con, NULL, 10);
                MCM_CFDMSG("assign member_size[" MCM_DTYPE_USIZE_PF "]",
                           self_model_member->member_size);
            }
            else
            if(read_step == 3)
            {
                fret = mcm_assign_model_default(self_model_member, read_con);
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_EMSG("call mcm_assign_model_default() fail");
                    goto FREE_01;
                }
            }
            else
            if(read_step == 4)
            {
                struct_index = MCM_DTYPE_USIZE_SB(read_con, NULL, 10);
                MCM_CFDMSG("read data_size_offset[" MCM_DTYPE_USIZE_PF "]", struct_index);

                self_model_member->offset_in_status =
                    mcm_config_data_size_offset_list[struct_index].data_status;
                MCM_CFDMSG("assign offset_in_status[" MCM_DTYPE_USIZE_PF "]",
                           self_model_member->offset_in_status);
                self_model_member->offset_in_value =
                    mcm_config_data_size_offset_list[struct_index].data_value;
                MCM_CFDMSG("assign offset_in_value[" MCM_DTYPE_USIZE_PF "]",
                           self_model_member->offset_in_value);
            }
            else
            if(read_step == 5)
            {
                if(self_model_member->member_type == MCM_DTYPE_EK_INDEX)
                {
                    parent_model_group->entry_key_offset_status =
                        self_model_member->offset_in_status;
                    MCM_CFDMSG("assign entry_key_offset_status[" MCM_DTYPE_USIZE_PF "]",
                               parent_model_group->entry_key_offset_status);
                    parent_model_group->entry_key_offset_value =
                        self_model_member->offset_in_value;
                    MCM_CFDMSG("assign entry_key_offset_value[" MCM_DTYPE_USIZE_PF "]",
                               parent_model_group->entry_key_offset_value);
                }

                if(parent_model_group->member_list == NULL)
                {
                    parent_model_group->member_list = self_model_member;
                    MCM_CFDMSG("link member_list[%p]", parent_model_group->member_list);
                }
                if(last_model_member != NULL)
                {
                    last_model_member->next_model_member = self_model_member;
                    MCM_CFDMSG("link neighbor->next_model_member[%p -> %p]",
                               last_model_member, last_model_member->next_model_member);
                }

                parent_model_group->member_tree =
                    mcm_tree_add_member(parent_model_group->member_tree, self_model_member);
                MCM_CFDMSG("link parent->member_tree[%s][%s]",
                           parent_model_group->member_tree->member_name,
                           self_model_member->member_name);

                break;
            }

            read_step++;
        }
    }

    if(new_model_group_buf != NULL)
        *new_model_group_buf = self_model_group;
    if(new_model_member_buf != NULL)
        *new_model_member_buf = self_model_member;

    return MCM_RCODE_PASS;
FREE_01:
    if(node_type == MCM_MNODE_GROUP)
    {
        if(self_model_group != NULL)
        {
            if(self_model_group->parent_model_group != NULL)
                if(self_model_group->parent_model_group->child_model_group_list ==
                   self_model_group)
                {
                    self_model_group->parent_model_group->child_model_group_list = NULL;
                }
            if(last_model_group == self_model_group)
                last_model_group->next_model_group = NULL;
            mcm_destory_model(self_model_group);
        }
    }
    else
    {
        if(self_model_member != NULL)
        {
            if(parent_model_group->member_list == self_model_member)
                parent_model_group->member_list = NULL;
            if(last_model_member == self_model_member)
                last_model_member->next_model_member = NULL;
            if(self_model_member->member_name != NULL)
                free(self_model_member->member_name);
            if(self_model_member->member_default_con != NULL)
                free(self_model_member->member_default_con);
            free(self_model_member);
        }
    }
    return fret;
}

// 讀取建立 model tree.
// file_path (I) :
//   model 設定檔路徑.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_load_model(
    char *file_path)
{
    int fret;
    FILE *file_fp;
    char *read_buf = NULL;
    MCM_DTYPE_USIZE_TD read_size = 0, read_len;
    MCM_DTYPE_BOOL_TD read_eof;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    file_fp = fopen(file_path, "r");
    if(file_fp == NULL)
    {
        MCM_EMSG("call fopen(%s) fail [%s]", file_path, strerror(errno));
        fret = MCM_RCODE_CONFIG_INTERNAL_ERROR;
        goto FREE_01;
    }

    fret = mcm_realloc_buf_config(&read_buf, &read_size);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_realloc_buf_config() fail");
        goto FREE_02;
    }

    fret = mcm_read_file(file_fp, &read_buf, &read_size, &read_len, &read_eof);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_read_file() fail");
        goto FREE_03;
    }
    else
    if(read_eof == 1)
    {
        MCM_EMSG("empty model profile");
        fret = MCM_RCODE_CONFIG_INTERNAL_ERROR;
        goto FREE_03;
    }

    fret = mcm_create_model(file_fp, &read_buf, &read_size, MCM_MNODE_GROUP, NULL, NULL, NULL,
                            &mcm_config_root_model_group, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_create_model() fail");
        goto FREE_03;
    }

    fret = MCM_RCODE_PASS;
FREE_03:
    if(read_buf != NULL)
        free(read_buf);
FREE_02:
    fclose(file_fp);
FREE_01:
    return fret;
}

// 釋放 model tree.
// return :
//   MCM_RCODE_PASS.
int mcm_config_free_model(
    void)
{
    MCM_CFDMSG("=> %s", __FUNCTION__);

    mcm_destory_model(mcm_config_root_model_group);

    return MCM_RCODE_PASS;
}

// 將 store 加入到 update_store 列表中.
// this_store (I) :
//   要處理的 store.
// return :
//   MCM_RCODE_PASS.
int mcm_add_update_store(
    struct mcm_config_store_t *this_store)
{
#if MCM_CFDMODE
    struct mcm_config_model_group_t *dbg_model_group;
    void *dbg_dloc;
    MCM_DTYPE_EK_TD dbg_key = 0;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    dbg_model_group = this_store->link_model_group;
    MCM_DBG_SHOW_ENTRY_PATH(dbg_model_group, this_store, dbg_dloc, dbg_key);
#endif

    this_store->prev_update_store = this_store->next_update_store = NULL;

    if(mcm_update_store_head == NULL)
    {
        MCM_CFDMSG("adjust mcm_update_store_head[%p]", this_store);
        mcm_update_store_head = this_store;
    }

    if(mcm_update_store_tail == NULL)
    {
        MCM_CFDMSG("adjust mcm_update_store_tail[%p]", this_store);
        mcm_update_store_tail = this_store;
    }
    else
    {
        MCM_CFDMSG("adjust tail[(%p)-> %p]", mcm_update_store_tail, this_store);
        mcm_update_store_tail->next_update_store = this_store;

        MCM_CFDMSG("adjust prev_update_store[%p]", mcm_update_store_tail);
        this_store->prev_update_store = mcm_update_store_tail;

        MCM_CFDMSG("adjust mcm_update_store_tail[%p>>%p]", mcm_update_store_tail, this_store);
        mcm_update_store_tail = this_store;
    }

    this_store->need_update = 1;

    return MCM_RCODE_PASS;
}

// 將 store 從 update_store 列表中移除.
// this_store (I) :
//   要處理的 store.
// return :
//   MCM_RCODE_PASS.
int mcm_del_update_store(
    struct mcm_config_store_t *this_store)
{
#if MCM_CFDMODE
    struct mcm_config_model_group_t *dbg_model_group;
    void *dbg_dloc;
    MCM_DTYPE_EK_TD dbg_key = 0;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    dbg_model_group = this_store->link_model_group;
    MCM_DBG_SHOW_ENTRY_PATH(dbg_model_group, this_store, dbg_dloc, dbg_key);
#endif

    if(mcm_update_store_head == this_store)
    {
        MCM_CFDMSG("adjust mcm_update_store_head[%p>>%p]",
                   mcm_update_store_head, this_store->next_update_store);
        mcm_update_store_head = this_store->next_update_store;
    }

    if(mcm_update_store_tail == this_store)
    {
        MCM_CFDMSG("adjust mcm_update_store_tail[%p>>%p]",
                   mcm_update_store_tail, this_store->prev_update_store);
        mcm_update_store_tail = this_store->prev_update_store;
    }

    if(this_store->prev_update_store != NULL)
    {
        MCM_CFDMSG("adjust neighbor[(%p<-)->%p>>%p]",
                   this_store->prev_update_store, this_store->prev_update_store->next_update_store,
                   this_store->next_update_store);
        this_store->prev_update_store->next_update_store = this_store->next_update_store;
    }

    if(this_store->next_update_store != NULL)
    {
        MCM_CFDMSG("adjust neighbor[(->%p)<-%p>>%p]",
                   this_store->next_update_store, this_store->next_update_store->prev_update_store,
                   this_store->prev_update_store);
        this_store->next_update_store->prev_update_store = this_store->prev_update_store;
    }

    this_store->need_update = 0;

    return MCM_RCODE_PASS;
}

// 產生 store 的緩衝以及設定內容為預設值.
// this_model_group (I) :
//   使用的 model group.
// this_store (I) :
//   要處理的 store.
// assign_default (I) :
//   是否要將 store 內容設為預設值.
//     0 : 否.
//     1 : 是.
// default_key (I) :
//   gd 類型的 group 的 entry key 值.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_assign_store_default(
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_BOOL_TD assign_default,
    MCM_DTYPE_EK_TD default_key)
{
    int fret;
    struct mcm_config_model_member_t *self_model_member;
    void *data_loc;
#if MCM_CFDMODE
    MCM_DTYPE_USIZE_TD dbg_dlen, dbg_tidx, dbg_tlen;
    char dbg_buf[MCM_DBG_BUFFER_SIZE];
#endif


#define MCM_DEFAULT_NUM_VALUE(type_def, type_fmt) \
    do                                                                                    \
    {                                                                                     \
        *((type_def *) data_loc) = *((type_def *) self_model_member->member_default_con); \
        MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] [%s][" type_fmt "]",                       \
                   this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,  \
                   self_model_member->member_name, *((type_def *) data_loc));             \
    }                                                                                     \
    while(0)


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] assign",
               this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key);

    this_store->data_status = calloc(1, this_model_group->data_status_size);
    if(this_store->data_status == NULL)
    {
        MCM_EMSG("[%s.%c" MCM_DTYPE_EK_PF "] call calloc() fail [%s]",
                 this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                 strerror(errno));
        fret = MCM_RCODE_CONFIG_ALLOC_FAIL;
        goto FREE_01;
    }
    MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] alloc data_status[" MCM_DTYPE_USIZE_PF "][%p]",
               this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
               this_model_group->data_status_size, this_store->data_status);

    this_store->data_value_sys = calloc(1, this_model_group->data_value_size);
    if(this_store->data_value_sys == NULL)
    {
        MCM_EMSG("[%s.%c" MCM_DTYPE_EK_PF "] call calloc() fail [%s]",
                 this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                 strerror(errno));
        fret = MCM_RCODE_CONFIG_ALLOC_FAIL;
        goto FREE_02;
    }
    MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] alloc data_value_sys[" MCM_DTYPE_USIZE_PF "][%p]",
               this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
               this_model_group->data_value_size, this_store->data_value_sys);

    if(assign_default == 0)
    {
        if(this_model_group->group_type == MCM_DTYPE_GD_INDEX)
        {
            data_loc = this_store->data_value_sys + this_model_group->entry_key_offset_value;
            *((MCM_DTYPE_EK_TD *) data_loc) = default_key;
            MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] [" MCM_DTYPE_EK_PF "]",
                       this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                       *((MCM_DTYPE_EK_TD *) data_loc));
        }
    }
    else
    {
        for(self_model_member = this_model_group->member_list; self_model_member != NULL;
            self_model_member = self_model_member->next_model_member)
        {
            data_loc = this_store->data_value_sys + self_model_member->offset_in_value;

            switch(self_model_member->member_type)
            {
                case MCM_DTYPE_EK_INDEX:
                    *((MCM_DTYPE_EK_TD *) data_loc) = default_key;
                    MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] [%s][" MCM_DTYPE_EK_PF "]",
                               this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                               default_key, self_model_member->member_name,
                               *((MCM_DTYPE_EK_TD *) data_loc));
                    break;
#if MCM_SUPPORT_DTYPE_RK
                case MCM_DTYPE_RK_INDEX:
                    MCM_DEFAULT_NUM_VALUE(MCM_DTYPE_RK_TD, MCM_DTYPE_RK_PF);
                    break;
#endif
#if MCM_SUPPORT_DTYPE_ISC
                case MCM_DTYPE_ISC_INDEX:
                    MCM_DEFAULT_NUM_VALUE(MCM_DTYPE_ISC_TD, MCM_DTYPE_ISC_PF);
                    break;
#endif
#if MCM_SUPPORT_DTYPE_IUC
                case MCM_DTYPE_IUC_INDEX:
                    MCM_DEFAULT_NUM_VALUE(MCM_DTYPE_IUC_TD, MCM_DTYPE_IUC_PF);
                    break;
#endif
#if MCM_SUPPORT_DTYPE_ISS
                case MCM_DTYPE_ISS_INDEX:
                    MCM_DEFAULT_NUM_VALUE(MCM_DTYPE_ISS_TD, MCM_DTYPE_ISS_PF);
                    break;
#endif
#if MCM_SUPPORT_DTYPE_IUS
                case MCM_DTYPE_IUS_INDEX:
                    MCM_DEFAULT_NUM_VALUE(MCM_DTYPE_IUS_TD, MCM_DTYPE_IUS_PF);
                    break;
#endif
#if MCM_SUPPORT_DTYPE_ISI
                case MCM_DTYPE_ISI_INDEX:
                    MCM_DEFAULT_NUM_VALUE(MCM_DTYPE_ISI_TD, MCM_DTYPE_ISI_PF);
                    break;
#endif
#if MCM_SUPPORT_DTYPE_IUI
                case MCM_DTYPE_IUI_INDEX:
                    MCM_DEFAULT_NUM_VALUE(MCM_DTYPE_IUI_TD, MCM_DTYPE_IUI_PF);
                    break;
#endif
#if MCM_SUPPORT_DTYPE_ISLL
                case MCM_DTYPE_ISLL_INDEX:
                    MCM_DEFAULT_NUM_VALUE(MCM_DTYPE_ISLL_TD, MCM_DTYPE_ISLL_PF);
                    break;
#endif
#if MCM_SUPPORT_DTYPE_IULL
                case MCM_DTYPE_IULL_INDEX:
                    MCM_DEFAULT_NUM_VALUE(MCM_DTYPE_IULL_TD, MCM_DTYPE_IULL_PF);
                    break;
#endif
#if MCM_SUPPORT_DTYPE_FF
                case MCM_DTYPE_FF_INDEX:
                    MCM_DEFAULT_NUM_VALUE(MCM_DTYPE_FF_TD, MCM_DTYPE_FF_PF);
                    break;
#endif
#if MCM_SUPPORT_DTYPE_FD
                case MCM_DTYPE_FD_INDEX:
                    MCM_DEFAULT_NUM_VALUE(MCM_DTYPE_FD_TD, MCM_DTYPE_FD_PF);
                    break;
#endif
#if MCM_SUPPORT_DTYPE_FLD
                case MCM_DTYPE_FLD_INDEX:
                    MCM_DEFAULT_NUM_VALUE(MCM_DTYPE_FLD_TD, MCM_DTYPE_FLD_PF);
                    break;
#endif
#if MCM_SUPPORT_DTYPE_S
                case MCM_DTYPE_S_INDEX:
                    memcpy(data_loc, self_model_member->member_default_con,
                           self_model_member->member_default_len);
#if MCM_CFDMODE
                    dbg_dlen = self_model_member->member_default_len - 1;
                    MCM_DBG_FORMAT_CONFIG_S_VALUE(dbg_buf, data_loc, dbg_dlen, dbg_tidx, dbg_tlen);
                    MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] [%s][%s]",
                               this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                               default_key, self_model_member->member_name, dbg_buf);
#endif
                    break;
#endif
#if MCM_SUPPORT_DTYPE_B
                case MCM_DTYPE_B_INDEX:
                    memcpy(data_loc, self_model_member->member_default_con,
                           self_model_member->member_default_len);
#if MCM_CFDMODE
                    dbg_dlen = self_model_member->member_default_len;
                    MCM_DBG_FORMAT_CONFIG_B_VALUE(dbg_buf, data_loc, dbg_dlen, dbg_tidx, dbg_tlen);
                    MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] [%s][%s]",
                               this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                               default_key, self_model_member->member_name, dbg_buf);
#endif
                    break;
#endif
            }
        }
    }

    return MCM_RCODE_PASS;
FREE_02:
    free(this_store->data_status);
    this_store->data_status = NULL;
FREE_01:
    return fret;
}

// 釋放 store.
// this_store (I) :
//   要釋放的 store.
// return :
//   MCM_RCODE_PASS.
int mcm_destory_store(
    struct mcm_config_store_t *this_store)
{
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *self_store, **store_head_in_parent;
    MCM_DTYPE_USIZE_TD cidx;
#if MCM_CFDMODE
    void *dbg_dloc;
    MCM_DTYPE_EK_TD dbg_key = 0;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

    while(this_store != NULL)
    {
        self_model_group = this_store->link_model_group;

#if MCM_CFDMODE
        MCM_DBG_SHOW_ENTRY_PATH(self_model_group, this_store, dbg_dloc, dbg_key);
#endif

        // 從子層往上層處理.
        for(cidx = 0; cidx < self_model_group->store_child_count; cidx++)
        {
            store_head_in_parent = this_store->child_store_list_head_array + cidx;
            MCM_CFDMSG("trace[%s.%c" MCM_DTYPE_EK_PF "] child[%p(%p)]",
                       self_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key,
                       store_head_in_parent, *store_head_in_parent);
            if(*store_head_in_parent != NULL)
                mcm_destory_store(*store_head_in_parent);
        }

        self_store = this_store;
        this_store = this_store->next_store;

        if(self_store->need_update != 0)
            mcm_del_update_store(self_store);

        if(self_model_group->member_list != NULL)
        {
            MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] free data_status[%p]",
                       self_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key,
                       self_store->data_status);
            if(self_store->data_status != NULL)
                free(self_store->data_status);

            MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] free data_value_sys[%p]",
                       self_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key,
                       self_store->data_value_sys);
            if(self_store->data_value_sys != NULL)
                free(self_store->data_value_sys);

            MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] free data_value_new[%p]",
                       self_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key,
                       self_store->data_value_new);
            if(self_store->data_value_new != NULL)
                free(self_store->data_value_new);
        }

        if(self_model_group->store_child_count > 0)
        {
            MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] free child_store_list_head_array[%p]",
                       self_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key,
                       self_store->child_store_list_head_array);
            if(self_store->child_store_list_head_array != NULL)
                free(self_store->child_store_list_head_array);

            MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] free child_store_list_tail_array[%p]",
                       self_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key,
                       self_store->child_store_list_tail_array);
            if(self_store->child_store_list_tail_array != NULL)
                free(self_store->child_store_list_tail_array);

            MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] free child_store_tree_array[%p]",
                       self_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key,
                       self_store->child_store_tree_array);
            if(self_store->child_store_tree_array != NULL)
                free(self_store->child_store_tree_array);

            MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] free child_store_count_array[%p]",
                       self_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key,
                       self_store->child_store_count_array);
            if(self_store->child_store_count_array != NULL)
                free(self_store->child_store_count_array);
        }

        MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] free store[%p]",
                   self_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key, self_store);
        free(self_store);
    }

    return MCM_RCODE_PASS;
}

// 建立 store.
// this_model_group (I) :
//   使用的 model group.
// parent_store (I) :
//   parnet store.
// assign_default (I) :
//   是否要將 store 內容設為預設值.
//     0 : 否.
//     1 : 是.
// default_key (I) :
//   gd 類型的 model group 的 entry key 值.
// create_child (I) :
//   是否一併建立相關的 child store.
//     0 : 否.
//     1 : 是.
// internal_flag (I) :
//   內部處理遞迴時使用, 固定設 0.
// new_store_buf (O) :
//   儲存建立的 store.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_create_store(
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *parent_store,
    MCM_DTYPE_BOOL_TD assign_default,
    MCM_DTYPE_EK_TD default_key,
    MCM_DTYPE_BOOL_TD create_child,
    MCM_DTYPE_BOOL_TD internal_flag,
    struct mcm_config_store_t **new_store_buf)
{
    int fret;
    struct mcm_config_store_t *self_store = NULL, *last_store,
        **store_head_in_parent = NULL, **store_tail_in_parent = NULL,
        **store_tree_in_parent = NULL;
    MCM_DTYPE_EK_TD *store_count_in_parent = NULL;
#if MCM_CFDMODE
    MCM_DTYPE_USIZE_TD dbg_idx;
    struct mcm_config_store_t *dbg_store;
    void *dbg_dloc;
    MCM_DTYPE_EK_TD dbg_key;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

    // 尚未進入遞迴的最外層時, 只會處理一個, 進入遞迴後, 會處理全部.
    for(; this_model_group != NULL; this_model_group = this_model_group->next_model_group)
    {
        // 尚未進入遞迴時的最外層 (internal_flag = 0), gs 或 gd 類型都會處理,
        // 進入遞迴後 (internal_flag = 1), 只處理 gs 類型.
        if(internal_flag != 0)
            if(this_model_group->group_type != MCM_DTYPE_GS_INDEX)
                continue;

        MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] create"
                   "[" MCM_DTYPE_USIZE_PF "][" MCM_DTYPE_USIZE_PF "]",
                   this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                   this_model_group->member_real_count, this_model_group->store_child_count);

        self_store = (struct mcm_config_store_t *) calloc(1, sizeof(struct mcm_config_store_t));
        if(self_store == NULL)
        {
            MCM_EMSG("[%s.%c" MCM_DTYPE_EK_PF "] call calloc() fail [%s]",
                     this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                     strerror(errno));
            fret = MCM_RCODE_CONFIG_ALLOC_FAIL;
            goto FREE_01;
        }
        MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] alloc store[%zu][%p]",
                   this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                   sizeof(struct mcm_config_store_t), self_store);

        // 如果有 model_member, 替 model_member 取得儲存空間.
        if(this_model_group->member_list != NULL)
        {
            fret = mcm_assign_store_default(this_model_group, self_store, assign_default,
                                            default_key);
            if(fret < MCM_RCODE_PASS)
            {
                MCM_EMSG("[%s.%c" MCM_DTYPE_EK_PF "] call mcm_assign_store_default() fail",
                         this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key);
                goto FREE_02;
            }
        }

        // 如果有 child_store, 替 child_store 列表指針取得儲存空間.
        if(this_model_group->store_child_count > 0)
        {
            self_store->child_store_list_head_array = (struct mcm_config_store_t **)
                calloc(this_model_group->store_child_count, sizeof(struct mcm_config_store_t **));
            if(self_store->child_store_list_head_array == NULL)
            {
                MCM_EMSG("[%s.%c" MCM_DTYPE_EK_PF "] call calloc() fail [%s]",
                         this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                         strerror(errno));
                fret = MCM_RCODE_CONFIG_ALLOC_FAIL;
                goto FREE_02;
            }
            MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] "
                       "alloc child_store_list_head_array[" MCM_DTYPE_USIZE_PF "][%p]",
                       this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                       this_model_group->store_child_count,
                       self_store->child_store_list_head_array);

            self_store->child_store_list_tail_array = (struct mcm_config_store_t **)
                calloc(this_model_group->store_child_count, sizeof(struct mcm_config_store_t **));
            if(self_store->child_store_list_tail_array == NULL)
            {
                MCM_EMSG("[%s.%c" MCM_DTYPE_EK_PF "] call calloc() fail [%s]",
                         this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                         strerror(errno));
                fret = MCM_RCODE_CONFIG_ALLOC_FAIL;
                goto FREE_02;
            }
            MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] "
                       "alloc child_store_list_tail_array[" MCM_DTYPE_USIZE_PF "][%p]",
                       this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                       this_model_group->store_child_count,
                       self_store->child_store_list_tail_array);

            self_store->child_store_tree_array = (struct mcm_config_store_t **)
                calloc(this_model_group->store_child_count, sizeof(struct mcm_config_store_t **));
            if(self_store->child_store_tree_array == NULL)
            {
                MCM_EMSG("[%s.%c" MCM_DTYPE_EK_PF "] call calloc() fail [%s]",
                         this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                         strerror(errno));
                fret = MCM_RCODE_CONFIG_ALLOC_FAIL;
                goto FREE_02;
            }
            MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] "
                       "alloc child_store_tree_array[" MCM_DTYPE_USIZE_PF "][%p]",
                       this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                       this_model_group->store_child_count, self_store->child_store_tree_array);

            self_store->child_store_count_array = (MCM_DTYPE_EK_TD *)
                calloc(this_model_group->store_child_count, sizeof(MCM_DTYPE_EK_TD));
            if(self_store->child_store_count_array == NULL)
            {
                MCM_EMSG("[%s.%c" MCM_DTYPE_EK_PF "] call calloc() fail [%s]",
                         this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                         strerror(errno));
                fret = MCM_RCODE_CONFIG_ALLOC_FAIL;
                goto FREE_02;
            }
            MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] "
                       "alloc child_store_count_array[" MCM_DTYPE_USIZE_PF "][%p]",
                       this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                       this_model_group->store_child_count, self_store->child_store_count_array);

#if MCM_CFDMODE
            for(dbg_idx = 0; dbg_idx < this_model_group->store_child_count; dbg_idx++)
            {
                MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] "
                           "child_store_list_head_array[" MCM_DTYPE_USIZE_PF "][%p]",
                           this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                           dbg_idx, self_store->child_store_list_head_array + dbg_idx);
                MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] "
                           "child_store_list_tail_array[" MCM_DTYPE_USIZE_PF "][%p]",
                           this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                           dbg_idx, self_store->child_store_list_tail_array + dbg_idx);
                MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] "
                           "child_store_count_array[" MCM_DTYPE_USIZE_PF "][%p]",
                           this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                           dbg_idx, self_store->child_store_count_array + dbg_idx);
            }
#endif
        }

        // 鏈結使用的 model_group.
        self_store->link_model_group = this_model_group;
        MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] link model[%p]",
                   this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                   self_store->link_model_group);

        // 如果不是最上層 (root), 設定鏈結.
        if(parent_store != NULL)
        {
            // 鏈結 parent_store.
            self_store->parent_store = parent_store;
            MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] link parent_store[%p]",
                       this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                       self_store->parent_store);

            // 取得此 store 在 parent_store 中紀錄 child_store 的儲存空間位址的列表指針的位址.
            store_head_in_parent = parent_store->child_store_list_head_array +
                                   this_model_group->store_index_in_parent;
            store_tail_in_parent = parent_store->child_store_list_tail_array +
                                   this_model_group->store_index_in_parent;
            // 鏈結 parent_store 中的紀錄 child_store 儲存空間位址的指針.
            if(*store_head_in_parent == NULL)
            {
                // 鏈結到串列的頭.
                *store_head_in_parent = self_store;
                MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] link store_head_in_parent[%p(%p)]",
                           this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                           store_head_in_parent, *store_head_in_parent);
            }
            else
            {
                // 鏈結串列上最後一筆資料.
                last_store = *store_tail_in_parent;
                last_store->next_store = self_store;
                self_store->prev_store = last_store;
                MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] link neighbor[<-%p][->%p]",
                           this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                           self_store->prev_store, self_store->next_store);
            }

            // 鏈結到串列的尾.
            *store_tail_in_parent = self_store;
            MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] link store_tail_in_parent[%p(%p)]",
                       this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                       store_tail_in_parent, *store_tail_in_parent);

            // 加入到樹中.
            store_tree_in_parent = parent_store->child_store_tree_array +
                                   this_model_group->store_index_in_parent;
            mcm_tree_add_store(self_store, store_tree_in_parent);
#if MCM_CFDMODE
            dbg_store = *store_tree_in_parent;
            dbg_dloc = dbg_store->data_value_new != NULL ?
                       dbg_store->data_value_new : dbg_store->data_value_sys;
            dbg_dloc += dbg_store->link_model_group->entry_key_offset_value;
            dbg_key = *((MCM_DTYPE_EK_TD *) dbg_dloc);
            MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] "
                       "link store_tree_in_parent[%p(%c" MCM_DTYPE_EK_PF ")]",
                       this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                       store_tree_in_parent, MCM_SPROFILE_PATH_KEY_KEY, dbg_key);
#endif

            // 串列上的資料總數加 1.
            store_count_in_parent = parent_store->child_store_count_array +
                                    this_model_group->store_index_in_parent;
            (*store_count_in_parent)++;
            MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] "
                       "count store_count_in_parent[" MCM_DTYPE_EK_PF "]",
                       this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, default_key,
                       *store_count_in_parent);
        }

        if(create_child != 0)
            if(this_model_group->child_model_group_list != NULL)
            {
                fret = mcm_create_store(this_model_group->child_model_group_list, self_store,
                                        1, 0, 1, 1, NULL);
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_EMSG("[%s.%c" MCM_DTYPE_EK_PF "] call mcm_create_store() fail",
                             this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                             default_key);
                    goto FREE_03;
                }
            }

        // 尚未進入遞迴的最外層時, 只會處理一個, 離開.
        if(internal_flag == 0)
            break;
    }

    if(new_store_buf != NULL)
        *new_store_buf = self_store;

    return MCM_RCODE_PASS;
FREE_03:
    if(store_head_in_parent != NULL)
        *store_head_in_parent = NULL;
    if(store_tail_in_parent != NULL)
        *store_tail_in_parent = NULL;
    if(store_tree_in_parent != NULL)
        *store_tree_in_parent = NULL;
    if(store_count_in_parent != NULL)
        *store_count_in_parent = 0;
FREE_02:
    mcm_destory_store(self_store);
FREE_01:
    return fret;
}

// 移除 store 和其他 store 的鏈結.
// this_model_group (I) :
//   目標 model group.
// this_store (I) :
//   目標 store.
// return :
//   MCM_RCODE_PASS.
int mcm_unlink_store(
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store)
{
    struct mcm_config_store_t **store_head_in_parent, **store_tail_in_parent,
        **store_tree_in_parent;
    MCM_DTYPE_EK_TD *store_count_in_parent;
#if MCM_CFDMODE
    void *dbg_dloc;
    MCM_DTYPE_EK_TD dbg_key = 0, dbg_base_key;
    struct mcm_config_store_t *dbg_store;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    MCM_DBG_SHOW_ENTRY_PATH(this_model_group, this_store, dbg_dloc, dbg_key);
#endif

    MCM_CFDMSG("neighbor[<-%p][->%p]", this_store->prev_store, this_store->next_store);
    if(this_store->prev_store != NULL)
    {
        MCM_CFDMSG("adjust neighbor[(%p<-)->%p>>%p]",
                   this_store->prev_store, this_store->prev_store->next_store,
                   this_store->next_store);
        this_store->prev_store->next_store = this_store->next_store;
    }
    if(this_store->next_store != NULL)
    {
        MCM_CFDMSG("adjust neighbor[(->%p)<-%p>>%p]",
                   this_store->next_store, this_store->next_store->prev_store,
                   this_store->prev_store);
        this_store->next_store->prev_store = this_store->prev_store;
    }

    store_head_in_parent = this_store->parent_store->child_store_list_head_array +
                           this_model_group->store_index_in_parent;
    if(*store_head_in_parent == this_store)
    {
        MCM_CFDMSG("adjust store_list_head_in_parent[%p(%p>>%p)]",
                   store_head_in_parent, *store_head_in_parent, this_store->next_store);
        *store_head_in_parent = this_store->next_store;
    }

    store_tail_in_parent = this_store->parent_store->child_store_list_tail_array +
                           this_model_group->store_index_in_parent;
    if(*store_tail_in_parent == this_store)
    {
        MCM_CFDMSG("adjust store_list_tail_in_parent[%p(%p>>%p)]",
                   store_tail_in_parent, *store_tail_in_parent, this_store->prev_store);
        *store_tail_in_parent = this_store->prev_store;
    }

    store_tree_in_parent = this_store->parent_store->child_store_tree_array +
                           this_model_group->store_index_in_parent;
#if MCM_CFDMODE
    dbg_store = *store_tree_in_parent;
    dbg_dloc = dbg_store->data_value_new != NULL ?
               dbg_store->data_value_new : dbg_store->data_value_sys;
    dbg_dloc += dbg_store->link_model_group->entry_key_offset_value;
    dbg_key = *((MCM_DTYPE_EK_TD *) dbg_dloc);
#endif
    mcm_tree_del_store(this_store, store_tree_in_parent);
#if MCM_CFDMODE
    if(*store_tree_in_parent != NULL)
    {
        dbg_store = *store_tree_in_parent;
        dbg_dloc = dbg_store->data_value_new != NULL ?
                   dbg_store->data_value_new : dbg_store->data_value_sys;
        dbg_dloc += dbg_store->link_model_group->entry_key_offset_value;
        dbg_base_key = *((MCM_DTYPE_EK_TD *) dbg_dloc);
        MCM_CFDMSG("adjust store_tree_in_parent"
                   "[%p(%c" MCM_DTYPE_EK_PF ">>%c" MCM_DTYPE_EK_PF ")]",
                   store_tail_in_parent, MCM_SPROFILE_PATH_KEY_KEY, dbg_key,
                   MCM_SPROFILE_PATH_KEY_KEY, dbg_base_key);
    }
    else
    {
        MCM_CFDMSG("adjust store_tree_in_parent[%p(%c" MCM_DTYPE_EK_PF ">>NULL)]",
                   store_tail_in_parent, MCM_SPROFILE_PATH_KEY_KEY, dbg_key);
    }
#endif

    store_count_in_parent = this_store->parent_store->child_store_count_array +
                            this_model_group->store_index_in_parent;
    MCM_CFDMSG("adjust store_count_in_parent[" MCM_DTYPE_EK_PF ">>" MCM_DTYPE_EK_PF "]",
               *store_count_in_parent, *store_count_in_parent - 1);
    (*store_count_in_parent)--;

    this_store->prev_store = this_store->next_store = NULL;

    return MCM_RCODE_PASS;
}

// 處理 store 資料檔的額外資料部分.
// read_con (I) :
//   讀取到的資料.
// read_len (I) :
//   讀取到的資料的長度.
// file_source (I) :
//   檔案來源類型.
//     MCM_FSOURCE_DEFAULT : 預設值檔.
//     MCM_FSOURCE_CURRENT : 現在值檔.
//     MCM_FSOURCE_VERIFY  : 待檢查檔.
// file_line (I) :
//   此資料是在檔案中的第幾行.
// base_data_buf (I)
//   儲存額外資料的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_load_store_base_data(
    char *read_con,
    MCM_DTYPE_USIZE_TD read_len,
    MCM_DTYPE_LIST_TD file_source,
    MCM_DTYPE_USIZE_TD file_line,
    struct mcm_config_base_t *base_data_buf)
{
    MCM_DTYPE_USIZE_TD ridx, vloc, vlen, clen;


    MCM_CFDMSG("=> %s", __FUNCTION__);
    MCM_CFDMSG("[" MCM_DTYPE_USIZE_PF "][%s]", read_len, read_con);

    if(read_len == 0)
    {
        MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                 "invalid base, empty string",
                 MCM_SSOURCE(file_source), file_line, "INVALID_BASE_01");
        return MCM_RCODE_CONFIG_INTERNAL_ERROR;
    }

    for(ridx = 0; ridx < read_len; ridx++)
        if(read_con[ridx] == MCM_SPROFILE_PARAMETER_SPLIT_KEY)
            break;
    if(ridx == read_len)
    {
        MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                 "invalid base, not find parameter split key [%c]",
                 MCM_SSOURCE(file_source), file_line, "INVALID_BASE_02",
                 MCM_SPROFILE_PARAMETER_SPLIT_KEY);
        return MCM_RCODE_CONFIG_INTERNAL_ERROR;
    }
    read_con[ridx] = '\0';

    vloc = ridx + 1;
    vlen = read_len - ridx;

    // 版本資料.
    if(strcmp(read_con, MCM_SPROFILE_BASE_VERSION_KEY) == 0)
    {
        if(base_data_buf->profile_current_version[0] == '\0')
        {
            clen = sizeof(base_data_buf->profile_current_version) - 1;
            memcpy(base_data_buf->profile_current_version, read_con + vloc,
                   clen > vlen ? vlen : clen);
            MCM_CFDMSG("[%s][%s]",
                       MCM_SPROFILE_BASE_VERSION_KEY, base_data_buf->profile_current_version);
        }
        else
        {
            MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                     "invalid base, duplic [%s]",
                     MCM_SSOURCE(file_source), file_line, "INVALID_BASE_03",
                     MCM_SPROFILE_BASE_VERSION_KEY);
            return MCM_RCODE_CONFIG_INTERNAL_ERROR;
        }
    }
    else
    {
        MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                 "invalid base, unknown extra [%s]",
                 MCM_SSOURCE(file_source), file_line, "INVALID_BASE_04", read_con);
        return MCM_RCODE_CONFIG_INTERNAL_ERROR;
    }

    return MCM_RCODE_PASS;
}

// 處理路徑上個個 store 的 model group 類型和 key 值.
// this_model_group (I) :
//   使用的 model group tree.
// full_path (I) :
//   要處理的路徑.
// path_len (I) :
//   要處理的路徑的長度.
// file_source (I) :
//   檔案來源類型.
//     MCM_FSOURCE_DEFAULT : 預設值檔.
//     MCM_FSOURCE_CURRENT : 現在值檔.
//     MCM_FSOURCE_VERIFY  : 待檢查檔.
// file_line (I) :
//   此資料是在檔案中的第幾行.
// load_store_con (O) :
//   儲存路徑上個個 store 的資料.
// level_buf (O) :
//   儲存路徑的 store 的層數.
// self_model_buf (O) :
//   儲存路徑最尾端的 store 的 model group 類型.
// return :
//   <= MCM_RCODE_PASS : 成功.
//   >  MCM_RCODE_PASS : 錯誤.
int mcm_load_store_find_model(
    struct mcm_config_model_group_t *this_model_group,
    char *full_path,
    MCM_DTYPE_USIZE_TD path_len,
    MCM_DTYPE_LIST_TD file_source,
    MCM_DTYPE_USIZE_TD file_line,
    struct mcm_load_store_t *load_store_con,
    MCM_DTYPE_USIZE_TD *level_buf,
    struct mcm_config_model_group_t **self_model_buf)
{
    MCM_DTYPE_USIZE_TD pidx, ploc, nlen;
    MCM_DTYPE_EK_TD self_int;
    MCM_DTYPE_LIST_TD next_part = MCM_CPPATH_NAME;


    MCM_CFDMSG("=> %s", __FUNCTION__);
    MCM_CFDMSG("[%s]", full_path);

    *level_buf = 0;

    for(ploc = pidx = 0; pidx <= path_len; pidx++)
    {
        if((full_path[pidx] == MCM_SPROFILE_PATH_SPLIT_KEY) || (full_path[pidx] == '\0'))
        {
            if((nlen = pidx - ploc) == 0)
            {
                MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                         "invalid entry, empty stage",
                         MCM_SSOURCE(file_source), file_line, "INVALID_ENTRY-FORMAT_01");
                return MCM_RCODE_CONFIG_INTERNAL_ERROR;
            }

            // 此段落是記錄 name.
            if(full_path[ploc] != MCM_SPROFILE_PATH_KEY_KEY)
            {
                if(next_part == MCM_CPPATH_IK)
                {
                    MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                             "invalid entry, this part must be key [%s]",
                             MCM_SSOURCE(file_source), file_line, "INVALID_ENTRY-FORMAT_02",
                             full_path + ploc);
                    return MCM_RCODE_CONFIG_INTERNAL_ERROR;
                }

                // ploc == 0, 表示目前是最上層 (root),
                // ploc > 0, 往下一層尋找.
                if(ploc > 0)
                    this_model_group = this_model_group->child_model_group_tree;

                // 尋找相同的 name.
                this_model_group = mcm_tree_search_group(this_model_group, full_path + ploc, nlen);
                if(this_model_group == NULL)
                {
                    MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                             "invalid entry, not fine group name [%s]",
                             MCM_SSOURCE(file_source), file_line, "INVALID_ENTRY-FORMAT_03",
                             full_path + ploc);
                    return MCM_RCODE_CONFIG_INTERNAL_ERROR;
                }

                memset(load_store_con + *level_buf, 0, sizeof(struct mcm_load_store_t));
                // 紀錄此層 store 的 model group 類型.
                load_store_con[*level_buf].link_model_group = this_model_group;

                // 對於 gd 類型, 下一個段落是記錄 key 值, 還是算同一層, 還不算下一層.
                if(this_model_group->group_type == MCM_DTYPE_GS_INDEX)
                    (*level_buf)++;

                // gs 類型的 model group, 下一階段的路徑必須是 name (下一層),
                // gd 類型的 model group, 下一階段的路徑必須是 key.
                next_part = this_model_group->group_type == MCM_DTYPE_GS_INDEX ?
                            MCM_CPPATH_NAME : MCM_CPPATH_IK;
            }
            // 此段落是記錄 key.
            else
            {
                if(next_part == MCM_CPPATH_NAME)
                {
                    MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                             "invalid entry, this part must be name [%s]",
                             MCM_SSOURCE(file_source), file_line, "INVALID_ENTRY-FORMAT_04",
                             full_path + ploc);
                    return MCM_RCODE_CONFIG_INTERNAL_ERROR;
                }

                self_int = MCM_DTYPE_EK_SB(full_path + ploc + 1, NULL, 10);
                if(self_int < 1)
                {
                    MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                             "invalid entry, invalid key [%s]",
                             MCM_SSOURCE(file_source), file_line, "INVALID_ENTRY-FORMAT_05",
                             full_path + ploc + 1);
                    return MCM_RCODE_CONFIG_INTERNAL_ERROR;
                }

                load_store_con[*level_buf].target_key = self_int;
                (*level_buf)++;

                // 下一階段的路徑必須是 name (下一層).
                next_part = MCM_CPPATH_NAME;
            }

            ploc = pidx + 1;
        }
    }

    if(next_part != MCM_CPPATH_NAME)
    {
        MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                 "invalid entry, lost last stage",
                 MCM_SSOURCE(file_source), file_line, "INVALID_ENTRY-FORMAT_06");
        return MCM_RCODE_CONFIG_INTERNAL_ERROR;
    }

    *self_model_buf = this_model_group;

    return MCM_RCODE_PASS;
}

// 檢查路徑上的個個 store 是否超過最大筆數限制.
// this_store (I) :
//   使用的 store tree.
// load_store_con (I) :
//   儲存路徑上個個 store 的資料.
// max_level (I) :
//   路徑的 store 層數.
// file_source (I) :
//   檔案來源類型.
//     MCM_FSOURCE_DEFAULT : 預設值檔.
//     MCM_FSOURCE_CURRENT : 現在值檔.
//     MCM_FSOURCE_VERIFY  : 待檢查檔.
// file_line (I) :
//   此資料是在檔案中的第幾行.
// return :
//   >= MCM_RCODE_PASS : 未超過.
//   <  MCM_RCODE_PASS : 已超過.
int mcm_load_store_check_count(
    struct mcm_config_store_t *this_store,
    struct mcm_load_store_t *load_store_con,
    MCM_DTYPE_USIZE_TD max_level,
    MCM_DTYPE_LIST_TD file_source,
    MCM_DTYPE_USIZE_TD file_line)
{
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *self_store = NULL, *parent_store, **store_tree_in_parent;
    MCM_DTYPE_USIZE_TD lidx;
    MCM_DTYPE_EK_TD *store_count_in_parent, self_count;
    MCM_DTYPE_BOOL_TD is_full;
    MCM_DTYPE_DS_TD tmp_status;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    parent_store = this_store;

    for(lidx = self_count = is_full = 0; lidx < max_level; lidx++)
    {
        self_model_group = load_store_con[lidx].link_model_group;

        if(parent_store != NULL)
        {
            if(lidx == 0)
            {
                self_store = parent_store;

                MCM_GET_ENTRY_SELF_STATUS(self_model_group, self_store, tmp_status);
                self_count = (tmp_status & MCM_DSERROR_LOSE_ENTRY) != 0 ? 0 : 1;
            }
            else
            {
                store_tree_in_parent = parent_store->child_store_tree_array +
                    load_store_con[lidx].link_model_group->store_index_in_parent;
                self_store = *store_tree_in_parent;

                store_count_in_parent = parent_store->child_store_count_array +
                    load_store_con[lidx].link_model_group->store_index_in_parent;
                self_count = *store_count_in_parent;
            }

            // 尋找是否存在.
            self_store = mcm_tree_search_store(self_store, load_store_con[lidx].target_key);

            // != NULL 表示存在現有的 store tree 中, 不用比對是否超過最大筆數.
            if((parent_store = self_store) == NULL)
                if(self_count >= self_model_group->group_max)
                    is_full = 1;
        }

        MCM_CFDMSG("check count[%s.%c" MCM_DTYPE_EK_PF "][%s]"
                   "[" MCM_DTYPE_EK_PF "/" MCM_DTYPE_EK_PF "][%s]",
                   self_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                   load_store_con[lidx].target_key,
                   self_model_group->group_type == MCM_DTYPE_GS_INDEX ?
                   MCM_DTYPE_GS_KEY : MCM_DTYPE_GD_KEY,
                   self_count, self_model_group->group_max, is_full != 0 ? "full" : "allow");

        if(is_full != 0)
            break;
    }

    if(is_full != 0)
    {
        MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                 "invalid entry, too many entry [%s][" MCM_DTYPE_EK_PF "]",
                 MCM_SSOURCE(file_source), file_line, "INVALID_ENTRY-MAX_01",
                 self_model_group->group_name, self_model_group->group_max);
    }

    return is_full == 0 ? MCM_RCODE_PASS : MCM_RCODE_CONFIG_INTERNAL_ERROR;
}

// 檢查路徑上的個個 store 是否存在 store tree 中.
// this_store (I) :
//   使用的 store tree.
// load_store_con (O) :
//   儲存路徑上個個 store 的資料.
// max_level (I) :
//   路徑的 store 層數.
// target_store_buf (O) :
//   儲存路徑上最尾端的 sotre.
// return :
//   >= MCM_RCODE_PASS : sotre 存在 store tree 中.
//   <  MCM_RCODE_PASS : sotre 不存在 store tree 中.
int mcm_load_store_find_store(
    struct mcm_config_store_t *this_store,
    struct mcm_load_store_t *load_store_con,
    MCM_DTYPE_USIZE_TD max_level,
    struct mcm_config_store_t **target_store_buf)
{
    struct mcm_config_store_t *self_store = NULL, *parent_store, **store_tree_in_parent;
    MCM_DTYPE_USIZE_TD lidx;
#if MCM_CFDMODE
    struct mcm_config_model_group_t *dbg_model_group;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

    parent_store = this_store;

    for(lidx = 0; lidx < max_level; lidx++)
    {
#if MCM_CFDMODE
        dbg_model_group = load_store_con[lidx].link_model_group;
#endif

        if(parent_store != NULL)
        {
            if(lidx == 0)
            {
                self_store = parent_store;
            }
            else
            {
                store_tree_in_parent = parent_store->child_store_tree_array +
                    load_store_con[lidx].link_model_group->store_index_in_parent;
                self_store = *store_tree_in_parent;
            }

            // 尋找是否存在.
            self_store = mcm_tree_search_store(self_store, load_store_con[lidx].target_key);

            // == NULL 表示不存在, 無法再往下一層處理.
            load_store_con[lidx].link_store = parent_store = self_store;
        }

#if MCM_CFDMODE
        MCM_CFDMSG("check lose[%s.%c" MCM_DTYPE_EK_PF "][%s]",
                   dbg_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                   load_store_con[lidx].target_key, self_store == NULL ? "lose" : "find");
#endif
    }

    *target_store_buf = self_store;

    return self_store != NULL ? MCM_RCODE_PASS : MCM_RCODE_CONFIG_INTERNAL_ERROR;
}

// 建立路徑上的個個 store.
// load_store_con (I) :
//   路徑上個個 store 的資料.
// max_level (I) :
//   路徑的 store 層數.
// new_store_buf (I) :
//   紀錄路徑上最尾端的 sotre.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_load_store_fill_store(
    struct mcm_load_store_t *load_store_con,
    MCM_DTYPE_USIZE_TD max_level,
    struct mcm_config_store_t **new_store_buf)
{
    int fret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *self_store = NULL, *parent_store;
    MCM_DTYPE_USIZE_TD lidx;
    MCM_DTYPE_EK_TD self_key;
    MCM_DTYPE_DS_TD tmp_status;
#if MCM_CFDMODE
    void *data_loc;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

    parent_store = load_store_con[0].link_store;

    for(lidx = 0; lidx < max_level; lidx++)
    {
        self_model_group = load_store_con[lidx].link_model_group;

        if(load_store_con[lidx].link_store != NULL)
        {
            self_store = load_store_con[lidx].link_store;
#if MCM_CFDMODE
            MCM_DBG_GET_ENTRY_KEY(self_model_group, self_store, data_loc, self_key);
            MCM_CFDMSG("find[%s.%c" MCM_DTYPE_EK_PF "]",
                       self_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, self_key);
#endif
        }
        else
        {
            // 建立缺少的 store.
            self_key = load_store_con[lidx].target_key;
            MCM_CFDMSG("fill[%s][%c" MCM_DTYPE_EK_PF "]",
                       self_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, self_key);

            fret = mcm_create_store(self_model_group, parent_store, 1, self_key, 0, 0, &self_store);
            if(fret < MCM_RCODE_PASS)
            {
                MCM_EMSG("call mcm_create_store() fail");
                return fret;
            }

            if(self_model_group->member_real_count > 0)
            {
                tmp_status = MCM_DSERROR_LOSE_MEMBER;
                if(lidx < (max_level - 1))
                    tmp_status |= MCM_DSERROR_LOSE_ENTRY;
                mcm_config_set_entry_self_status(NULL, self_model_group, self_store,
                                                 MCM_DSASSIGN_SET, tmp_status);

                tmp_status = MCM_DSERROR_LOSE_MEMBER;
                mcm_config_set_entry_all_status(NULL, self_model_group, self_store,
                                                MCM_DSASSIGN_SET, tmp_status, 1);

            }
        }

        parent_store = self_store;
    }

    *new_store_buf = self_store;

    return MCM_RCODE_PASS;
}

// 檢查整數類型資料.
// this_model_member (I) :
//   使用的 model member.
// read_con (I) :
//   讀取到的資料.
// read_len (I) :
//   讀取到的資料的長度.
// file_source (I) :
//   檔案來源類型.
//     MCM_FSOURCE_DEFAULT : 預設值檔.
//     MCM_FSOURCE_CURRENT : 現在值檔.
//     MCM_FSOURCE_VERIFY  : 待檢查檔.
// file_line (I) :
//   此資料是在檔案中的第幾行.
// value_buf (O) :
//   儲存轉換後的資料緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_load_store_check_integer(
    struct mcm_config_model_member_t *this_model_member,
    char *read_con,
    MCM_DTYPE_USIZE_TD read_len,
    MCM_DTYPE_LIST_TD file_source,
    MCM_DTYPE_USIZE_TD file_line,
    void *value_buf)
{
#if MCM_SUPPORT_DTYPE_RK || \
    MCM_SUPPORT_DTYPE_ISC || MCM_SUPPORT_DTYPE_IUC || \
    MCM_SUPPORT_DTYPE_ISS || MCM_SUPPORT_DTYPE_IUS || \
    MCM_SUPPORT_DTYPE_ISI || MCM_SUPPORT_DTYPE_IUI || \
    MCM_SUPPORT_DTYPE_ISLL || MCM_SUPPORT_DTYPE_IULL
    int tmp_error;
    char *tmp_tail;
#endif
#if MCM_SUPPORT_DTYPE_RK
    MCM_CLIMIT_RK_TD tmp_rk;
#endif
#if MCM_SUPPORT_DTYPE_ISC
    MCM_CLIMIT_ISC_TD tmp_isc;
#endif
#if MCM_SUPPORT_DTYPE_IUC
    MCM_CLIMIT_IUC_TD tmp_iuc;
#endif
#if MCM_SUPPORT_DTYPE_ISS
    MCM_CLIMIT_ISS_TD tmp_iss;
#endif
#if MCM_SUPPORT_DTYPE_IUS
    MCM_CLIMIT_IUS_TD tmp_ius;
#endif
#if MCM_SUPPORT_DTYPE_ISI
    MCM_CLIMIT_ISI_TD tmp_isi;
#endif
#if MCM_SUPPORT_DTYPE_IUI
    MCM_CLIMIT_IUI_TD tmp_iui;
#endif
#if MCM_SUPPORT_DTYPE_ISLL
    MCM_CLIMIT_ISLL_TD tmp_isll;
#endif
#if MCM_SUPPORT_DTYPE_IULL
    MCM_CLIMIT_IULL_TD tmp_iull;
#endif


#define MCM_CHECK_INT_VALUE(limit_api, limit_min, limit_max, type_def, type_fmt, tmp_value) \
    do                                                                                \
    {                                                                                 \
        limit_api(read_con, tmp_tail, tmp_error, tmp_value);                          \
        if(tmp_error != 0)                                                            \
        {                                                                             \
            MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG                                    \
                     "invalid value [integer], must be integer "                      \
                     type_fmt "~" type_fmt " [%s]",                                   \
                     MCM_SSOURCE(file_source), file_line, "INVALID_VALUE-INTEGER_02", \
                     limit_min, limit_max, this_model_member->member_name);           \
            return MCM_RCODE_CONFIG_INTERNAL_ERROR;                                   \
        }                                                                             \
        *((type_def *) value_buf) = tmp_value;                                        \
        MCM_CFDMSG("[%s][" type_fmt "]",                                              \
                   this_model_member->member_name, *((type_def *) value_buf));        \
    }                                                                                 \
    while(0)


    MCM_CFDMSG("=> %s", __FUNCTION__);

    if(read_len == 0)
    {
        MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                 "invalid value [integer], empty value [%s]",
                 MCM_SSOURCE(file_source), file_line, "INVALID_VALUE-INTEGER_01",
                 this_model_member->member_name);
        return MCM_RCODE_CONFIG_INTERNAL_ERROR;
    }

    switch(this_model_member->member_type)
    {
        case MCM_DTYPE_EK_INDEX:
            break;
#if MCM_SUPPORT_DTYPE_RK
        case MCM_DTYPE_RK_INDEX:
            MCM_CHECK_INT_VALUE(MCM_CLIMIT_RK_API, MCM_CLIMIT_RK_MIN, MCM_CLIMIT_RK_MAX,
                                MCM_DTYPE_RK_TD, MCM_DTYPE_RK_PF, tmp_rk);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISC
        case MCM_DTYPE_ISC_INDEX:
            MCM_CHECK_INT_VALUE(MCM_CLIMIT_ISC_API, MCM_CLIMIT_ISC_MIN, MCM_CLIMIT_ISC_MAX,
                                MCM_DTYPE_ISC_TD, MCM_DTYPE_ISC_PF, tmp_isc);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IUC
        case MCM_DTYPE_IUC_INDEX:
            MCM_CHECK_INT_VALUE(MCM_CLIMIT_IUC_API, MCM_CLIMIT_IUC_MIN, MCM_CLIMIT_IUC_MAX,
                                MCM_DTYPE_IUC_TD, MCM_DTYPE_IUC_PF, tmp_iuc);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISS
        case MCM_DTYPE_ISS_INDEX:
            MCM_CHECK_INT_VALUE(MCM_CLIMIT_ISS_API, MCM_CLIMIT_ISS_MIN, MCM_CLIMIT_ISS_MAX,
                                MCM_DTYPE_ISS_TD, MCM_DTYPE_ISS_PF, tmp_iss);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IUS
        case MCM_DTYPE_IUS_INDEX:
            MCM_CHECK_INT_VALUE(MCM_CLIMIT_IUS_API, MCM_CLIMIT_IUS_MIN, MCM_CLIMIT_IUS_MAX,
                                MCM_DTYPE_IUS_TD, MCM_DTYPE_IUS_PF, tmp_ius);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISI
        case MCM_DTYPE_ISI_INDEX:
            MCM_CHECK_INT_VALUE(MCM_CLIMIT_ISI_API, MCM_CLIMIT_ISI_MIN, MCM_CLIMIT_ISI_MAX,
                                MCM_DTYPE_ISI_TD, MCM_DTYPE_ISI_PF, tmp_isi);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IUI
        case MCM_DTYPE_IUI_INDEX:
            MCM_CHECK_INT_VALUE(MCM_CLIMIT_IUI_API, MCM_CLIMIT_IUI_MIN, MCM_CLIMIT_IUI_MAX,
                                MCM_DTYPE_IUI_TD, MCM_DTYPE_IUI_PF, tmp_iui);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISLL
        case MCM_DTYPE_ISLL_INDEX:
            MCM_CHECK_INT_VALUE(MCM_CLIMIT_ISLL_API, MCM_CLIMIT_ISLL_MIN, MCM_CLIMIT_ISLL_MAX,
                                MCM_DTYPE_ISLL_TD, MCM_DTYPE_ISLL_PF, tmp_isll);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IULL
        case MCM_DTYPE_IULL_INDEX:
            MCM_CHECK_INT_VALUE(MCM_CLIMIT_IULL_API, MCM_CLIMIT_IULL_MIN, MCM_CLIMIT_IULL_MAX,
                                MCM_DTYPE_IULL_TD, MCM_DTYPE_IULL_PF, tmp_iull);
            break;
#endif
    }

    return MCM_RCODE_PASS;
}

// 檢查浮點數類型資料.
// this_model_member (I) :
//   使用的 model member.
// read_con (I) :
//   讀取到的資料.
// read_len (I) :
//   讀取到的資料的長度.
// file_source (I) :
//   檔案來源類型.
//     MCM_FSOURCE_DEFAULT : 預設值檔.
//     MCM_FSOURCE_CURRENT : 現在值檔.
//     MCM_FSOURCE_VERIFY  : 待檢查檔.
// file_line (I) :
//   此資料是在檔案中的第幾行.
// value_buf (O) :
//   儲存轉換後的資料緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
#if MCM_SUPPORT_DTYPE_FF || MCM_SUPPORT_DTYPE_FD || MCM_SUPPORT_DTYPE_FLD
int mcm_load_store_check_float(
    struct mcm_config_model_member_t *this_model_member,
    char *read_con,
    MCM_DTYPE_USIZE_TD read_len,
    MCM_DTYPE_LIST_TD file_source,
    MCM_DTYPE_USIZE_TD file_line,
    void *value_buf)
{
    int tmp_error;
    char *tmp_tail;
#if MCM_SUPPORT_DTYPE_FF
    MCM_CLIMIT_FF_TD tmp_ff;
#endif
#if MCM_SUPPORT_DTYPE_FD
    MCM_CLIMIT_FD_TD tmp_fd;
#endif
#if MCM_SUPPORT_DTYPE_FLD
    MCM_CLIMIT_FLD_TD tmp_fld;
#endif


#define MCM_CHECK_FLO_VALUE(limit_api, limit_min, limit_max, type_def, type_fmt, tmp_value) \
    do                                                                              \
    {                                                                               \
        limit_api(read_con, tmp_tail, tmp_error, tmp_value);                        \
        if(tmp_error != 0)                                                          \
        {                                                                           \
            MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG                                  \
                     "invalid value [float], must be float "                        \
                     type_fmt "~" type_fmt " [%s]",                                 \
                     MCM_SSOURCE(file_source), file_line, "INVALID_VALUE-FLOAT_02", \
                     limit_min, limit_max, this_model_member->member_name);         \
            return MCM_RCODE_CONFIG_INTERNAL_ERROR;                                 \
        }                                                                           \
        *((type_def *) value_buf) = tmp_value;                                      \
        MCM_CFDMSG("[%s][" type_fmt "]",                                            \
                   this_model_member->member_name, *((type_def *) value_buf));      \
    }                                                                               \
    while(0)


    MCM_CFDMSG("=> %s", __FUNCTION__);

    if(read_len == 0)
    {
        MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                 "invalid value [float], empty value [%s]",
                 MCM_SSOURCE(file_source), file_line, "INVALID_VALUE-FLOAT_01",
                 this_model_member->member_name);
        return MCM_RCODE_CONFIG_INTERNAL_ERROR;
    }

    switch(this_model_member->member_type)
    {
#if MCM_SUPPORT_DTYPE_FF
        case MCM_DTYPE_FF_INDEX:
            MCM_CHECK_FLO_VALUE(MCM_CLIMIT_FF_API, MCM_CLIMIT_FF_MIN, MCM_CLIMIT_FF_MAX,
                                MCM_DTYPE_FF_TD, MCM_DTYPE_FF_PF, tmp_ff);
            break;
#endif
#if MCM_SUPPORT_DTYPE_FD
        case MCM_DTYPE_FD_INDEX:
            MCM_CHECK_FLO_VALUE(MCM_CLIMIT_FD_API, MCM_CLIMIT_FD_MIN, MCM_CLIMIT_FD_MAX,
                                MCM_DTYPE_FD_TD, MCM_DTYPE_FD_PF, tmp_fd);
            break;
#endif
#if MCM_SUPPORT_DTYPE_FLD
        case MCM_DTYPE_FLD_INDEX:
            MCM_CHECK_FLO_VALUE(MCM_CLIMIT_FLD_API, MCM_CLIMIT_FLD_MIN, MCM_CLIMIT_FLD_MAX,
                                MCM_DTYPE_FLD_TD, MCM_DTYPE_FLD_PF, tmp_fld);
            break;
#endif
    }

    return MCM_RCODE_PASS;
}
#endif

// 檢查字串類型資料.
// this_model_member (I) :
//   使用的 model member.
// read_con (I) :
//   讀取到的資料.
// read_len (I) :
//   讀取到的資料的長度.
// file_source (I) :
//   檔案來源類型.
//     MCM_FSOURCE_DEFAULT : 預設值檔.
//     MCM_FSOURCE_CURRENT : 現在值檔.
//     MCM_FSOURCE_VERIFY  : 待檢查檔.
// file_line (I) :
//   此資料是在檔案中的第幾行.
// value_buf (O) :
//   儲存處理後的資料緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
#if MCM_SUPPORT_DTYPE_S
int mcm_load_store_check_string(
    struct mcm_config_model_member_t *this_model_member,
    char *read_con,
    MCM_DTYPE_USIZE_TD read_len,
    MCM_DTYPE_LIST_TD file_source,
    MCM_DTYPE_USIZE_TD file_line,
    void *value_buf)
{
    unsigned char value_mask[7] = {0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE};
    unsigned char check_mask[7] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
    MCM_DTYPE_USIZE_TD code_len[7] = {1, 0, 2, 3, 4, 5, 6};
    MCM_DTYPE_USIZE_TD didx1, didx2, dlen, midx, clen;
    char tmp_hex, *tmp_loc;
#if MCM_CFDMODE
    MCM_DTYPE_USIZE_TD dbg_dlen;
    char dbg_buf[MCM_DBG_BUFFER_SIZE];
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

    for(didx1 = dlen = 0; didx1 < read_len; dlen++)
        if(read_con[didx1] != MCM_CSTR_SPECIAL_KEY)
        {
            // 特殊字元必須以 hex 表示.
            if((read_con[didx1] < MCM_CSTR_MIN_PRINTABLE_KEY) ||
               (MCM_CSTR_MAX_PRINTABLE_KEY < read_con[didx1]) ||
               (read_con[didx1] == MCM_CSTR_RESERVE_KEY1))
            {
                MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                         "invalid value [string], "
                         "special character (exclude 0x%X~0x%X, '%c', '%c') "
                         "must use %%XX (XX = character's hex value (01~FF)) [%s]",
                         MCM_SSOURCE(file_source), file_line, "INVALID_VALUE-STRING_01",
                         MCM_CSTR_MIN_PRINTABLE_KEY, MCM_CSTR_MAX_PRINTABLE_KEY,
                         MCM_CSTR_RESERVE_KEY1, MCM_CSTR_RESERVE_KEY2,
                         this_model_member->member_name);
                return MCM_RCODE_CONFIG_INTERNAL_ERROR;
            }
            didx1++;
        }
        else
        {
            // hex 必須是 01 ~ FF.
            for(didx2 = didx1 + 1; didx2 < (didx1 + 3); didx2++)
            {
                MCM_CHECK_HEX_RANGE(read_con[didx2]);
                MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                         "invalid value [string], %%XX must be hex (01~FF) [%s]",
                         MCM_SSOURCE(file_source), file_line, "INVALID_VALUE-STRING_02",
                         this_model_member->member_name);
                return MCM_RCODE_CONFIG_INTERNAL_ERROR;
            }
            mcm_hex_to_char(read_con + didx1 + 1, &tmp_hex);
            if(tmp_hex == 0)
            {
                MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                         "invalid value [string], %%XX must be large 0 (01~FF) [%s]",
                         MCM_SSOURCE(file_source), file_line, "INVALID_VALUE-STRING_03",
                         this_model_member->member_name);
                return MCM_RCODE_CONFIG_INTERNAL_ERROR;
            }
            didx1 += 3;
        }

    // 檢查 UTF-8 格式是否正確.
    for(didx1 = 0; didx1 < read_len;)
        if(read_con[didx1] != MCM_CSTR_SPECIAL_KEY)
        {
            didx1++;
        }
        else
        {
            mcm_hex_to_char(read_con + didx1 + 1, &tmp_hex);
            if((tmp_hex & value_mask[0]) == check_mask[0])
            {
                didx1 += code_len[0] * 3;
            }
            else
            {
                for(midx = 2; midx < 7; midx++)
                    if((tmp_hex & value_mask[midx]) == check_mask[midx])
                        break;
                if(midx == 7)
                {
                    MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                             "invalid value [string], invalid UTF-8 encode [%s]",
                             MCM_SSOURCE(file_source), file_line, "INVALID_VALUE-STRING_04",
                             this_model_member->member_name);
                    return MCM_RCODE_CONFIG_INTERNAL_ERROR;
                }

                tmp_loc = read_con + didx1 + 3;
                clen = code_len[midx] - 1;
                for(didx2 = 0; didx2 < clen; didx2++, tmp_loc += 3)
                {
                    mcm_hex_to_char(tmp_loc + 1, &tmp_hex);
                    if((tmp_hex & value_mask[1]) != check_mask[1])
                    {
                        MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                                 "invalid value [string], invalid UTF-8 encode [%s]",
                                 MCM_SSOURCE(file_source), file_line, "INVALID_VALUE-STRING_05",
                                 this_model_member->member_name);
                        return MCM_RCODE_CONFIG_INTERNAL_ERROR;
                    }
                }

                didx1 += code_len[midx] * 3;
            }
        }

    if(dlen >= this_model_member->member_size)
    {
        MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                 "invalid value [string], string length over buffer size "
                 "(" MCM_DTYPE_USIZE_PF "/" MCM_DTYPE_USIZE_PF ") [%s]",
                 MCM_SSOURCE(file_source), file_line, "INVALID_VALUE-STRING_06", dlen,
                 this_model_member->member_size, this_model_member->member_name);
        return MCM_RCODE_CONFIG_INTERNAL_ERROR;
    }

    for(didx1 = didx2 = 0; didx1 < read_len; didx2++)
        if(read_con[didx1] != MCM_CSTR_SPECIAL_KEY)
        {
            *(((MCM_DTYPE_S_TD *) value_buf) + didx2) = read_con[didx1];
            didx1++;
        }
        else
        {
            mcm_hex_to_char(read_con + didx1 + 1, ((MCM_DTYPE_S_TD *) value_buf) + didx2);
            didx1 += 3;
        }
    *(((MCM_DTYPE_S_TD *) value_buf) + didx2) = '\0';

#if MCM_CFDMODE
    dbg_dlen = read_len == 0 ? 0 : didx2 - 1;
    MCM_DBG_FORMAT_CONFIG_S_VALUE(dbg_buf, value_buf, dbg_dlen, didx1, didx2);
    MCM_CFDMSG("[%s][%s]", this_model_member->member_name, dbg_buf);
#endif

    return MCM_RCODE_PASS;
}
#endif

// 檢查字節流類型資料.
// this_model_member (I) :
//   使用的 model member.
// read_con (I) :
//   讀取到的資料.
// read_len (I) :
//   讀取到的資料的長度.
// file_source (I) :
//   檔案來源類型.
//     MCM_FSOURCE_DEFAULT : 預設值檔.
//     MCM_FSOURCE_CURRENT : 現在值檔.
//     MCM_FSOURCE_VERIFY  : 待檢查檔.
// file_line (I) :
//   此資料是在檔案中的第幾行.
// value_buf (O) :
//   儲存處理後的資料緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
#if MCM_SUPPORT_DTYPE_B
int mcm_load_store_check_bytes(
    struct mcm_config_model_member_t *this_model_member,
    char *read_con,
    MCM_DTYPE_USIZE_TD read_len,
    MCM_DTYPE_LIST_TD file_source,
    MCM_DTYPE_USIZE_TD file_line,
    void *value_buf)
{
    MCM_DTYPE_USIZE_TD didx1, didx2 = 0;
#if MCM_CFDMODE
    MCM_DTYPE_USIZE_TD dbg_dlen;
    char dbg_buf[MCM_DBG_BUFFER_SIZE];
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

    // 必須是 hex 格式.
    for(didx1 = 0; didx1 < read_len; didx1++)
    {
        MCM_CHECK_HEX_RANGE(read_con[didx1]);
        MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                 "invalid value [bytes], %%XX must be hex (00~FF) [%s]",
                 MCM_SSOURCE(file_source), file_line, "INVALID_VALUE-BYTES_01",
                 this_model_member->member_name);
        return MCM_RCODE_CONFIG_INTERNAL_ERROR;
    }

    if(didx1 > 0)
    {
        if((didx1 % 2) != 0)
        {
            MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                     "invalid value [bytes], %%XX must be hex (00~FF) [%s]",
                     MCM_SSOURCE(file_source), file_line, "INVALID_VALUE-BYTES_02",
                     this_model_member->member_name);
            return MCM_RCODE_CONFIG_INTERNAL_ERROR;
        }

        didx1 /= 2;
        if(didx1 > this_model_member->member_size)
        {
            MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                     "invalid value [bytes], bytes length over buffer size "
                     "(" MCM_DTYPE_USIZE_PF "/" MCM_DTYPE_USIZE_PF ") [%s]",
                     MCM_SSOURCE(file_source), file_line, "INVALID_VALUE-BYTES_03", didx1,
                     this_model_member->member_size, this_model_member->member_name);
            return MCM_RCODE_CONFIG_INTERNAL_ERROR;
        }

        for(didx1 = didx2 = 0; didx1 < read_len; didx1 += 2, didx2++)
            mcm_hex_to_hex(read_con + didx1, ((MCM_DTYPE_B_TD *) value_buf) + didx2);
    }
    else
    {
        memset(value_buf, 0, this_model_member->member_size);
    }

#if MCM_CFDMODE
    dbg_dlen = didx1 > 0 ? read_len == 0 ? 0 : didx2 : this_model_member->member_size;
    MCM_DBG_FORMAT_CONFIG_B_VALUE(dbg_buf, value_buf, dbg_dlen, didx1, didx2);
    MCM_CFDMSG("[%s][%s]", this_model_member->member_name, dbg_buf);
#endif

    return MCM_RCODE_PASS;
}
#endif

// 設定讀取到的 member 數值.
// this_model_group (I) :
//   使用的 model group.
// this_model_member (I) :
//   使用的 model member.
// this_store (I) :
//   使用的 store.
// read_con (I) :
//   讀取到的資料.
// read_len (I) :
//   讀取到的資料的長度.
// file_source (I) :
//   檔案來源類型.
//     MCM_FSOURCE_DEFAULT : 預設值檔.
//     MCM_FSOURCE_CURRENT : 現在值檔.
//     MCM_FSOURCE_VERIFY  : 待檢查檔.
// file_line (I) :
//   此資料是在檔案中的第幾行.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_load_store_assign_value(
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_model_member_t *this_model_member,
    struct mcm_config_store_t *this_store,
    char *read_con,
    MCM_DTYPE_USIZE_TD read_len,
    MCM_DTYPE_LIST_TD file_source,
    MCM_DTYPE_USIZE_TD file_line)
{
    int fret;
    void *data_loc;


    MCM_CFDMSG("=> %s", __FUNCTION__);
    MCM_CFDMSG("[" MCM_DTYPE_USIZE_PF "][%s]", read_len, read_con);

    data_loc = this_store->data_value_sys + this_model_member->offset_in_value;

    switch(this_model_member->member_type)
    {
        case MCM_DTYPE_EK_INDEX:
#if MCM_SUPPORT_DTYPE_RK
        case MCM_DTYPE_RK_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISC
        case MCM_DTYPE_ISC_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUC
        case MCM_DTYPE_IUC_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISS
        case MCM_DTYPE_ISS_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUS
        case MCM_DTYPE_IUS_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISI
        case MCM_DTYPE_ISI_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUI
        case MCM_DTYPE_IUI_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISLL
        case MCM_DTYPE_ISLL_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IULL
        case MCM_DTYPE_IULL_INDEX:
#endif
            fret = mcm_load_store_check_integer(this_model_member, read_con, read_len,
                                                file_source, file_line, data_loc);
            if(fret < MCM_RCODE_PASS)
                return fret;
            break;
#if MCM_SUPPORT_DTYPE_FF
        case MCM_DTYPE_FF_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FD
        case MCM_DTYPE_FD_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FLD
        case MCM_DTYPE_FLD_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FF || MCM_SUPPORT_DTYPE_FD || MCM_SUPPORT_DTYPE_FLD
            fret = mcm_load_store_check_float(this_model_member, read_con, read_len,
                                              file_source, file_line, data_loc);
            if(fret < MCM_RCODE_PASS)
                return fret;
            break;
#endif
#if MCM_SUPPORT_DTYPE_S
        case MCM_DTYPE_S_INDEX:
            fret = mcm_load_store_check_string(this_model_member, read_con, read_len,
                                               file_source, file_line, data_loc);
            if(fret < MCM_RCODE_PASS)
                return fret;
            break;
#endif
#if MCM_SUPPORT_DTYPE_B
        case MCM_DTYPE_B_INDEX:
            fret = mcm_load_store_check_bytes(this_model_member, read_con, read_len,
                                              file_source, file_line, data_loc);
            if(fret < MCM_RCODE_PASS)
                return fret;
            break;
#endif
    }

    return MCM_RCODE_PASS;
}

// 分析讀取到的 member 串列資料.
// this_model_group (I) :
//   使用的 model group.
// this_store (I) :
//   使用的 store.
// read_con (I) :
//   讀取到的資料.
// read_len (I) :
//   讀取到的資料的長度.
// file_source (I) :
//   檔案來源類型.
//     MCM_FSOURCE_DEFAULT : 預設值檔.
//     MCM_FSOURCE_CURRENT : 現在值檔.
//     MCM_FSOURCE_VERIFY  : 待檢查檔.
// file_line (I) :
//   此資料是在檔案中的第幾行.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_load_store_anysis_member(
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    char *read_con,
    MCM_DTYPE_USIZE_TD read_len,
    MCM_DTYPE_LIST_TD file_source,
    MCM_DTYPE_USIZE_TD file_line)
{
    int fret;
    struct mcm_config_model_member_t *self_model_member;
    MCM_DTYPE_USIZE_TD ridx, ploc, plen, xidx, nloc, nlen, dloc, dlen;
    MCM_DTYPE_DS_TD tmp_status1, tmp_status2 = 0;
    MCM_DTYPE_BOOL_TD lose_member = 0;


    MCM_CFDMSG("=> %s", __FUNCTION__);
    MCM_CFDMSG("[" MCM_DTYPE_USIZE_PF "][%s]", read_len, read_con);

    for(ploc = ridx = 0; ridx <= read_len; ridx++)
    {
        if((read_con[ridx] == MCM_SPROFILE_PARAMETER_SPLIT_KEY) || (read_con[ridx] == '\0'))
        {
            read_con[ridx] = '\0';

            plen = ridx - ploc;
            MCM_CFDMSG("parameter[" MCM_DTYPE_USIZE_PF "][%s]", plen, read_con + ploc);

            if(plen == 0)
            {
                MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                         "invalid member, not find any member",
                         MCM_SSOURCE(file_source), file_line, "INVALID_MEMBER_01");
                if(file_source == MCM_FSOURCE_DEFAULT)
                    return MCM_RCODE_CONFIG_INTERNAL_ERROR;
                else
                    goto NEXT_PARAMETER;
            }

            for(xidx = 0; xidx < plen; xidx++)
                if(read_con[ploc + xidx] == MCM_SPROFILE_MEMBER_SPLIT_KEY)
                    break;
            if(xidx == plen)
            {
                MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                         "invalid member, not find member-value split key [%c] [%s]",
                         MCM_SSOURCE(file_source), file_line, "INVALID_MEMBER_02",
                         MCM_SPROFILE_MEMBER_SPLIT_KEY, read_con + ploc);
                if(file_source == MCM_FSOURCE_DEFAULT)
                {
                    return MCM_RCODE_CONFIG_INTERNAL_ERROR;
                }
                else
                {
                    mcm_config_set_entry_self_status(NULL, this_model_group, this_store,
                                                     MCM_DSASSIGN_ADD,
                                                     MCM_DSERROR_UNKNOWN_PARAMETER);
                    goto NEXT_PARAMETER;
                }
            }
            read_con[ploc + xidx] = '\0';

            nloc = ploc;
            nlen = xidx;
            dloc = ploc + xidx + 1;
            dlen = plen - (xidx + 1);

            self_model_member = mcm_tree_search_member(this_model_group->member_tree,
                                                       read_con + nloc, nlen);
            MCM_CFDMSG("search name(member)[" MCM_DTYPE_USIZE_PF "][%s][%s]",
                       nlen, read_con + nloc, self_model_member == NULL ? "unknown" : "find");
            if(self_model_member == NULL)
            {
                MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                         "invalid member, unknown member [%s]",
                         MCM_SSOURCE(file_source), file_line, "INVALID_MEMBER_03", read_con + nloc);
                if(file_source == MCM_FSOURCE_DEFAULT)
                {
                    return MCM_RCODE_CONFIG_INTERNAL_ERROR;
                }
                else
                {
                    mcm_config_set_entry_self_status(NULL, this_model_group, this_store,
                                                     MCM_DSASSIGN_ADD, MCM_DSERROR_UNKNOWN_MEMBER);
                    goto NEXT_PARAMETER;
                }
            }

            MCM_GET_ALONE_STATUS(self_model_member, this_store, tmp_status1);

            MCM_CFDMSG("check duplic(member)[%s][%s]",
                       self_model_member->member_name,
                       (tmp_status1 & MCM_DSERROR_LOSE_MEMBER) == 0 ? "duplic" : "first");
            if((tmp_status1 & MCM_DSERROR_LOSE_MEMBER) == 0)
            {
                MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                         "invalid member, member duplic [%s]",
                         MCM_SSOURCE(file_source), file_line, "INVALID_MEMBER_04", read_con + nloc);
                if(file_source == MCM_FSOURCE_DEFAULT)
                {
                    return MCM_RCODE_CONFIG_INTERNAL_ERROR;
                }
                else
                {
                    mcm_config_set_alone_status(NULL, this_model_group, self_model_member,
                                                this_store, MCM_DSASSIGN_ADD,
                                                MCM_DSERROR_DUPLIC_MEMBER);
                    goto NEXT_PARAMETER;
                }
            }

            mcm_config_set_alone_status(NULL, this_model_group, self_model_member, this_store,
                                        MCM_DSASSIGN_DEL, MCM_DSERROR_LOSE_MEMBER);

            fret = mcm_load_store_assign_value(this_model_group, self_model_member, this_store,
                                               read_con + dloc, dlen, file_source, file_line);
            if(fret < MCM_RCODE_PASS)
            {
                if(file_source == MCM_FSOURCE_DEFAULT)
                {
                    return fret;
                }
                else
                {
                    mcm_config_set_alone_status(NULL, this_model_group, self_model_member,
                                                this_store, MCM_DSASSIGN_ADD,
                                                MCM_DSERROR_INVALID_VALUE);
                    goto NEXT_PARAMETER;
                }
            }

NEXT_PARAMETER:
            ploc = ridx + 1;
        }
    }

    for(self_model_member = this_model_group->member_list; self_model_member != NULL;
        self_model_member = self_model_member->next_model_member)
    {
        if(self_model_member->member_type == MCM_DTYPE_EK_INDEX)
            continue;

        MCM_GET_ALONE_STATUS(self_model_member, this_store, tmp_status1);

        if((tmp_status1 & MCM_DSERROR_LOSE_MEMBER) != 0)
        {
            MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                     "invalid member, member lose [%s]",
                     MCM_SSOURCE(file_source), file_line, "INVALID_MEMBER_05",
                     self_model_member->member_name);
            if(file_source == MCM_FSOURCE_DEFAULT)
                return MCM_RCODE_CONFIG_INTERNAL_ERROR;
        }

        MCM_CFDMSG("check status(member)[%s][%s]",
                   self_model_member->member_name,
                   (tmp_status1 & MCM_DSERROR_MASK) != 0 ? "fail" : "pass");
        if((tmp_status1 & MCM_DSERROR_MASK) != 0)
        {
            tmp_status2 |= tmp_status1 & MCM_DSERROR_DUPLIC_MEMBER ? MCM_DSERROR_DUPLIC_MEMBER : 0;
            tmp_status2 |= tmp_status1 & MCM_DSERROR_INVALID_VALUE ? MCM_DSERROR_INVALID_VALUE : 0;
            lose_member |= tmp_status1 & MCM_DSERROR_LOSE_MEMBER ? 1 : 0;
        }
        else
        {
            mcm_config_set_alone_status(NULL, this_model_group, self_model_member, this_store,
                                        MCM_DSASSIGN_SET, 0);
        }
    }

    MCM_GET_ENTRY_SELF_STATUS(this_model_group, this_store, tmp_status1);

    tmp_status1 |= tmp_status2;
    if(lose_member == 0)
        tmp_status1 &= ~MCM_DSERROR_LOSE_MEMBER;

    mcm_config_set_entry_self_status(NULL, this_model_group, this_store,
                                     MCM_DSASSIGN_SET, tmp_status1);

    return MCM_RCODE_PASS;
}

// 建立缺少的 store.
// this_store (I) :
//   使用的 store tree.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_load_store_fill_lose_store(
    struct mcm_config_store_t *this_store)
{
    int fret;
    struct mcm_config_model_group_t *self_model_group, *child_model_group;
    struct mcm_config_store_t *child_store, **store_head_in_parent;
    MCM_DTYPE_DS_TD tmp_status;
    MCM_DTYPE_USIZE_TD cidx;
#if MCM_CFDMODE
    void *dbg_dloc;
    MCM_DTYPE_EK_TD dbg_key = 0;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

    // 對於建立的 store, 檢查其 child 是否為 gs 類型而且不存在, 是的話建立此 child.
    for(; this_store != NULL; this_store = this_store->next_store)
    {
        self_model_group = this_store->link_model_group;

#if MCM_CFDMODE
        MCM_DBG_SHOW_ENTRY_PATH(self_model_group, this_store, dbg_dloc, dbg_key);
#endif

        for(child_model_group = self_model_group->child_model_group_list;
            child_model_group != NULL;
            child_model_group = child_model_group->next_model_group)
        {
            if(child_model_group->group_type == MCM_DTYPE_GS_INDEX)
            {
                store_head_in_parent = this_store->child_store_list_head_array +
                                       child_model_group->store_index_in_parent;
                MCM_CFDMSG("check lose[%s.%c" MCM_DTYPE_EK_PF "][%s][" MCM_DTYPE_BOOL_PF "]",
                           child_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, 0,
                           *store_head_in_parent == NULL ? "lose" : "find",
                           child_model_group->group_save);
                if(*store_head_in_parent == NULL)
                {
                    fret = mcm_create_store(child_model_group, this_store, 1, 0, 0, 0,
                                            &child_store);
                    if(fret < MCM_RCODE_PASS)
                    {
                        MCM_EMSG("call mcm_create_store() fail");
                        return fret;
                    }

                    if(child_model_group->member_real_count > 0)
                        if(child_model_group->group_save != 0)
                        {
                            tmp_status = MCM_DSERROR_LOSE_ENTRY | MCM_DSERROR_LOSE_MEMBER;
                            mcm_config_set_entry_self_status(NULL, child_model_group, child_store,
                                                             MCM_DSASSIGN_SET, tmp_status);
                            tmp_status = MCM_DSERROR_LOSE_MEMBER;
                            mcm_config_set_entry_all_status(NULL, child_model_group, child_store,
                                                            MCM_DSASSIGN_SET, tmp_status, 1);
                        }
                }
            }
        }

        for(cidx = 0; cidx < self_model_group->store_child_count; cidx++)
        {
            store_head_in_parent = this_store->child_store_list_head_array + cidx;
            MCM_CFDMSG("trace[%s.%c" MCM_DTYPE_EK_PF "] child[%p(%p)]",
                       self_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key,
                       store_head_in_parent, *store_head_in_parent);
            if((child_store = *store_head_in_parent) != NULL)
            {
                fret = mcm_load_store_fill_lose_store(child_store);
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_EMSG("call mcm_load_store_fill_lose_store() fail");
                    return fret;
                }
            }
        }
    }

    return MCM_RCODE_PASS;
}

// 檢查處理錯誤情況.
// this_model_group (I) :
//   使用的 model group tree.
// this_store (I) :
//   使用的 store tree.
// file_source (I) :
//   檔案來源類型.
//     MCM_FSOURCE_DEFAULT : 預設值檔.
//     MCM_FSOURCE_CURRENT : 現在值檔.
//     MCM_FSOURCE_VERIFY  : 待檢查檔.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_load_store_fill_status(
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_LIST_TD file_source)
{
    int fret;
    struct mcm_config_store_t *each_store, *child_store, **store_head_in_parent;
    MCM_DTYPE_DS_TD tmp_status;
    MCM_DTYPE_USIZE_TD cidx;
#if MCM_CFDMODE
    void *dbg_dloc;
    MCM_DTYPE_EK_TD dbg_key1 = 0, dbg_key2;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    MCM_DBG_SHOW_ENTRY_PATH(this_model_group, this_store, dbg_dloc, dbg_key1);
#endif

    for(; this_store != NULL; this_store = this_store->next_store)
    {
        MCM_GET_ENTRY_SELF_STATUS(this_model_group, this_store, tmp_status);

#if MCM_CFDMODE
        MCM_DBG_GET_ENTRY_KEY(this_model_group, this_store, dbg_dloc, dbg_key2);
        MCM_CFDMSG("check error(entry)[%s.%c" MCM_DTYPE_EK_PF "][%s]",
                   this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key2,
                   (tmp_status & MCM_DSERROR_MASK) != 0 ? "still" : "clear");
#endif
        if((tmp_status & MCM_DSERROR_MASK) != 0)
        {
            MCM_CFDMSG("check lose(entry)[%s.%c" MCM_DTYPE_EK_PF "]"
                       "[" MCM_DTYPE_USIZE_PF "][%s][%s]",
                       this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key2,
                       this_model_group->store_child_count,
                       (tmp_status & MCM_DSERROR_LOSE_ENTRY) != 0 ? "lose-self" : "find-self",
                       (tmp_status & MCM_DSERROR_LOSE_PARENT) != 0 ? "lose-parent" : "find-parent");

            if((tmp_status & MCM_DSERROR_LOSE_ENTRY) != 0)
            {
                MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                         "invalid entry, entry lose [%s]",
                         MCM_SSOURCE(file_source), 0, "INVALID_ENTRY-LOSE_01",
                         this_store->link_model_group->group_name);
                if(file_source == MCM_FSOURCE_DEFAULT)
                    return MCM_RCODE_CONFIG_INTERNAL_ERROR;
            }

            if((tmp_status & (MCM_DSERROR_LOSE_ENTRY | MCM_DSERROR_LOSE_PARENT)) != 0)
                for(cidx = 0; cidx < this_model_group->store_child_count; cidx++)
                {
                    store_head_in_parent = this_store->child_store_list_head_array + cidx;
                    for(each_store = *store_head_in_parent; each_store != NULL;
                        each_store = each_store->next_store)
                    {
                        if(each_store->link_model_group->group_save != 0)
                        {
                            mcm_config_set_entry_self_status(NULL, each_store->link_model_group,
                                                             each_store, MCM_DSASSIGN_ADD,
                                                             MCM_DSERROR_LOSE_PARENT);
                        }
                    }
                }
        }
        else
        {
            mcm_config_set_entry_self_status(NULL, this_model_group, this_store,
                                             MCM_DSASSIGN_SET, 0);
        }

        for(cidx = 0; cidx < this_model_group->store_child_count; cidx++)
        {
            store_head_in_parent = this_store->child_store_list_head_array + cidx;
            MCM_CFDMSG("trace[%s.%c" MCM_DTYPE_EK_PF "] child[%p(%p)]",
                       this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key1,
                       store_head_in_parent, *store_head_in_parent);
            if((child_store = *store_head_in_parent) != NULL)
            {
                fret = mcm_load_store_fill_status(child_store->link_model_group, child_store,
                                                  file_source);
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_EMSG("call mcm_load_store_fill_status() fail");
                    return fret;
                }
            }
        }
    }

    return MCM_RCODE_PASS;
}

// 檢查建立的 strore 是否有錯誤.
// this_model_group (I) :
//   使用的 model group tree.
// this_store (I) :
//   使用的 store tree.
// return :
//   >= MCM_RCODE_PASS : 無錯誤.
//   <  MCM_RCODE_PASS : 有錯誤.
int mcm_load_store_check_error(
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store)
{
    struct mcm_config_store_t *child_store, **store_head_in_parent;
    MCM_DTYPE_DS_TD tmp_status;
    MCM_DTYPE_USIZE_TD cidx;
#if MCM_CFDMODE
    void *dbg_dloc;
    MCM_DTYPE_EK_TD dbg_key1 = 0, dbg_key2;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    MCM_DBG_SHOW_ENTRY_PATH(this_model_group, this_store, dbg_dloc, dbg_key1);
#endif

    for(; this_store != NULL; this_store = this_store->next_store)
    {
        MCM_GET_ENTRY_SELF_STATUS(this_model_group, this_store, tmp_status);

#if MCM_CFDMODE
        MCM_DBG_GET_ENTRY_KEY(this_model_group, this_store, dbg_dloc, dbg_key2);
        MCM_CFDMSG("check (entry)[%s.%c" MCM_DTYPE_EK_PF "][" MCM_DTYPE_DS_PF "]",
                   this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key2, tmp_status);
#endif
        if(tmp_status != 0)
            return MCM_RCODE_CONFIG_INTERNAL_ERROR;

        for(cidx = 0; cidx < this_model_group->store_child_count; cidx++)
        {
            store_head_in_parent = this_store->child_store_list_head_array + cidx;
            MCM_CFDMSG("trace[%s.%c" MCM_DTYPE_EK_PF "] child[%p(%p)]",
                       this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key1,
                       store_head_in_parent, *store_head_in_parent);
            if((child_store = *store_head_in_parent) != NULL)
                if(mcm_load_store_check_error(child_store->link_model_group, child_store)
                                              < MCM_RCODE_PASS)
                {
                    return MCM_RCODE_CONFIG_INTERNAL_ERROR;
                }
        }
    }

    return MCM_RCODE_PASS;
}

// 處理讀取 store 檔案.
// this_model_group (I) :
//   使用的 model group.
// file_fp (I) :
//   目標檔案.
// file_source (I) :
//   檔案來源類型.
//     MCM_FSOURCE_DEFAULT : 預設值檔.
//     MCM_FSOURCE_CURRENT : 現在值檔.
//     MCM_FSOURCE_VERIFY  : 待檢查檔.
// read_buf_buf (I) :
//   讀取緩衝.
// read_size_buf (I) :
//   讀取緩衝的大小.
// base_data_buf (I)
//   儲存額外資料的緩衝.
// new_store_buf (O) :
//   要建立的 store tree 的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_load_store_process(
    struct mcm_config_model_group_t *this_model_group,
    FILE *file_fp,
    MCM_DTYPE_LIST_TD file_source,
    char **read_buf_buf,
    MCM_DTYPE_USIZE_TD *read_size_buf,
    struct mcm_config_base_t *base_data_buf,
    struct mcm_config_store_t **new_store_buf)
{
    int fret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *self_store;
    struct mcm_load_store_t self_load_store[MCM_CONFIG_PATH_MAX_LEVEL];
    char *read_con;
    MCM_DTYPE_USIZE_TD read_len, ridx, ploc, plen, mloc, mlen, file_line = 0, self_level;
    MCM_DTYPE_BOOL_TD read_eof;
    MCM_DTYPE_DS_TD tmp_status;
#if MCM_CFDMODE
    void *dbg_dloc;
    MCM_DTYPE_EK_TD dbg_key;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

    // 預先建立根結點, 供後續加入的結點掛載.
    fret = mcm_create_store(mcm_config_root_model_group, NULL, 1, 0, 0, 0, new_store_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_create_store() fail");
        goto FREE_01;
    }
    if(mcm_config_root_model_group->member_real_count > 0)
        if(mcm_config_root_model_group->group_save != 0)
        {
            tmp_status = MCM_DSERROR_LOSE_MEMBER | MCM_DSERROR_LOSE_ENTRY;
            mcm_config_set_entry_self_status(NULL, mcm_config_root_model_group, *new_store_buf,
                                             MCM_DSASSIGN_SET, tmp_status);
            tmp_status = MCM_DSERROR_LOSE_MEMBER;
            mcm_config_set_entry_all_status(NULL, mcm_config_root_model_group, *new_store_buf,
                                            MCM_DSASSIGN_SET, tmp_status, 1);
        }

    while(1)
    {
        file_line++;

        fret = mcm_read_file(file_fp, read_buf_buf, read_size_buf, &read_len, &read_eof);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_EMSG("call mcm_read_file() fail");
            goto FREE_02;
        }
        else
        if(read_eof == 1)
            break;

        if(read_len == 0)
            continue;

        read_con = *read_buf_buf;
        if(read_con[0] == MCM_SPROFILE_COMMENT_KEY)
            continue;

        MCM_CFDMSG("<" MCM_DTYPE_USIZE_PF ">[%s]", file_line, read_con);

        if(read_con[0] == MCM_SPROFILE_BASE_DATA_KEY)
        {
            fret = mcm_load_store_base_data(read_con + 1, read_len - 1, file_source, file_line,
                                            base_data_buf);
            if(fret < MCM_RCODE_PASS)
            {
                if(file_source == MCM_FSOURCE_DEFAULT)
                    goto FREE_02;
                else
                    continue;
            }
            continue;
        }

        for(ridx = 0; ridx < read_len; ridx++)
            if(read_con[ridx] == MCM_SPROFILE_PARAMETER_SPLIT_KEY)
                break;
        if(ridx == read_len)
        {
            MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                     "invalid format, not find split key [%c]",
                     MCM_SSOURCE(file_source), file_line, "INVALID_FORMAT_01",
                     MCM_SPROFILE_PARAMETER_SPLIT_KEY);
            if(file_source == MCM_FSOURCE_DEFAULT)
            {
                fret = MCM_RCODE_CONFIG_INTERNAL_ERROR;
                goto FREE_02;
            }
            else
            {
                continue;
            }
        }
        read_con[ridx] = '\0';

        ploc = 0;
        plen = ridx;
        mloc = ridx < read_len ? ridx + 1 : ridx;
        mlen = ridx < read_len ? read_len - (ridx + 1) : 0;

        fret = mcm_load_store_find_model(this_model_group, read_con + ploc, plen,
                                         file_source, file_line, self_load_store, &self_level,
                                         &self_model_group);
        if(fret < MCM_RCODE_PASS)
        {
            if(file_source == MCM_FSOURCE_DEFAULT)
                goto FREE_02;
            else
                continue;
        }

        if(self_model_group->group_save == 0)
        {
            MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                     "invalid entry, this group not save any entry",
                     MCM_SSOURCE(file_source), file_line, "INVALID_ENTRY-SAVE_01");
            if(file_source == MCM_FSOURCE_DEFAULT)
            {
                fret = MCM_RCODE_CONFIG_INTERNAL_ERROR;
                goto FREE_02;
            }
            else
            {
                continue;
            }
        }

        fret = mcm_load_store_check_count(*new_store_buf, self_load_store, self_level,
                                          file_source, file_line);
        if(fret < MCM_RCODE_PASS)
        {
            if(file_source == MCM_FSOURCE_DEFAULT)
                return MCM_RCODE_CONFIG_INTERNAL_ERROR;
            else
                continue;
        }

        // 沒有任何 member 的 group 不會紀錄.
        if(self_model_group->member_real_count == 0)
        {
            MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                     "invalid entry, this group no any member",
                     MCM_SSOURCE(file_source), file_line, "INVALID_ENTRY-MEMBER_01");
            if(file_source == MCM_FSOURCE_DEFAULT)
            {
                fret = MCM_RCODE_CONFIG_INTERNAL_ERROR;
                goto FREE_02;
            }
            else
            {
                continue;
            }
        }

        if(mcm_load_store_find_store(*new_store_buf, self_load_store, self_level, &self_store)
                                     < MCM_RCODE_PASS)
        {
            fret = mcm_load_store_fill_store(self_load_store, self_level, &self_store);
            if(fret < MCM_RCODE_PASS)
            {
                MCM_EMSG("call mcm_load_store_fill_store() fail");
                goto FREE_02;
            }
        }
        else
        {
            MCM_GET_ENTRY_SELF_STATUS(self_model_group, self_store, tmp_status);

#if MCM_CFDMODE
            MCM_DBG_GET_ENTRY_KEY(self_model_group, self_store, dbg_dloc, dbg_key);
            MCM_CFDMSG("check duplic(entry)[%s.%c" MCM_DTYPE_EK_PF "][%s]",
                       self_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key,
                       (tmp_status & MCM_DSERROR_LOSE_ENTRY) == 0 ? "duplic" : "first");
#endif
            if((tmp_status & MCM_DSERROR_LOSE_ENTRY) != 0)
            {
                mcm_config_set_entry_self_status(NULL, self_model_group, self_store,
                                                 MCM_DSASSIGN_DEL, MCM_DSERROR_LOSE_ENTRY);
            }
            else
            {
                MCM_EMSG(MCM_SPROFILE_ERROR_PREFIX_MSG
                         "invalid entry, path duplic",
                         MCM_SSOURCE(file_source), file_line, "INVALID_ENTRY-DUPLIC_01");
                if(file_source == MCM_FSOURCE_DEFAULT)
                {
                    fret = MCM_RCODE_CONFIG_INTERNAL_ERROR;
                    goto FREE_02;
                }
                else
                {
                    mcm_config_set_entry_self_status(NULL, self_model_group, self_store,
                                                     MCM_DSASSIGN_ADD, MCM_DSERROR_DUPLIC_ENTRY);
                    continue;
                }
            }
        }

        fret = mcm_load_store_anysis_member(self_model_group, self_store, read_con + mloc, mlen,
                                            file_source, file_line);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_EMSG("call mcm_load_store_anysis_member() fail");
            goto FREE_02;
        }
    }

    fret = mcm_load_store_fill_lose_store(*new_store_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_load_store_fill_lose_store() fail");
        goto FREE_02;
    }

    fret = mcm_load_store_fill_status(this_model_group, *new_store_buf, file_source);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_load_store_fill_status() fail");
        goto FREE_02;
    }

    fret = MCM_RCODE_PASS;
FREE_02:
    if(fret < MCM_RCODE_PASS)
        mcm_destory_store(*new_store_buf);
FREE_01:
    return fret;
}

// 讀取檔案建立 store tree.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_load_store(
    void)
{
    int fret;
    FILE *file_fp;
    char *target_file, *read_buf = NULL;
    MCM_DTYPE_LIST_TD file_source;
    MCM_DTYPE_USIZE_TD read_size = 0;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    memset(&mcm_config_base_data, 0, sizeof(struct mcm_config_base_t));

    if(access(mcm_config_store_current_profile_path, F_OK) == 0)
    {
        target_file = mcm_config_store_current_profile_path;
        file_source = MCM_FSOURCE_CURRENT;
    }
    else
    if(access(mcm_config_store_default_profile_path, F_OK) == 0)
    {
        target_file = mcm_config_store_default_profile_path;
        file_source = MCM_FSOURCE_DEFAULT;
    }
    else
    {
        MCM_EMSG("not find[%s][%s]",
                 mcm_config_store_current_profile_path, mcm_config_store_default_profile_path);
        fret = MCM_RCODE_CONFIG_INTERNAL_ERROR;
        goto FREE_01;
    }
    mcm_config_store_profile_source = file_source;
    MCM_CFDMSG("load store[%s]",
               target_file == mcm_config_store_default_profile_path ? "default" : "current");

    file_fp = fopen(target_file, "r");
    if(file_fp == NULL)
    {
        MCM_EMSG("call fopen(%s) fail [%s]", target_file, strerror(errno));
        fret = MCM_RCODE_CONFIG_INTERNAL_ERROR;
        goto FREE_01;
    }

    fret = mcm_realloc_buf_config(&read_buf, &read_size);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_realloc_buf_config() fail");
        goto FREE_02;
    }

    fret = mcm_load_store_process(mcm_config_root_model_group, file_fp, file_source,
                                  &read_buf, &read_size, &mcm_config_base_data,
                                  &mcm_config_root_store);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_load_store_process() fail");
        goto FREE_03;
    }

    mcm_config_data_error = 0;
    fret = mcm_load_store_check_error(mcm_config_root_model_group, mcm_config_root_store);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_load_store_check_error() fail");

        // mcm_do_save :
        // 如果是 MCM_EHANDLE_MANUAL_HANDEL_INTERNAL / MCM_EHANDLE_MANUAL_HANDEL_EXTERNAL,
        // 並且在 mcm_action_boot_profile_run() 或 mcm_action_boot_other_run()
        // 都沒有執行內部模組, 或是有執行內部模組但是沒有修改任何資料,
        // 會造成 mcm_update_store_head 或 mcm_do_save 沒被設定,
        // 之後在 mcm_action_boot_other_run() 最後要儲存資料時,
        // 因為 mcm_do_save 沒被設定所以判定不用儲存, 導致正確的資料沒儲存,
        // 發生資料錯誤時, 設定 mcm_do_save = 1.
        mcm_config_data_error = mcm_do_save = 1;

        if(file_source == MCM_FSOURCE_DEFAULT)
            goto FREE_04;
    }

    fret = MCM_RCODE_PASS;
FREE_04:
    if(fret < MCM_RCODE_PASS)
        mcm_config_free_store();
FREE_03:
    free(read_buf);
FREE_02:
    fclose(file_fp);
FREE_01:
    return fret;
}

// 將 store 資料寫入檔案.
// this_store (I) :
//   使用的 store tree.
// store_path_con (I) :
//   紀錄目前的路徑的內容.
// store_path_len (I) :
//   紀錄目前的路徑的長度.
// file_fp (I) :
//   目標檔案.
// return :
//   MCM_RCODE_PASS.
int mcm_save_store_process(
    struct mcm_config_store_t *this_store,
    char *store_path_con,
    MCM_DTYPE_USIZE_TD store_path_len,
    FILE *file_fp)
{
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_model_member_t *self_model_member;
    struct mcm_config_store_t **store_head_in_parent;
    MCM_DTYPE_EK_TD self_key;
    MCM_DTYPE_USIZE_TD blen, didx;
#if MCM_SUPPORT_DTYPE_S
    MCM_DTYPE_USIZE_TD dlen;
    MCM_DTYPE_S_TD *tmp_str;
#endif
#if MCM_SUPPORT_DTYPE_B
    MCM_DTYPE_B_TD *tmp_byte;
#endif
    void *data_loc;
#if MCM_CFDMODE
    MCM_DTYPE_USIZE_TD dbg_dlen, dbg_tidx, dbg_tlen;
    char dbg_buf[MCM_DBG_BUFFER_SIZE];
    void *dbg_dloc;
    MCM_DTYPE_EK_TD dbg_key;
#endif


#define MCM_SAVE_NUM_VALUE(type_def, type_fmt) \
    do                                                        \
    {                                                         \
        fprintf(file_fp, type_fmt, *((type_def *) data_loc)); \
        MCM_CFDMSG("[%s][" type_fmt "]",                      \
                   self_model_member->member_name,            \
                   *((type_def *) data_loc));                 \
    }                                                         \
    while(0)


    MCM_CFDMSG("=> %s", __FUNCTION__);

    // 填充路徑, 加入 name.
    store_path_len += sprintf(store_path_con + store_path_len, "%s",
                              this_store->link_model_group->group_name);
    // 紀錄基本路經長度.
    blen = store_path_len;

    for(; this_store != NULL; this_store = this_store->next_store)
    {
        self_model_group = this_store->link_model_group;

#if MCM_CFDMODE
        self_key = 0;
        if(self_model_group->group_type == MCM_DTYPE_GD_INDEX)
        {
            data_loc = this_store->data_value_sys + self_model_group->entry_key_offset_value;
            self_key = *((MCM_DTYPE_EK_TD *) data_loc);
            store_path_len += sprintf(store_path_con + store_path_len, "%c%c" MCM_DTYPE_EK_PF,
                                      MCM_SPROFILE_PATH_SPLIT_KEY, MCM_SPROFILE_PATH_KEY_KEY,
                                      self_key);
        }
        if(self_model_group->group_save == 0)
        {
            MCM_CFDMSG("store[%s], skip, no save", store_path_con);
            continue;
        }
        if(self_model_group->member_real_count == 0)
        {
            MCM_CFDMSG("store[%s], skip, no any member", store_path_con);
        }
        else
        {
            MCM_CFDMSG("store[%s]", store_path_con);
        }
#else
        if(self_model_group->group_save == 0)
            continue;

        self_key = 0;
        // 填充路徑, gd 類型的 group, 加入 "." + "#" + key.
        if(self_model_group->group_type == MCM_DTYPE_GD_INDEX)
        {
            data_loc = this_store->data_value_sys + self_model_group->entry_key_offset_value;
            self_key = *((MCM_DTYPE_EK_TD *) data_loc);
            store_path_len += sprintf(store_path_con + store_path_len, "%c%c" MCM_DTYPE_EK_PF,
                                      MCM_SPROFILE_PATH_SPLIT_KEY, MCM_SPROFILE_PATH_KEY_KEY,
                                      self_key);
        }
#endif

        if(self_model_group->member_real_count > 0)
        {
            fprintf(file_fp, "%s%c", store_path_con, MCM_SPROFILE_PARAMETER_SPLIT_KEY);

            for(self_model_member = self_model_group->member_list; self_model_member != NULL;
                self_model_member = self_model_member->next_model_member)
            {
                if(self_model_member->member_type == MCM_DTYPE_EK_INDEX)
                    continue;

                data_loc = this_store->data_value_sys + self_model_member->offset_in_value;

                fprintf(file_fp, "%s%c",
                        self_model_member->member_name, MCM_SPROFILE_MEMBER_SPLIT_KEY);

                switch(self_model_member->member_type)
                {
                    case MCM_DTYPE_EK_INDEX:
                        break;
#if MCM_SUPPORT_DTYPE_RK
                    case MCM_DTYPE_RK_INDEX:
                        MCM_SAVE_NUM_VALUE(MCM_DTYPE_RK_TD, MCM_DTYPE_RK_PF);
                        break;
#endif
#if MCM_SUPPORT_DTYPE_ISC
                    case MCM_DTYPE_ISC_INDEX:
                        MCM_SAVE_NUM_VALUE(MCM_DTYPE_ISC_TD, MCM_DTYPE_ISC_PF);
                        break;
#endif
#if MCM_SUPPORT_DTYPE_IUC
                    case MCM_DTYPE_IUC_INDEX:
                        MCM_SAVE_NUM_VALUE(MCM_DTYPE_IUC_TD, MCM_DTYPE_IUC_PF);
                        break;
#endif
#if MCM_SUPPORT_DTYPE_ISS
                    case MCM_DTYPE_ISS_INDEX:
                        MCM_SAVE_NUM_VALUE(MCM_DTYPE_ISS_TD, MCM_DTYPE_ISS_PF);
                        break;
#endif
#if MCM_SUPPORT_DTYPE_IUS
                    case MCM_DTYPE_IUS_INDEX:
                        MCM_SAVE_NUM_VALUE(MCM_DTYPE_IUS_TD, MCM_DTYPE_IUS_PF);
                        break;
#endif
#if MCM_SUPPORT_DTYPE_ISI
                    case MCM_DTYPE_ISI_INDEX:
                        MCM_SAVE_NUM_VALUE(MCM_DTYPE_ISI_TD, MCM_DTYPE_ISI_PF);
                        break;
#endif
#if MCM_SUPPORT_DTYPE_IUI
                    case MCM_DTYPE_IUI_INDEX:
                        MCM_SAVE_NUM_VALUE(MCM_DTYPE_IUI_TD, MCM_DTYPE_IUI_PF);
                        break;
#endif
#if MCM_SUPPORT_DTYPE_ISLL
                    case MCM_DTYPE_ISLL_INDEX:
                        MCM_SAVE_NUM_VALUE(MCM_DTYPE_ISLL_TD, MCM_DTYPE_ISLL_PF);
                        break;
#endif
#if MCM_SUPPORT_DTYPE_IULL
                    case MCM_DTYPE_IULL_INDEX:
                        MCM_SAVE_NUM_VALUE(MCM_DTYPE_IULL_TD, MCM_DTYPE_IULL_PF);
                        break;
#endif
#if MCM_SUPPORT_DTYPE_FF
                    case MCM_DTYPE_FF_INDEX:
                        MCM_SAVE_NUM_VALUE(MCM_DTYPE_FF_TD, MCM_DTYPE_FF_PF);
                        break;
#endif
#if MCM_SUPPORT_DTYPE_FD
                    case MCM_DTYPE_FD_INDEX:
                        MCM_SAVE_NUM_VALUE(MCM_DTYPE_FD_TD, MCM_DTYPE_FD_PF);
                        break;
#endif
#if MCM_SUPPORT_DTYPE_FLD
                    case MCM_DTYPE_FLD_INDEX:
                        MCM_SAVE_NUM_VALUE(MCM_DTYPE_FLD_TD, MCM_DTYPE_FLD_PF);
                        break;
#endif
#if MCM_SUPPORT_DTYPE_S
                    case MCM_DTYPE_S_INDEX:
                        tmp_str = (MCM_DTYPE_S_TD *) data_loc;
                        dlen = strlen(tmp_str);
                        for(didx = 0; didx < dlen; didx++)
                            if((MCM_CSTR_MIN_PRINTABLE_KEY <= tmp_str[didx]) &&
                               (tmp_str[didx] <= MCM_CSTR_MAX_PRINTABLE_KEY) &&
                               (tmp_str[didx] != MCM_CSTR_RESERVE_KEY1) &&
                               (tmp_str[didx] != MCM_CSTR_RESERVE_KEY2))
                            {
                                fprintf(file_fp, "%c", tmp_str[didx]);
                            }
                            else
                            {
                                fprintf(file_fp, "%c%02X", MCM_CSTR_SPECIAL_KEY,
                                        tmp_str[didx] & 0xFF);
                            }
#if MCM_CFDMODE
                        dbg_dlen = dlen;
                        MCM_DBG_FORMAT_CONFIG_S_VALUE(dbg_buf, data_loc, dbg_dlen,
                                                      dbg_tidx, dbg_tlen);
                        MCM_CFDMSG("[%s][%s]", self_model_member->member_name, dbg_buf);
#endif
                        break;
#endif
#if MCM_SUPPORT_DTYPE_B
                    case MCM_DTYPE_B_INDEX:
                        tmp_byte = (MCM_DTYPE_B_TD *) data_loc;
                        for(didx = 0; didx < self_model_member->member_size; didx++)
                            fprintf(file_fp, MCM_DTYPE_B_PF, tmp_byte[didx] & 0xFF);
#if MCM_CFDMODE
                        dbg_dlen = self_model_member->member_size;
                        MCM_DBG_FORMAT_CONFIG_B_VALUE(dbg_buf, data_loc, dbg_dlen,
                                                      dbg_tidx, dbg_tlen);
                        MCM_CFDMSG("[%s][%s]", self_model_member->member_name, dbg_buf);
#endif    
                        break;
#endif
                }

                // 分隔 member:value, 最後一個加上 "\n".
                fprintf(file_fp, "%c",
                        self_model_member->next_model_member != NULL ?
                        MCM_SPROFILE_PARAMETER_SPLIT_KEY : '\n');
            }
        }

        // 填充路徑, 加入 ".", 讓子層往後面填充.
        store_path_con[store_path_len] = MCM_SPROFILE_PATH_SPLIT_KEY;
        store_path_len++;

        // 往子層處理.
        for(didx = 0; didx < self_model_group->store_child_count; didx++)
        {
            store_head_in_parent = this_store->child_store_list_head_array + didx;
#if MCM_CFDMODE
            MCM_DBG_GET_ENTRY_KEY(self_model_group, this_store, dbg_dloc, dbg_key);
            MCM_CFDMSG("trace[%s.%c" MCM_DTYPE_EK_PF "] child[%p(%p)]",
                       self_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key,
                       store_head_in_parent, *store_head_in_parent);
#endif
            if(*store_head_in_parent != NULL)
            {
                mcm_save_store_process(*store_head_in_parent, store_path_con, store_path_len,
                                       file_fp);
            }
        }

        // 子層處理完後, 重設路徑.
        store_path_len = blen;
        store_path_con[store_path_len] = '\0';
    }

    return MCM_RCODE_PASS;
}

// 將 store tree 存入檔案.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_save_store(
    void)
{
    int fret = MCM_RCODE_PASS;
    FILE *file_fp;
    char store_path_buf[MCM_CONFIG_PATH_MAX_LENGTH];


    MCM_CFDMSG("=> %s", __FUNCTION__);

    if(mcm_config_store_current_profile_path == NULL)
        goto FREE_01;

    file_fp = fopen(mcm_config_store_current_profile_path, "w");
    if(file_fp == NULL)
    {
        MCM_EMSG("call fopen(%s) fail [%s]",
                 mcm_config_store_current_profile_path, strerror(errno));
        fret = MCM_RCODE_CONFIG_INTERNAL_ERROR;
        goto FREE_01;
    }

    fprintf(file_fp, "%c%s%c%s\n",
            MCM_SPROFILE_BASE_DATA_KEY, MCM_SPROFILE_BASE_VERSION_KEY,
            MCM_SPROFILE_PARAMETER_SPLIT_KEY, MCM_CONFIG_PROFILE_VERSION);

    mcm_save_store_process(mcm_config_root_store, store_path_buf, 0, file_fp);

    mcm_do_save = 0;

    do
    {
        if(fclose(file_fp) == EOF)
        {
            MCM_EMSG("call fclose() fail [%s]", strerror(errno));
            if(errno == EINTR)
                continue;
            fret = MCM_RCODE_CONFIG_INTERNAL_ERROR;
            goto FREE_01;
        }
    }
    while(0);

FREE_01:
    return fret;
}

// 釋放 store tree 和其他資料.
// return :
//   MCM_RCODE_PASS.
int mcm_config_free_store(
    void)
{
    MCM_CFDMSG("=> %s", __FUNCTION__);

    mcm_destory_store(mcm_config_root_store);

    return MCM_RCODE_PASS;
}

// 載入 module 內的函式庫.
// file_path (I) :
//   函式庫檔案路徑.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_load_module(
    char *file_path)
{
    mcm_config_module_fp = dlopen(file_path, RTLD_NOW);
    if(mcm_config_module_fp == NULL)
    {
        MCM_EMSG("call dlopen(%s) fail [%s]", file_path, dlerror());
        return MCM_RCODE_CONFIG_INTERNAL_ERROR;
    }

    return MCM_RCODE_PASS;
}

// 釋放 module 資源.
// return :
//   MCM_RCODE_PASS.
int mcm_config_free_module(
    void)
{
    MCM_CFDMSG("dlclose mcm_config_module_fp[%p]", mcm_config_module_fp);
    dlclose(mcm_config_module_fp);

    return MCM_RCODE_PASS;
}

// 處理發生 mcm_config_store_current_profile 發生錯誤的處理.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_store_profile_error_process(
    MCM_DTYPE_BOOL_TD *exit_buf)
{
    int fret;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    switch(mcm_config_store_profile_error_handle)
    {
        case MCM_EHANDLE_INTERNAL_RESET_DEFAULT:
            if(mcm_config_data_error != 0)
            {
                MCM_CFDMSG("store profile error, MCM_EHANDLE_INTERNAL_RESET_DEFAULT");

                mcm_config_free_store();

                mcm_config_remove_store_current_profile(NULL);

                fret = mcm_config_load_store();
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_EMSG("call mcm_config_load_store() fail");
                    goto FREE_01;
                }
            }

            break;

        case MCM_EHANDLE_EXTERNAL_RESET_DEFAULT:
            if(mcm_config_data_error != 0)
            {
                MCM_CFDMSG("store profile error, MCM_EHANDLE_EXTERNAL_RESET_DEFAULT");

                mcm_config_remove_store_current_profile(NULL);

                if(mcm_action_reset_default_run() < MCM_RCODE_PASS)
                {
                    MCM_EMSG("call mcm_action_reset_default_run() fail");
                }

                *exit_buf = 1;
            }

            break;

        case MCM_EHANDLE_MANUAL_HANDEL_INTERNAL:
#if MCM_CFDMODE
            if(mcm_config_data_error != 0)
            {
                MCM_CFDMSG("store profile error, MCM_EHANDLE_MANUAL_HANDEL_INTERNAL");
            }
#endif

            fret = mcm_action_boot_profile_run();
            if(fret < MCM_RCODE_PASS)
            {
                MCM_EMSG("srote proflie error, do reset to default");

                mcm_config_free_store();

                mcm_config_remove_store_current_profile(NULL);

                fret = mcm_config_load_store();
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_EMSG("call mcm_config_load_store() fail");
                    goto FREE_01;
                }
            }

            break;

        case MCM_EHANDLE_MANUAL_HANDEL_EXTERNAL:
#if MCM_CFDMODE
            if(mcm_config_data_error != 0)
            {
                MCM_CFDMSG("store profile error, MCM_EHANDLE_MANUAL_HANDEL_EXTERNAL");
            }
#endif

            fret = mcm_action_boot_profile_run();
            if(fret < MCM_RCODE_PASS)
            {
                MCM_EMSG("srote proflie error, do reset to default");

                mcm_config_remove_store_current_profile(NULL);

                if(mcm_action_reset_default_run() < MCM_RCODE_PASS)
                {
                    MCM_EMSG("call mcm_action_reset_default_run() fail");
                }

                *exit_buf = 1;
            }

            break;
    }

    // 第一次啟動或還原預設值後的第一次啟動, 不會有現在值檔案,
    // 檢查沒有存在的話儲存.
    if(access(mcm_config_store_current_profile_path, F_OK) == -1)
        mcm_do_save = 1;

    fret = MCM_RCODE_PASS;
FREE_01:
    return fret;
}

// 做 設定 / 增加 / 刪除 操作時, 檢查是否允許.
// this_model_group (I) :
//   目標 model group.
// this_store (I) :
//   目標 store.
// deny_status (I) :
//   禁止的資料狀態.
// check_child (I) :
//   是否檢查 child store.
//     0 : 否.
//     1 : 是.
// internal_flag (I) :
//   內部處理遞迴時使用, 固定設 0.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_check_access(
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_DS_TD deny_status,
    MCM_DTYPE_BOOL_TD check_child,
    MCM_DTYPE_BOOL_TD internal_flag)
{
    int fret;
    MCM_DTYPE_DS_TD tmp_status;
    MCM_DTYPE_USIZE_TD cidx;
    struct mcm_config_store_t *child_store, **store_head_in_parent;
#if MCM_CFDMODE
    void *dbg_dloc;
    MCM_DTYPE_EK_TD dbg_key = 0;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

    for(; this_store != NULL; this_store = this_store->next_store)
    {
#if MCM_CFDMODE
        MCM_DBG_SHOW_ENTRY_PATH(this_model_group, this_store, dbg_dloc, dbg_key);
#endif

        MCM_GET_ENTRY_SELF_STATUS(this_model_group, this_store, tmp_status);
        MCM_CFDMSG("status[" MCM_DTYPE_DS_PF "/" MCM_DTYPE_DS_PF "]", tmp_status, deny_status);

        if((tmp_status & deny_status) != 0)
            return MCM_RCODE_CONFIG_ACCESS_DENY;

        if(check_child != 0)
            for(cidx = 0; cidx < this_model_group->store_child_count; cidx++)
            {
                store_head_in_parent = this_store->child_store_list_head_array + cidx;
                MCM_CFDMSG("trace[%s.%c" MCM_DTYPE_EK_PF "] child[%p(%p)]",
                           this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key,
                           store_head_in_parent, *store_head_in_parent);
                if((child_store = *store_head_in_parent) != NULL)
                {
                    fret = mcm_check_access(child_store->link_model_group, child_store,
                                            deny_status, check_child, 1);
                    if(fret < MCM_RCODE_PASS)
                        return fret;
                }
            }

        if(internal_flag == 0)
            break;
    }

    return MCM_RCODE_PASS;
}

// 使用 MCM_DACCESS_NEW 模式做 設定 / 增加 / 刪除 操作時, 調整資料的位置和狀態.
// this_model_group (I) :
//   目標 model group.
// this_model_member (I) :
//   目標 model member.
// this_store (I) :
//   目標 store.
// modify_method (I) :
//   修改的方式.
//     MCM_DMODIFY_SET_NEW : 做設定操作.
//     MCM_DMODIFY_ADD_NEW : 做增加操作.
//     MCM_DMODIFY_DEL_NEW : 做刪除操作.
// modify_object (I) :
//   修改的對象.
//     MCM_MOBJECT_ALONE : 修改 member.
//     MCM_MOBJECT_ENTRY : 修改 entry.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_adjust_data(
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_model_member_t *this_model_member,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_LIST_TD modify_method,
    MCM_DTYPE_LIST_TD modify_object)
{
    MCM_DTYPE_DS_TD tmp_status;
#if MCM_CFDMODE
    void *dbg_dloc;
    MCM_DTYPE_EK_TD dbg_key = 0;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    if(modify_object == MCM_MOBJECT_ALONE)
    {
        MCM_DBG_SHOW_ALONE_PATH(this_model_group, this_model_member, this_store, dbg_dloc, dbg_key);
    }
    else
    {
        MCM_DBG_SHOW_ENTRY_PATH(this_model_group, this_store, dbg_dloc, dbg_key);
    }
#endif

    MCM_GET_ENTRY_SELF_STATUS(this_model_group, this_store, tmp_status);

    // 操作     目前狀態     資料調整         狀態調整.
    // SET-NEW  NONE         複製 SYS 到 NEW  改為 SET.
    //          SET          不需調整         不需調整.
    //          ADD          不需調整         不需調整.
    //          DEL          不需調整         不需調整.
    // ADD-NEW  parent-NONE  移動 SYS 到 NEW  改為 ADD.
    //          parent-SET   移動 SYS 到 NEW  改為 ADD.
    //          parent-ADD   移動 SYS 到 NEW  改為 ADD.
    //          parent-DEL   禁止             .
    // DEL-NEW  NONE         複製 SYS 到 NEW  改為 DEL.
    //          SET          不需調整         改為 DEL.
    //          ADD          禁止             .
    //          DEL          不需調整         不需調整.

    if(modify_method == MCM_DMODIFY_SET_NEW)
    {
        if(tmp_status == MCM_DSCHANGE_NONE)
        {
            MCM_CFDMSG("adjust[SET to NONE]");
            this_store->data_value_new = malloc(this_model_group->data_value_size);
            if(this_store == NULL)
            {
                MCM_EMSG("call malloc() fail [%s]", strerror(errno));
                return MCM_RCODE_CONFIG_ALLOC_FAIL;
            }
            MCM_CFDMSG("alloc data_value_new[" MCM_DTYPE_USIZE_PF "][%p]",
                       this_model_group->data_value_size, this_store->data_value_new);
            memcpy(this_store->data_value_new, this_store->data_value_sys,
                   this_model_group->data_value_size);

            if(modify_object == MCM_MOBJECT_ALONE)
            {
                mcm_config_set_entry_self_status(NULL, this_model_group, this_store,
                                                 MCM_DSASSIGN_SET, MCM_DSCHANGE_SET);
                mcm_config_set_alone_status(NULL, this_model_group, this_model_member, this_store,
                                            MCM_DSASSIGN_SET, MCM_DSCHANGE_SET);
            }
            else
            {
               mcm_config_set_entry_all_status(NULL, this_model_group, this_store,
                                               MCM_DSASSIGN_SET, MCM_DSCHANGE_SET, 0);
            }
        }
        else
        if(tmp_status == MCM_DSCHANGE_SET)
        {
            MCM_CFDMSG("adjust[SET to SET]");
            if(modify_object == MCM_MOBJECT_ALONE)
            {
                mcm_config_set_alone_status(NULL, this_model_group, this_model_member, this_store,
                                            MCM_DSASSIGN_SET, MCM_DSCHANGE_SET);
            }
        }
    }
    else
    if(modify_method == MCM_DMODIFY_ADD_NEW)
    {
        MCM_CFDMSG("adjust[ADD to NULL]");
        this_store->data_value_new = this_store->data_value_sys;
        this_store->data_value_sys = NULL;

        mcm_config_set_entry_all_status(NULL, this_model_group, this_store,
                                        MCM_DSASSIGN_SET, MCM_DSCHANGE_ADD, 0);
    }
    else
    if(modify_method == MCM_DMODIFY_DEL_NEW)
    {
        if(tmp_status == MCM_DSCHANGE_NONE)
        {
            MCM_CFDMSG("adjust[DEL to NONE]");
            this_store->data_value_new = malloc(this_model_group->data_value_size);
            if(this_store->data_value_new == NULL)
            {
                MCM_EMSG("call malloc() fail [%s]", strerror(errno));
                return MCM_RCODE_CONFIG_ALLOC_FAIL;
            }
            MCM_CFDMSG("alloc data_value_new[" MCM_DTYPE_USIZE_PF "][%p]",
                       this_model_group->data_value_size, this_store->data_value_new);
            memcpy(this_store->data_value_new, this_store->data_value_sys,
                   this_model_group->data_value_size);

            mcm_config_set_entry_all_status(NULL, this_model_group, this_store,
                                            MCM_DSASSIGN_SET, MCM_DSCHANGE_DEL, 0);
        }
        else
        if(tmp_status == MCM_DSCHANGE_SET)
        {
            MCM_CFDMSG("adjust[DEL to SET]");
            mcm_config_set_entry_all_status(NULL, this_model_group, this_store,
                                            MCM_DSASSIGN_SET, MCM_DSCHANGE_DEL, 0);
        }
    }

    if(this_store->need_update == 0)
        mcm_add_update_store(this_store);

    return MCM_RCODE_PASS;
}

// 做 設定 / 增加 / 刪除 操作時, 調整 child store 的資料的位置和狀態.
// this_model_group (I) :
//   目標 model group.
// this_model_member (I) :
//   目標 model member.
// this_store (I) :
//   目標 store.
// modify_method (I) :
//   修改的方式.
//     MCM_DMODIFY_SET_NEW : 做設定操作.
//     MCM_DMODIFY_ADD_NEW : 做增加操作.
//     MCM_DMODIFY_DEL_NEW : 做刪除操作.
// modify_object (I) :
//   修改的對象.
//     MCM_MOBJECT_ALONE : 修改 member.
//     MCM_MOBJECT_ENTRY : 修改 entry.
// internal_flag (I) :
//   內部處理遞迴時使用, 固定設 0.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_adjust_child_data(
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_model_member_t *this_model_member,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_LIST_TD modify_method,
    MCM_DTYPE_LIST_TD modify_object,
    MCM_DTYPE_BOOL_TD internal_flag)
{
    int fret;
    struct mcm_config_store_t *child_store, **store_head_in_parent;
    MCM_DTYPE_USIZE_TD cidx;
#if MCM_CFDMODE
    void *dbg_dloc;
    MCM_DTYPE_EK_TD dbg_key = 0;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    if(modify_object == MCM_MOBJECT_ALONE)
    {
        MCM_DBG_SHOW_ALONE_PATH(this_model_group, this_model_member, this_store, dbg_dloc, dbg_key);
    }
    else
    {
        MCM_DBG_SHOW_ENTRY_PATH(this_model_group, this_store, dbg_dloc, dbg_key);
    }
#endif

    // 對於開始層, 不處理,
    // 進入遞迴後, 處理全部的 child store.
    for(; this_store != NULL; this_store = this_store->next_store)
    {
        // 開始層不處理.
        if(internal_flag != 0)
        {
            fret = mcm_adjust_data(this_model_group, this_model_member, this_store,
                                   modify_method, modify_object);
            if(fret < MCM_RCODE_PASS)
            {
                MCM_EMSG("call mcm_adjust_data fail");
                return fret;
            }
        }

        for(cidx = 0; cidx < this_model_group->store_child_count; cidx++)
        {
            store_head_in_parent = this_store->child_store_list_head_array + cidx;
            MCM_CFDMSG("trace[%s.%c" MCM_DTYPE_EK_PF "] child[%p(%p)]",
                       this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key,
                       store_head_in_parent, *store_head_in_parent);
            if((child_store = *store_head_in_parent) != NULL)
            {
                fret = mcm_adjust_child_data(child_store->link_model_group, NULL, child_store,
                                             modify_method, modify_object, 1);
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_EMSG("call mcm_adjust_child_data fail");
                    return fret;
                }
            }
        }

        if(internal_flag == 0)
            break;
    }

    return MCM_RCODE_PASS;
}

// 更新資料.
// this_session (I) :
//   session 資料.
// update_method (I) :
//   更新方式 :
//     MCM_DUPDATE_SYNC : 同步資料.
//     MCM_DUPDATE_DROP : 丟棄資料.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_update(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD update_method)
{
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *self_store;
    void *data_loc;
    MCM_DTYPE_DS_TD tmp_status;
#if MCM_CFDMODE
    MCM_DTYPE_EK_TD dbg_key = 0;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

    if(this_session->session_permission != MCM_SPERMISSION_RW)
    {
        MCM_EMSG("session permission is read only");
        return MCM_RCODE_CONFIG_PERMISSION_DENY;
    }

    while(mcm_update_store_head != NULL)
    {
        self_model_group = mcm_update_store_head->link_model_group;
        self_store = mcm_update_store_head;

#if MCM_CFDMODE
        if(self_model_group->group_type == MCM_DTYPE_GD_INDEX)
        {
            data_loc = self_store->data_value_new + self_model_group->entry_key_offset_value;
            dbg_key = *((MCM_DTYPE_EK_TD *) data_loc);
        }
#endif
        data_loc = self_store->data_status + self_model_group->entry_key_offset_status;
        tmp_status = *((MCM_DTYPE_DS_TD *) data_loc);
        if((tmp_status & MCM_DSCHANGE_ADD) != 0)
            tmp_status = MCM_DSCHANGE_ADD;
        else
        if((tmp_status & MCM_DSCHANGE_DEL) != 0)
            tmp_status = MCM_DSCHANGE_DEL;
        else
            tmp_status = MCM_DSCHANGE_SET;

        MCM_CFDMSG("update_data[%s][" MCM_DTYPE_FLAG_PF "][%s.%c" MCM_DTYPE_EK_PF "]",
                   update_method == MCM_DUPDATE_SYNC ? "SYNC" : "DROP", tmp_status,
                   self_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key);

        // MCM_DUPDATE_SYNC 表示資料處理過程沒錯誤, 套用資料.
        if(update_method == MCM_DUPDATE_SYNC)
        {
            if(tmp_status == MCM_DSCHANGE_ADD)
            {
                // 對於 增加 操作, 資料給 SYS.
                MCM_CFDMSG("[sync add]");
                memset(self_store->data_status, 0, self_model_group->data_status_size);
                self_store->data_value_sys = self_store->data_value_new;
                self_store->data_value_new = NULL;
                mcm_del_update_store(self_store);
            }
            else
            if(tmp_status == MCM_DSCHANGE_DEL)
            {
                // 對於 刪除 操作, 將資料刪除.
                MCM_CFDMSG("[sync del]");
                mcm_unlink_store(self_model_group, self_store);
                mcm_destory_store(self_store);
            }
            else
            {
                // 對於 設定 操作, 資料寫回 SYS.
                MCM_CFDMSG("[sync set]");
                memset(self_store->data_status, 0, self_model_group->data_status_size);
                free(self_store->data_value_sys);
                self_store->data_value_sys = self_store->data_value_new;
                self_store->data_value_new = NULL;
                mcm_del_update_store(self_store);
            }

            if(self_model_group->group_save != 0)
                mcm_do_save = 1;
        }
        // MCM_DUPDATE_DROP 表示資料處理過程發現錯誤, 放棄修改.
        else
        {
            if(tmp_status == MCM_DSCHANGE_ADD)
            {
                // 對於 增加 操作, 放棄資料.
                MCM_CFDMSG("[drop add]");
                mcm_unlink_store(self_model_group, self_store);
                mcm_destory_store(self_store);
            }
            else
            if(tmp_status == MCM_DSCHANGE_DEL)
            {
                // 對於 刪除 操作, 回復資料.
                MCM_CFDMSG("[drop del]");
                memset(self_store->data_status, 0, self_model_group->data_status_size);
                free(self_store->data_value_new);
                self_store->data_value_new = NULL;
                mcm_del_update_store(self_store);
            }
            else
            {
                // 對於 設定 操作, 放棄修改的資料.
                MCM_CFDMSG("[drop set]");
                memset(self_store->data_status, 0, self_model_group->data_status_size);
                free(self_store->data_value_new);
                self_store->data_value_new = NULL;
                mcm_del_update_store(self_store);
            }
        }
    }

    return MCM_RCODE_PASS;
}

// 更新並儲存資料到檔案.
// this_session (I) :
//   session 資料.
// update_method (I) :
//   更新方式 :
//     MCM_DUPDATE_SYNC : 同步資料.
//     MCM_DUPDATE_DROP : 丟棄資料.
// check_save_mode (I) :
//   是否依據 MCM_SSAVE_AUTO / MCM_SSAVE_MANUAL 來決定是否要儲存.
//     0 : 否, 都要儲存.
//     1 : 是, 是 MCM_SSAVE_AUTO 才儲存.
// force_save (I) :
//   是否強制儲存.
//     0 : 否, 有需要儲存的資料被修改才儲存.
//     1 : 是, 不論有無修改都要儲存.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_save(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD update_method,
    MCM_DTYPE_BOOL_TD check_save_mode,
    MCM_DTYPE_BOOL_TD force_save)
{
    int fret;
    MCM_DTYPE_BOOL_TD need_save = 0;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    if(this_session->session_permission != MCM_SPERMISSION_RW)
    {
        MCM_EMSG("session permission is read only");
        return MCM_RCODE_CONFIG_PERMISSION_DENY;
    }

    fret = mcm_config_update(this_session, update_method);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_update() fail");
        return fret;
    }

    // 是否存檔的判斷條件.
    // 1. 判斷 check_save_mode :
    //    a. check_save_mode == 0 :
    //         往 [2] 檢查.
    //    b. check_save_mode == 1 :
    //         mcm_config_store_profile_save_mode == MCM_SSAVE_AUTO, 往 [2] 檢查.
    //         mcm_config_store_profile_save_mode == MCM_SSAVE_MANUAL, 不儲存.
    // 2. 判斷 force_save :
    //    a. force_save == 0 :
    //         mcm_do_save == 0, 不儲存.
    //         mcm_do_save == 1, 儲存.
    //    b. force_save == 1 :
    //         儲存.

    if(check_save_mode == 0)
        need_save = 1;
    else
        need_save = mcm_config_store_profile_save_mode == MCM_SSAVE_AUTO ? 1 : 0;

    if(need_save != 0)
    {
        need_save = force_save == 0 ? mcm_do_save : 1;

        if(need_save != 0)
        {
            fret = mcm_config_save_store();
            if(fret < MCM_RCODE_PASS)
            {
                MCM_EMSG("call mcm_config_save_store() fail");
                return fret;
            }
        }
    }

    return fret;
}

// 關閉程式.
// this_session (I) :
//   session 資料.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_shutdown(
    struct mcm_service_session_t *this_session)
{
    MCM_CFDMSG("=> %s", __FUNCTION__);

    if(this_session->session_permission != MCM_SPERMISSION_RW)
    {
        MCM_EMSG("session permission is read only");
        return MCM_RCODE_CONFIG_PERMISSION_DENY;
    }

    this_session->need_shutdown = 1;

    return MCM_RCODE_PASS;
}

// 將儲存目前資料的檔案移除.
// this_session (I) :
//   session 資料.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_remove_store_current_profile(
    struct mcm_service_session_t *this_session)
{
    MCM_CFDMSG("=> %s", __FUNCTION__);

    if(this_session != NULL)
        if(this_session->session_permission != MCM_SPERMISSION_RW)
        {
            MCM_EMSG("session permission is read only");
            return MCM_RCODE_CONFIG_PERMISSION_DENY;
        }

    if(mcm_config_store_current_profile_path != NULL)
        if(unlink(mcm_config_store_current_profile_path) == -1)
        {
            MCM_EMSG("call unlink(%s) fail [%s]",
                     mcm_config_store_current_profile_path, strerror(errno));
            return MCM_RCODE_CONFIG_INTERNAL_ERROR;
        }

    return MCM_RCODE_PASS;
}

// 以路徑 (mask_path) 尋找對應的 model group.
// this_session (I) :
//   session 資料.
// mask_path (I) :
//   要處理的路徑.
// self_model_group_buf (O) :
//   儲存找到的 model group 的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_find_group_by_mask(
    struct mcm_service_session_t *this_session,
    char *mask_path,
    struct mcm_config_model_group_t **self_model_group_buf)
{
     struct mcm_config_model_group_t *self_model_group_link;
    MCM_DTYPE_USIZE_TD plen, pidx, ploc, nclen;
    MCM_DTYPE_LIST_TD next_part = MCM_CPPATH_NAME, last_part = 0;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", mask_path);

    self_model_group_link = mcm_config_root_model_group;
    plen = strlen(mask_path);

    for(ploc = pidx = 0; pidx <= plen; pidx++)
    {
        // 找到分隔字元 ".".
        if((mask_path[pidx] == MCM_SPROFILE_PATH_SPLIT_KEY) || (mask_path[pidx] == '\0'))
        {
            if((nclen = (pidx - ploc)) == 0)
            {
                MCM_EMSG("invalid path, empty stage [%s]", mask_path);
                return MCM_RCODE_CONFIG_INVALID_PATH;
            }

            // 不是 mask, 搜尋 name.
            if(mask_path[ploc] != MCM_SPROFILE_PATH_MASK_KEY)
            {
                if(next_part == MCM_CPPATH_IK)
                {
                    MCM_EMSG("invalid path, this part must be mask [%s]", mask_path);
                    return MCM_RCODE_CONFIG_INVALID_PATH;
                }

                // == 0, 表示在最開始, 在最上層 model 尋找,
                // > 0, 往下一層 model 尋找.
                if(ploc > 0)
                    self_model_group_link = self_model_group_link->child_model_group_tree;

                // 尋找符合的 name.
                self_model_group_link = mcm_tree_search_group(self_model_group_link,
                                                              mask_path + ploc, nclen);
                if(self_model_group_link == NULL)
                {
                    MCM_EMSG("invalid path, not find name [%s]", mask_path);
                    return MCM_RCODE_CONFIG_INVALID_PATH;
                }

                // gs 類型的 group, 下一階段的路徑必須是 name (下一層),
                // gd 類型的 group, 下一階段的路徑必須是 mask.
                next_part = self_model_group_link->group_type == MCM_DTYPE_GS_INDEX ?
                            MCM_CPPATH_NAME : MCM_CPPATH_IK;
                // 紀錄最後的路徑是 name 類型.
                if(pidx == plen)
                    last_part = MCM_CPPATH_NAME;
            }
            else
            {
                if(next_part == MCM_CPPATH_NAME)
                {
                    MCM_EMSG("invalid path, this part must be name [%s]", mask_path);
                    return MCM_RCODE_CONFIG_INVALID_PATH;
                }

                // 階段徑必須是 mask "*".
                if((mask_path[ploc + 1] != MCM_SPROFILE_PATH_SPLIT_KEY) &&
                   (mask_path[ploc + 1] != '\0'))
                {
                    MCM_EMSG("invalid path, just allow single mask character [%s]", mask_path);
                    return MCM_RCODE_CONFIG_INVALID_PATH;
                }

                // 下一階段的路徑必須是 name (下一層).
                next_part = MCM_CPPATH_NAME;
                // 紀錄最後的路徑是 mask 類型.
                if(pidx == plen)
                    last_part = MCM_CPPATH_IK;
            }

            ploc = pidx + 1;
        }
    }

    if(self_model_group_link->group_type == MCM_DTYPE_GS_INDEX)
    {
        // gs 類型 group, 最後必須是 name 結尾.
        if(last_part != MCM_CPPATH_NAME)
        {
            MCM_EMSG("invalid path, last part must be name [%s]", mask_path);
            return MCM_RCODE_CONFIG_INVALID_PATH;
        }
    }
    else
    {
        // gd 類型 group, 最後必須是 mask 結尾.
        if(last_part != MCM_CPPATH_IK)
        {
            MCM_EMSG("invalid path, last part must be mask [%s]", mask_path);
            return MCM_RCODE_CONFIG_INVALID_PATH;
        }
    }

    if(self_model_group_buf != NULL)
        *self_model_group_buf = self_model_group_link;

    return MCM_RCODE_PASS;
}

// 以路徑 (mix_path) 尋找對應的 model group, sotre.
// this_session (I) :
//   session 資料.
// mix_path (I) :
//   要處理的路徑.
// check_number (I) :
//   檢查 index / key 是否指定正確.
//     MCM_PLIMIT_BOTH  : 不限制 index 或 key.
//     MCM_PLIMIT_INDEX : 必須是 index.
//     MCM_PLIMIT_KEY   : 必須是 key.
// self_model_group_buf (O) :
//   儲存找到的 model group 的緩衝.
// self_store_list_buf (O) :
//   儲存找到的 store 的串列頭緩衝.
// self_store_tree_buf (O) :
//   儲存找到的 store 的樹根緩衝.
// parent_store_buf (O) :
//   儲存找到的 store 的 parent 的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_find_entry_use_mix(
    struct mcm_service_session_t *this_session,
    char *mix_path,
    MCM_DTYPE_LIST_TD check_number,
    struct mcm_config_model_group_t **self_model_group_buf,
    struct mcm_config_store_t **self_store_list_buf,
    struct mcm_config_store_t **self_store_tree_buf,
    struct mcm_config_store_t **parent_store_buf)
{
    struct mcm_config_model_group_t *self_model_group_link;
    struct mcm_config_store_t *self_store_target, *self_store_list, *self_store_tree,
        *parent_store = NULL, **store_in_parent;
    MCM_DTYPE_USIZE_TD plen, pidx, ploc, nclen;
    MCM_DTYPE_LIST_TD next_part = MCM_CPPATH_NAME, last_part = 0, ik_type;
    MCM_DTYPE_BOOL_TD last_mask = 0;
    MCM_DTYPE_EK_TD target_ik = 0, tmp_ik;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", mix_path);

    self_model_group_link = mcm_config_root_model_group;
    self_store_target = self_store_list = self_store_tree = mcm_config_root_store;
    plen = strlen(mix_path);

    for(ploc = pidx = 0; pidx <= plen; pidx++)
    {
        // 找到分隔字元 ".".
        if((mix_path[pidx] == MCM_SPROFILE_PATH_SPLIT_KEY) || (mix_path[pidx] == '\0'))
        {
            if((nclen = (pidx - ploc)) == 0)
            {
                MCM_EMSG("invalid path, empty stage [%s]", mix_path);
                return MCM_RCODE_CONFIG_INVALID_PATH;
            }

            // 尋找 name.
            if((mix_path[ploc] != MCM_SPROFILE_PATH_INDEX_KEY) &&
               (mix_path[ploc] != MCM_SPROFILE_PATH_KEY_KEY) &&
               (mix_path[ploc] != MCM_SPROFILE_PATH_MASK_KEY))
            {
                if(next_part == MCM_CPPATH_IK)
                {
                    MCM_EMSG("invalid path, this part must be index/key/mask [%s]", mix_path);
                    return MCM_RCODE_CONFIG_INVALID_PATH;
                }

                // == 0, 表示在最開始, 在最上層 model group 尋找,
                // > 0, 往下一層 model group 尋找.
                if(ploc > 0)
                    self_model_group_link = self_model_group_link->child_model_group_tree;

                // 尋找符合的 name.
                self_model_group_link = mcm_tree_search_group(self_model_group_link,
                                                              mix_path + ploc, nclen);
                if(self_model_group_link == NULL)
                {
                    MCM_EMSG("invalid path, not find name [%s]", mix_path);
                    return MCM_RCODE_CONFIG_INVALID_PATH;
                }

                // 找到對應的 store, 最上層固定只有一個所以最上層不用找.
                if(self_model_group_link->parent_model_group != NULL)
                {
                    parent_store = self_store_target;

                    store_in_parent = self_store_target->child_store_tree_array +
                                      self_model_group_link->store_index_in_parent;
                    self_store_tree = *store_in_parent;

                    store_in_parent = self_store_target->child_store_list_head_array +
                                      self_model_group_link->store_index_in_parent;
                    self_store_list = *store_in_parent;

                    // 如果是 gs 類型, 不會進入 index / key 部份處理,
                    // 設定此 store 為下一層 store 的 parent store.
                    if(self_model_group_link->group_type == MCM_DTYPE_GS_INDEX)
                        self_store_target = self_store_tree;
                }

                // gs 類型的 group, 下一階段的路徑必須是 name (下一層),
                // gd 類型的 group, 下一階段的路徑必須是 index / key.
                next_part = self_model_group_link->group_type == MCM_DTYPE_GS_INDEX ?
                            MCM_CPPATH_NAME : MCM_CPPATH_IK;
                // 紀錄最後的路徑是 name 類型.
                if(pidx == plen)
                    last_part = MCM_CPPATH_NAME;
            }
            else
            {
                if(next_part == MCM_CPPATH_NAME)
                {
                    MCM_EMSG("invalid path, this part must be name [%s]", mix_path);
                    return MCM_RCODE_CONFIG_INVALID_PATH;
                }

                // 不是最後一段必須是 index / key, 最後一段必須是 mask.
                if(mix_path[ploc] == MCM_SPROFILE_PATH_MASK_KEY)
                {
                    if(pidx < plen)
                    {
                        MCM_EMSG("invalid path, this part must be index/key [%s]", mix_path);
                        return MCM_RCODE_CONFIG_INVALID_PATH;
                    }
                    last_mask = 1;
                }
                else
                {
                    ik_type = mix_path[ploc] == MCM_SPROFILE_PATH_INDEX_KEY ?
                              MCM_PLIMIT_INDEX : MCM_PLIMIT_KEY;

                    if((check_number & ik_type) == 0)
                    {
                        MCM_EMSG("invalid path, this part must be %s [%s]",
                                 check_number == MCM_PLIMIT_INDEX ? "index" : "key", mix_path);
                        return MCM_RCODE_CONFIG_INVALID_PATH;
                    }

                    target_ik = MCM_DTYPE_EK_SB(mix_path + ploc + 1, NULL, 10);
                    if(target_ik < 1)
                    {
                        MCM_EMSG("invalid path, invalid index/key [%s][" MCM_DTYPE_EK_PF "]",
                                 mix_path, target_ik);
                        return MCM_RCODE_CONFIG_INVALID_PATH;
                    }

                    // 尋找目標 index / key.
                    if(ik_type == MCM_PLIMIT_INDEX)
                    {
                        for(tmp_ik = 1; (tmp_ik < target_ik) && (self_store_list != NULL);
                            tmp_ik++, self_store_list = self_store_list->next_store);
                        self_store_target = self_store_list;
                    }
                    else
                    {
                        self_store_tree = mcm_tree_search_store(self_store_tree, target_ik);
                        self_store_target = self_store_tree;
                    }
                    if(self_store_target == NULL)
                    {
                        MCM_EMSG("invalid path, not find %s [%s][" MCM_DTYPE_EK_PF "]",
                                 ik_type == MCM_PLIMIT_INDEX ? "index" : "key",
                                 mix_path, target_ik);
                        return MCM_RCODE_CONFIG_INVALID_PATH;
                    }
                }

                // 下一階段的路徑必須是 name (下一層).
                next_part = MCM_CPPATH_NAME;
                // 紀錄最後的路徑是 index / key / mask 類型.
                if(pidx == plen)
                    last_part = last_mask == 0 ? MCM_CPPATH_IK : MCM_CPPATH_MASK;
            }

            ploc = pidx + 1;
        }
    }

    // gd 類型 group, 最後必須是 mask 結尾.
    if(self_model_group_link->group_type == MCM_DTYPE_GD_INDEX)
        if(last_part != MCM_CPPATH_MASK)
        {
            MCM_EMSG("invalid path, last part must be mask [%s]", mix_path);
            return MCM_RCODE_CONFIG_INVALID_PATH;
        }

    if(self_model_group_buf != NULL)
        *self_model_group_buf = self_model_group_link;
    if(self_store_list_buf != NULL)
        *self_store_list_buf = self_store_list;
    if(self_store_tree_buf != NULL)
        *self_store_tree_buf = self_store_tree;
    if(parent_store_buf != NULL)
        *parent_store_buf = parent_store;

    return MCM_RCODE_PASS;
}

// 以路徑 (mix_path) 尋找對應的 model group, sotre.
// this_session (I) :
//   session 資料.
// mix_path (I) :
//   要處理的路徑.
// self_model_group_buf (O) :
//   儲存找到的 model group 的緩衝.
// self_store_list_buf (O) :
//   儲存找到的 store 的串列頭緩衝.
// self_store_tree_buf (O) :
//   儲存找到的 store 的樹根緩衝.
// parent_store_buf (O) :
//   儲存找到的 store 的 parent 的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_find_entry_by_mix(
    struct mcm_service_session_t *this_session,
    char *mix_path,
    struct mcm_config_model_group_t **self_model_group_buf,
    struct mcm_config_store_t **self_store_list_buf,
    struct mcm_config_store_t **self_store_tree_buf,
    struct mcm_config_store_t **parent_store_buf)
{
    int fret;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    fret = mcm_config_find_entry_use_mix(this_session, mix_path, MCM_PLIMIT_BOTH,
                                         self_model_group_buf, self_store_list_buf,
                                         self_store_tree_buf, parent_store_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_find_entry_use_mix() fail");
        return fret;
    }

    return fret;
}

// 以路徑 (full_path) 尋找對應的 model group, model member, sotre.
// this_session (I) :
//   session 資料.
// full_path (I) :
//   要處理的路徑.
// check_number (I) :
//   檢查 index / key 是否指定正確.
//     MCM_PLIMIT_BOTH  : 不限制 index 或 key.
//     MCM_PLIMIT_INDEX : 必須是 index.
//     MCM_PLIMIT_KEY   : 必須是 key.
// self_model_group_buf (O) :
//   儲存找到的 model group 的緩衝.
// self_model_member_buf (O) :
//   儲存找到的 model member 的緩衝.
// self_store_buf (O) :
//   儲存找到的 store 的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_find_alone_use_full(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_LIST_TD check_number,
    struct mcm_config_model_group_t **self_model_group_buf,
    struct mcm_config_model_member_t **self_model_member_buf,
    struct mcm_config_store_t **self_store_buf)
{
    struct mcm_config_model_group_t *self_model_group_link;
    struct mcm_config_model_member_t *self_model_member_link;
    struct mcm_config_store_t *self_store_list, *self_store_tree, **store_in_parent;
    MCM_DTYPE_USIZE_TD plen, pidx, ploc, nclen;
    MCM_DTYPE_LIST_TD next_part = MCM_CPPATH_NAME, ik_type;
    MCM_DTYPE_EK_TD target_ik, tmp_ik;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", full_path);

    self_model_group_link = mcm_config_root_model_group;
    self_store_list = self_store_tree = mcm_config_root_store;
    plen = strlen(full_path);

    // 處理 model group 部分.
    for(ploc = pidx = 0; pidx < plen; pidx++)
    {
        // 找到分隔字元 ".".
        if(full_path[pidx] == MCM_SPROFILE_PATH_SPLIT_KEY)
        {
            if((nclen = (pidx - ploc)) == 0)
            {
                MCM_EMSG("invalid path, empty stage [%s]", full_path);
                return MCM_RCODE_CONFIG_INVALID_PATH;
            }

            // 尋找 name.
            if((full_path[ploc] != MCM_SPROFILE_PATH_INDEX_KEY) &&
               (full_path[ploc] != MCM_SPROFILE_PATH_KEY_KEY))
            {
                if(next_part == MCM_CPPATH_IK)
                {
                    MCM_EMSG("invalid path, this part must be index/key [%s]", full_path);
                    return MCM_RCODE_CONFIG_INVALID_PATH;
                }

                // == 0, 表示在最開始, 在最上層 model group 尋找,
                // > 0, 往下一層 model group 尋找.
                if(ploc > 0)
                    self_model_group_link = self_model_group_link->child_model_group_tree;

                // 尋找符合的 name.
                self_model_group_link = mcm_tree_search_group(self_model_group_link,
                                                              full_path + ploc, nclen);
                if(self_model_group_link == NULL)
                {
                    MCM_EMSG("invalid path, not fine name [%s]", full_path);
                    return MCM_RCODE_CONFIG_INVALID_PATH;
                }

                // 找到對應的 store, 最上層固定只有一個所以最上層不用找.
                if(self_model_group_link->parent_model_group != NULL)
                {
                    store_in_parent = self_store_list->child_store_tree_array +
                                      self_model_group_link->store_index_in_parent;
                    self_store_tree = *store_in_parent;

                    store_in_parent = self_store_list->child_store_list_head_array +
                                      self_model_group_link->store_index_in_parent;
                    self_store_list = *store_in_parent;
                }

                // gs 類型的 group, 下一階段的路徑必須是 name (下一層),
                // gd 類型的 group, 下一階段的路徑必須是 index / key.
                next_part = self_model_group_link->group_type == MCM_DTYPE_GS_INDEX ?
                            MCM_CPPATH_NAME : MCM_CPPATH_IK;
            }
            else
            {
                if(next_part == MCM_CPPATH_NAME)
                {
                    MCM_EMSG("invalid path, this part must be string [%s]", full_path);
                    return MCM_RCODE_CONFIG_INVALID_PATH;
                }

                ik_type = full_path[ploc] == MCM_SPROFILE_PATH_INDEX_KEY ?
                          MCM_PLIMIT_INDEX : MCM_PLIMIT_KEY;

                if((check_number & ik_type) == 0)
                {
                    MCM_EMSG("invalid path, this part must be %s [%s]",
                             check_number == MCM_PLIMIT_INDEX ? "index" : "key", full_path);
                    return MCM_RCODE_CONFIG_INVALID_PATH;
                }

                target_ik = MCM_DTYPE_EK_SB(full_path + ploc + 1, NULL, 10);
                if(target_ik < 1)
                {
                    MCM_EMSG("invalid path, invalid index/key [%s][" MCM_DTYPE_EK_PF "]",
                             full_path, target_ik);
                    return MCM_RCODE_CONFIG_INVALID_PATH;
                }

                // 尋找目標 index / key.
                if(ik_type == MCM_PLIMIT_INDEX)
                {
                    for(tmp_ik = 1; (tmp_ik < target_ik) && (self_store_list != NULL);
                        tmp_ik++, self_store_list = self_store_list->next_store);
                }
                else
                {
                    self_store_list = mcm_tree_search_store(self_store_tree, target_ik);
                }
                if(self_store_list == NULL)
                {
                    MCM_EMSG("invalid path, not find %s [%s][" MCM_DTYPE_EK_PF "]",
                             ik_type == MCM_PLIMIT_INDEX ? "index" : "key", full_path, target_ik);
                    return MCM_RCODE_CONFIG_INVALID_PATH;
                }

                // 下一階段的路徑必須是 name (下一層).
                next_part = MCM_CPPATH_NAME;
            }

            ploc = pidx + 1;
        }
    }
    if(ploc == 0)
    {
        MCM_EMSG("invalid path, not fine name [%s]", full_path);
        return MCM_RCODE_CONFIG_INVALID_PATH;
    }

    // 處理 model member 部分.
    self_model_member_link = mcm_tree_search_member(self_model_group_link->member_tree,
                                                    full_path + ploc, strlen(full_path + ploc));
    if(self_model_member_link == NULL)
    {
        MCM_EMSG("invalid path, not find member [%s]", full_path);
        return MCM_RCODE_CONFIG_INVALID_PATH;
    }

    if(self_model_group_buf != NULL)
        *self_model_group_buf = self_model_group_link;
    if(self_model_member_buf != NULL)
        *self_model_member_buf = self_model_member_link;
    if(self_store_buf != NULL)
        *self_store_buf = self_store_list;

    return MCM_RCODE_PASS;
}

// 以路徑 (full_path) 尋找對應的 model group, model member, sotre.
// this_session (I) :
//   session 資料.
// full_path (I) :
//   要處理的路徑.
// self_model_group_buf (O) :
//   儲存找到的 model group 的緩衝.
// self_model_member_buf (O) :
//   儲存找到的 model member 的緩衝.
// self_store_buf (O) :
//   儲存找到的 store 的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_find_alone_by_full(
    struct mcm_service_session_t *this_session,
    char *full_path,
    struct mcm_config_model_group_t **self_model_group_buf,
    struct mcm_config_model_member_t **self_model_member_buf,
    struct mcm_config_store_t **self_store_buf)
{
    int fret;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    fret = mcm_config_find_alone_use_full(this_session, full_path, MCM_PLIMIT_BOTH,
                                          self_model_group_buf, self_model_member_buf,
                                          self_store_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_find_alone_use_full() fail");
        return fret;
    }

    return fret;
}

// 以路徑 (full_path) 尋找對應的 model group, sotre.
// this_session (I) :
//   session 資料.
// full_path (I) :
//   要處理的路徑.
// check_non_last_number (I) :
//   檢查除了最後的 index / key 是否指定正確.
//     MCM_PLIMIT_BOTH  : 不限制 index 或 key.
//     MCM_PLIMIT_INDEX : 必須是 index.
//     MCM_PLIMIT_KEY   : 必須是 key.
// check_last_number (I) :
//   檢查最後的 index / key 是否指定正確.
//     MCM_PLIMIT_BOTH  : 不限制 index 或 key.
//     MCM_PLIMIT_INDEX : 必須是 index.
//     MCM_PLIMIT_KEY   : 必須是 key.
// check_last_exist (I) :
//   是否檢查最後的 index / key 是否存在.
//     0 : 否, 對於 add 操作.
//     1 : 是, 對於非 add 操作.
// self_model_group_buf (O) :
//   儲存找到的 model group 的緩衝.
// self_store_buf (O) :
//   儲存找到的 store 的緩衝.
// parent_store_buf (O) :
//   儲存找到的 store 的 parent 的緩衝.
// self_ik_buf (O) :
//   儲存找到的 key 的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_find_entry_use_full(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_LIST_TD check_non_last_number,
    MCM_DTYPE_LIST_TD check_last_number,
    MCM_DTYPE_BOOL_TD check_last_exist,
    struct mcm_config_model_group_t **self_model_group_buf,
    struct mcm_config_store_t **self_store_buf,
    struct mcm_config_store_t **parent_store_buf,
    MCM_DTYPE_EK_TD *self_ik_buf)
{
    struct mcm_config_model_group_t *self_model_group_link;
    struct mcm_config_store_t *self_store_list, *self_store_tree,
        *parent_store = NULL, **store_in_parent;
    MCM_DTYPE_USIZE_TD plen, pidx, ploc, nclen;
    MCM_DTYPE_LIST_TD next_part = MCM_CPPATH_NAME, last_part = 0, ik_type;
    MCM_DTYPE_BOOL_TD check_exist;
    MCM_DTYPE_EK_TD target_ik = 0, tmp_ik;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", full_path);

    self_model_group_link = mcm_config_root_model_group;
    self_store_list = self_store_tree = mcm_config_root_store;
    plen = strlen(full_path);

    for(ploc = pidx = 0; pidx <= plen; pidx++)
    {
        // 找到分隔字元 ".".
        if((full_path[pidx] == MCM_SPROFILE_PATH_SPLIT_KEY) || (full_path[pidx] == '\0'))
        {
            if((nclen = (pidx - ploc)) == 0)
            {
                MCM_EMSG("invalid path, empty stage [%s]", full_path);
                return MCM_RCODE_CONFIG_INVALID_PATH;
            }

            // 尋找 name.
            if((full_path[ploc] != MCM_SPROFILE_PATH_INDEX_KEY) &&
               (full_path[ploc] != MCM_SPROFILE_PATH_KEY_KEY))
            {
                if(next_part == MCM_CPPATH_IK)
                {
                    MCM_EMSG("invalid path, this part must be index/key [%s]", full_path);
                    return MCM_RCODE_CONFIG_INVALID_PATH;
                }

                // == 0, 表示在最開始, 在最上層 model group 尋找,
                // > 0, 往下一層 model group 尋找.
                if(ploc > 0)
                    self_model_group_link = self_model_group_link->child_model_group_tree;

                // 尋找符合的 name.
                self_model_group_link = mcm_tree_search_group(self_model_group_link,
                                                              full_path + ploc, nclen);
                if(self_model_group_link == NULL)
                {
                    MCM_EMSG("invalid path, not find name [%s]", full_path);
                    return MCM_RCODE_CONFIG_INVALID_PATH;
                }

                // 找到對應的 store, 最上層固定只有一個所以最上層不用找.
                if(self_model_group_link->parent_model_group != NULL)
                {
                    parent_store = self_store_list;

                    store_in_parent = self_store_list->child_store_tree_array +
                                      self_model_group_link->store_index_in_parent;
                    self_store_tree = *store_in_parent;

                    store_in_parent = self_store_list->child_store_list_head_array +
                                      self_model_group_link->store_index_in_parent;
                    self_store_list = *store_in_parent;

                    // 如果是 gs 類型, 不會進入 index / key 部份處理,
                    // 設定此 store 為下一層 store 的 parent store.
                    if(self_model_group_link->group_type == MCM_DTYPE_GS_INDEX)
                        self_store_list = self_store_tree;
                }

                // gs 類型的 group, 下一階段的路徑必須是 name (下一層),
                // gd 類型的 group, 下一階段的路徑必須是 index / key.
                next_part = self_model_group_link->group_type == MCM_DTYPE_GS_INDEX ?
                            MCM_CPPATH_NAME : MCM_CPPATH_IK;
                // 紀錄最後的路徑是 name 類型.
                if(pidx == plen)
                    last_part = MCM_CPPATH_NAME;
            }
            else
            {
                if(next_part == MCM_CPPATH_NAME)
                {
                    MCM_EMSG("invalid path, this part must be name [%s]", full_path);
                    return MCM_RCODE_CONFIG_INVALID_PATH;
                }

                ik_type = full_path[ploc] == MCM_SPROFILE_PATH_INDEX_KEY ?
                          MCM_PLIMIT_INDEX : MCM_PLIMIT_KEY;

                if(pidx < plen)
                {
                    if((check_non_last_number & ik_type) == 0)
                    {
                        MCM_EMSG("invalid path, this part must be %s [%s]",
                                 check_non_last_number == MCM_PLIMIT_INDEX ? "index" : "key",
                                 full_path);
                        return MCM_RCODE_CONFIG_INVALID_PATH;
                    }
                }
                else
                {
                    if((check_last_number & ik_type) == 0)
                    {
                        MCM_EMSG("invalid path, this part must be %s [%s]",
                                 check_last_number == MCM_PLIMIT_INDEX ? "index" : "key",
                                 full_path);
                        return MCM_RCODE_CONFIG_INVALID_PATH;
                    }
                }

                target_ik = MCM_DTYPE_EK_SB(full_path + ploc + 1, NULL, 10);
                if(target_ik < 1)
                {
                    MCM_EMSG("invalid path, invalid index/key [%s][" MCM_DTYPE_EK_PF "]",
                             full_path, target_ik);
                    return MCM_RCODE_CONFIG_INVALID_PATH;
                }

                if(pidx < plen)
                    check_exist = 1;
                else
                    check_exist = check_last_exist == 0 ? 0 : 1;

                if(check_exist != 0)
                {
                    // 尋找目標 index / key.
                    if(ik_type == MCM_PLIMIT_INDEX)
                    {
                        for(tmp_ik = 1; (tmp_ik < target_ik) && (self_store_list != NULL);
                            tmp_ik++, self_store_list = self_store_list->next_store);
                    }
                    else
                    {
                        self_store_list = mcm_tree_search_store(self_store_tree, target_ik);
                    }
                    if(self_store_list == NULL)
                    {
                        MCM_EMSG("invalid path, not find %s [%s][" MCM_DTYPE_EK_PF "]",
                                 ik_type == MCM_PLIMIT_INDEX ? "index" : "key", full_path,
                                 target_ik);
                        return MCM_RCODE_CONFIG_INVALID_PATH;
                    }
                }

                // 下一階段的路徑必須是 name (下一層).
                next_part = MCM_CPPATH_NAME;
                // 紀錄最後的路徑是 index / key 類型.
                if(pidx == plen)
                    last_part = MCM_CPPATH_IK;
            }

            ploc = pidx + 1;
        }
    }

    // gd 類型 group, 最後必須是 index / key 結尾.
    if(self_model_group_link->group_type == MCM_DTYPE_GD_INDEX)
        if(last_part != MCM_CPPATH_IK)
        {
            MCM_EMSG("invalid path, last part must be index/key [%s]", full_path);
            return MCM_RCODE_CONFIG_INVALID_PATH;
        }

    if(self_model_group_buf != NULL)
        *self_model_group_buf = self_model_group_link;
    if(self_store_buf != NULL)
        *self_store_buf = self_store_list;
    if(parent_store_buf != NULL)
        *parent_store_buf = parent_store;
    if(self_ik_buf != NULL)
        *self_ik_buf = target_ik;

    return MCM_RCODE_PASS;
}

// 以路徑 (full_path) 尋找對應的 model group, sotre.
// this_session (I) :
//   session 資料.
// full_path (I) :
//   要處理的路徑.
// self_model_group_buf (O) :
//   儲存找到的 model group 的緩衝.
// self_store_buf (O) :
//   儲存找到的 store 的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_find_entry_by_full(
    struct mcm_service_session_t *this_session,
    char *full_path,
    struct mcm_config_model_group_t **self_model_group_buf,
    struct mcm_config_store_t **self_store_buf)
{
    int fret;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    fret = mcm_config_find_entry_use_full(this_session, full_path, MCM_PLIMIT_BOTH,
                                          MCM_PLIMIT_BOTH, 1, self_model_group_buf,
                                          self_store_buf, NULL, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_find_entry_use_full() fail");
        return fret;
    }

    return fret;
}

// 取得 member 資料數值 (進階模式).
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// this_model_member (I) :
//   目標 model member.
// this_store (I) :
//   目標 store.
// data_access (I) :
//   要從何處取出數值.
//     MCM_DACCESS_SYS  : 已存在的.
//     MCM_DACCESS_NEW  : 新設定的.
//     MCM_DACCESS_AUTO : 優先取新設定的資料, 沒有才找已存在的資料.
// data_buf (O) :
//   儲存資料數值的緩衝.
// return :
//   == MCM_DACCESS_SYS : 成功, 資料由已存在的資料中取出.
//   == MCM_DACCESS_NEW : 成功, 資料由新設定的資料中取出.
//   <  MCM_RCODE_PASS  : 錯誤.
int mcm_config_get_alone_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_model_member_t *this_model_member,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_FLAG_TD data_access,
    void *data_buf)
{
    void *data_loc = NULL;
    MCM_DTYPE_FLAG_TD store_loc;
#if MCM_SUPPORT_DTYPE_S
    MCM_DTYPE_USIZE_TD dlen = 0;
#endif
#if MCM_CFDMODE
    MCM_DTYPE_EK_TD dbg_key = 0;
    MCM_DTYPE_USIZE_TD dbg_dlen, dbg_tidx, dbg_tlen;
    char dbg_buf[MCM_DBG_BUFFER_SIZE];
#endif


#define MCM_GET_NUM_VALUE(type_def, type_fmt) \
    do                                                          \
    {                                                           \
        *((type_def *) data_buf) = *((type_def *) data_loc);    \
        MCM_CFDMSG("[" type_fmt "]", *((type_def *) data_buf)); \
    }                                                           \
    while(0)


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    MCM_DBG_SHOW_ALONE_PATH(this_model_group, this_model_member, this_store, data_loc, dbg_key);
#endif

    data_loc = data_access & MCM_DACCESS_NEW ? this_store->data_value_new : NULL;
    if(data_loc == NULL)
        if(data_access & MCM_DACCESS_SYS)
            data_loc = this_store->data_value_sys;
#if MCM_CFDMODE
    if(data_loc == NULL)
    {
         MCM_CFDMSG("access[" MCM_DTYPE_FLAG_PF " -> NULL]", data_access);
    }
    else
    {
         MCM_CFDMSG("access[" MCM_DTYPE_FLAG_PF " -> %s]",
                    data_access, data_loc == this_store->data_value_new ? "NEW" : "SYS");
    }
#endif
    if(data_loc == NULL)
        return MCM_RCODE_CONFIG_INVALID_STORE;

    store_loc = data_loc == this_store->data_value_new ? MCM_DACCESS_NEW : MCM_DACCESS_SYS;

    data_loc += this_model_member->offset_in_value;

    switch(this_model_member->member_type)
    {
        case MCM_DTYPE_EK_INDEX:
            MCM_GET_NUM_VALUE(MCM_DTYPE_EK_TD, MCM_DTYPE_EK_PF);
            break;
#if MCM_SUPPORT_DTYPE_RK
        case MCM_DTYPE_RK_INDEX:
            MCM_GET_NUM_VALUE(MCM_DTYPE_RK_TD, MCM_DTYPE_RK_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISC
        case MCM_DTYPE_ISC_INDEX:
            MCM_GET_NUM_VALUE(MCM_DTYPE_ISC_TD, MCM_DTYPE_ISC_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IUC
        case MCM_DTYPE_IUC_INDEX:
            MCM_GET_NUM_VALUE(MCM_DTYPE_IUC_TD, MCM_DTYPE_IUC_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISS
        case MCM_DTYPE_ISS_INDEX:
            MCM_GET_NUM_VALUE(MCM_DTYPE_ISS_TD, MCM_DTYPE_ISS_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IUS
        case MCM_DTYPE_IUS_INDEX:
            MCM_GET_NUM_VALUE(MCM_DTYPE_IUS_TD, MCM_DTYPE_IUS_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISI
        case MCM_DTYPE_ISI_INDEX:
            MCM_GET_NUM_VALUE(MCM_DTYPE_ISI_TD, MCM_DTYPE_ISI_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IUI
        case MCM_DTYPE_IUI_INDEX:
            MCM_GET_NUM_VALUE(MCM_DTYPE_IUI_TD, MCM_DTYPE_IUI_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISLL
        case MCM_DTYPE_ISLL_INDEX:
            MCM_GET_NUM_VALUE(MCM_DTYPE_ISLL_TD, MCM_DTYPE_ISLL_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IULL
        case MCM_DTYPE_IULL_INDEX:
            MCM_GET_NUM_VALUE(MCM_DTYPE_IULL_TD, MCM_DTYPE_IULL_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_FF
        case MCM_DTYPE_FF_INDEX:
            MCM_GET_NUM_VALUE(MCM_DTYPE_FF_TD, MCM_DTYPE_FF_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_FD
        case MCM_DTYPE_FD_INDEX:
            MCM_GET_NUM_VALUE(MCM_DTYPE_FD_TD, MCM_DTYPE_FD_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_FLD
        case MCM_DTYPE_FLD_INDEX:
            MCM_GET_NUM_VALUE(MCM_DTYPE_FLD_TD, MCM_DTYPE_FLD_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_S
        case MCM_DTYPE_S_INDEX:
            dlen = strlen((MCM_DTYPE_S_TD *) data_loc) + 1;
            memcpy(data_buf, data_loc, dlen);
#if MCM_CFDMODE
            dbg_dlen = dlen - 1;
            MCM_DBG_FORMAT_CONFIG_S_VALUE(dbg_buf, data_buf, dbg_dlen, dbg_tidx, dbg_tlen);
            MCM_CFDMSG("[%s]", dbg_buf);
#endif
            break;
#endif
#if MCM_SUPPORT_DTYPE_B
        case MCM_DTYPE_B_INDEX:
            memcpy(data_buf, data_loc, this_model_member->member_size);
#if MCM_CFDMODE
            dbg_dlen = this_model_member->member_size;
            MCM_DBG_FORMAT_CONFIG_B_VALUE(dbg_buf, data_buf, dbg_dlen, dbg_tidx, dbg_tlen);
            MCM_CFDMSG("[%s]", dbg_buf);
#endif
            break;
#endif
    }

    return store_loc;
}

// 取得 member 資料數值 (基本模式).
// this_session (I) :
//   session 資料.
// full_path (I) :
//   目標路徑.
// data_access (I) :
//   要從何處取出數值.
//     MCM_DACCESS_SYS  : 已存在的.
//     MCM_DACCESS_NEW  : 新設定的.
//     MCM_DACCESS_AUTO : 優先取新設定的資料, 沒有才找已存在的資料.
// data_buf (O) :
//   儲存資料數值的緩衝.
// return :
//   == MCM_DACCESS_SYS : 成功, 資料由已存在的資料中取出.
//   == MCM_DACCESS_NEW : 成功, 資料由新設定的資料中取出.
//   <  MCM_RCODE_PASS  : 錯誤.
int mcm_config_get_alone_by_path(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_FLAG_TD data_access,
    void *data_buf)
{
    int fret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_model_member_t *self_model_member;
    struct mcm_config_store_t *self_store;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", full_path);

    fret = mcm_config_find_alone_use_full(this_session, full_path, MCM_PLIMIT_BOTH,
                                          &self_model_group, &self_model_member, &self_store);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_find_alone_use_full() fail");
        return fret;
    }

    fret = mcm_config_get_alone_by_info(this_session, self_model_group, self_model_member,
                                        self_store, data_access, data_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_get_alone_by_info() fail");
        return fret;
    }

    return fret;
}

// 設定 member 資料數值 (進階模式).
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// this_model_member (I) :
//   目標 model member.
// this_store (I) :
//   目標 store.
// data_access (I) :
//   要將數值寫入何處.
//     MCM_DACCESS_SYS  : 到系統區.
//     MCM_DACCESS_NEW  : 到暫存區.
// data_con (I) :
//   要寫入的資料.
// data_len (I) :
//   資料的長度.
// return :
//   == MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_set_alone_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_model_member_t *this_model_member,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_FLAG_TD data_access,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    int fret;
    void *data_loc;
    MCM_DTYPE_DS_TD deny_status;
#if MCM_SUPPORT_DTYPE_S
    MCM_DTYPE_USIZE_TD dlen;
#endif
#if MCM_CFDMODE
    MCM_DTYPE_EK_TD dbg_key = 0;
    MCM_DTYPE_USIZE_TD dbg_tidx, dbg_tlen;
    char dbg_buf[MCM_DBG_BUFFER_SIZE];
#endif


#define MCM_SET_NUM_VALUE(type_def, type_fmt) \
    do                                                                  \
    {                                                                   \
        if(data_len != this_model_member->member_size)                  \
        {                                                               \
            MCM_EMSG("data length not match "                           \
                     "[" MCM_DTYPE_USIZE_PF "/" MCM_DTYPE_USIZE_PF "]", \
                     data_len, this_model_member->member_size);         \
            return MCM_RCODE_CONFIG_INVALID_SIZE;                       \
        }                                                               \
        *((type_def *) data_loc) = *((type_def *) data_con);            \
        MCM_CFDMSG("[" type_fmt "]", *((type_def *) data_loc));         \
    }                                                                   \
    while(0)


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    MCM_DBG_SHOW_ALONE_PATH(this_model_group, this_model_member, this_store, data_loc, dbg_key);
#endif

    if(this_session->session_permission != MCM_SPERMISSION_RW)
    {
        MCM_EMSG("session permission is read only");
        return MCM_RCODE_CONFIG_PERMISSION_DENY;
    }

    if(this_model_member->member_type == MCM_DTYPE_EK_INDEX)
    {
        MCM_EMSG("{%s} is system reserve", this_model_member->member_name);
        return MCM_RCODE_CONFIG_ACCESS_DENY;
    }

    // SET-SYS NONE 允許.
    //         SET  允許.
    //         ADD  禁止.
    //         DEL  允許.
    // SET-NEW NONE 允許.
    //         SET  允許.
    //         ADD  允許.
    //         DEL  允許.

    MCM_CFDMSG("access[%s]", data_access == MCM_DACCESS_SYS ? "SYS" : "NEW");

    if(data_access == MCM_DACCESS_SYS)
    {
        deny_status = MCM_DSCHANGE_ADD;
        fret = mcm_check_access(this_model_group, this_store, deny_status, 0, 0);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_EMSG("invalid access [SET (SYS) -> ADD]");
            return fret;
        }
    }
    else
    {
    }

    if(data_access == MCM_DACCESS_SYS)
    {
        data_loc = this_store->data_value_sys;
    }
    else
    {
        fret = mcm_adjust_data(this_model_group, this_model_member, this_store,
                               MCM_DMODIFY_SET_NEW, MCM_MOBJECT_ALONE);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_EMSG("call mcm_adjust_data() fail");
            return fret;
        }

        data_loc = this_store->data_value_new;
    }

    data_loc += this_model_member->offset_in_value;

    switch(this_model_member->member_type)
    {
        case MCM_DTYPE_EK_INDEX:
            break;
#if MCM_SUPPORT_DTYPE_RK
        case MCM_DTYPE_RK_INDEX:
            MCM_SET_NUM_VALUE(MCM_DTYPE_RK_TD, MCM_DTYPE_RK_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISC
        case MCM_DTYPE_ISC_INDEX:
            MCM_SET_NUM_VALUE(MCM_DTYPE_ISC_TD, MCM_DTYPE_ISC_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IUC
        case MCM_DTYPE_IUC_INDEX:
            MCM_SET_NUM_VALUE(MCM_DTYPE_IUC_TD, MCM_DTYPE_IUC_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISS
        case MCM_DTYPE_ISS_INDEX:
            MCM_SET_NUM_VALUE(MCM_DTYPE_ISS_TD, MCM_DTYPE_ISS_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IUS
        case MCM_DTYPE_IUS_INDEX:
            MCM_SET_NUM_VALUE(MCM_DTYPE_IUS_TD, MCM_DTYPE_IUS_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISI
        case MCM_DTYPE_ISI_INDEX:
            MCM_SET_NUM_VALUE(MCM_DTYPE_ISI_TD, MCM_DTYPE_ISI_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IUI
        case MCM_DTYPE_IUI_INDEX:
            MCM_SET_NUM_VALUE(MCM_DTYPE_IUI_TD, MCM_DTYPE_IUI_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISLL
        case MCM_DTYPE_ISLL_INDEX:
            MCM_SET_NUM_VALUE(MCM_DTYPE_ISLL_TD, MCM_DTYPE_ISLL_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IULL
        case MCM_DTYPE_IULL_INDEX:
            MCM_SET_NUM_VALUE(MCM_DTYPE_IULL_TD, MCM_DTYPE_IULL_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_FF
        case MCM_DTYPE_FF_INDEX:
            MCM_SET_NUM_VALUE(MCM_DTYPE_FF_TD, MCM_DTYPE_FF_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_FD
        case MCM_DTYPE_FD_INDEX:
            MCM_SET_NUM_VALUE(MCM_DTYPE_FD_TD, MCM_DTYPE_FD_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_FLD
        case MCM_DTYPE_FLD_INDEX:
            MCM_SET_NUM_VALUE(MCM_DTYPE_FLD_TD, MCM_DTYPE_FLD_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_S
        case MCM_DTYPE_S_INDEX:
            dlen = data_len + 1;
            if(dlen > this_model_member->member_size)
            {
                MCM_EMSG("data length too large "
                         "[" MCM_DTYPE_USIZE_PF "/" MCM_DTYPE_USIZE_PF "]",
                         dlen, this_model_member->member_size);
                return MCM_RCODE_CONFIG_INVALID_SIZE;
            }
            if(data_len == 0)
                *((MCM_DTYPE_S_TD *) data_loc) = '\0';
            else
                memcpy(data_loc, data_con, dlen);
#if MCM_CFDMODE
            MCM_DBG_FORMAT_CONFIG_S_VALUE(dbg_buf, data_loc, data_len, dbg_tidx, dbg_tlen);
            MCM_CFDMSG("[%s]", dbg_buf);
#endif
            break;
#endif
#if MCM_SUPPORT_DTYPE_B
        case MCM_DTYPE_B_INDEX:
            if(data_len > this_model_member->member_size)
            {
                MCM_EMSG("data length too large "
                         "[" MCM_DTYPE_USIZE_PF "/" MCM_DTYPE_USIZE_PF "]",
                         data_len, this_model_member->member_size);
                return MCM_RCODE_CONFIG_INVALID_SIZE;
            }
            memcpy(data_loc, data_con, data_len);
#if MCM_CFDMODE
            MCM_DBG_FORMAT_CONFIG_B_VALUE(dbg_buf, data_loc, data_len, dbg_tidx, dbg_tlen);
            MCM_CFDMSG("[%s]", dbg_buf);
#endif
            break;
#endif
    }

    if(data_access == MCM_DACCESS_SYS)
        if(this_model_group->group_save != 0)
            mcm_do_save = 1;

    return MCM_RCODE_PASS;
}

// 設定 member 資料數值 (基本模式).
// this_session (I) :
//   session 資料.
// full_path (I) :
//   目標路徑.
// data_access (I) :
//   要將數值寫入何處.
//     MCM_DACCESS_SYS  : 到系統區.
//     MCM_DACCESS_NEW  : 到暫存區.
// data_con (I) :
//   要寫入的資料.
// data_len (I) :
//   資料的長度.
// return :
//   == MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_set_alone_by_path(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_FLAG_TD data_access,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    int fret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_model_member_t *self_model_member;
    struct mcm_config_store_t *self_store;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", full_path);

    fret = mcm_config_find_alone_use_full(this_session, full_path, MCM_PLIMIT_BOTH,
                                          &self_model_group, &self_model_member, &self_store);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_find_alone_use_full() fail");
        return fret;
    }

    fret = mcm_config_set_alone_by_info(this_session, self_model_group, self_model_member,
                                        self_store, data_access, data_con, data_len);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_set_alone_by_info() fail");
        return fret;
    }

    return fret;
}

// 取得 group 的 entry 資料數值 (進階模式).
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// this_store (I) :
//   目標 store.
// data_access (I) :
//   要從何處取出數值.
//     MCM_DACCESS_SYS  : 已存在的.
//     MCM_DACCESS_NEW  : 新設定的.
//     MCM_DACCESS_AUTO : 優先取新設定的資料, 沒有才找已存在的資料.
// data_buf (O) :
//   紀錄資料數值的緩衝.
// return :
//   == MCM_DACCESS_SYS : 成功, 資料由已存在的資料中取出.
//   == MCM_DACCESS_NEW : 成功, 資料由新設定的資料中取出.
//   <  MCM_RCODE_PASS  : 錯誤.
int mcm_config_get_entry_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_FLAG_TD data_access,
    void *data_buf)
{
    void *data_loc = NULL;
#if MCM_CFDMODE
    MCM_DTYPE_EK_TD dbg_key = 0;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    MCM_DBG_SHOW_ENTRY_PATH(this_model_group, this_store, data_loc, dbg_key);
#endif

    data_loc = data_access & MCM_DACCESS_NEW ? this_store->data_value_new : NULL;
    if(data_loc == NULL)
        if(data_access & MCM_DACCESS_SYS)
            data_loc = this_store->data_value_sys;
#if MCM_CFDMODE
    if(data_loc == NULL)
    {
        MCM_CFDMSG("access[" MCM_DTYPE_FLAG_PF " -> NULL]", data_access);
    }
    else
    {
        MCM_CFDMSG("access[" MCM_DTYPE_FLAG_PF " -> %s]",
                   data_access, data_loc == this_store->data_value_new ? "NEW" : "SYS");
    }
#endif
    if(data_loc == NULL)
        return MCM_RCODE_CONFIG_INVALID_STORE;

    memcpy(data_buf, data_loc, this_model_group->data_value_size);
    MCM_CFDMSG("[copy(" MCM_DTYPE_USIZE_PF ")]", this_model_group->data_value_size);

    return data_loc == this_store->data_value_new ? MCM_DACCESS_NEW : MCM_DACCESS_SYS;
}

// 取得 group 的 entry 資料數值 (基本模式).
// this_session (I) :
//   session 資料.
// full_path (I) :
//   目標路徑.
// data_access (I) :
//   要從何處取出數值.
//     MCM_DACCESS_SYS  : 已存在的.
//     MCM_DACCESS_NEW  : 新設定的.
//     MCM_DACCESS_AUTO : 優先取新設定的資料, 沒有才找已存在的資料.
// data_buf (O) :
//   儲存資料數值的緩衝.
// return :
//   == MCM_DACCESS_SYS : 成功, 資料由已存在的資料中取出.
//   == MCM_DACCESS_NEW : 成功, 資料由新設定的資料中取出.
//   <  MCM_RCODE_PASS  : 錯誤.
int mcm_config_get_entry_by_path(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_FLAG_TD data_access,
    void *data_buf)
{
    int fret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *self_store;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", full_path);

    fret = mcm_config_find_entry_use_full(this_session, full_path, MCM_PLIMIT_BOTH,
                                          MCM_PLIMIT_BOTH, 1, &self_model_group, &self_store,
                                          NULL, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_find_entry_use_full() fail");
        return fret;
    }

    fret = mcm_config_get_entry_by_info(this_session, self_model_group, self_store, data_access,
                                        data_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_get_entry_by_info() fail");
        return fret;
    }

    return fret;
}

// 設定 group 的 entry 資料數值 (進階模式).
// this_session (I) :
//   session 資料.

// this_model_group (I) :
//   目標 model group.
// this_store (I) :
//   目標 store.
// data_access (I) :
//   要將數值寫入何處.
//     MCM_DACCESS_SYS  : 到系統區.
//     MCM_DACCESS_NEW  : 到暫存區.
// data_con (I) :
//   要寫入的資料.
// return :
//   == MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_set_entry_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_FLAG_TD data_access,
    void *data_con)
{
    int fret;
    void *data_loc, *key_loc = NULL;
    MCM_DTYPE_DS_TD deny_status;
    MCM_DTYPE_EK_TD self_key = 0;


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    MCM_DBG_SHOW_ENTRY_PATH(this_model_group, this_store, data_loc, self_key);
#endif

    if(this_session->session_permission != MCM_SPERMISSION_RW)
    {
        MCM_EMSG("session permission is read only");
        return MCM_RCODE_CONFIG_PERMISSION_DENY;
    }

    // SET-SYS NONE 允許.
    //         SET  允許.
    //         ADD  禁止.
    //         DEL  允許.
    // SET-NEW NONE 允許.
    //         SET  允許.
    //         ADD  允許.
    //         DEL  允許.

    MCM_CFDMSG("access[%s]", data_access == MCM_DACCESS_SYS ? "SYS" : "NEW");

    if(data_access == MCM_DACCESS_SYS)
    {
        deny_status = MCM_DSCHANGE_ADD;
        fret = mcm_check_access(this_model_group, this_store, deny_status, 0, 0);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_EMSG("invalid access [SET (SYS) -> ADD]");
            return fret;
        }
    }
    else
    {
    }

    if(data_access == MCM_DACCESS_SYS)
    {
        data_loc = this_store->data_value_sys;
    }
    else
    {
        fret = mcm_adjust_data(this_model_group, NULL, this_store,
                               MCM_DMODIFY_SET_NEW, MCM_MOBJECT_ENTRY);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_EMSG("call mcm_adjust_data() fail");
            return fret;
        }

        data_loc = this_store->data_value_new;
    }

    if(this_model_group->group_type == MCM_DTYPE_GD_INDEX)
    {
        key_loc = data_loc + this_model_group->entry_key_offset_value;
        self_key = *((MCM_DTYPE_EK_TD *) key_loc);
    }

    memcpy(data_loc, data_con, this_model_group->data_value_size);

    if(this_model_group->group_type == MCM_DTYPE_GD_INDEX)
        *((MCM_DTYPE_EK_TD *) key_loc) = self_key;

    MCM_CFDMSG("[copy(" MCM_DTYPE_USIZE_PF ")]", this_model_group->data_value_size);

    if(data_access == MCM_DACCESS_SYS)
        if(this_model_group->group_save != 0)
            mcm_do_save = 1;

    return MCM_RCODE_PASS;
}

// 設定 group 的 entry 資料數值 (基本模式).
// this_session (I) :
//   session 資料.
// full_path (I) :
//   目標路徑.
// data_access (I) :
//   要將數值寫入何處.
//     MCM_DACCESS_SYS  : 到系統.
//     MCM_DACCESS_NEW  : 到暫存區.
// data_con (I) :
//   要寫入的資料.
// return :
//   == MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_set_entry_by_path(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_FLAG_TD data_access,
    void *data_con)
{
    int fret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *self_store;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", full_path);

    fret = mcm_config_find_entry_use_full(this_session, full_path, MCM_PLIMIT_BOTH,
                                          MCM_PLIMIT_BOTH, 1, &self_model_group, &self_store,
                                          NULL, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_find_entry_use_full() fail");
        return fret;
    }

    fret = mcm_config_set_entry_by_info(this_session, self_model_group, self_store, data_access,
                                        data_con);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_set_entry_by_info() fail");
        return fret;
    }

    return fret;
}

// 增加 group 的 entry (進階模式).
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// parent_store (I) :
//   要新增的 store 的 parent.
// data_access (I) :
//   新增的資料要放在何處.
//     MCM_DACCESS_SYS  : 到系統.
//     MCM_DACCESS_NEW  : 到暫存區.
// this_key (I) :
//   要新增的 store 的 key.
// data_con (I) :
//   要寫入的資料, 可以是 NULL.
// new_store_buf (O) :
//   儲存新增的 store 的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_add_entry_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *parent_store,
    MCM_DTYPE_FLAG_TD data_access,
    MCM_DTYPE_EK_TD this_key,
    void *data_con,
    struct mcm_config_store_t **new_store_buf)
{
    int fret;
    struct mcm_config_store_t *self_store, **store_tree_in_parent;
    void *data_loc;
    MCM_DTYPE_DS_TD deny_status;
    MCM_DTYPE_EK_TD *store_count_in_parent, self_count;


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    MCM_CFDMSG("[%s." MCM_DTYPE_EK_PF "]", this_model_group->group_name, this_key);
#endif

    if(this_session->session_permission != MCM_SPERMISSION_RW)
    {
        MCM_EMSG("session permission is read only");
        return MCM_RCODE_CONFIG_PERMISSION_DENY;
    }

    if(this_model_group->group_type != MCM_DTYPE_GD_INDEX)
    {
        MCM_EMSG("only {%s} can add entry", MCM_DTYPE_GD_KEY);
        return MCM_RCODE_CONFIG_ACCESS_DENY;
    }

    // ADD-SYS parent-NONE 允許.
    //         parent-SET  允許.
    //         parent-ADD  禁止.
    //         parent-DEL  禁止.
    // ADD-NEW parent-NONE 允許.
    //         parent-SET  允許.
    //         parent-ADD  允許.
    //         parent-DEL  禁止.

    MCM_CFDMSG("access[%s]", data_access == MCM_DACCESS_SYS ? "SYS" : "NEW");

    deny_status = data_access == MCM_DACCESS_SYS ?
                  MCM_DSCHANGE_ADD | MCM_DSCHANGE_DEL : MCM_DSCHANGE_DEL;

    fret = mcm_check_access(this_model_group->parent_model_group, parent_store, deny_status,
                            0, 0);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("invalid access [ADD (%s) -> %s]",
                 data_access == MCM_DACCESS_SYS ? "SYS" : "NEW",
                 data_access == MCM_DACCESS_SYS ? "parent-ADD / parent-DEL" : "parent-DEL");
        return fret;
    }

    // 檢查是否超過最大筆數限制.
    store_count_in_parent = parent_store->child_store_count_array +
                            this_model_group->store_index_in_parent;
    self_count = *store_count_in_parent;
    if(self_count >= this_model_group->group_max)
    {
        MCM_EMSG("too many entry [" MCM_DTYPE_EK_PF "]", this_model_group->group_max);
        return MCM_RCODE_CONFIG_TOO_MANY_ENTRY;
    }

    // 檢查 key 是否重複.
    store_tree_in_parent = parent_store->child_store_tree_array +
                           this_model_group->store_index_in_parent;
    self_store = *store_tree_in_parent;
    if(mcm_tree_search_store(self_store, this_key) != NULL)
    {
        MCM_EMSG("this entry already exist [%c" MCM_DTYPE_EK_PF "]",
                 MCM_SPROFILE_PATH_KEY_KEY, this_key);
        return MCM_RCODE_CONFIG_DUPLIC_ENTRY;
    }

    // 建立 store.
    fret = mcm_create_store(this_model_group, parent_store, data_con == NULL ? 1 : 0,
                            this_key, 1, 0, &self_store);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_create_store() fail");
        return fret;
    }

    // 填入要設定的資料.
    if(data_con != NULL)
    {
        memcpy(self_store->data_value_sys, data_con, this_model_group->data_value_size);
        data_loc = self_store->data_value_sys + this_model_group->entry_key_offset_value;
        *((MCM_DTYPE_EK_TD *) data_loc) = this_key;
    }

    if(data_access == MCM_DACCESS_SYS)
    {
        if(this_model_group->group_save != 0)
            mcm_do_save = 1;
    }
    else
    {
        fret = mcm_adjust_data(this_model_group, NULL, self_store,
                               MCM_DMODIFY_ADD_NEW, MCM_MOBJECT_ENTRY);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_EMSG("call mcm_adjust_data() fail");
            return fret;
        }
        fret = mcm_adjust_child_data(this_model_group, NULL, self_store,
                                     MCM_DMODIFY_ADD_NEW, MCM_MOBJECT_ENTRY, 0);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_EMSG("call mcm_adjust_child_data() fail");
            return fret;
        }
    }

    if(new_store_buf != NULL)
        *new_store_buf = self_store;

    return MCM_RCODE_PASS;
}

// 增加 group 的 entry (基本模式).
// this_session (I) :
//   session 資料.
// full_path (I) :
//   目標路徑.
// data_access (I) :
//   新增的資料要放在何處.
//     MCM_DACCESS_SYS  : 到系統.
//     MCM_DACCESS_NEW  : 到暫存區.
// data_con (I) :
//   要寫入的資料, 可以是 NULL.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_add_entry_by_path(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_FLAG_TD data_access,
    void *data_con)
{
    int fret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *self_store, *parent_store;
    MCM_DTYPE_EK_TD self_key;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", full_path);

    fret = mcm_config_find_entry_use_full(this_session, full_path, MCM_PLIMIT_BOTH,
                                          MCM_PLIMIT_KEY, 0, &self_model_group, &self_store,
                                          &parent_store, &self_key);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_find_entry_use_full() fail");
        return fret;
    }

    fret = mcm_config_add_entry_by_info(this_session, self_model_group, parent_store, data_access,
                                        self_key, data_con, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_add_entry_by_info() fail");
        return fret;
    }

    return fret;
}

// 刪除 group 的 entry (進階模式).
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// this_store (I) :
//   目標 store.
// data_access (I) :
//   刪除的方式.
//     MCM_DACCESS_SYS  : 馬上刪除.
//     MCM_DACCESS_NEW  : 僅做標記, 之後由系統刪除.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_del_entry_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_FLAG_TD data_access)
{
    int fret;
    MCM_DTYPE_DS_TD deny_status;
#if MCM_CFDMODE
    void *dbg_dloc;
    MCM_DTYPE_EK_TD dbg_key = 0;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    MCM_DBG_SHOW_ENTRY_PATH(this_model_group, this_store, dbg_dloc, dbg_key);
#endif

    if(this_session->session_permission != MCM_SPERMISSION_RW)
    {
        MCM_EMSG("session permission is read only");
        return MCM_RCODE_CONFIG_PERMISSION_DENY;
    }

    if(this_model_group->group_type != MCM_DTYPE_GD_INDEX)
    {
        MCM_EMSG("only {%s} can del entry", MCM_DTYPE_GD_KEY);
        return MCM_RCODE_CONFIG_ACCESS_DENY;
    }

    // DEL-SYS NONE 允許.
    //         SET  允許.
    //         ADD  允許.
    //         DEL  允許.
    // DEL-NEW NONE 允許.
    //         SET  允許.
    //         ADD  禁止.
    //         DEL  允許.

    MCM_CFDMSG("access[%s]", data_access == MCM_DACCESS_SYS ? "SYS" : "NEW");

    if(data_access == MCM_DACCESS_SYS)
    {
    }
    else
    {
        deny_status = MCM_DSCHANGE_ADD;
        fret = mcm_check_access(this_model_group, this_store, deny_status, 1, 0);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_EMSG("invalid access [DEL (NEW) -> ADD]");
            return fret;
        }
    }

    if(data_access == MCM_DACCESS_SYS)
    {
        mcm_unlink_store(this_model_group, this_store);

        // this_store->data_value_sys == NULL 表示是 ADD 狀態,
        // this_store->data_value_sys 沒有資料所以不設定需要儲存.
        if(this_model_group->group_save != 0)
            if(this_store->data_value_sys != NULL)
                mcm_do_save = 1;

        mcm_destory_store(this_store);
    }
    else
    {
        fret = mcm_adjust_data(this_model_group, NULL, this_store,
                               MCM_DMODIFY_DEL_NEW, MCM_MOBJECT_ENTRY);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_EMSG("call mcm_adjust_data() fail");
            return fret;
        }
        fret = mcm_adjust_child_data(this_model_group, NULL, this_store,
                                     MCM_DMODIFY_DEL_NEW, MCM_MOBJECT_ENTRY, 0);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_EMSG("call mcm_adjust_child_data() fail");
            return fret;
        }
    }

    return MCM_RCODE_PASS;
}

// 刪除 group 的 entry (基本模式).
// this_session (I) :
//   session 資料.
// full_path (I) :
//   目標路徑.
// data_access (I) :
//   刪除的方式.
//     MCM_DACCESS_SYS  : 馬上刪除.
//     MCM_DACCESS_NEW  : 僅做標記, 之後由系統刪除.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_del_entry_by_path(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_FLAG_TD data_access)
{
    int fret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *self_store;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", full_path);

    fret = mcm_config_find_entry_use_full(this_session, full_path, MCM_PLIMIT_BOTH,
                                          MCM_PLIMIT_BOTH, 1, &self_model_group, &self_store,
                                          NULL, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_find_entry_use_full() fail");
        return fret;
    }

    fret = mcm_config_del_entry_by_info(this_session, self_model_group, self_store, data_access);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_del_entry_by_info() fail");
        return fret;
    }

    return fret;
}

// 取得 group 的全部 entry 的資料數值 (進階模式).
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// parent_store (I) :
//   目標 store 的 parent.
// data_access (I) :
//   要從何處取出數值.
//     MCM_DACCESS_SYS  : 已存在的.
//     MCM_DACCESS_NEW  : 新設定的.
//     MCM_DACCESS_AUTO : 優先取新設定的資料, 沒有才找已存在的資料.
// count_buf (O) :
//   儲存筆數的緩衝.
// data_buf (O) :
//   紀錄資料數值的緩衝.
//     == NULL : 表示由內部配置紀錄資料數值的緩衝.
//     != NULL : 表示由外部指定紀錄資料數值的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_get_all_entry_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *parent_store,
    MCM_DTYPE_FLAG_TD data_access,
    MCM_DTYPE_EK_TD *count_buf,
    void **data_buf)
{
    int fret;
    struct mcm_config_store_t **store_head_in_parent = NULL, *list_store, *each_store;
    MCM_DTYPE_EK_TD *store_count_in_parent, self_count = 0;
    MCM_DTYPE_USIZE_TD dlen;
    void *data_loc = NULL, *tmp_buf, *buf_loc;
#if MCM_CFDMODE
    MCM_DTYPE_EK_TD dbg_key = 0;
    void *dbg_dloc;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

    if(parent_store == NULL)
    {
        list_store = mcm_config_root_store;

        for(each_store = list_store; each_store != NULL; each_store = each_store->next_store)
            self_count++;
    }
    else
    {
        store_head_in_parent = parent_store->child_store_list_head_array +
                               this_model_group->store_index_in_parent;
        list_store = *store_head_in_parent;

        store_count_in_parent = parent_store->child_store_count_array +
                                this_model_group->store_index_in_parent;
        self_count = *store_count_in_parent;
    }

#if MCM_CFDMODE
    MCM_CFDMSG("[%s] [%p(%p)][" MCM_DTYPE_EK_PF "]",
               this_model_group->group_name, store_head_in_parent, list_store, self_count);
#endif

    if(list_store != NULL)
    {
        dlen = this_model_group->data_value_size * self_count;

        if(*data_buf != NULL)
        {
            tmp_buf = buf_loc = *data_buf;
        }
        else
        {
            tmp_buf = buf_loc = malloc(dlen);
            if(tmp_buf == NULL)
            {
                MCM_EMSG("call malloc() fail [%s]", strerror(errno));
                fret = MCM_RCODE_CONFIG_ALLOC_FAIL;
                goto FREE_01;
            }
        }
        MCM_CFDMSG("%s data_buf[" MCM_DTYPE_USIZE_PF "][%p]",
                   *data_buf != NULL ? "self" : "alloc", dlen, tmp_buf);

        MCM_CFDMSG("access[" MCM_DTYPE_FLAG_PF "]", data_access);

        for(each_store = list_store; each_store != NULL; each_store = each_store->next_store)
        {
            data_loc = data_access & MCM_DACCESS_NEW ? each_store->data_value_new : NULL;
            if(data_loc == NULL)
                if(data_access & MCM_DACCESS_SYS)
                    data_loc = each_store->data_value_sys;
#if MCM_CFDMODE
            if(this_model_group->group_type == MCM_DTYPE_GD_INDEX)
            {
                dbg_dloc = each_store->data_value_new != NULL ?
                           each_store->data_value_new : each_store->data_value_sys;
                dbg_dloc += this_model_group->entry_key_offset_value;
                dbg_key = *((MCM_DTYPE_EK_TD *) dbg_dloc);
            }
            MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] [%s]",
                       this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key,
                       data_loc == NULL ? "NULL" :
                       data_loc == each_store->data_value_new ? "NEW" : "SYS");
#endif
            if(data_loc == NULL)
            {
                fret = MCM_RCODE_CONFIG_INVALID_STORE;
                goto FREE_02;
            }

            memcpy(buf_loc, data_loc, this_model_group->data_value_size);
            buf_loc += this_model_group->data_value_size;
        }

        *count_buf = self_count;
        if(*data_buf == NULL)
            *data_buf = tmp_buf;
    }
    else
    {
        *count_buf = 0;
    }

    return MCM_RCODE_PASS;
FREE_02:
    if(*data_buf == NULL)
        free(tmp_buf);
FREE_01:
    return fret;
}

// 取得 group 的全部 entry 的資料數值 (基本模式).
// this_session (I) :
//   session 資料.
// mix_path (I) :
//   目標路徑.
// data_access (I) :
//   要從何處取出數值.
//     MCM_DACCESS_SYS  : 已存在的.
//     MCM_DACCESS_NEW  : 新設定的.
//     MCM_DACCESS_AUTO : 優先取新設定的資料, 沒有才找已存在的資料.
// count_buf (O) :
//   儲存筆數的緩衝.
// data_buf (O) :
//   紀錄資料數值的緩衝.
//     == NULL : 表示由內部配置紀錄資料數值的緩衝.
//     != NULL : 表示由外部指定紀錄資料數值的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_get_all_entry_by_path(
    struct mcm_service_session_t *this_session,
    char *mix_path,
    MCM_DTYPE_FLAG_TD data_access,
    MCM_DTYPE_EK_TD *count_buf,
    void **data_buf)
{
    int fret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *parent_store;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", mix_path);

    fret = mcm_config_find_entry_use_mix(this_session, mix_path, MCM_PLIMIT_BOTH,
                                         &self_model_group, NULL, NULL, &parent_store);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_find_entry_use_mix() fail");
        return fret;
    }

    fret = mcm_config_get_all_entry_by_info(this_session, self_model_group, parent_store,
                                            data_access, count_buf, data_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_get_all_entry_by_info() fail");
        return fret;
    }

    return fret;
}

// 刪除 group 的全部 entry (進階模式).
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// parent_store (I) :
//   目標 store 的 parent.
// data_access (I) :
//   刪除的方式.
//     MCM_DACCESS_SYS  : 馬上刪除.
//     MCM_DACCESS_NEW  : 僅做標記, 之後由系統刪除.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_del_all_entry_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *parent_store,
    MCM_DTYPE_FLAG_TD data_access)
{
    int fret;
    struct mcm_config_store_t **store_head_in_parent = NULL, **store_tail_in_parent,
        **store_tree_in_parent, *list_store, *each_store;
    MCM_DTYPE_EK_TD *store_count_in_parent;
    MCM_DTYPE_DS_TD deny_status;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    if(this_session->session_permission != MCM_SPERMISSION_RW)
    {
        MCM_EMSG("session permission is read only");
        return MCM_RCODE_CONFIG_PERMISSION_DENY;
    }

    if(parent_store == NULL)
    {
        list_store = mcm_config_root_store;
    }
    else
    {
        store_head_in_parent = parent_store->child_store_list_head_array +
                               this_model_group->store_index_in_parent;
        list_store = *store_head_in_parent;
    }

#if MCM_CFDMODE
    MCM_CFDMSG("[%s] [%p(%p)]",
               this_model_group->group_name, store_head_in_parent, list_store);
#endif

    if(this_model_group->group_type != MCM_DTYPE_GD_INDEX)
    {
        MCM_EMSG("only {%s} can delall entry", MCM_DTYPE_GD_KEY);
        return MCM_RCODE_CONFIG_ACCESS_DENY;
    }

    // DEL-SYS NONE 允許.
    //         SET  允許.
    //         ADD  允許.
    //         DEL  允許.
    // DEL-NEW NONE 允許.
    //         SET  允許.
    //         ADD  禁止.
    //         DEL  允許.

    MCM_CFDMSG("access[%s]", data_access == MCM_DACCESS_SYS ? "SYS" : "NEW");

    if(list_store != NULL)
    {
        if(data_access == MCM_DACCESS_SYS)
        {
        }
        else
        {
            deny_status = MCM_DSCHANGE_ADD;
            fret = mcm_check_access(this_model_group, list_store, deny_status, 1, 0);
            if(fret < MCM_RCODE_PASS)
            {
                MCM_EMSG("invalid access [DEL (NEW) -> ADD]");
                return fret;
            }
        }
    }

    if(list_store != NULL)
    {
        if(data_access == MCM_DACCESS_SYS)
        {
            store_head_in_parent = parent_store->child_store_list_head_array +
                                   this_model_group->store_index_in_parent;
            MCM_CFDMSG("adjust store_list_head_in_parent[%p(%p>>NULL)]",
                       store_head_in_parent, *store_head_in_parent);
            *store_head_in_parent = NULL;

            store_tail_in_parent = parent_store->child_store_list_tail_array +
                                   this_model_group->store_index_in_parent;
            MCM_CFDMSG("adjust store_list_tail_in_parent[%p(%p>>NULL)]",
                       store_tail_in_parent, *store_tail_in_parent);
            *store_tail_in_parent = NULL;

            store_tree_in_parent = parent_store->child_store_tree_array +
                                   this_model_group->store_index_in_parent;
            MCM_CFDMSG("adjust store_tree_in_parent[%p(%p>>NULL)]",
                       store_tree_in_parent, *store_tree_in_parent);
            *store_tree_in_parent = NULL;

            store_count_in_parent = parent_store->child_store_count_array +
                                    this_model_group->store_index_in_parent;
            MCM_CFDMSG("adjust store_count_in_parent[" MCM_DTYPE_EK_PF ">>0]",
                       *store_count_in_parent);
            *store_count_in_parent = 0;

            // list_store->data_value_sys == NULL 表示是 ADD 狀態,
            // list_store->data_value_sys 沒有資料所以不設定需要儲存.
            if(this_model_group->group_save != 0)
                if(list_store->data_value_sys != NULL)
                    mcm_do_save = 1;

            mcm_destory_store(list_store);
        }
        else
        {
            for(each_store = list_store; each_store != NULL; each_store = each_store->next_store)
            {
                fret = mcm_adjust_data(this_model_group, NULL, each_store,
                                       MCM_DMODIFY_DEL_NEW, MCM_MOBJECT_ENTRY);
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_EMSG("call mcm_adjust_data() fail");
                    return fret;
                }
                fret = mcm_adjust_child_data(this_model_group, NULL, each_store,
                                             MCM_DMODIFY_DEL_NEW, MCM_MOBJECT_ENTRY, 0);
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_EMSG("call mcm_adjust_child_data() fail");
                    return fret;
                }
            }
        }
    }

    return MCM_RCODE_PASS;
}

// 刪除 group 的全部 entry (基本模式).
// this_session (I) :
//   session 資料.
// mix_path (I) :
//   目標路徑.
// data_access (I) :
//   刪除的方式.
//     MCM_DACCESS_SYS  : 馬上刪除.
//     MCM_DACCESS_NEW  : 僅做標記, 之後由系統刪除.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_del_all_entry_by_path(
    struct mcm_service_session_t *this_session,
    char *mix_path,
    MCM_DTYPE_FLAG_TD data_access)
{
    int fret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *parent_store;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", mix_path);

    fret = mcm_config_find_entry_use_mix(this_session, mix_path, MCM_PLIMIT_BOTH,
                                         &self_model_group, NULL, NULL, &parent_store);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_find_entry_use_mix() fail");
        return fret;
    }

    fret = mcm_config_del_all_entry_by_info(this_session, self_model_group, parent_store,
                                            data_access);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_del_all_entry_by_info() fail");
        return fret;
    }

    return fret;
}

// 取得 group 的最大筆數限制 (進階模式).
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// count_buf (O) :
//   儲存最大筆數的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_get_max_count_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    MCM_DTYPE_EK_TD *count_buf)
{
    MCM_CFDMSG("=> %s", __FUNCTION__);

    *count_buf = this_model_group->group_max;
    MCM_CFDMSG("max_count[" MCM_DTYPE_EK_PF "]", *count_buf);

    return MCM_RCODE_PASS;
}

// 取得 group 的最大筆數限制 (基本模式).
// this_session (I) :
//   session 資料.
// mask_path (I) :
//   目標路徑.
// count_buf (O) :
//   儲存最大筆數的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_get_max_count_by_path(
    struct mcm_service_session_t *this_session,
    char *mask_path,
    MCM_DTYPE_EK_TD *count_buf)
{
    int fret;
    struct mcm_config_model_group_t *self_model_group;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", mask_path);

    fret = mcm_config_find_group_by_mask(this_session, mask_path, &self_model_group);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_find_group_by_mask() fail");
        return fret;
    }

    fret = mcm_config_get_max_count_by_info(this_session, self_model_group, count_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_get_max_count_by_info() fail");
        return fret;
    }

    return fret;
}

// 取得 group 目前的 entry 數 (進階模式).
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// parent_store (I) :
//   目標 store 的 parent.
// count_buf (O) :
//   儲存筆數的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_get_count_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *parent_store,
    MCM_DTYPE_EK_TD *count_buf)
{
    struct mcm_config_store_t *each_store;
    MCM_DTYPE_EK_TD *store_count_in_parent, self_count = 0;
#if MCM_CFDMODE
    struct mcm_config_store_t **dbg_store_head_in_parent = NULL, *dbg_store;
    void *dbg_dloc;
    MCM_DTYPE_EK_TD dbg_key = 0;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    if(parent_store == NULL)
    {
        dbg_store = mcm_config_root_store;
    }
    else
    {
        dbg_store_head_in_parent = parent_store->child_store_list_head_array +
                                   this_model_group->store_index_in_parent;
        dbg_store = *dbg_store_head_in_parent;
    }
    MCM_CFDMSG("[%s] [%p(%p)]",
               this_model_group->group_name, dbg_store_head_in_parent, dbg_store);

    for(each_store = dbg_store; each_store != NULL; each_store = each_store->next_store)
    {
        if(this_model_group->group_type == MCM_DTYPE_GD_INDEX)
        {
            dbg_dloc = each_store->data_value_new != NULL ?
                       each_store->data_value_new : each_store->data_value_sys;
            dbg_dloc += this_model_group->entry_key_offset_value;
            dbg_key = *((MCM_DTYPE_EK_TD *) dbg_dloc);
        }
        MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] [%p]",
                   this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, dbg_key, each_store);
    }
#endif

    if(parent_store == NULL)
    {
        for(each_store = mcm_config_root_store; each_store != NULL;
            each_store = each_store->next_store)
        {
            self_count++;
        }
    }
    else
    {
        store_count_in_parent = parent_store->child_store_count_array +
                                this_model_group->store_index_in_parent;
        self_count = *store_count_in_parent;
    }

    *count_buf = self_count;
    MCM_CFDMSG("count[" MCM_DTYPE_EK_PF "]", *count_buf);

    return MCM_RCODE_PASS;
}

// 取得 group 目前的 entry 數 (基本模式).
// this_session (I) :
//   session 資料.
// mix_path (I) :
//   目標路徑.
// count_buf (O) :
//   儲存筆數的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_get_count_by_path(
    struct mcm_service_session_t *this_session,
    char *mix_path,
    MCM_DTYPE_EK_TD *count_buf)
{
    int fret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *parent_store;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", mix_path);

    fret = mcm_config_find_entry_use_mix(this_session, mix_path, MCM_PLIMIT_BOTH,
                                         &self_model_group, NULL, NULL, &parent_store);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_find_entry_use_mix() fail");
        return fret;
    }

    fret = mcm_config_get_count_by_info(this_session, self_model_group, parent_store, count_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_get_count_by_info() fail");
        return fret;
    }

    return fret;
}

// 取得可用的 key (進階模式).
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// parent_store (I) :
//   目標 store 的 parent.
// key_buf (O) :
//   儲存 key 的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_get_usable_key_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *parent_store,
    MCM_DTYPE_EK_TD *key_buf)
{
    int fret = MCM_RCODE_CONFIG_INTERNAL_ERROR;
    struct mcm_config_store_t **store_head_in_parent = NULL, *list_store = NULL,
        **store_tree_in_parent = NULL, *tree_store = NULL, *each_store;
    MCM_DTYPE_EK_TD *store_count_in_parent, self_count = 0, self_key = 0, tmp_key = 0;
    MCM_DTYPE_USIZE_TD *key_array = NULL, array_size, array_unit, size_idx, unit_idx,
        tmp_mask, tmp_bit;
    void *data_loc;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    if(parent_store == NULL)
    {
        list_store = tree_store = mcm_config_root_store;

        for(each_store = list_store; each_store != NULL; each_store = each_store->next_store)
            self_count++;
    }
    else
    {
        store_head_in_parent = parent_store->child_store_list_head_array +
                               this_model_group->store_index_in_parent;
        list_store = *store_head_in_parent;

        store_tree_in_parent = parent_store->child_store_tree_array +
                               this_model_group->store_index_in_parent;
        tree_store = *store_tree_in_parent;

        store_count_in_parent = parent_store->child_store_count_array +
                                this_model_group->store_index_in_parent;
        self_count = *store_count_in_parent;
    }

#if MCM_CFDMODE
    MCM_CFDMSG("[%s] [%p(%p)]",
               this_model_group->group_name, store_head_in_parent, list_store);

    for(each_store = list_store; each_store != NULL; each_store = each_store->next_store)
    {
        if(this_model_group->group_type == MCM_DTYPE_GD_INDEX)
        {
            data_loc = each_store->data_value_new != NULL ?
                       each_store->data_value_new : each_store->data_value_sys;
            data_loc += this_model_group->entry_key_offset_value;
            tmp_key = *((MCM_DTYPE_EK_TD *) data_loc);
        }
        MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "] [%p]",
                   this_model_group->group_name, MCM_SPROFILE_PATH_KEY_KEY, tmp_key, each_store);
    }
#endif

    if(self_count < this_model_group->group_max)
    {
        tmp_key = 1;
        if(tree_store != NULL)
        {
            for(each_store = tree_store; each_store->rtree_store != NULL;
                each_store = each_store->rtree_store);
            data_loc = each_store->data_value_new != NULL ?
                       each_store->data_value_new : each_store->data_value_sys;
            data_loc += this_model_group->entry_key_offset_value;
            tmp_key = *((MCM_DTYPE_EK_TD *) data_loc) + 1;
        }

        array_unit = sizeof(MCM_DTYPE_USIZE_TD) * 8;
        array_size = (tmp_key / array_unit) + 1;
        key_array = (MCM_DTYPE_USIZE_TD *) calloc(array_size, sizeof(MCM_DTYPE_USIZE_TD));
        if(key_array == NULL)
        {
            MCM_EMSG("call calloc() fail [%s]", strerror(errno));
            fret = MCM_RCODE_CONFIG_ALLOC_FAIL;
            goto FREE_01;
        }

        for(each_store = list_store; each_store != NULL; each_store = each_store->next_store)
        {
            data_loc = each_store->data_value_new != NULL ?
                       each_store->data_value_new : each_store->data_value_sys;
            data_loc += this_model_group->entry_key_offset_value;
            tmp_key = *((MCM_DTYPE_EK_TD *) data_loc) - 1;

            size_idx = tmp_key / array_unit;
            unit_idx = 0x1 << (tmp_key % array_unit);
            key_array[size_idx] |= unit_idx;
        }

        for(size_idx = 0, tmp_mask = ~0x0; self_key == 0; size_idx++)
            if(key_array[size_idx] != tmp_mask)
                for(unit_idx = 0; unit_idx < array_unit; unit_idx++)
                {
                    tmp_bit = 0x1 << unit_idx;
                    if((key_array[size_idx] & tmp_bit) == 0)
                    {
                        self_key = (size_idx * array_unit) + unit_idx + 1;
                        break;
                    }
                }
    }

    *key_buf = self_key;
    MCM_CFDMSG("usable_key[" MCM_DTYPE_EK_PF "]", *key_buf);

    fret = MCM_RCODE_PASS;
    if(key_array != NULL)
        free(key_array);
FREE_01:
    return fret;
}

// 取得可用的 key (基本模式).
// this_session (I) :
//   session 資料.
// mix_path (I) :
//   目標路徑.
// key_buf (O) :
//   儲存 key 的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_get_usable_key_by_path(
    struct mcm_service_session_t *this_session,
    char *mix_path,
    MCM_DTYPE_EK_TD *key_buf)
{
    int fret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *parent_store;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", mix_path);

    fret = mcm_config_find_entry_use_mix(this_session, mix_path, MCM_PLIMIT_BOTH,
                                         &self_model_group, NULL, NULL, &parent_store);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_find_entry_use_mix() fail");
        return fret;
    }

    fret = mcm_config_get_usable_key_by_info(this_session, self_model_group, parent_store, key_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_get_usable_key_by_info() fail");
        return fret;
    }

    return fret;
}

// 取得 member 的資料狀態 (進階模式).
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// this_model_member (I) :
//   目標 model member.
// this_store (I) :
//   目標 store.
// data_buf (O) :
//   儲存資料狀態的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_get_alone_status_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_model_member_t *this_model_member,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_DS_TD *data_buf)
{
#if MCM_CFDMODE
    void *dbg_dloc;
    MCM_DTYPE_EK_TD dbg_key = 0;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    MCM_DBG_SHOW_ALONE_PATH(this_model_group, this_model_member, this_store, dbg_dloc, dbg_key);
#endif

    MCM_GET_ALONE_STATUS(this_model_member, this_store, *data_buf);
    MCM_CFDMSG("status[" MCM_DTYPE_DS_PF "]", *data_buf);

    return MCM_RCODE_PASS;
}

// 取得 member 的資料狀態 (基本模式).
// this_session (I) :
//   session 資料.
// full_path (I) :
//   目標路徑.
// data_buf (O) :
//   儲存資料狀態的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_get_alone_status_by_path(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_DS_TD *data_buf)
{
    int fret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_model_member_t *self_model_member;
    struct mcm_config_store_t *self_store;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", full_path);

    fret = mcm_config_find_alone_use_full(this_session, full_path, MCM_PLIMIT_BOTH,
                                          &self_model_group, &self_model_member, &self_store);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_find_alone_use_full() fail");
        return fret;
    }

    fret = mcm_config_get_alone_status_by_info(this_session, self_model_group,
                                               self_model_member, self_store, data_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_get_alone_status_by_info() fail");
        return fret;
    }

    return fret;
}

// 取得 group 的 entry 本身的資料狀態 (進階模式).
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// this_store (I) :
//   目標 store.
// data_buf (O) :
//   儲存資料狀態的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_get_entry_self_status_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_DS_TD *data_buf)
{
#if MCM_CFDMODE
    void *dbg_dloc;
    MCM_DTYPE_EK_TD dbg_key = 0;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    MCM_DBG_SHOW_ENTRY_PATH(this_model_group, this_store, dbg_dloc, dbg_key);
#endif

    MCM_GET_ENTRY_SELF_STATUS(this_model_group, this_store, *data_buf);
    MCM_CFDMSG("status[" MCM_DTYPE_DS_PF "]", *data_buf);

    return MCM_RCODE_PASS;
}

// 取得 group 的 entry 本身的資料狀態 (基本模式).
// this_session (I) :
//   session 資料.
// full_path (I) :
//   目標路徑.
// data_buf (O) :
//   儲存資料狀態的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_get_entry_self_status_by_path(
    struct mcm_service_session_t *this_session,
    char *full_path,
    MCM_DTYPE_DS_TD *data_buf)
{
    int fret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *self_store;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", full_path);

    fret = mcm_config_find_entry_use_full(this_session, full_path, MCM_PLIMIT_BOTH,
                                          MCM_PLIMIT_BOTH, 1, &self_model_group, &self_store,
                                          NULL, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_find_entry_use_full() fail");
        return fret;
    }

    fret = mcm_config_get_entry_self_status_by_info(this_session, self_model_group, self_store,
                                                    data_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_get_entry_self_status_by_info() fail");
        return fret;
    }

    return fret;
}

// 取得 group 的 entry 中所有 member 的資料狀態 (進階模式).
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// this_store (I) :
//   目標 store.
// data_buf (O) :
//   儲存資料狀態的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_get_entry_all_status_by_info(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    void *data_buf)
{
    struct mcm_config_model_member_t *self_model_member;
    void *buf_loc;
#if MCM_CFDMODE
    void *dbg_dloc;
    MCM_DTYPE_EK_TD dbg_key = 0;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    MCM_DBG_SHOW_ENTRY_PATH(this_model_group, this_store, dbg_dloc, dbg_key);
#endif

    for(self_model_member = this_model_group->member_list; self_model_member != NULL;
        self_model_member = self_model_member->next_model_member)
    {
        buf_loc = data_buf + self_model_member->offset_in_status;
        MCM_GET_ALONE_STATUS(self_model_member, this_store, *((MCM_DTYPE_DS_TD *) buf_loc));
        MCM_CFDMSG("status[%s][" MCM_DTYPE_DS_PF "]",
                   self_model_member->member_name, *((MCM_DTYPE_DS_TD *) buf_loc));
    }

    return MCM_RCODE_PASS;
}

// 取得 group 的 entry 中所有 member 的資料狀態 (基本模式).
// this_session (I) :
//   session 資料.
// full_path (I) :
//   目標路徑.
// data_buf (O) :
//   儲存資料狀態的緩衝.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_get_entry_all_status_by_path(
    struct mcm_service_session_t *this_session,
    char *full_path,
    void *data_buf)
{
    int fret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *self_store;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", full_path);

    fret = mcm_config_find_entry_use_full(this_session, full_path, MCM_PLIMIT_BOTH,
                                          MCM_PLIMIT_BOTH, 1, &self_model_group, &self_store,
                                          NULL, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_find_entry_use_full() fail");
        return fret;
    }

    fret = mcm_config_get_entry_all_status_by_info(this_session, self_model_group, self_store,
                                                   data_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_config_get_entry_all_status_by_info() fail");
        return fret;
    }

    return fret;
}

// 設定 member 的資料狀態.
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// this_model_member (I) :
//   目標 model member.
// this_store (I) :
//   目標 store.
// assign_type (I) :
//   設定的方式.
//     MCM_DSASSIGN_SET : 覆蓋.
//     MCM_DSASSIGN_ADD : 將指定的狀態加入到已存在的狀態.
//     MCM_DSASSIGN_DEL : 將指定的狀態從已存在的狀態移除.
// data_con (I) :
//   要設定的狀態.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_set_alone_status(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_model_member_t *this_model_member,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_LIST_TD assign_type,
    MCM_DTYPE_DS_TD data_con)
{
    void *data_loc;
#if MCM_CFDMODE
    MCM_DTYPE_EK_TD dbg_key = 0;
    MCM_DTYPE_DS_TD dbg_status;
    MCM_DTYPE_USIZE_TD dbg_tcnt, dbg_tlen;
    char dbg_buf[MCM_DBG_BUFFER_SIZE];
#endif

    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    MCM_DBG_SHOW_ALONE_PATH(this_model_group, this_model_member, this_store, data_loc, dbg_key);
#endif

    if(this_model_member->member_type == MCM_DTYPE_EK_INDEX)
    {
        MCM_EMSG("{%s} is system reserve", this_model_member->member_name);
        return MCM_RCODE_CONFIG_ACCESS_DENY;
    }

    data_loc = this_store->data_status + this_model_member->offset_in_status;

#if MCM_CFDMODE
    dbg_status = *((MCM_DTYPE_DS_TD *) data_loc);
#endif

    switch(assign_type)
    {
        case MCM_DSASSIGN_SET:
            *((MCM_DTYPE_DS_TD *) data_loc) = data_con;
            break;
        case MCM_DSASSIGN_ADD:
            *((MCM_DTYPE_DS_TD *) data_loc) |= data_con;
            break;
        case MCM_DSASSIGN_DEL:
            *((MCM_DTYPE_DS_TD *) data_loc) &= ~data_con;
            break;
    }
#if MCM_CFDMODE
    MCM_DBG_FORMAT_STATUS_LIST(dbg_buf, mcm_dbg_status_msg_list, assign_type, data_con, dbg_status,
                               data_loc, dbg_tcnt, dbg_tlen);
    MCM_CFDMSG("status%s", dbg_buf);
#endif

    return MCM_RCODE_PASS;
}

// 設定 group 的 entry 本身的資料狀態.
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// this_store (I) :
//   目標 store.
// assign_type (I) :
//   設定的方式.
//     MCM_DSASSIGN_SET : 覆蓋.
//     MCM_DSASSIGN_ADD : 將指定的狀態加入到已存在的狀態.
//     MCM_DSASSIGN_DEL : 將指定的狀態從已存在的狀態移除.
// data_con (I) :
//   要設定的狀態.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_set_entry_self_status(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_LIST_TD assign_type,
    MCM_DTYPE_DS_TD data_con)
{
    void *data_loc;
#if MCM_CFDMODE
    MCM_DTYPE_EK_TD dbg_key = 0;
    MCM_DTYPE_DS_TD dbg_status;
    MCM_DTYPE_USIZE_TD dbg_tcnt, dbg_tlen;
    char dbg_buf[MCM_DBG_BUFFER_SIZE];
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    MCM_DBG_SHOW_ENTRY_PATH(this_model_group, this_store, data_loc, dbg_key);
#endif

    data_loc = this_store->data_status + this_model_group->entry_key_offset_status;

#if MCM_CFDMODE
    dbg_status = *((MCM_DTYPE_DS_TD *) data_loc);
#endif

    switch(assign_type)
    {
        case MCM_DSASSIGN_SET:
            *((MCM_DTYPE_DS_TD *) data_loc) = data_con;
            break;
        case MCM_DSASSIGN_ADD:
            *((MCM_DTYPE_DS_TD *) data_loc) |= data_con;
            break;
        case MCM_DSASSIGN_DEL:
            *((MCM_DTYPE_DS_TD *) data_loc) &= ~data_con;
            break;
    }
#if MCM_CFDMODE
    MCM_DBG_FORMAT_STATUS_LIST(dbg_buf, mcm_dbg_status_msg_list, assign_type, data_con, dbg_status,
                               data_loc, dbg_tcnt, dbg_tlen);
    MCM_CFDMSG("status%s", dbg_buf);
#endif

    return MCM_RCODE_PASS;
}

// 設定 group 的 entry 中所有 member 的資料狀態.
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// this_store (I) :
//   目標 store.
// assign_type (I) :
//   設定的方式.
//     MCM_DSASSIGN_SET : 覆蓋.
//     MCM_DSASSIGN_ADD : 將指定的狀態加入到已存在的狀態.
//     MCM_DSASSIGN_DEL : 將指定的狀態從已存在的狀態移除.
// data_con (I) :
//   要設定的狀態.
// skip_entry_self (I) :
//   是否跳過設定 entry self member (ek 類型的 member).
//     0 : 否.
//     1 : 是.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_set_entry_all_status(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_LIST_TD assign_type,
    MCM_DTYPE_DS_TD data_con,
    MCM_DTYPE_BOOL_TD skip_entry_self)
{
    struct mcm_config_model_member_t *self_model_member;
    void *data_loc;
#if MCM_CFDMODE
    MCM_DTYPE_EK_TD dbg_key = 0;
    MCM_DTYPE_DS_TD dbg_status;
    MCM_DTYPE_USIZE_TD dbg_tcnt, dbg_tlen;
    char dbg_buf[MCM_DBG_BUFFER_SIZE];
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    MCM_DBG_SHOW_ENTRY_PATH(this_model_group, this_store, data_loc, dbg_key);
#endif

    for(self_model_member = this_model_group->member_list; self_model_member != NULL;
        self_model_member = self_model_member->next_model_member)
    {
        if(self_model_member->member_type == MCM_DTYPE_EK_INDEX)
            if(skip_entry_self != 0)
                continue;

        data_loc = this_store->data_status + self_model_member->offset_in_status;

#if MCM_CFDMODE
        dbg_status = *((MCM_DTYPE_DS_TD *) data_loc);
#endif

        switch(assign_type)
        {
            case MCM_DSASSIGN_SET:
                *((MCM_DTYPE_DS_TD *) data_loc) = data_con;
                break;
            case MCM_DSASSIGN_ADD:
                *((MCM_DTYPE_DS_TD *) data_loc) |= data_con;
                break;
            case MCM_DSASSIGN_DEL:
                *((MCM_DTYPE_DS_TD *) data_loc) &= ~data_con;
                break;
        }
#if MCM_CFDMODE
        MCM_DBG_FORMAT_STATUS_LIST(dbg_buf, mcm_dbg_status_msg_list, assign_type,
                                   data_con, dbg_status, data_loc, dbg_tcnt, dbg_tlen);
        MCM_CFDMSG("status[%s]%s", self_model_member->member_name, dbg_buf);
#endif
    }

    return MCM_RCODE_PASS;
}

// 檢查資料檔案是否有錯誤.
// this_session (I) :
// session 資料.
// file_path (I) :
//   資料檔案路徑.
// store_result_buf (O) :
//   資料檔案的檢查結果 :
//     >= MCM_RCODE_PASS : 正確.
//     <  MCM_RCODE_PASS : 錯誤.
// store_version_buf (O) :
//   紀錄資料檔案內的版本資訊的緩衝.
// store_version_size (I) :
//   紀錄資料檔案內的版本資訊的緩衝的大小.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_config_check_store_file(
    struct mcm_service_session_t *this_session,
    char *file_path,
    MCM_DTYPE_LIST_TD *store_result_buf,
    char *store_version_buf,
    MCM_DTYPE_USIZE_TD store_version_size)
{
    int fret;
    struct mcm_config_base_t self_base;
    struct mcm_config_store_t *self_store = NULL;
    FILE *file_fp;
    char *read_buf = NULL;
    MCM_DTYPE_USIZE_TD read_size = 0;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    memset(&self_base, 0, sizeof(struct mcm_config_base_t));

    file_fp = fopen(file_path, "r");
    if(file_fp == NULL)
    {
        MCM_EMSG("call fopen(%s) fail [%s]", file_path, strerror(errno));
        fret = MCM_RCODE_CONFIG_INTERNAL_ERROR;
        goto FREE_01;
    }

    fret = mcm_realloc_buf_config(&read_buf, &read_size);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_realloc_buf_config() fail");
        goto FREE_02;
    }

    fret = mcm_load_store_process(mcm_config_root_model_group, file_fp, MCM_FSOURCE_VERIFY,
                                  &read_buf, &read_size, &self_base, &self_store);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_load_store_process() fail");
        goto FREE_03;
    }

    fret = mcm_load_store_check_error(mcm_config_root_model_group, self_store);
    *store_result_buf = fret < MCM_RCODE_PASS ? MCM_RCODE_CONFIG_INTERNAL_ERROR : MCM_RCODE_PASS;

    snprintf(store_version_buf, store_version_size, "%s", self_base.profile_current_version);

    fret = MCM_RCODE_PASS;
    mcm_destory_store(self_store);
FREE_03:
    free(read_buf);
FREE_02:
    fclose(file_fp);
FREE_01:
    return fret;
}

// 取得路徑的最大長度 (包含 \0 的空間).
// this_session (I) :
//   session 資料.
// max_len_buf (O) :
//   儲存最大長度的緩衝.
// return :
//   MCM_RCODE_PASS.
int mcm_config_get_path_max_length(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_USIZE_TD *max_len_buf)
{
    MCM_CFDMSG("=> %s", __FUNCTION__);

    *max_len_buf = MCM_CONFIG_PATH_MAX_LENGTH;
    MCM_CFDMSG("path_max_length[" MCM_DTYPE_USIZE_PF "]", *max_len_buf);

    return MCM_RCODE_PASS;
}

// 取得目標 model group 的所有的 member 的 name 資訊的大小.
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// size_buf (O) :
//   儲存大小的緩衝.
// return :
//   MCM_RCODE_PASS.
int mcm_config_get_list_name_size(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    MCM_DTYPE_USIZE_TD *size_buf)
{
    struct mcm_config_model_member_t *self_model_member;
    MCM_DTYPE_USIZE_TD tmp_size = 0;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", this_model_group->group_name);

    for(self_model_member = this_model_group->member_list; self_model_member != NULL;
        self_model_member = self_model_member->next_model_member)
    {
        // 格式 : [name_len][name][0].
        tmp_size += sizeof(MCM_DTYPE_SNLEN_TD);
        tmp_size += self_model_member->member_nlen + 1;
    }

    *size_buf = tmp_size;
    MCM_CFDMSG("name_size[" MCM_DTYPE_USIZE_PF "]", *size_buf);

    return MCM_RCODE_PASS;
}

// 取得目標 model group 的所有的 member 的 name.
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// data_buf (O) :
//   儲存 member 的 name 的緩衝.
// return :
//   MCM_RCODE_PASS.
int mcm_config_get_list_name_data(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    void *data_buf)
{
    struct mcm_config_model_member_t *self_model_member;
    MCM_DTYPE_USIZE_TD dlen;
    void *buf_loc;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    buf_loc = data_buf;

    for(self_model_member = this_model_group->member_list; self_model_member != NULL;
        self_model_member = self_model_member->next_model_member)
    {
        // 格式 : [name_len][name][0].

        // 填充 name_len.
        dlen = self_model_member->member_nlen + 1;
        *((MCM_DTYPE_SNLEN_TD *) buf_loc) = (MCM_DTYPE_SNLEN_TD) dlen;
        buf_loc += sizeof(MCM_DTYPE_SNLEN_TD);

        // 填充 name.
        memcpy(buf_loc, self_model_member->member_name, dlen);
        buf_loc += dlen;
    }

    return MCM_RCODE_PASS;
}

// 取得目標 model group 的所有的 member 的 data_type 資訊的大小.
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// size_buf (O) :
//   儲存大小的緩衝.
// return :
//   MCM_RCODE_PASS.
int mcm_config_get_list_type_size(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    MCM_DTYPE_USIZE_TD *size_buf)
{
    struct mcm_config_model_member_t *self_model_member;
    MCM_DTYPE_USIZE_TD tmp_size = 0;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    MCM_CFDMSG("[%s]", this_model_group->group_name);

    for(self_model_member = this_model_group->member_list; self_model_member != NULL;
        self_model_member = self_model_member->next_model_member)
    {
        // 格式 : [data_type].
        tmp_size += sizeof(MCM_DTYPE_SDTYPE_TD);
    }

    *size_buf = tmp_size;
    MCM_CFDMSG("type_size[" MCM_DTYPE_USIZE_PF "]", *size_buf);

    return MCM_RCODE_PASS;
}

// 取得目標 model group 的所有的 member 的 data_type.
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// data_buf (O) :
//   儲存 member 的 data_type 的緩衝.
// return :
//   MCM_RCODE_PASS : 成功.
//   other        : 錯誤.
int mcm_config_get_list_type_data(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    void *data_buf)
{
    struct mcm_config_model_member_t *self_model_member;
    void *buf_loc;


    MCM_CFDMSG("=> %s", __FUNCTION__);

    buf_loc = data_buf;

    for(self_model_member = this_model_group->member_list; self_model_member != NULL;
        self_model_member = self_model_member->next_model_member)
    {
        // 格式 : [data_type].
        // 填充 data_type.
        *((MCM_DTYPE_SDTYPE_TD *) buf_loc) = (MCM_DTYPE_SDTYPE_TD) self_model_member->member_type;
        buf_loc += sizeof(MCM_DTYPE_SDTYPE_TD);
    }

    return MCM_RCODE_PASS;
}

// 取得目標 store 的所有的 member 的 data_value 資訊的大小.
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// this_sotr (I) :
//   目標 store.
// size_buf (O) :
//   儲存大小的緩衝.
// return :
//   MCM_RCODE_PASS.
int mcm_config_get_list_value_size(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    MCM_DTYPE_USIZE_TD *size_buf)
{
    struct mcm_config_model_member_t *self_model_member;
    MCM_DTYPE_USIZE_TD tmp_size = 0;
#if MCM_SUPPORT_DTYPE_S
    void *data_loc = NULL, *tmp_loc;
#endif
#if MCM_CFDMODE
    MCM_DTYPE_EK_TD dbg_key = 0;
#endif


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    MCM_DBG_SHOW_ENTRY_PATH(this_model_group, this_store, data_loc, dbg_key);
#endif

#if MCM_SUPPORT_DTYPE_S
    data_loc = this_store->data_value_new != NULL ?
               this_store->data_value_new : this_store->data_value_sys;
#endif

    for(self_model_member = this_model_group->member_list; self_model_member != NULL;
        self_model_member = self_model_member->next_model_member)
    {
        // 格式 : [data_value_size][data_value].
        tmp_size += sizeof(MCM_DTYPE_USIZE_TD);
        switch(self_model_member->member_type)
        {
            case MCM_DTYPE_EK_INDEX:
#if MCM_SUPPORT_DTYPE_RK
            case MCM_DTYPE_RK_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISC
            case MCM_DTYPE_ISC_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUC
            case MCM_DTYPE_IUC_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISS
            case MCM_DTYPE_ISS_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUS
            case MCM_DTYPE_IUS_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISI
            case MCM_DTYPE_ISI_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUI
            case MCM_DTYPE_IUI_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISLL
            case MCM_DTYPE_ISLL_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IULL
            case MCM_DTYPE_IULL_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FF
            case MCM_DTYPE_FF_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FD
            case MCM_DTYPE_FD_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FLD
            case MCM_DTYPE_FLD_INDEX:
#endif
                tmp_size += self_model_member->member_size;
                break;
#if MCM_SUPPORT_DTYPE_S
            case MCM_DTYPE_S_INDEX:
                tmp_loc = data_loc + self_model_member->offset_in_value;
                tmp_size += strlen((MCM_DTYPE_S_TD *) tmp_loc);
                break;
#endif
#if MCM_SUPPORT_DTYPE_B
            case MCM_DTYPE_B_INDEX:
                tmp_size += self_model_member->member_size;
                break;
#endif
        }
    }

    *size_buf = tmp_size;
    MCM_CFDMSG("value_size[" MCM_DTYPE_USIZE_PF "]", *size_buf);

    return MCM_RCODE_PASS;
}

// 取得目標 model 的所有的 member 的 data_value.
// this_session (I) :
//   session 資料.
// this_model_group (I) :
//   目標 model group.
// this_sotr (I) :
//   目標 store.
// data_buf (O) :
//   儲存 member 的 data_value 的緩衝.
// return :
//   MCM_RCODE_PASS.
int mcm_config_get_list_value_data(
    struct mcm_service_session_t *this_session,
    struct mcm_config_model_group_t *this_model_group,
    struct mcm_config_store_t *this_store,
    void *data_buf)
{
    struct mcm_config_model_member_t *self_model_member;
    void *data_loc = NULL, *tmp_loc, *buf_loc;
#if MCM_SUPPORT_DTYPE_S || MCM_SUPPORT_DTYPE_B
    MCM_DTYPE_USIZE_TD dlen;
#endif
#if MCM_CFDMODE
    MCM_DTYPE_EK_TD dbg_key = 0;
    void *dbg_dloc;
    MCM_DTYPE_USIZE_TD dbg_tidx, dbg_tlen;
    char dbg_buf[MCM_DBG_BUFFER_SIZE];
#endif


#define MCM_FILL_NUM_VALUE(type_def, type_fmt) \
    do                                                                       \
    {                                                                        \
        *((MCM_DTYPE_USIZE_TD *) buf_loc) = self_model_member->member_size;  \
        buf_loc += sizeof(MCM_DTYPE_USIZE_TD);                               \
        *((type_def *) buf_loc) = *((type_def *) tmp_loc);                   \
        MCM_CFDMSG("[%s][" type_fmt "]",                                     \
                   self_model_member->member_name, *((type_def *) buf_loc)); \
        buf_loc += self_model_member->member_size;                           \
    }                                                                        \
    while(0)


    MCM_CFDMSG("=> %s", __FUNCTION__);

#if MCM_CFDMODE
    MCM_DBG_SHOW_ENTRY_PATH(this_model_group, this_store, data_loc, dbg_key);
#endif

    data_loc = this_store->data_value_new != NULL ?
               this_store->data_value_new : this_store->data_value_sys;

    buf_loc = data_buf;

    for(self_model_member = this_model_group->member_list; self_model_member != NULL;
        self_model_member = self_model_member->next_model_member)
    {
        // 格式 : [data_value_size][data_value].
        tmp_loc = data_loc + self_model_member->offset_in_value;
        switch(self_model_member->member_type)
        {
            case MCM_DTYPE_EK_INDEX:
                MCM_FILL_NUM_VALUE(MCM_DTYPE_EK_TD, MCM_DTYPE_EK_PF);
                break;
#if MCM_SUPPORT_DTYPE_RK
            case MCM_DTYPE_RK_INDEX:
                MCM_FILL_NUM_VALUE(MCM_DTYPE_RK_TD, MCM_DTYPE_RK_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_ISC
            case MCM_DTYPE_ISC_INDEX:
                MCM_FILL_NUM_VALUE(MCM_DTYPE_ISC_TD, MCM_DTYPE_ISC_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_IUC
            case MCM_DTYPE_IUC_INDEX:
                MCM_FILL_NUM_VALUE(MCM_DTYPE_IUC_TD, MCM_DTYPE_IUC_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_ISS
            case MCM_DTYPE_ISS_INDEX:
                MCM_FILL_NUM_VALUE(MCM_DTYPE_ISS_TD, MCM_DTYPE_ISS_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_IUS
            case MCM_DTYPE_IUS_INDEX:
                MCM_FILL_NUM_VALUE(MCM_DTYPE_IUS_TD, MCM_DTYPE_IUS_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_ISI
            case MCM_DTYPE_ISI_INDEX:
                MCM_FILL_NUM_VALUE(MCM_DTYPE_ISI_TD, MCM_DTYPE_ISI_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_IUI
            case MCM_DTYPE_IUI_INDEX:
                MCM_FILL_NUM_VALUE(MCM_DTYPE_IUI_TD, MCM_DTYPE_IUI_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_ISLL
            case MCM_DTYPE_ISLL_INDEX:
                MCM_FILL_NUM_VALUE(MCM_DTYPE_ISLL_TD, MCM_DTYPE_ISLL_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_IULL
            case MCM_DTYPE_IULL_INDEX:
                MCM_FILL_NUM_VALUE(MCM_DTYPE_IULL_TD, MCM_DTYPE_IULL_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_FF
            case MCM_DTYPE_FF_INDEX:
                MCM_FILL_NUM_VALUE(MCM_DTYPE_FF_TD, MCM_DTYPE_FF_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_FD
            case MCM_DTYPE_FD_INDEX:
                MCM_FILL_NUM_VALUE(MCM_DTYPE_FD_TD, MCM_DTYPE_FD_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_FLD
            case MCM_DTYPE_FLD_INDEX:
                MCM_FILL_NUM_VALUE(MCM_DTYPE_FLD_TD, MCM_DTYPE_FLD_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_S
            case MCM_DTYPE_S_INDEX:
                dlen = strlen((MCM_DTYPE_S_TD *) tmp_loc);
                *((MCM_DTYPE_USIZE_TD *) buf_loc) = dlen;
                buf_loc += sizeof(MCM_DTYPE_USIZE_TD);
                memcpy(buf_loc, tmp_loc, dlen);
                buf_loc += dlen;
#if MCM_CFDMODE
                dbg_dloc = buf_loc - dlen;
                MCM_DBG_FORMAT_CONFIG_S_VALUE(dbg_buf, dbg_dloc, dlen, dbg_tidx, dbg_tlen);
                MCM_CFDMSG("[%s][%s]", self_model_member->member_name, dbg_buf);
#endif
                break;
#endif
#if MCM_SUPPORT_DTYPE_B
            case MCM_DTYPE_B_INDEX:
                dlen = self_model_member->member_size;
                *((MCM_DTYPE_USIZE_TD *) buf_loc) = dlen;
                buf_loc += sizeof(MCM_DTYPE_USIZE_TD);
                memcpy(buf_loc, tmp_loc, dlen);
                buf_loc += dlen;
#if MCM_CFDMODE
                dbg_dloc = buf_loc - dlen;
                MCM_DBG_FORMAT_CONFIG_B_VALUE(dbg_buf, dbg_dloc, dlen, dbg_tidx, dbg_tlen);
                MCM_CFDMSG("[%s][%s]", self_model_member->member_name, dbg_buf);
#endif
                break;
#endif
        }
    }

    return MCM_RCODE_PASS;
}

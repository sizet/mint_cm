// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "../mcm_lib/mcm_lheader/mcm_type.h"
#include "../mcm_lib/mcm_lheader/mcm_keyword.h"
#include "../mcm_lib/mcm_lheader/mcm_connect.h"
#include "../mcm_lib/mcm_lheader/mcm_return.h"
#include "../mcm_lib/mcm_lheader/mcm_limit.h"
#include "../mcm_lib/mcm_lulib/mcm_lulib_api.h"
#include "mcm_cgi_common_extern.h"

#if MCM_CGIEMODE | MCM_CGIECTMODE | MCM_CCDMODE
    #include <fcntl.h>
    #include <sys/stat.h>
#endif




#if MCM_CGIEMODE | MCM_CGIECTMODE | MCM_CCDMODE
int dbg_console_fd;
char dbg_msg_buf[MCM_DBG_BUFFER_SIZE];
#endif

#if MCM_CGIEMODE
    #define MCM_CEMSG(msg_fmt, msg_args...) \
        MCM_CGI_CONSOLE_MSG(dbg_console_fd, dbg_msg_buf, msg_fmt, ##msg_args)
#else
    #define MCM_CEMSG(msg_fmt, msg_args...)
#endif

#if MCM_CGIECTMODE
    #define MCM_CECTMSG(msg_fmt, msg_args...) \
        MCM_CGI_CONSOLE_MSG(dbg_console_fd, dbg_msg_buf, msg_fmt, ##msg_args)
#else
    #define MCM_CECTMSG(msg_fmt, msg_args...)
#endif

#if MCM_CCDMODE
    #define MCM_CCDMSG(msg_fmt, msg_args...) \
        MCM_CGI_CONSOLE_MSG(dbg_console_fd, dbg_msg_buf, msg_fmt, ##msg_args)
#else
    #define MCM_CCDMSG(msg_fmt, msg_args...)
#endif

#define MCM_CONFIG_MODULE_PATH "./mcm_cgi_config_module.lib"

#define MCM_MAX_COUNT_KEY       "$max_count"
#define MCM_MIN_PRINTABLE_KEY   0x20
#define MCM_MAX_PRINTABLE_KEY   0x7E
#define MCM_SPECIAL_KEY         '%'
#define MCM_PARAMETER_SPLIT_KEY '&'
#define MCM_VALUE_SPLIT_KEY     '='

// query 的參數表.
enum MCM_QUERY_PARAMETER_INDEX
{
    MCM_REQUEST_ACTION_INDEX = 0,
    MCM_SOCKET_PATH_INDEX,
    MCM_SESSION_PERMISSION_INDEX,
    MCM_SESSION_STACK_SIZE_INDEX,
    MCM_DATA_FORMAT_INDEX,
    MCM_AFTER_COMPLETE_INDEX
};
#define MCM_REQUEST_ACTION_KEY     "request_action"
#define MCM_SOCKET_PATH_KEY        "socket_path"
#define MCM_SESSION_PERMISSION_KEY "session_permission"
#define MCM_SESSION_STACK_SIZE_KEY "session_stack_size"
#define MCM_DATA_FORMAT_KEY        "data_format"
#define MCM_AFTER_COMPLETE_KEY     "after_complete"

// 需求.
enum MCM_REQUEST_ACTION
{
    // 取得 group 的最大 entry 數目.
    MCM_RACTION_OBTAIN_MAX_COUNT = 0,
    // 取得資料.
    MCM_RACTION_OBTAIN_CONFIG,
    // 提交資料.
    MCM_RACTION_SUBMIT_CONFIG
};

// OBTAIN_CONFIG 的輸出數值是否轉為字串格式.
enum MCM_DATA_FORMAT
{
    MCM_DFORMAT_ALL_DEFAULT = 0x0000,
    MCM_DFORMAT_EK_STRING   = 0x0001,
#if MCM_SUPPORT_DTYPE_RK
    MCM_DFORMAT_RK_STRING   = 0x0002,
#endif
#if MCM_SUPPORT_DTYPE_ISC
    MCM_DFORMAT_ISC_STRING  = 0x0004,
#endif
#if MCM_SUPPORT_DTYPE_IUC
    MCM_DFORMAT_IUC_STRING  = 0x0008,
#endif
#if MCM_SUPPORT_DTYPE_ISS
    MCM_DFORMAT_ISS_STRING  = 0x0010,
#endif
#if MCM_SUPPORT_DTYPE_IUS
    MCM_DFORMAT_IUS_STRING  = 0x0020,
#endif
#if MCM_SUPPORT_DTYPE_ISI
    MCM_DFORMAT_ISI_STRING  = 0x0040,
#endif
#if MCM_SUPPORT_DTYPE_IUI
    MCM_DFORMAT_IUI_STRING  = 0x0080,
#endif
#if MCM_SUPPORT_DTYPE_ISLL
    MCM_DFORMAT_ISLL_STRING = 0x0100,
#endif
#if MCM_SUPPORT_DTYPE_IULL
    MCM_DFORMAT_IULL_STRING = 0x0200,
#endif
#if MCM_SUPPORT_DTYPE_FF
    MCM_DFORMAT_FF_STRING   = 0x0400,
#endif
#if MCM_SUPPORT_DTYPE_FD
    MCM_DFORMAT_FD_STRING   = 0x0800,
#endif
#if MCM_SUPPORT_DTYPE_FLD
    MCM_DFORMAT_FLD_STRING  = 0x1000,
#endif
    MCM_DFORMAT_ALL_STRING  = (MCM_DFORMAT_EK_STRING |
#if MCM_SUPPORT_DTYPE_RK
                               MCM_DFORMAT_RK_STRING |
#endif
#if MCM_SUPPORT_DTYPE_ISC
                               MCM_DFORMAT_ISC_STRING |
#endif
#if MCM_SUPPORT_DTYPE_IUC
                               MCM_DFORMAT_IUC_STRING |
#endif
#if MCM_SUPPORT_DTYPE_ISS
                               MCM_DFORMAT_ISS_STRING |
#endif
#if MCM_SUPPORT_DTYPE_IUS
                               MCM_DFORMAT_IUS_STRING |
#endif
#if MCM_SUPPORT_DTYPE_ISI
                               MCM_DFORMAT_ISI_STRING |
#endif
#if MCM_SUPPORT_DTYPE_IUI
                               MCM_DFORMAT_IUI_STRING |
#endif
#if MCM_SUPPORT_DTYPE_ISLL
                               MCM_DFORMAT_ISLL_STRING |
#endif
#if MCM_SUPPORT_DTYPE_IULL
                               MCM_DFORMAT_IULL_STRING |
#endif
#if MCM_SUPPORT_DTYPE_FF
                               MCM_DFORMAT_FF_STRING |
#endif
#if MCM_SUPPORT_DTYPE_FD
                               MCM_DFORMAT_FD_STRING |
#endif
#if MCM_SUPPORT_DTYPE_FLD
                               MCM_DFORMAT_FLD_STRING |
#endif
                               0)
};

// SUBMIT_CONFIG 處理後要做哪種處理.
enum MCM_AFTER_COMPLETE
{
    // 不做任何事.
    MCM_ACOMPLETE_NONE   = 0,
    // 重新載入頁面.
    MCM_ACOMPLETE_RELOAD = 1
};

// 指令的控制碼 (編號).
enum MCM_OPERATE_PARAMETER_INDEX
{
    // 取得資料.
    MCM_CONFIG_GET_INDEX = 0,
    // 設定資料.
    MCM_CONFIG_SET_INDEX,
    // 增加資料.
    MCM_CONFIG_ADD_INDEX,
    // 刪除資料.
    MCM_CONFIG_DEL_INDEX,
    // 刪除 group 的全部 entry 資料.
    MCM_CONFIG_DEL_ALL_INDEX,
    // 執行模組.
    MCM_MODULE_RUN_INDEX
};
// 指令的控制碼 (對映的字串).
#define MCM_CONFIG_GET_KEY     "get"
#define MCM_CONFIG_SET_KEY     "set"
#define MCM_CONFIG_ADD_KEY     "add"
#define MCM_CONFIG_DEL_KEY     "del"
#define MCM_CONFIG_DEL_ALL_KEY "delall"
#define MCM_MODULE_RUN_KEY     "run"

// 填充哪種類型的路徑.
enum MCM_FILL_PATH_METHOD
{
    // mask 類型.
    MCM_FPATH_MASK = 0,
    // mix 類型.
    MCM_FPATH_MIX,
    // full 類型.
    MCM_FPATH_FULL
};




struct mcm_query_para_t
{
    char *data_name;
    char *data_value;
};
struct mcm_query_para_t mcm_query_para_info[] =
{
    {MCM_REQUEST_ACTION_KEY,     NULL},
    {MCM_SOCKET_PATH_KEY,        NULL},
    {MCM_SESSION_PERMISSION_KEY, NULL},
    {MCM_SESSION_STACK_SIZE_KEY, NULL},
    {MCM_DATA_FORMAT_KEY,        NULL},
    {MCM_AFTER_COMPLETE_KEY,     NULL},
    {NULL, NULL}
};

struct mcm_request_para_t
{
    MCM_DTYPE_LIST_TD request_action;
    char *socket_path;
    MCM_DTYPE_LIST_TD session_permission;
    MCM_DTYPE_USIZE_TD session_stack_size;
    MCM_DTYPE_FLAG_TD data_format;
    MCM_DTYPE_LIST_TD after_complete;
};

struct mcm_operate_type_map_t
{
    char *operate_key;
    MCM_DTYPE_LIST_TD operate_index;
};
struct mcm_operate_type_map_t mcm_operate_type_map_info[] =
{
    {MCM_CONFIG_GET_KEY,     MCM_CONFIG_GET_INDEX},
    {MCM_CONFIG_SET_KEY,     MCM_CONFIG_SET_INDEX},
    {MCM_CONFIG_ADD_KEY,     MCM_CONFIG_ADD_INDEX},
    {MCM_CONFIG_DEL_KEY,     MCM_CONFIG_DEL_INDEX},
    {MCM_CONFIG_DEL_ALL_KEY, MCM_CONFIG_DEL_ALL_INDEX},
    {MCM_MODULE_RUN_KEY,     MCM_MODULE_RUN_INDEX},
    {NULL, 0}
};

struct mcm_command_list_t
{
    // 指令的控制碼類型 (MCM_OPERATE_PARAMETER_INDEX).
    MCM_DTYPE_LIST_TD operate_type;
    // 指令內容.
    char *raw_content;

    // pull 類型的指令的路徑.
    char *pull_config_path;
    // pull 類型的指令的路徑的層數.
    MCM_DTYPE_USIZE_TD pull_config_level;
    // pull 類型的指令中每一層路徑的名稱.
    char **pull_config_part_name;
    // pull 類型的指令中每一層路徑的種類.
    MCM_DTYPE_LIST_TD *pull_config_part_type;
    // pull 類型的指令中每一層路徑的 key.
    MCM_DTYPE_EK_TD *pull_config_part_key;
    // pull 類型的指令的模組的路徑.
    char *pull_config_module_path;

    // push 類型的指令的路徑.
    char *push_config_path;
    // push 類型的 set 指令的資料內容.
    char *push_config_data_value;

    // module 類型的指令的路徑.
    char *module_path;
    // 是否是最後一個 run 指令.
    MCM_DTYPE_BOOL_TD module_last;
};

// 紀錄各種指令的類型的數目.
struct mcm_command_count_t
{
    // 全部.
    MCM_DTYPE_USIZE_TD total_count;
    // pull 類型 (get).
    MCM_DTYPE_USIZE_TD pull_count;
    // push 類型 (set, add, del, delall).
    MCM_DTYPE_USIZE_TD push_count;
    // module 類型 (run).
    MCM_DTYPE_USIZE_TD module_count;
};

struct mcm_model_t
{
    // model 的 group 的名稱.
    char *group_name;
    // model 的 group 的類型. (MCM_DTYPE_GS_INDEX, MCM_DTYPE_GD_INDEX).
    MCM_DTYPE_LIST_TD group_type;
    // model 的 group 的最大 entry 數目.
    MCM_DTYPE_EK_TD group_max;
    // model 的 group 的 member 名稱表.
    void *member_name_list;
    // model 的 group 的 member 名稱表的長度.
    MCM_DTYPE_USIZE_TD member_name_len;
    // model 的 group 的 member 的資料形態表.
    void *member_type_list;
    // model 的 group 的 member 的資料形態表的長度.
    MCM_DTYPE_USIZE_TD member_type_len;
    // 指向第一個 child model.
    struct mcm_model_t *child_model;
    // 指向下一個 model.
    struct mcm_model_t *next_model;
};

struct mcm_store_t
{
    // store 使用的 model.
    struct mcm_model_t *link_model;
    // 對於沒有任何 store 的 entry 會建立一個空的 sotre.
    // 紀錄此 store 是否為空的 store.
    MCM_DTYPE_BOOL_TD empty_store;
    // store 的 key.
    MCM_DTYPE_EK_TD entry_key;
    // store 的 member 的資料內容表.
    void *member_value_list;
    // store 的 member 的資料內容表的長度.
    MCM_DTYPE_USIZE_TD member_value_len;
    // 指向開頭的 child store.
    struct mcm_store_t *child_store_head;
    // 指向尾巴的 child store.
    struct mcm_store_t *child_store_tail;
    // 指向下一個 store.
    struct mcm_store_t *next_store;
    // 指向下一個使用不同 model 的 store.
    struct mcm_store_t *next_other_store;
};




// 轉換 JavaScript 的特殊字元.
char mcm_js_special_list[] = {'\'', '\"', '\\', '\0'};




// 分析 URL 帶的參數.
int mcm_parse_parameter(
    char **data_con,
    MCM_DTYPE_USIZE_TD *data_len,
    char **data_name_buf,
    char **data_value_buf)
{
    char *tmp_con;
    MCM_DTYPE_USIZE_TD tmp_len, didx1, didx2;
    MCM_DTYPE_BOOL_TD remain_data = 0;


    tmp_con = *data_con;
    tmp_len = *data_len;

    if(tmp_len <= 0)
        return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;

    for(didx1 = 0; didx1 < tmp_len; didx1++)
        if(tmp_con[didx1] == MCM_PARAMETER_SPLIT_KEY)
        {
            tmp_con[didx1] = '\0';
            remain_data = 1;
            break;
        }

    for(didx2 = 0; didx2 < didx1; didx2++)
        if(tmp_con[didx2] == MCM_VALUE_SPLIT_KEY)
        {
            tmp_con[didx2] = '\0';
            break;
        }

    *data_name_buf = didx2 < didx1 ? tmp_con : tmp_con + didx1;
    *data_value_buf = didx2 < didx1 ? tmp_con + didx2 + 1 : tmp_con;

    didx1 += remain_data != 0 ? 1 : 0;
    *data_con = tmp_con + didx1;
    *data_len = tmp_len - didx1;

    return MCM_RCODE_PASS;
}

// 取出必要的參數.
int mcm_parse_query(
    char *query_con,
    MCM_DTYPE_USIZE_TD query_len,
    struct mcm_request_para_t *request_info)
{
    struct mcm_query_para_t *each_query_para;
    char *tmp_name, *tmp_value;
    MCM_DTYPE_BOOL_TD find_para;


    MCM_CCDMSG("=> %s", __FUNCTION__);

    MCM_CCDMSG("query [" MCM_DTYPE_USIZE_PF "][%s]", query_len, query_con);

    while(1)
    {
        if(mcm_parse_parameter(&query_con, &query_len, &tmp_name, &tmp_value) < MCM_RCODE_PASS)
            break;
        MCM_CCDMSG("[%s][%s]", tmp_name, tmp_value);

        for(each_query_para = mcm_query_para_info; each_query_para->data_name != NULL;
            each_query_para++)
        {
            if(each_query_para->data_value == NULL)
                if(strcmp(each_query_para->data_name, tmp_name) == 0)
                {
                    each_query_para->data_value = tmp_value;
                    break;
                }
        }
    }

    find_para = 0;
    tmp_value = mcm_query_para_info[MCM_REQUEST_ACTION_INDEX].data_value;
    if(tmp_value != NULL)
        if(tmp_value[0] != '\0')
        {
            find_para = 1;
            request_info->request_action = MCM_DTYPE_LIST_SB(tmp_value, NULL, 10);
            MCM_CCDMSG("request_action [" MCM_DTYPE_LIST_PF "]", request_info->request_action);
        }
    if(find_para == 0)
    {
        MCM_CEMSG("invalid, unknown request_action [%s]", tmp_value);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                      "invalid, unknown request_action\\n[%s]", tmp_value);
        return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
    }

    find_para = 0;
    tmp_value = mcm_query_para_info[MCM_SOCKET_PATH_INDEX].data_value;
    if(tmp_value != NULL)
        if(tmp_value[0] != '\0')
        {
            find_para = 1;
            request_info->socket_path = tmp_value;
            MCM_CCDMSG("socket_path [%s]", request_info->socket_path);
        }
    if(find_para == 0)
    {
        MCM_CEMSG("invalid, empty socket_path [%s]", tmp_value);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                      "invalid, empty socket_path [%s]", tmp_value);
        return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
    }

    find_para = 0;
    tmp_value = mcm_query_para_info[MCM_SESSION_PERMISSION_INDEX].data_value;
    if(tmp_value != NULL)
        if(tmp_value[0] != '\0')
        {
            find_para = 1;
            request_info->session_permission = MCM_DTYPE_LIST_SB(tmp_value, NULL, 10);
            MCM_CCDMSG("session_permission [" MCM_DTYPE_LIST_PF "]",
                       request_info->session_permission);
        }
    if(find_para == 0)
    {
        MCM_CEMSG("invalid, empty session_permission [%s]", tmp_value);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                      "invalid, empty session_permission [%s]", tmp_value);
        return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
    }

    find_para = 0;
    tmp_value = mcm_query_para_info[MCM_SESSION_STACK_SIZE_INDEX].data_value;
    if(tmp_value != NULL)
        if(tmp_value[0] != '\0')
        {
            find_para = 1;
            request_info->session_stack_size = MCM_DTYPE_USIZE_SB(tmp_value, NULL, 10);
            MCM_CCDMSG("session_stack_size [" MCM_DTYPE_USIZE_PF "]",
                       request_info->session_stack_size);
        }
    if(find_para == 0)
    {
        MCM_CEMSG("invalid, empty session_stack_size [%s]", tmp_value);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                      "invalid, empty session_stack_size [%s]", tmp_value);
        return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
    }

    if(request_info->request_action == MCM_RACTION_OBTAIN_CONFIG)
    {
        find_para = 0;
        tmp_value = mcm_query_para_info[MCM_DATA_FORMAT_INDEX].data_value;
        if(tmp_value != NULL)
            if(tmp_value[0] != '\0')
            {
                find_para = 1;
                request_info->data_format = MCM_DTYPE_FLAG_SB(tmp_value, NULL, 10);
                MCM_CCDMSG("data_format [" MCM_DTYPE_FLAG_PF "]", request_info->data_format);
            }
        if(find_para == 0)
        {
            MCM_CEMSG("invalid, unknown data_format [%s]", tmp_value);
            MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                          "invalid, unknown data_format\\n[%s]", tmp_value);
            return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
        }
    }
    else
    if(request_info->request_action == MCM_RACTION_SUBMIT_CONFIG)
    {
        find_para = 0;
        tmp_value = mcm_query_para_info[MCM_AFTER_COMPLETE_INDEX].data_value;
        if(tmp_value != NULL)
            if(tmp_value[0] != '\0')
            {
                find_para = 1;
                request_info->after_complete = MCM_DTYPE_LIST_SB(tmp_value, NULL, 10);
                MCM_CCDMSG("after_complete [" MCM_DTYPE_LIST_PF "]", request_info->after_complete);
            }
        if(find_para == 0)
        {
            MCM_CEMSG("invalid, unknown after_complete [%s]", tmp_value);
            MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                          "invalid, unknown after_complete\\n[%s]", tmp_value);
            return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
        }
    }

    return MCM_RCODE_PASS;
}

// 將每條指令分離並分類.
int mcm_split_command(
    char *post_con,
    MCM_DTYPE_USIZE_TD post_len,
    struct mcm_command_count_t *command_count_info,
    struct mcm_command_list_t **command_list_buf)
{
    int fret = MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
    struct mcm_command_list_t *tmp_command_list, *each_command;
    MCM_DTYPE_USIZE_TD ridx, rloc, tidx, oidx, cmd_cnt = 0;
    char *tmp_raw;


    MCM_CCDMSG("=> %s", __FUNCTION__);

    // 找到 "&", 分離每條指令.
    for(ridx = rloc = 0; ridx <= post_len; ridx++)
        if((post_con[ridx] == MCM_PARAMETER_SPLIT_KEY) || (post_con[ridx] == '\0'))
        {
            post_con[ridx] = '\0';

            if((ridx - rloc) > 0)
            {
                MCM_CCDMSG("find cmd[" MCM_DTYPE_USIZE_PF "][%s]", cmd_cnt, post_con + rloc);
                // 紀錄總共有幾條指令.
                cmd_cnt++;
            }

            rloc = ridx + 1;
        }

    tmp_command_list = (struct mcm_command_list_t *)
        calloc(cmd_cnt, sizeof(struct mcm_command_list_t));
    if(tmp_command_list == NULL)
    {
        MCM_CEMSG("call malloc() fail [%s]", strerror(errno));
        MCM_CGI_AEMSG(fret, 0, "call malloc() fail\\n[%s]", strerror(errno));
        goto FREE_01;
    }
    MCM_CCDMSG("alloc command_list[" MCM_DTYPE_USIZE_PF "][%p]", cmd_cnt, tmp_command_list);

    // 記錄每條指令的內容.
    for(ridx = rloc = tidx = 0; ridx <= post_len; ridx++)
        if(post_con[ridx] == '\0')
        {
            // 紀錄指令的開頭位址.
            if(post_con[rloc] != '\0')
            {
                tmp_command_list[tidx].raw_content = post_con + rloc;
                tidx++;
            }

            rloc = ridx + 1;
        }

    // 分析每條指令.
    for(ridx = 0; ridx < cmd_cnt; ridx++)
    {
        each_command = tmp_command_list + ridx;
        tmp_raw = each_command->raw_content;

        // 找到指令中第一個 ".", 分離指令的控制碼.
        for(tidx = 0; tmp_raw[tidx] != '\0'; tidx++)
            if(tmp_raw[tidx] == MCM_SPROFILE_PATH_SPLIT_KEY)
                break;
        if(tmp_raw[tidx] == '\0')
        {
            MCM_CEMSG("invalid, empty operate_type [%s]", tmp_raw);
            MCM_CGI_AEMSG(fret, 0, "invalid, empty operate_type\\n[%s]", tmp_raw);
            goto FREE_02;
        }
        tmp_raw[tidx] = '\0';

        // 比對是哪種控制碼.
        for(oidx = 0; mcm_operate_type_map_info[oidx].operate_key != NULL; oidx++)
            if(strcmp(tmp_raw, mcm_operate_type_map_info[oidx].operate_key) == 0)
                break;
        if(mcm_operate_type_map_info[oidx].operate_key == NULL)
        {
            MCM_CEMSG("invalid, unknown operate_type [%s]", tmp_raw);
            MCM_CGI_AEMSG(fret, 0, "invalid, unknown operate_type\\n[%s]", tmp_raw);
            goto FREE_02;
        }

        // 紀錄控制碼編號.
        each_command->operate_type = mcm_operate_type_map_info[oidx].operate_index;
        // 指令的內容往後移動, 跳過控碼部分.
        each_command->raw_content += tidx + 1;

        // 紀錄各種指令數目.
        command_count_info->total_count++;
        switch(each_command->operate_type)
        {
            case MCM_CONFIG_GET_INDEX:
                command_count_info->pull_count++;
                break;
            case MCM_CONFIG_SET_INDEX:
            case MCM_CONFIG_ADD_INDEX:
            case MCM_CONFIG_DEL_INDEX:
            case MCM_CONFIG_DEL_ALL_INDEX:
                command_count_info->push_count++;
                break;
            case MCM_MODULE_RUN_INDEX:
                command_count_info->module_count++;
                break;
        }
    }

    *command_list_buf = tmp_command_list;

    fret = MCM_RCODE_PASS;
FREE_02:
    if(fret < MCM_RCODE_PASS)
        free(tmp_command_list);
FREE_01:
    return fret;
}

// 檢查需求和指令類型是否批配.
int mcm_check_action(
    struct mcm_request_para_t *request_info,
    struct mcm_command_count_t *command_count_info)
{
    MCM_CCDMSG("=> %s", __FUNCTION__);

    // OBTAIN_MAX_COUNT 的限制 :
    // 要有 pull 類型的指令.
    // 不可以有 push 或 module 類型的指令.
    if(request_info->request_action == MCM_RACTION_OBTAIN_MAX_COUNT)
    {
        if(command_count_info->pull_count == 0)
        {
            MCM_CEMSG("invalid, [obtain_max_count] not find any [%s] command", MCM_CONFIG_GET_KEY);
            MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                          "invalid, [obtain_max_count] not find any [%s] command",
                          MCM_CONFIG_GET_KEY);
            return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
        }
        if((command_count_info->push_count > 0) || (command_count_info->module_count > 0))
        {
            MCM_CEMSG("invalid, [obtain_max_count] only allow [%s] command", MCM_CONFIG_GET_KEY);
            MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                          "invalid, [obtain_max_count] only allow [%s] command",
                          MCM_CONFIG_GET_KEY);
            return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
        }
    }
    else
    // OBTAIN_CONFIG 的限制 :
    // 要有 pull 類型指令.
    if(request_info->request_action == MCM_RACTION_OBTAIN_CONFIG)
    {
        if(command_count_info->pull_count == 0)
        {
            MCM_CEMSG("invalid, [obtain_config] not find any [%s] command", MCM_CONFIG_GET_KEY);
            MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                          "invalid, [obtain_config] not find any [%s] command", MCM_CONFIG_GET_KEY);
            return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
        }
    }
    else
    // SUBMIT_CONFIG 的限制 :
    // 要有 push 或 module 類型的指令.
    // 不可以有 pull 類型的指令.
    if(request_info->request_action == MCM_RACTION_SUBMIT_CONFIG)
    {
        if((command_count_info->push_count == 0) && (command_count_info->module_count == 0))
        {
            MCM_CEMSG("invalid, [submit_config] not find any [%s/%s/%s/%s/%s] command",
                      MCM_CONFIG_SET_KEY, MCM_CONFIG_ADD_KEY, MCM_CONFIG_DEL_KEY,
                      MCM_CONFIG_DEL_ALL_KEY, MCM_MODULE_RUN_KEY);
            MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                          "invalid, [submit_config] not find any [%s/%s/%s/%s/%s] command",
                          MCM_CONFIG_SET_KEY, MCM_CONFIG_ADD_KEY, MCM_CONFIG_DEL_KEY,
                          MCM_CONFIG_DEL_ALL_KEY, MCM_MODULE_RUN_KEY);
            return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
        }
        if(command_count_info->pull_count > 0)
        {
            MCM_CEMSG("invalid, [submit_config] only allow [%s/%s/%s/%s/%s] command",
                      MCM_CONFIG_SET_KEY, MCM_CONFIG_ADD_KEY, MCM_CONFIG_DEL_KEY,
                      MCM_CONFIG_DEL_ALL_KEY, MCM_MODULE_RUN_KEY);
            MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                          "invalid, [submit_config] only allow [%s/%s/%s/%s/%s] command",
                          MCM_CONFIG_SET_KEY, MCM_CONFIG_ADD_KEY, MCM_CONFIG_DEL_KEY,
                          MCM_CONFIG_DEL_ALL_KEY, MCM_MODULE_RUN_KEY);
            return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
        }
    }

    return MCM_RCODE_PASS;
}

// 找出指令的路徑部分.
int mcm_find_config_path(
    struct mcm_command_list_t *this_command,
    MCM_DTYPE_BOOL_TD *extra_flag_buf)
{
    char *tmp_raw;
    MCM_DTYPE_USIZE_TD sidx;


    MCM_CCDMSG("=> %s", __FUNCTION__);

    tmp_raw = this_command->raw_content;

    // 找到 "=", 分離路徑.
    for(sidx = 0; tmp_raw[sidx] != '\0'; sidx++)
        if(tmp_raw[sidx] == MCM_VALUE_SPLIT_KEY)
            break;
    if(tmp_raw[sidx] == '\0')
        if(sidx == 0)
        {
            MCM_CEMSG("invalid, empty config_path [%s]", tmp_raw);
            MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                          "invalid, empty config_path\\n[%s]", tmp_raw);
            return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
        }
    if(tmp_raw[sidx] == MCM_VALUE_SPLIT_KEY)
    {
        if(sidx == 0)
        {
            MCM_CEMSG("invalid, empty config_path [%s]", tmp_raw);
            MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                          "invalid, empty config_path\\n[%s]", tmp_raw);
            return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
        }
        *extra_flag_buf = 1;
    }

    // 紀錄路徑.
    if(this_command->operate_type == MCM_CONFIG_GET_INDEX)
        this_command->pull_config_path = tmp_raw;
    else
        this_command->push_config_path = tmp_raw;

    // 將指令的內容往後移動, 跳過路徑部分.
    this_command->raw_content += sidx + (tmp_raw[sidx] == '\0' ? 0 : 1);

    tmp_raw[sidx] = '\0';

    return MCM_RCODE_PASS;
}

// 找出指令的路徑的後面部分 (pull 類型的指令的模組, push 類型的 set / add 指令的資料內容)
void mcm_find_config_extra(
    struct mcm_command_list_t *this_command)
{
    MCM_CCDMSG("=> %s", __FUNCTION__);

    if(this_command->operate_type == MCM_CONFIG_GET_INDEX)
    {
        if(this_command->raw_content[0] != '\0')
            this_command->pull_config_module_path = this_command->raw_content;
    }
    else
    {
        this_command->push_config_data_value = this_command->raw_content;
    }
}

// 分析 pull 類型的指令.
int mcm_analysis_pull_command(
    struct mcm_command_list_t *this_command)
{
    int fret;
    MCM_DTYPE_BOOL_TD have_extra = 0;


    MCM_CCDMSG("=> %s", __FUNCTION__);

    // 找出指令的路徑部分.
    fret = mcm_find_config_path(this_command, &have_extra);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CECTMSG("call mcm_find_config_path() fail");
        return fret;
    }

    // 找出指令的模組部分.
    if(have_extra != 0)
        mcm_find_config_extra(this_command);

    return MCM_RCODE_PASS;
}

// 檢查 pull 類型的指令的路徑是否重複.
int mcm_check_pull_command_duplic(
    struct mcm_command_list_t *command_list_info,
    struct mcm_command_count_t *command_count_info)
{
    struct mcm_command_list_t *each_command1, *each_command2;
    MCM_DTYPE_USIZE_TD cidx1, cidx2;


    MCM_CCDMSG("=> %s", __FUNCTION__);

    for(cidx1 = 0; cidx1 < command_count_info->total_count; cidx1++)
    {
        each_command1 = command_list_info + cidx1;
        if(each_command1->operate_type == MCM_CONFIG_GET_INDEX)
            for(cidx2 = cidx1 + 1; cidx2 < command_count_info->total_count; cidx2++)
            {
                each_command2 = command_list_info + cidx2;
                if(each_command2->operate_type == MCM_CONFIG_GET_INDEX)
                    if(strcmp(each_command1->pull_config_path,
                              each_command2->pull_config_path) == 0)
                    {
                        MCM_CEMSG("invalid, duplic command [%s]", each_command1->pull_config_path);
                        MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                                      "invalid, duplic command\\n[%s]",
                                      each_command1->pull_config_path);
                        return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
                    }
            }
    }

    return MCM_RCODE_PASS;
}

// 檢查 pull 類型的指令的路徑是否有效.
int mcm_check_pull_command_valid(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_list_t *command_list_info,
    struct mcm_command_count_t *command_count_info)
{
    int fret;
    struct mcm_command_list_t *each_command;
    MCM_DTYPE_USIZE_TD cidx;


    MCM_CCDMSG("=> %s", __FUNCTION__);

    // 檢查指令的路徑是否正確.
    for(cidx = 0; cidx < command_count_info->total_count; cidx++)
    {
        each_command = command_list_info + cidx;
        if(each_command->operate_type == MCM_CONFIG_GET_INDEX)
        {
            fret = mcm_lulib_check_mask_path(this_lulib, each_command->pull_config_path);
            if(fret < MCM_RCODE_PASS)
            {
                MCM_CEMSG("call mcm_lulib_check_mask_path(%s) fail",
                          each_command->pull_config_path);
                MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                              "call mcm_lulib_check_mask_path() fail\\n[%s]",
                              each_command->pull_config_path);
                return fret;
            }
        }
    }

    return MCM_RCODE_PASS;
}

// 分析 push 類型的指令.
int mcm_analysis_push_command(
    struct mcm_command_list_t *this_command)
{
    int fret;
    MCM_DTYPE_BOOL_TD have_extra = 0;


    MCM_CCDMSG("=> %s", __FUNCTION__);

    // 找出指令的路徑部分.
    fret = mcm_find_config_path(this_command, &have_extra);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CECTMSG("call mcm_find_config_path() fail");
        return fret;
    }

    // 找出指令的資料內容部分.
    if(have_extra != 0)
        if((this_command->operate_type == MCM_CONFIG_SET_INDEX) ||
           (this_command->operate_type == MCM_CONFIG_ADD_INDEX))
        {
            mcm_find_config_extra(this_command);
        }

    return MCM_RCODE_PASS;
}

// 分析 module 類型的指令.
int mcm_analysis_module_command(
    struct mcm_command_list_t *this_command)
{
    MCM_CCDMSG("=> %s", __FUNCTION__);

    // 紀錄路徑部分.
    this_command->module_path = this_command->raw_content;

    return MCM_RCODE_PASS;
}

// 標記最最後一個 module 類型的指令.
void mcm_mark_last_module_command(
    struct mcm_command_list_t *command_list_info,
    struct mcm_command_count_t *command_count_info)
{
    struct mcm_command_list_t *each_command;
    MCM_DTYPE_USIZE_TD cidx;


    MCM_CCDMSG("=> %s", __FUNCTION__);

    cidx = command_count_info->total_count - 1;
    do
    {
        each_command = command_list_info + cidx;
        if(each_command->operate_type == MCM_MODULE_RUN_INDEX)
        {
            each_command->module_last = 1;
            break;
        }
        cidx--;
    }
    while(cidx > 0);
}

// 釋放.
int mcm_destory_pull_command(
    struct mcm_command_list_t *command_list_info,
    struct mcm_command_count_t *command_count_info)
{
    struct mcm_command_list_t *each_command;
    MCM_DTYPE_USIZE_TD cidx;


    MCM_CCDMSG("=> %s", __FUNCTION__);

    for(cidx = 0; cidx < command_count_info->total_count; cidx++)
    {
        each_command = command_list_info + cidx;
        if(each_command->operate_type == MCM_CONFIG_GET_INDEX)
        {
            MCM_CCDMSG("free pull_config_part_name[%p]", each_command->pull_config_part_name);
            if(each_command->pull_config_part_name != NULL)
                free(each_command->pull_config_part_name);

            MCM_CCDMSG("free pull_config_part_type[%p]", each_command->pull_config_part_type);
            if(each_command->pull_config_part_type != NULL)
                free(each_command->pull_config_part_type);

            MCM_CCDMSG("free pull_config_part_key[%p]", each_command->pull_config_part_key);
            if(each_command->pull_config_part_key != NULL)
                free(each_command->pull_config_part_key);
        }
    }

    return MCM_RCODE_PASS;
}

// 分析 pull 類型的指令.
int mcm_create_pull_command(
    struct mcm_command_list_t *command_list_info,
    struct mcm_command_count_t *command_count_info)
{
    struct mcm_command_list_t *each_command;
    MCM_DTYPE_USIZE_TD cidx, pidx, ploc, scnt, path_len;
    char *path_con;


    MCM_CCDMSG("=> %s", __FUNCTION__);

    for(cidx = 0; cidx < command_count_info->total_count; cidx++)
    {
        each_command = command_list_info + cidx;
        if(each_command->operate_type == MCM_CONFIG_GET_INDEX)
        {
            path_con = each_command->pull_config_path;
            path_len = strlen(path_con);

            MCM_CCDMSG("cmd [%s][%s][%s]",
                       MCM_CONFIG_GET_KEY, path_con, each_command->pull_config_module_path);

            // 計算路徑的層數.
            // 範例 :
            // device = 1.
            // device.system = 2.
            // device.vap.* = 2.
            // device.vap.*.station.* = 3.
            for(ploc = pidx = scnt = 0; pidx <= path_len; pidx++)
                if((path_con[pidx] == MCM_SPROFILE_PATH_SPLIT_KEY) || (path_con[pidx] == '\0'))
                {
                    if(path_con[ploc] != MCM_SPROFILE_PATH_MASK_KEY)
                        scnt++;
                    ploc = pidx + 1;
                }

            each_command->pull_config_level = scnt;

            each_command->pull_config_part_name = (char **)
                calloc(each_command->pull_config_level, sizeof(char *));
            if(each_command->pull_config_part_name == NULL)
            {
                MCM_CEMSG("call calloc() fail [%s]", strerror(errno));
                MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                              "call calloc() fail\\n[%s]", strerror(errno));
                goto FREE_01;
            }
            MCM_CCDMSG("alloc pull_config_part_name[%p]", each_command->pull_config_part_name);

            each_command->pull_config_part_type = (MCM_DTYPE_LIST_TD *)
                calloc(each_command->pull_config_level, sizeof(MCM_DTYPE_LIST_TD));
            if(each_command->pull_config_part_type == NULL)
            {
                MCM_CEMSG("call calloc() fail [%s]", strerror(errno));
                MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                              "call calloc() fail\\n[%s]", strerror(errno));
                goto FREE_01;
            }
            MCM_CCDMSG("alloc pull_config_part_type[%p]", each_command->pull_config_part_type);

            each_command->pull_config_part_key = (MCM_DTYPE_EK_TD *)
                calloc(each_command->pull_config_level, sizeof(MCM_DTYPE_EK_TD));
            if(each_command->pull_config_part_key == NULL)
            {
                MCM_CEMSG("call calloc() fail [%s]", strerror(errno));
                MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                              "call calloc() fail\\n[%s]", strerror(errno));
                goto FREE_01;
            }
            MCM_CCDMSG("alloc pull_config_part_key[%p]", each_command->pull_config_part_key);

            // 處理每一段路徑, 紀錄每一層資料.
            for(ploc = pidx = scnt = 0; pidx <= path_len; pidx++)
                if((path_con[pidx] == MCM_SPROFILE_PATH_SPLIT_KEY) || (path_con[pidx] == '\0'))
                {
                    // 範例 :
                    // device.vap.*
                    if(path_con[ploc] != MCM_SPROFILE_PATH_MASK_KEY)
                    {
                        // 移除 ".".
                        path_con[pidx] = '\0';
                        // 紀錄每一層的名稱 (device, vap).
                        each_command->pull_config_part_name[scnt] = path_con + ploc;
                        // 先設定這一層是 gs 類型.
                        each_command->pull_config_part_type[scnt] = MCM_DTYPE_GS_INDEX;
                        // 往下一層紀錄區移動.
                        scnt++;
                    }
                    else
                    {
                        // 遇到 "*", 重新設定上一層是 gd 類型.
                        each_command->pull_config_part_type[scnt - 1] = MCM_DTYPE_GD_INDEX;
                    }
                    ploc = pidx + 1;
                }

#if MCM_CCDMODE
            for(pidx = 0; pidx < each_command->pull_config_level; pidx++)
            {
                MCM_CCDMSG("pull_config_part_name "
                           "[" MCM_DTYPE_USIZE_PF "/" MCM_DTYPE_USIZE_PF "][%s/%s]",
                           pidx + 1, each_command->pull_config_level,
                           each_command->pull_config_part_name[pidx],
                           each_command->pull_config_part_type[pidx] == MCM_DTYPE_GS_INDEX ?
                           MCM_DTYPE_GS_KEY : MCM_DTYPE_GD_KEY);
            }
#endif
        }
    }

   return MCM_RCODE_PASS;
FREE_01:
    mcm_destory_pull_command(command_list_info, command_count_info);
    return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
}

// 填充處理資料的路徑.
void mcm_fill_path(
    struct mcm_command_list_t *command_list_info,
    MCM_DTYPE_USIZE_TD part_level,
    MCM_DTYPE_LIST_TD fill_type,
    char *path_buf)
{
    MCM_DTYPE_USIZE_TD lidx, plen;
    MCM_DTYPE_BOOL_TD tflag;

    // 從第一層開始往後填充到指定的層.
    for(lidx = plen = 0; lidx <= part_level; lidx++)
    {
        // 填充名稱.
        plen += sprintf(path_buf + plen, "%s", command_list_info->pull_config_part_name[lidx]);

        // tflag == 0 : 有下一層, 不填充尾端的 ".".
        // tflag == 1 : 無下一層, 要填充尾端的 ".".
        tflag = 0;
        // 還沒到指定的層, 要填充尾端的 ".".
        if(lidx < part_level)
            tflag = 1;
        else
        // 已經到指定的層, 不填充尾端的 ".", 以下情況還是要填充尾端的 ".".
        // 對於 gd 類型的層, 如果路徑類型是 mask 或 mix 或 key 不為 0,
        // 需要在尾端加上 "*" (mask, mix) 或是 "#key" (full).
        if(command_list_info->pull_config_part_type[lidx] == MCM_DTYPE_GD_INDEX)
            if((fill_type == MCM_FPATH_MASK) ||
               (fill_type == MCM_FPATH_MIX) ||
               (command_list_info->pull_config_part_key[lidx] != 0))
            {
                tflag = 1;
            }
        // 填充 ".".
        if(tflag != 0)
        {
            path_buf[plen] = MCM_SPROFILE_PATH_SPLIT_KEY;
            plen++;
            path_buf[plen] = '\0';
        }

        // 對於 gd 類型的層, 可能需要填充 "*" (mask, mix) 或 "#key" (full).
        if(command_list_info->pull_config_part_type[lidx] == MCM_DTYPE_GD_INDEX)
        {
            // 路徑為 mask 類型.
            if(fill_type == MCM_FPATH_MASK)
            {
                // 填充 "*".
                path_buf[plen] = MCM_SPROFILE_PATH_MASK_KEY;
                plen++;
                // 還沒到最後一層需要填充尾端的 ".".
                if(lidx < part_level)
                {
                    path_buf[plen] = MCM_SPROFILE_PATH_SPLIT_KEY;
                    plen++;
                }
                path_buf[plen] = '\0';
            }
            else
            {
                // 路徑為 full 類型.
                if(command_list_info->pull_config_part_key[lidx] != 0)
                {
                    // 填充 "#key".
                    plen += sprintf(path_buf + plen, "%c" MCM_DTYPE_EK_PF,
                                    MCM_SPROFILE_PATH_KEY_KEY,
                                    command_list_info->pull_config_part_key[lidx]);
                    // 還沒到最後一層需要填充尾端的 ".".
                    if(lidx < part_level)
                    {
                        path_buf[plen] = MCM_SPROFILE_PATH_SPLIT_KEY;
                        plen++;
                        path_buf[plen] = '\0';
                    }
                }
                // 路徑為 mix 類型.
                else
                {
                    // mix 類型只有在最後一層才能填充 "*".
                    if(lidx == part_level)
                        if(fill_type == MCM_FPATH_MIX)
                        {
                            path_buf[plen] = MCM_SPROFILE_PATH_MASK_KEY;
                            plen++;
                            path_buf[plen] = '\0';
                        }
                }
            }
        }
    }
}

// 釋放.
int mcm_destory_model(
    struct mcm_model_t *this_model)
{
    MCM_CCDMSG("=> %s", __FUNCTION__);

    for(; this_model != NULL; this_model = this_model->next_model)
    {
        if(this_model->child_model != NULL)
            mcm_destory_model(this_model->child_model);

        MCM_CCDMSG("destory [%s]", this_model->group_name);

        MCM_CCDMSG("free member_name_list[%p]", this_model->member_name_list);
        if(this_model->member_name_list != NULL)
            free(this_model->member_name_list);

        MCM_CCDMSG("free member_type_list[%p]", this_model->member_type_list);
        if(this_model->member_type_list != NULL)
            free(this_model->member_type_list);

        MCM_CCDMSG("free model[%p]", this_model);
        free(this_model);
    }

    return MCM_RCODE_PASS;
}

// 建立 model tree.
int mcm_create_model(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_request_para_t *request_info,
    struct mcm_command_list_t *command_list_info,
    MCM_DTYPE_USIZE_TD part_level,
    char *cache_path,
    struct mcm_model_t *this_model,
    struct mcm_model_t *parent_model,
    struct mcm_model_t **new_model_buf)
{
    int fret = MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
    struct mcm_model_t *self_model = NULL, *tail_model = NULL;


    MCM_CCDMSG("=> %s", __FUNCTION__);

#if MCM_CCDMODE
    mcm_fill_path(command_list_info, command_list_info->pull_config_level - 1, MCM_FPATH_MASK,
                  cache_path);

    MCM_CCDMSG("create model[%s][%s][" MCM_DTYPE_USIZE_PF "/" MCM_DTYPE_USIZE_PF "]",
               cache_path, command_list_info->pull_config_part_name[part_level],
               part_level + 1, command_list_info->pull_config_level);
#endif

    // 檢查此層 model 是否已經存在.
    // 例如 :
    // 指令一 = device.system
    // 指令二 = device.vap.*
    // 處理指令二時 device 已經在處理指令一時建立.
    for(self_model = this_model; self_model != NULL; self_model = self_model->next_model)
        if(strcmp(self_model->group_name,
                  command_list_info->pull_config_part_name[part_level]) == 0)
        {
            break;
        }
    MCM_CCDMSG("search model[%s][%p]",
               command_list_info->pull_config_part_name[part_level], self_model);

    // 不存在, 建立.
    if(self_model == NULL)
    {
        // 找到串列上的最後一個 model.
        if((tail_model = this_model) != NULL)
            for(; tail_model->next_model != NULL; tail_model = tail_model->next_model);

        self_model = (struct mcm_model_t *) calloc(1, sizeof(struct mcm_model_t));
        if(self_model == NULL)
        {
            MCM_CEMSG("call calloc() fail [%s]", strerror(errno));
            MCM_CGI_AEMSG(fret, 0, "call calloc() fail\\n[%s]", strerror(errno));
            return fret;
        }
        MCM_CCDMSG("alloc model[%p]", self_model);

        // 設定 group_name.
        self_model->group_name = command_list_info->pull_config_part_name[part_level];
        MCM_CCDMSG("assign group_name[%s]", self_model->group_name);

        // 設定 group_type.
        self_model->group_type = command_list_info->pull_config_part_type[part_level];
        MCM_CCDMSG("assign group_type[%s]",
                   self_model->group_type == MCM_DTYPE_GS_INDEX ?
                   MCM_DTYPE_GS_KEY : MCM_DTYPE_GD_KEY);

        if(parent_model != NULL)
            if(parent_model->child_model == NULL)
            {
                parent_model->child_model = self_model;
                MCM_CCDMSG("link parent->child[%s ->]", parent_model->group_name);
            }

        if(tail_model != NULL)
        {
            tail_model->next_model = self_model;
            MCM_CCDMSG("link neighbor[%s ->]", tail_model->group_name);
        }

        // 設定 root model.
        if(new_model_buf != NULL)
            *new_model_buf = self_model;
    }

    if((part_level + 1) < command_list_info->pull_config_level)
    {
        // 處理下一層 model.
        fret = mcm_create_model(this_lulib, request_info, command_list_info, part_level + 1,
                                cache_path, self_model->child_model, self_model, NULL);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_CECTMSG("call mcm_create_model() fail");
            return fret;
        }
    }
    else
    {
        // 填充處理資料的路徑.
        // 例如 :
        // 第一層 = device.
        // 第二層 = vap.
        // 第三層 = station.
        // 組合成 device.vap.*.station.*
        mcm_fill_path(command_list_info, command_list_info->pull_config_level - 1, MCM_FPATH_MASK,
                      cache_path);

        if(request_info->request_action == MCM_RACTION_OBTAIN_MAX_COUNT)
        {
            // 取得 group_max.
            fret = mcm_lulib_get_max_count(this_lulib, cache_path, &self_model->group_max);
            if(fret < MCM_RCODE_PASS)
            {
                MCM_CEMSG("call mcm_lulib_get_max_count(%s) fail", cache_path);
                MCM_CGI_AEMSG(fret, 0, "call mcm_lulib_get_max_count() fail\\n[%s]", cache_path);
                return fret;
            }
        }
        else
        if(request_info->request_action == MCM_RACTION_OBTAIN_CONFIG)
        {
            // 取得 group 的 member 的名稱列表.
            fret = mcm_lulib_get_list_name(this_lulib, cache_path, &self_model->member_name_list,
                                           &self_model->member_name_len);
            if(fret < MCM_RCODE_PASS)
            {
                MCM_CEMSG("call mcm_lulib_get_list_name(%s) fail", cache_path);
                MCM_CGI_AEMSG(fret, 0, "call mcm_lulib_get_list_name() fail\\n[%s]", cache_path);
                return fret;
            }
            MCM_CCDMSG("alloc entry_info_name[" MCM_DTYPE_USIZE_PF "][%p]",
                       self_model->member_name_len, self_model->member_name_list);

            // 取得 group 的 menber 的資料類型列表.
            fret = mcm_lulib_get_list_type(this_lulib, cache_path, &self_model->member_type_list,
                                           &self_model->member_type_len);
            if(fret < MCM_RCODE_PASS)
            {
                MCM_CEMSG("call mcm_lulib_get_list_type(%s) fail", cache_path);
                MCM_CGI_AEMSG(fret, 0, "call mcm_lulib_get_list_type() fail\\n[%s]", cache_path);
                return fret;
            }
            MCM_CCDMSG("alloc entry_info_type[" MCM_DTYPE_USIZE_PF "][%p]",
                       self_model->member_type_len, self_model->member_type_list);
        }
    }

    return fret;
}

// 釋放.
int mcm_destory_store(
    struct mcm_store_t *this_store)
{
    MCM_CCDMSG("=> %s", __FUNCTION__);

    for(; this_store != NULL; this_store = this_store->next_store)
    {
        if(this_store->child_store_head != NULL)
            mcm_destory_store(this_store->child_store_head);

        MCM_CCDMSG("[%s.%c" MCM_DTYPE_EK_PF "] destory",
                   this_store->link_model->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                   this_store->entry_key);

        MCM_CCDMSG("[%s.%c" MCM_DTYPE_EK_PF "] free member_value_list[%p]",
                   this_store->link_model->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                   this_store->entry_key, this_store->member_value_list);
        if(this_store->member_value_list != NULL)
            free(this_store->member_value_list);

        MCM_CCDMSG("[%s.%c" MCM_DTYPE_EK_PF "] free store[%p]",
                   this_store->link_model->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                   this_store->entry_key, this_store);
        free(this_store);
    }

    return MCM_RCODE_PASS;
}

// 建立 store tree.
int mcm_create_store(
    void *this_module_fp,
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_list_t *this_command,
    MCM_DTYPE_USIZE_TD part_level,
    char *cache_path,
    struct mcm_model_t *this_model,
    struct mcm_store_t *this_store,
    struct mcm_store_t *parent_store,
    struct mcm_store_t **new_store_buf)
{
    int fret = MCM_RCODE_PASS;
    struct mcm_model_t *self_model;
    struct mcm_store_t *self_store = NULL, *prev_other_store = NULL, *tail_store = NULL,
        *head_store = NULL;
    MCM_DTYPE_EK_TD kidx = 0, key_count = 0, *key_list = NULL;
    char *dl_err;
    int (*module_cb)(struct mcm_lulib_lib_t *this_lulib,
                     MCM_DTYPE_USIZE_TD part_level,
                     MCM_DTYPE_EK_TD *part_key,
                     MCM_DTYPE_EK_TD **key_list_buf,
                     MCM_DTYPE_EK_TD *key_count_buf);


    MCM_CCDMSG("=> %s", __FUNCTION__);

    // 進入遞迴處理時, 清除上一次指令處理的內容.
    this_command->pull_config_part_key[part_level] = 0;

#if MCM_CCDMODE
    mcm_fill_path(this_command, this_command->pull_config_level - 1, MCM_FPATH_MIX, cache_path);

    MCM_CCDMSG("create store[%s][%s][" MCM_DTYPE_USIZE_PF "/" MCM_DTYPE_USIZE_PF "]",
               cache_path, this_command->pull_config_part_name[part_level],
               part_level + 1, this_command->pull_config_level);
#endif

    // 尋找使用的 model.
    for(self_model = this_model; self_model != NULL; self_model = self_model->next_model)
        if(strcmp(self_model->group_name, this_command->pull_config_part_name[part_level]) == 0)
            break;
    MCM_CCDMSG("use model[%s]", self_model->group_name);

    // 檢查此 store 是否已存在.
    // 例如 :
    // 指令一 = device.vap.*
    // 指令二 = device.vap.*.station.*
    // 處理指令二時, vap 在指令一時已經處理過.
    //
    // 找到使用相同 model 的 store, next_other_store 會鍊結到下一個使用不同 model 的 store.
    // 例如 store tree 如下 :
    // device
    //  + system
    //  + vap.#1
    //  |  + extern
    //  |  + station.#1
    //  + vap.#2
    //  |  + extern
    //  + limit.#1
    //  + limit.#2
    // system 的 next_other_store 會是 vap.#1,
    // vap.#1 的 next_other_store 會是 limit.#1,
    // limit.#1 的 next_other_store 會是 NULL.
    for(self_store = this_store; self_store != NULL; self_store = self_store->next_other_store)
    {
        prev_other_store = self_store;
        if(self_store->link_model == self_model)
            break;
    }
    MCM_CCDMSG("search store[%s][%p]",
               this_command->pull_config_part_name[part_level], self_store);

    // 不存在, 建立.
    if(self_store == NULL)
    {
        // 取得該層的 entry 數目和所有的 key.
        if(this_command->pull_config_part_type[part_level] == MCM_DTYPE_GS_INDEX)
        {
            // gs 類型固定只有 1 個 entry.
            key_count = 1;
            key_list = calloc(key_count, sizeof(MCM_DTYPE_EK_TD));
            if(key_list == NULL)
            {
                fret = MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
                MCM_CEMSG("call calloc() fail [%s]", strerror(errno));
                MCM_CGI_AEMSG(fret, 0, "call calloc() fail\\n[%s]", strerror(errno));
                goto FREE_01;
            }
        }
        else
        {
            if(this_command->pull_config_module_path != NULL)
            {
                // 執行模組取得 key 列表.
                module_cb = dlsym(this_module_fp, this_command->pull_config_module_path);
                dl_err = dlerror();
                if(dl_err != NULL)
                {
                    fret = MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
                    MCM_CEMSG("call dlsym(%s) fail [%s]",
                              this_command->pull_config_module_path, dl_err);
                    MCM_CGI_AEMSG(fret, 0, "call dlsym() fail\\n[%s]\\n[%s]",
                                  this_command->pull_config_module_path, dl_err);
                    goto FREE_01;
                }

                fret = module_cb(this_lulib, part_level + 1, this_command->pull_config_part_key,
                                 &key_list, &key_count);
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_CEMSG("call module(%s) fail",
                              this_command->pull_config_module_path);
                    MCM_CGI_AEMSG(fret, 0, "call module() fail\\n[%s]",
                                  this_command->pull_config_module_path);
                    goto FREE_01;
                }
            }
            else
            {
                // 填充 mix 類型的路徑用來取得 key 列表.
                // 其他層的 pull_config_part_key 會在其他層的 mcm_create_store() 遞迴呼叫中填入.
                mcm_fill_path(this_command, part_level, MCM_FPATH_MIX, cache_path);

                // 取得 key 列表.
                fret = mcm_lulib_get_all_key(this_lulib, cache_path,
                                             (MCM_DTYPE_EK_TD **) &key_list, &key_count);
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_CEMSG("call mcm_lulib_get_all_key(%s) fail", cache_path);
                    MCM_CGI_AEMSG(fret, 0, "call mcm_lulib_get_all_key() fail\\n[%s]", cache_path);
                    goto FREE_01;
                }
            }
        }
        MCM_CCDMSG("key_list[" MCM_DTYPE_EK_PF "][%p] [%s]",
                   key_count, key_list, 
                   this_command->pull_config_part_type[part_level] == MCM_DTYPE_GS_INDEX ?
                   MCM_DTYPE_GS_KEY : MCM_DTYPE_GD_KEY);

        // 每個 entry 建立一個 store, 沒有任何 entry 的話會建立一個空的 store.
        do
        {
            self_store = (struct mcm_store_t *) calloc(1, sizeof(struct mcm_store_t));
            if(self_store == NULL)
            {
                fret = MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
                MCM_CEMSG("call calloc() fail [%s]", strerror(errno));
                MCM_CGI_AEMSG(fret, 0, "call calloc() fail\\n[%s]", strerror(errno));
                goto FREE_02;
            }
            MCM_CCDMSG("[%s] [%s.%c" MCM_DTYPE_EK_PF "] alloc store[%p]",
                       key_count == 0 ? "empty" : "normal",
                       self_model->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                       key_count == 0 ? 0 : key_list[kidx], self_store);

            // 鍊結使用的 model.
            self_store->link_model = self_model;
            MCM_CCDMSG("[%s.%c" MCM_DTYPE_EK_PF "] link model[%p]",
                       self_model->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                       self_store->entry_key, self_model);

            if(key_count == 0)
                self_store->empty_store = 1;
            else
                self_store->entry_key = key_list[kidx];

            if(tail_store == NULL)
            {
                // 串列的開頭.
                head_store = self_store;
                MCM_CCDMSG("[%s.%c" MCM_DTYPE_EK_PF "] head",
                           self_model->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                           self_store->entry_key);

                if(parent_store != NULL)
                {
                    // parent 還沒有 child 串列, 設定此串列為 parent 的 child 開頭.
                    if(parent_store->child_store_head == NULL)
                    {
                        parent_store->child_store_head = head_store;
                        MCM_CCDMSG("[%s.%c" MCM_DTYPE_EK_PF "] link parent->child_head"
                                   "[%s.%c" MCM_DTYPE_EK_PF "]",
                                   parent_store->link_model->group_name,
                                   MCM_SPROFILE_PATH_KEY_KEY, parent_store->entry_key,
                                   self_model->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                                   head_store->entry_key);
                    }
                    // parent 已經有其他 child 串列,
                    // 鍊結 parent 的 child 串列的下一個串列為此串列.
                    else
                    {
                        parent_store->child_store_tail->next_store = head_store;
                        MCM_CCDMSG("[%s.%c" MCM_DTYPE_EK_PF "] link parent->child neighbor"
                                   "[%s.%c" MCM_DTYPE_EK_PF "->]",
                                   self_model->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                                   head_store->entry_key,
                                   parent_store->child_store_tail->link_model->group_name,
                                   MCM_SPROFILE_PATH_KEY_KEY,
                                   parent_store->child_store_tail->entry_key);
                    }
                }

                // 設定 root store.
                if(new_store_buf != NULL)
                    *new_store_buf = head_store;
            }
            else
            {
                // 串列的前一個 store 鍊結到目前的 store.
                tail_store->next_store = self_store;
                MCM_CCDMSG("[%s.%c" MCM_DTYPE_EK_PF "] "
                           "link neighbor[%s.%c" MCM_DTYPE_EK_PF "->]",
                           self_model->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                           self_store->entry_key, self_model->group_name,
                           MCM_SPROFILE_PATH_KEY_KEY, tail_store->entry_key);
            }
            tail_store = self_store;

            kidx++;
        }
        while(kidx < key_count);

        MCM_CCDMSG("[%s.%c" MCM_DTYPE_EK_PF "] tail",
                   self_model->group_name, MCM_SPROFILE_PATH_KEY_KEY, self_store->entry_key);

        // 重新設定 parent 的 child 節尾.
        if(parent_store != NULL)
        {
            parent_store->child_store_tail = tail_store;
            MCM_CCDMSG("[%s.%c" MCM_DTYPE_EK_PF "] link parent->child_tail"
                       "[%s.%c" MCM_DTYPE_EK_PF "]",
                       parent_store->link_model->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                       parent_store->entry_key, self_model->group_name,
                       MCM_SPROFILE_PATH_KEY_KEY, tail_store->entry_key);
        }

        // 鍊結前一個使用其他 model 的 store 到此串列.
        if(prev_other_store != NULL)
        {
            prev_other_store->next_other_store = head_store;
            MCM_CCDMSG("[%s.%c" MCM_DTYPE_EK_PF "] link other neighbor"
                       "[%s.%c" MCM_DTYPE_EK_PF "->]",
                       self_model->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                       head_store->entry_key, prev_other_store->link_model->group_name,
                       MCM_SPROFILE_PATH_KEY_KEY, prev_other_store->entry_key);
        }

        self_store = head_store;
    }

    // 填充每個 store 需要的資料.
    if(self_store->empty_store == 0)
        for(; (self_store != NULL) && (self_store->link_model == self_model);
            self_store = self_store->next_store)
        {
            MCM_CCDMSG("[%s.%c" MCM_DTYPE_EK_PF "] do",
                       self_model->group_name, MCM_SPROFILE_PATH_KEY_KEY, self_store->entry_key);

            this_command->pull_config_part_key[part_level] = self_store->entry_key;

            if((part_level + 1) < this_command->pull_config_level)
            {
                // 處理下一層 store.
                fret = mcm_create_store(this_module_fp, this_lulib, this_command, part_level + 1,
                                        cache_path, self_model->child_model,
                                        self_store->child_store_head, self_store, NULL);
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_CECTMSG("call mcm_create_store() fail");
                    goto FREE_02;
                }
            }
            else
            {
                // 填充 full 類型的路徑用來取得資料內容
                mcm_fill_path(this_command, part_level, MCM_FPATH_FULL, cache_path);

                MCM_CCDMSG("[%s.%c" MCM_DTYPE_EK_PF "] entry_info_value[%s]",
                           self_store->link_model->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                           self_store->entry_key, cache_path);

                // 取出每個 entry 的資料內容.
                fret = mcm_lulib_get_list_value(this_lulib, cache_path,
                                                &self_store->member_value_list,
                                                &self_store->member_value_len);
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_CEMSG("call mcm_lulib_get_list_value(%s) fail", cache_path);
                    MCM_CGI_AEMSG(fret, 0, "call mcm_lulib_get_list_value() fail\\n[%s]",
                                  cache_path);
                    goto FREE_02;
                }
                MCM_CCDMSG("[%s.%c" MCM_DTYPE_EK_PF "] alloc entry_info_value"
                           "[" MCM_DTYPE_USIZE_PF "][%p]",
                           self_store->link_model->group_name, MCM_SPROFILE_PATH_KEY_KEY,
                           self_store->entry_key, self_store->member_value_len,
                           self_store->member_value_list);
            }
        }

FREE_02:
    if(key_list != NULL)
        free(key_list);
FREE_01:
    return fret;
}

// utf8 轉為 unicode.
int mcm_convert_utf8_to_unicode(
    char *data_con,
    MCM_DTYPE_USIZE_TD *unicode_buf,
    MCM_DTYPE_USIZE_TD *utf8_len_buf)
{
    unsigned char tmp_ch, tmp_mask;
    unsigned char check_mask[6] = {0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
    unsigned char value_mask[6] = {0x7F, 0x1F, 0x0F, 0x07, 0x03, 0x01};
    MCM_DTYPE_USIZE_TD code_len[6] = {1, 2, 3, 4, 5, 6};
    MCM_DTYPE_USIZE_TD code_offset[6] = {0, 6, 12, 18, 24, 30};
    MCM_DTYPE_USIZE_TD tmp_code = 0, i = 0, j;


    tmp_ch = (*data_con) & 0xFF;
    if((tmp_ch & (~value_mask[0])) == check_mask[0])
    {
        tmp_code = tmp_ch;
    }
    else
    {
        for(i = 1; i < 6; i++)
            if((tmp_ch & (~value_mask[i])) == check_mask[i])
                break;
        if(i == 6)
        {
            *unicode_buf = tmp_ch;
            *utf8_len_buf = 1;
            return MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
        }

        for(j = 0; j < code_len[i]; j++)
        {
            tmp_mask = j == 0 ? value_mask[i] : 0x3F;
            tmp_code |= ((data_con[j] & 0xFF) & tmp_mask) << code_offset[code_len[i] - (j + 1)];
        }
    }

    *unicode_buf = tmp_code;
    *utf8_len_buf = code_len[i];

    return MCM_RCODE_PASS;
}

// 輸出 max count 的 json 資料.
void mcm_output_max_count_json(
    struct mcm_model_t *this_model,
    MCM_DTYPE_BOOL_TD internal_flag)
{
    // == 0 : 表示在最外層.
    if(internal_flag == 0)
    {
        mcm_cgi_fill_response_header(1, 1, MCM_RCODE_PASS);
        printf("{");
    }

    for(; this_model != NULL; this_model = this_model->next_model)
    {
        printf("\"%s\":", this_model->group_name);

        printf("{");

        printf("\"%s\":" MCM_DTYPE_EK_PF, MCM_MAX_COUNT_KEY, this_model->group_max);
        if(this_model->child_model != NULL)
            printf(",");

        if(this_model->child_model != NULL)
            mcm_output_max_count_json(this_model->child_model, 1);

        printf("}");

        if(this_model->next_model != NULL)
            printf(",");
    }

    if(internal_flag == 0)
        printf("}");
}

// 輸出資料內容的 json 資料 (資料內容部分).
void mcm_output_data_json_data(
    struct mcm_model_t *this_model,
    struct mcm_store_t *this_store,
    struct mcm_request_para_t *request_info)
{
    MCM_DTYPE_USIZE_TD name_len, value_len;
#if MCM_SUPPORT_DTYPE_S
    MCM_DTYPE_USIZE_TD kidx, ucode, ulen = 0;
    MCM_DTYPE_BOOL_TD is_special;
#endif
#if MCM_SUPPORT_DTYPE_S || MCM_SUPPORT_DTYPE_B
    MCM_DTYPE_USIZE_TD sidx;
#endif
    void *name_loc, *name_end, *type_loc, *value_loc, *value_data;
    char *name_data;
    MCM_DTYPE_LIST_TD type_data;


#define MCM_OUTPUT_NUM_VAULE(output_fmt, type_def, type_fmt) \
    do                                                              \
    {                                                               \
        if((request_info->data_format & output_fmt) == 0)           \
            printf(type_fmt, *((type_def *) value_data));           \
        else                                                        \
            printf("\"" type_fmt "\"", *((type_def *) value_data)); \
    }                                                               \
    while(0)


    name_loc = this_model->member_name_list;
    name_end = this_model->member_name_list + this_model->member_name_len;
    type_loc = this_model->member_type_list;
    value_loc = this_store->member_value_list;

    while(name_loc < name_end)
    {
        name_len = (MCM_DTYPE_USIZE_TD) *((MCM_DTYPE_SNLEN_TD *) name_loc);
        name_loc += sizeof(MCM_DTYPE_SNLEN_TD);
        name_data = (char *) name_loc;
        name_loc += name_len;

        type_data = (MCM_DTYPE_LIST_TD) *((MCM_DTYPE_SDTYPE_TD *) type_loc);
        type_loc += sizeof(MCM_DTYPE_SDTYPE_TD);

        value_len = *((MCM_DTYPE_USIZE_TD *) value_loc);
        value_loc += sizeof(MCM_DTYPE_USIZE_TD);
        value_data = value_loc;
        value_loc += value_len;

        printf("\"%s\":", name_data);

        switch(type_data)
        {
            case MCM_DTYPE_EK_INDEX:
                MCM_OUTPUT_NUM_VAULE(MCM_DFORMAT_EK_STRING,
                                     MCM_DTYPE_EK_TD, MCM_DTYPE_EK_PF);
                break;
#if MCM_SUPPORT_DTYPE_RK
            case MCM_DTYPE_RK_INDEX:
                MCM_OUTPUT_NUM_VAULE(MCM_DFORMAT_RK_STRING,
                                     MCM_DTYPE_RK_TD, MCM_DTYPE_RK_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_ISC
            case MCM_DTYPE_ISC_INDEX:
                MCM_OUTPUT_NUM_VAULE(MCM_DFORMAT_ISC_STRING,
                                     MCM_DTYPE_ISC_TD, MCM_DTYPE_ISC_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_IUC
            case MCM_DTYPE_IUC_INDEX:
                MCM_OUTPUT_NUM_VAULE(MCM_DFORMAT_IUC_STRING,
                                     MCM_DTYPE_IUC_TD, MCM_DTYPE_IUC_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_ISS
            case MCM_DTYPE_ISS_INDEX:
                MCM_OUTPUT_NUM_VAULE(MCM_DFORMAT_ISS_STRING,
                                     MCM_DTYPE_ISS_TD, MCM_DTYPE_ISS_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_IUS
            case MCM_DTYPE_IUS_INDEX:
                MCM_OUTPUT_NUM_VAULE(MCM_DFORMAT_IUS_STRING,
                                     MCM_DTYPE_IUS_TD, MCM_DTYPE_IUS_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_ISI
            case MCM_DTYPE_ISI_INDEX:
                MCM_OUTPUT_NUM_VAULE(MCM_DFORMAT_ISI_STRING,
                                     MCM_DTYPE_ISI_TD, MCM_DTYPE_ISI_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_IUI
            case MCM_DTYPE_IUI_INDEX:
                MCM_OUTPUT_NUM_VAULE(MCM_DFORMAT_IUI_STRING,
                                     MCM_DTYPE_IUI_TD, MCM_DTYPE_IUI_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_ISLL
            case MCM_DTYPE_ISLL_INDEX:
                MCM_OUTPUT_NUM_VAULE(MCM_DFORMAT_ISLL_STRING,
                                     MCM_DTYPE_ISLL_TD, MCM_DTYPE_ISLL_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_IULL
            case MCM_DTYPE_IULL_INDEX:
                MCM_OUTPUT_NUM_VAULE(MCM_DFORMAT_IULL_STRING,
                                     MCM_DTYPE_IULL_TD, MCM_DTYPE_IULL_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_FF
            case MCM_DTYPE_FF_INDEX:
                MCM_OUTPUT_NUM_VAULE(MCM_DFORMAT_FF_STRING,
                                     MCM_DTYPE_FF_TD, MCM_DTYPE_FF_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_FD
            case MCM_DTYPE_FD_INDEX:
                MCM_OUTPUT_NUM_VAULE(MCM_DFORMAT_FD_STRING,
                                     MCM_DTYPE_FD_TD, MCM_DTYPE_FD_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_FLD
            case MCM_DTYPE_FLD_INDEX:
                MCM_OUTPUT_NUM_VAULE(MCM_DFORMAT_FLD_STRING,
                                     MCM_DTYPE_FLD_TD, MCM_DTYPE_FLD_PF);
                break;
#endif
#if MCM_SUPPORT_DTYPE_S
            case MCM_DTYPE_S_INDEX:
                printf("\"");
                for(sidx = 0; sidx < value_len; sidx += ulen)
                {
                    mcm_convert_utf8_to_unicode(((MCM_DTYPE_S_TD *) value_data) + sidx,
                                                &ucode, &ulen);
                    is_special = 1;
                    if((MCM_MIN_PRINTABLE_KEY <= ucode) && (ucode <= MCM_MAX_PRINTABLE_KEY))
                    {
                        for(kidx = 0; mcm_js_special_list[kidx] != '\0'; kidx++)
                            if(ucode == mcm_js_special_list[kidx])
                                break;
                        if(mcm_js_special_list[kidx] == '\0')
                            is_special = 0;
                    }
                    printf(is_special == 0 ? "%c" : "\\u%04X", ucode & 0xFFFF);
                }
                printf("\"");
                break;
#endif
#if MCM_SUPPORT_DTYPE_B
            case MCM_DTYPE_B_INDEX:
                printf("\"");
                for(sidx = 0; sidx < value_len; sidx++)
                    printf(MCM_DTYPE_B_PF, *(((MCM_DTYPE_B_TD *) value_data) + sidx) & 0xFF);
                printf("\"");
                break;
#endif
        }

        if(name_loc < name_end)
            printf(",");
        else
            if(this_model->child_model != NULL)
                printf(",");
    }
}

// 輸出資料內容的 json 資料 (資料架構部分).
void mcm_output_data_json_tree(
    struct mcm_model_t *this_model,
    struct mcm_store_t *this_store,
    struct mcm_request_para_t *request_info,
    MCM_DTYPE_USIZE_TD internal_flag)
{
    // == 0 : 表示在最外層.
    if(internal_flag == 0)
    {
        mcm_cgi_fill_response_header(1, 1, MCM_RCODE_PASS);
        printf("{");
    }

    // 處理所有的 model.
    for(; this_model != NULL; this_model = this_model->next_model)
    {
        printf("\"%s\":", this_model->group_name);

        if(this_model->group_type == MCM_DTYPE_GD_INDEX)
            printf("[");

        // model 和 store 的順序一致, 依序處理 store 即可.
        // 例如 :
        // model tree :
        // device
        //  + system
        //  + vap
        //  |  + extra
        //  |  + station
        //  + limit
        //  + client
        // store tree :
        // device
        //  + system
        //  + vap.#1
        //  |  + extra
        //  |  + station.#1
        //  |  + station.#2
        //  + vap.#2
        //  |  + extra
        //  + client.#1
        //  + client.#2
        // 依序處理 store, 遇到使用其他 model 的 store 表示此 model 的資料都處理完,
        // 跳出換下一個 model.
        if(this_store != NULL)
            if(this_store->link_model == this_model)
                for(; this_store != NULL; this_store = this_store->next_store)
                {
                    if(this_store->link_model != this_model)
                        break;

                    if(this_store->empty_store != 0)
                    {
                        this_store = this_store->next_store;
                        break;
                    }

                    printf("{");

                    if(this_store->member_value_list != NULL)
                        mcm_output_data_json_data(this_model, this_store, request_info);

                    if(this_model->child_model != NULL)
                        mcm_output_data_json_tree(this_model->child_model,
                                                  this_store->child_store_head, request_info, 1); 

                    printf("}");

                    if(this_model->group_type == MCM_DTYPE_GD_INDEX)
                    {
                        if(this_store->next_store != NULL)
                            if(this_store->next_store->link_model == this_model)
                                printf(",");
                    }
                    else
                    {
                        if(this_model->next_model != NULL)
                            printf(",");
                    }
                }

        if(this_model->group_type == MCM_DTYPE_GD_INDEX)
        {
            printf("]");
            if(this_model->next_model != NULL)
                printf(",");
        }
    }

    if(internal_flag == 0)
        printf("}");
}

// 處理 push 類型的指令.
int mcm_do_push_command(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_list_t *this_command)
{
    int fret;


    MCM_CCDMSG("=> %s", __FUNCTION__);

    if(this_command->operate_type == MCM_CONFIG_SET_INDEX)
    {
        MCM_CCDMSG("cmd [%s][%s][%s]",
                   MCM_CONFIG_SET_KEY, this_command->push_config_path,
                   this_command->push_config_data_value);
        fret = mcm_lulib_set_any_type_alone(this_lulib, this_command->push_config_path,
                                            this_command->push_config_data_value);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_CEMSG("call mcm_lulib_set_any_type_alone([%s][%s]) fail",
                      this_command->push_config_path, this_command->push_config_data_value);
            MCM_CGI_AEMSG(fret, 0, "call mcm_lulib_set_any_type_alone() fail\\n[%s][%s]",
                          this_command->push_config_path, this_command->push_config_data_value);
            return fret;
        }
    }
    else
    if(this_command->operate_type == MCM_CONFIG_ADD_INDEX)
    {
        MCM_CCDMSG("cmd [%s][%s][%s]",
                   MCM_CONFIG_ADD_KEY, this_command->push_config_path,
                   this_command->push_config_data_value);
        fret = mcm_lulib_add_entry(this_lulib, this_command->push_config_path,
                                   this_command->push_config_data_value, NULL, 0);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_CEMSG("call mcm_lulib_add_entry([%s][%s]) fail",
                      this_command->push_config_path, this_command->push_config_data_value);
            MCM_CGI_AEMSG(fret, 0, "call mcm_lulib_add_entry() fail\\n[%s][%s]",
                          this_command->push_config_path, this_command->push_config_data_value);
            return fret;
        }
    }
    else
    if(this_command->operate_type == MCM_CONFIG_DEL_INDEX)
    {
        MCM_CCDMSG("cmd [%s][%s]", MCM_CONFIG_DEL_KEY, this_command->push_config_path);
        fret = mcm_lulib_del_entry(this_lulib, this_command->push_config_path);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_CEMSG("call mcm_lulib_del_entry(%s) fail", this_command->push_config_path);
            MCM_CGI_AEMSG(fret, 0, "call mcm_lulib_del_entry() fail\\n[%s]",
                          this_command->push_config_path);
            return fret;
        }
    }
    else
    if(this_command->operate_type == MCM_CONFIG_DEL_ALL_INDEX)
    {
        MCM_CCDMSG("cmd [%s][%s]", MCM_CONFIG_DEL_ALL_KEY, this_command->push_config_path);
        fret = mcm_lulib_del_all_entry(this_lulib, this_command->push_config_path);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_CEMSG("call mcm_lulib_del_all_entry(%s) fail", this_command->push_config_path);
            MCM_CGI_AEMSG(fret, 0, "call mcm_lulib_del_all_entry() fail\\n[%s]",
                          this_command->push_config_path);
            return fret;
        }
    }

    return MCM_RCODE_PASS;
}

// 填充執行 MCM_RACTION_SUBMIT_CONFIG 行為後要回應給網頁的訊息.
void mcm_fill_submit_response(
    struct mcm_command_list_t *this_command,
    struct mcm_request_para_t *request_info,
    int rep_code,
    char *rep_msg)
{
    mcm_cgi_fill_response_header(1, 1, rep_code);

    // 如果執行 run 指令後 module 有回傳自訂的訊息則輸出自訂的訊息.
    if(rep_msg != NULL)
    {
        printf("%s", rep_msg);
    }
    else
    {
        if(rep_code < MCM_RCODE_PASS)
        {
            printf("alert(\"call mcm_lulib_run() fail\\n[%s]\");", this_command->module_path);
        }
        else
        {
            switch(request_info->after_complete)
            {
                case MCM_ACOMPLETE_NONE:
                    break;
                case MCM_ACOMPLETE_RELOAD:
                    printf("window.location.reload();");
                    break;
            }
        }
    }
}

// 處理 module 類型的指令.
int mcm_do_module_command(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_list_t *this_command,
    struct mcm_request_para_t *request_info,
    MCM_DTYPE_BOOL_TD *fill_report_buf)
{
    int fret;
    void *tmp_buf;


    MCM_CCDMSG("=> %s", __FUNCTION__);

    MCM_CCDMSG("cmd [%s][%s]", MCM_MODULE_RUN_KEY, this_command->module_path);
    fret = mcm_lulib_run(this_lulib, this_command->module_path, NULL, 0, &tmp_buf, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CEMSG("call mcm_lulib_run(%s) fail", this_command->module_path);
        mcm_fill_submit_response(this_command, request_info, fret, tmp_buf);
        goto FREE_01;
    }
    else
    {
        // 只有最後一個 run 指令所執行的 module 所產生的自訂訊息會傳送到網頁.
        if(request_info->request_action == MCM_RACTION_SUBMIT_CONFIG)
            if(this_command->module_last != 0)
            {
                mcm_fill_submit_response(this_command, request_info, fret, tmp_buf);
                *fill_report_buf = 1;
            }
    }

    fret = MCM_RCODE_PASS;
FREE_01:
    if(tmp_buf != NULL)
        free(tmp_buf);
    return fret;
}

// 處理 POST 資料.
int mcm_process_request(
    char *post_con,
    MCM_DTYPE_USIZE_TD post_len,
    struct mcm_request_para_t *request_info)
{
    int fret;
    void *pull_module_fp = NULL;
    struct mcm_lulib_lib_t self_lulib;
    struct mcm_command_list_t *command_list_info, *each_command;
    struct mcm_command_count_t command_count_info;
    struct mcm_model_t *self_model = NULL;
    struct mcm_store_t *self_store = NULL;
    MCM_DTYPE_USIZE_TD path_max_len, cidx;
    char *cache_path = NULL;
    MCM_DTYPE_BOOL_TD need_pull_module = 0, do_update = 0, fill_report = 0;


    MCM_CCDMSG("=> %s", __FUNCTION__);

    memset(&command_count_info, 0, sizeof(struct mcm_command_count_t));
    fret = mcm_split_command(post_con, post_len, &command_count_info, &command_list_info);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CECTMSG("call mcm_split_command() fail");
        goto FREE_01;
    }

    fret = mcm_check_action(request_info, &command_count_info);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CECTMSG("call mcm_check_action() fail");
        goto FREE_02;
    }

    for(cidx = 0; cidx < command_count_info.total_count; cidx++)
    {
        each_command = command_list_info + cidx;
        switch(each_command->operate_type)
        {
            case MCM_CONFIG_GET_INDEX:
                fret = mcm_analysis_pull_command(each_command);
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_CECTMSG("call mcm_analysis_pull_command() fail");
                    goto FREE_02;
                }

                if(each_command->pull_config_module_path != NULL)
                    need_pull_module = 1;

                break;

            case MCM_CONFIG_SET_INDEX:
            case MCM_CONFIG_ADD_INDEX:
            case MCM_CONFIG_DEL_INDEX:
            case MCM_CONFIG_DEL_ALL_INDEX:
                fret = mcm_analysis_push_command(each_command);
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_CECTMSG("call mcm_analysis_push_command() fail");
                    goto FREE_02;
                }
                break;

            case MCM_MODULE_RUN_INDEX:
                fret = mcm_analysis_module_command(each_command);
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_CECTMSG("call mcm_analysis_module_command() fail");
                    goto FREE_02;
                }
                break;
        }
    }

    if(command_count_info.pull_count > 0)
    {
        fret = mcm_check_pull_command_duplic(command_list_info, &command_count_info);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_CECTMSG("call mcm_check_pull_command_duplic() fail");
            goto FREE_02;
        }
    }

    if(command_count_info.module_count > 0)
        mcm_mark_last_module_command(command_list_info, &command_count_info);

    if(need_pull_module != 0)
    {
        pull_module_fp = dlopen(MCM_CONFIG_MODULE_PATH, RTLD_LAZY);
        if(pull_module_fp == NULL)
        {
            fret = MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
            MCM_CEMSG("call dlopen(%s) fail [%s]", MCM_CONFIG_MODULE_PATH, dlerror());
            MCM_CGI_AEMSG(fret, 0, "call dlopen(%s) fail\\n[%s]",
                          MCM_CONFIG_MODULE_PATH, dlerror());
            goto FREE_02;
        }
        MCM_CCDMSG("dlopen %s[%p]", MCM_CONFIG_MODULE_PATH, pull_module_fp);
    }

    memset(&self_lulib, 0, sizeof(struct mcm_lulib_lib_t));
    self_lulib.socket_path = request_info->socket_path;
    self_lulib.call_from = MCM_CFROM_WEB;
    self_lulib.session_permission = request_info->session_permission;
    self_lulib.session_stack_size = request_info->session_stack_size;
    fret = mcm_lulib_init(&self_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CEMSG("call mcm_lulib_init() fail");
        MCM_CGI_AEMSG(fret, 0, "call mcm_lulib_init() fail");
        goto FREE_03;
    }

    if(command_count_info.pull_count > 0)
    {
        fret = mcm_check_pull_command_valid(&self_lulib, command_list_info, &command_count_info);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_CECTMSG("call mcm_check_pull_command_valid() fail");
            goto FREE_04;
        }

        fret = mcm_lulib_get_path_max_length(&self_lulib, &path_max_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_CEMSG("call mcm_lulib_get_path_max_length() fail");
            MCM_CGI_AEMSG(fret, 0, "call mcm_lulib_get_path_max_length() fail");
            goto FREE_04;
        }

        cache_path = (char *) malloc(path_max_len);
        if(cache_path == NULL)
        {
            fret = MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
            MCM_CEMSG("call malloc() fail [%s]", strerror(errno));
            MCM_CGI_AEMSG(fret, 0, "call malloc() fail\\n[%s]", strerror(errno));
            goto FREE_04;
        }
        MCM_CCDMSG("alloc cache_path[" MCM_DTYPE_USIZE_PF "][%p]", path_max_len, cache_path);

        fret = mcm_create_pull_command(command_list_info, &command_count_info);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_CECTMSG("call mcm_create_pull_command() fail");
            goto FREE_05;
        }

        for(cidx = 0; cidx < command_count_info.total_count; cidx++)
        {
            each_command = command_list_info + cidx;
            if(each_command->operate_type == MCM_CONFIG_GET_INDEX)
            {
                fret = mcm_create_model(&self_lulib, request_info, each_command, 0, cache_path,
                                        self_model, NULL, self_model == NULL ? &self_model : NULL);
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_CECTMSG("call mcm_create_model() fail");
                    goto FREE_06;
                }
            }
        }
    }

    if(request_info->request_action == MCM_RACTION_OBTAIN_MAX_COUNT)
    {
        mcm_output_max_count_json(self_model, 0);
    }
    else
    {
        for(cidx = 0; cidx < command_count_info.total_count; cidx++)
        {
            each_command = command_list_info + cidx;
            switch(each_command->operate_type)
            {
                case MCM_CONFIG_GET_INDEX:
                    if(do_update != 0)
                    {
                        fret = mcm_lulib_update(&self_lulib);
                        if(fret < MCM_RCODE_PASS)
                        {
                            MCM_CEMSG("call mcm_lulib_update() fail");
                            goto FREE_07;
                        }
                        do_update = 0;
                    }

                    fret = mcm_create_store(pull_module_fp, &self_lulib, each_command, 0,
                                            cache_path, self_model, self_store, NULL,
                                            self_store == NULL ? &self_store : NULL);
                    if(fret < MCM_RCODE_PASS)
                    {
                        MCM_CECTMSG("call mcm_create_store() fail");
                        goto FREE_07;
                    }
                    break;

                case MCM_CONFIG_SET_INDEX:
                case MCM_CONFIG_ADD_INDEX:
                case MCM_CONFIG_DEL_INDEX:
                case MCM_CONFIG_DEL_ALL_INDEX:
                    fret = mcm_do_push_command(&self_lulib, each_command);
                    if(fret < MCM_RCODE_PASS)
                    {
                        MCM_CECTMSG("call mcm_do_push_command() fail");
                        goto FREE_07;
                    }
                    do_update = 1;
                    break;

                case MCM_MODULE_RUN_INDEX:
                    fret = mcm_do_module_command(&self_lulib, each_command, request_info,
                                                 &fill_report);
                    if(fret < MCM_RCODE_PASS)
                    {
                        MCM_CECTMSG("call mcm_do_module_command() fail");
                        goto FREE_07;
                    }
                    do_update = 1;
                    break;
            }
        }

        if(request_info->request_action == MCM_RACTION_OBTAIN_CONFIG)
            mcm_output_data_json_tree(self_model, self_store, request_info, 0);
        else
        if(request_info->request_action == MCM_RACTION_SUBMIT_CONFIG)
            if(fill_report == 0)
                mcm_fill_submit_response(command_list_info, request_info, MCM_RCODE_PASS, NULL);
    }

    fret = MCM_RCODE_PASS;
FREE_07:
    if(self_store != NULL)
        mcm_destory_store(self_store);
FREE_06:
    if(self_model != NULL)
        mcm_destory_model(self_model);
    if(command_count_info.pull_count > 0)
        mcm_destory_pull_command(command_list_info, &command_count_info);
FREE_05:
    MCM_CCDMSG("free cache_path[%p]", cache_path);
    free(cache_path);
FREE_04:
    mcm_lulib_exit(&self_lulib);
FREE_03:
    if(need_pull_module != 0)
    {
        MCM_CCDMSG("dlclose pull_module_fp[%p]", pull_module_fp);
        dlclose(pull_module_fp);
    }
FREE_02:
    MCM_CCDMSG("free command_list_info[%p]", command_list_info);
    free(command_list_info);
FREE_01:
    return fret;
}

int main(
    int arg_cnt,
    char **arg_list)
{
    int fret = MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR;
    char *env_loc, *post_buf;
    MCM_DTYPE_USIZE_TD post_len, rcnt;
    ssize_t rlen;
    struct mcm_request_para_t request_info;


#if MCM_CGIEMODE | MCM_CGIECTMODE | MCM_CCDMODE
    dbg_console_fd = open(MCM_DBG_CONSOLE, O_WRONLY);
    if(dbg_console_fd == -1)
    {
        MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                      "call open() fail\\n[%s]\\n[%s]", MCM_DBG_CONSOLE, strerror(errno));
        goto FREE_01;
    }
#endif

    mcm_lulib_show_msg = 0;

    // 確認是 POST 方法.
    env_loc = getenv(MCM_CGI_REQUEST_METHOD_KEY);
    if(env_loc == NULL)
    {
        MCM_CEMSG("invalid, not find %s", MCM_CGI_REQUEST_METHOD_KEY);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                      "invalid, not find %s", MCM_CGI_REQUEST_METHOD_KEY);
        goto FREE_01;
    }
    if(strcmp(env_loc, MCM_CGI_POST_KEY) != 0)
    {
        MCM_CEMSG("invalid, %s must be %s [%s]",
                  MCM_CGI_REQUEST_METHOD_KEY, MCM_CGI_POST_KEY, env_loc);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                      "invalid, %s must be %s\\n[%s]",
                      MCM_CGI_REQUEST_METHOD_KEY, MCM_CGI_POST_KEY, env_loc);
        goto FREE_01;
    }

    // 取出 POST 後面的 query 參數.
    env_loc = getenv(MCM_CGI_QUERY_STRING_KEY);
    if(env_loc == NULL)
    {
        MCM_CEMSG("invalid, not find %s", MCM_CGI_QUERY_STRING_KEY);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                      "invalid, not find %s", MCM_CGI_QUERY_STRING_KEY);
        goto FREE_01;
    }
    memset(&request_info, 0, sizeof(struct mcm_request_para_t));
    fret = mcm_parse_query(env_loc, strlen(env_loc), &request_info);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CECTMSG("call mcm_parse_query() fail");
        goto FREE_01;
    }

    // 取得 POST 的資料的長度.
    env_loc = getenv(MCM_CGI_CONTENT_LENGTH_KRY);
    if(env_loc == NULL)
    {
        MCM_CEMSG("invalid, not find %s", MCM_CGI_CONTENT_LENGTH_KRY);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                      "invalid, not find %s", MCM_CGI_CONTENT_LENGTH_KRY);
        goto FREE_01;
    }
    post_len = MCM_DTYPE_USIZE_SB(env_loc, NULL, 10);
    if(post_len == 0)
    {
        MCM_CEMSG("invalid, %s can not be 0", MCM_CGI_CONTENT_LENGTH_KRY);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                      "invalid, %s can not be 0", MCM_CGI_CONTENT_LENGTH_KRY);
        goto FREE_01;
    }

    post_buf = (char *) malloc(post_len + 1);
    if(post_buf == NULL)
    {
        MCM_CEMSG("call malloc() fail [%s]", strerror(errno));
        MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                      "call malloc() fail\\n[%s]", strerror(errno));
        goto FREE_01;
    }
    MCM_CCDMSG("alloc post_buf[" MCM_DTYPE_USIZE_PF "][%p]", post_len + 1, post_buf);
    post_buf[post_len] = '\0';

    // 讀取 POST 資料.
    for(rcnt = 0; rcnt < post_len; rcnt += rlen)
    {
        rlen = read(STDIN_FILENO, post_buf + rcnt, post_len - rcnt);
        if(rlen == -1)
        {
            MCM_CEMSG("call read(STDIN_FILENO) fail [%s]", strerror(errno));
            MCM_CGI_AEMSG(MCM_RCODE_CGI_CONFIG_INTERNAL_ERROR, 0,
                          "call read(STDIN_FILENO) fail\\n[%s]", strerror(errno));
            goto FREE_01;
        }
    }
    post_buf[post_len] = '\0';

    fret = mcm_process_request(post_buf, post_len, &request_info);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CECTMSG("call mcm_process_post() fail");
        goto FREE_02;
    }

    fret = MCM_RCODE_PASS;
FREE_02:
    MCM_CCDMSG("free post_buf[%p]", post_buf);
    free(post_buf);
FREE_01:
#if MCM_CGIEMODE | MCM_CGIECTMODE | MCM_CCDMODE
    close(dbg_console_fd);
#endif
    return fret;
}

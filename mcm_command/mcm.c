// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../mcm_lib/mcm_lheader/mcm_type.h"
#include "../mcm_lib/mcm_lheader/mcm_keyword.h"
#include "../mcm_lib/mcm_lheader/mcm_connect.h"
#include "../mcm_lib/mcm_lheader/mcm_return.h"
#include "../mcm_lib/mcm_lheader/mcm_limit.h"
#include "../mcm_lib/mcm_lheader/mcm_debug.h"
#include "../mcm_lib/mcm_lulib/mcm_lulib_api.h"




#define MCM_DATA_SPLIT_KEY '='

#define MCM_PERMISSION_RO_KEY "ro"
#define MCM_PERMISSION_RW_KEY "rw"

// 指令的控制碼 (編號).
enum MCM_OPERATE_PARAMETER_INDEX
{
    // 取得資料.
    MCM_OPERATE_GET_INDEX = 0,
    // 設定資料.
    MCM_OPERATE_SET_INDEX,
    // 增加資料.
    MCM_OPERATE_ADD_INDEX,
    // 刪除資料.
    MCM_OPERATE_DEL_INDEX,
    // 刪除 group 的全部 entry 資料.
    MCM_OPERATE_DEL_ALL_INDEX,
    // 取得 group 的最大筆數限制.
    MCM_OPERATE_MAX_COUNT_INDEX,
    // 取得 group 目前的 entry 數.
    MCM_OPERATE_COUNT_INDEX,
    // 取得可用的 key.
    MCM_OPERATE_USABLE_KEY_INDEX,
    // 執行模組函式.
    MCM_OPERATE_RUN_INDEX,
    // 更新資料.
    MCM_OPERATE_UPDATE_INDEX,
    // 儲存資料到資料現在值檔案.
    MCM_OPERATE_SAVE_INDEX,
    // 關閉 mcm_daemon.
    MCM_OPERATE_SHUTDOWN_INDEX
};
// 指令的控制碼 (對映的字串).
#define MCM_OPERATE_GET_KEY        "get"
#define MCM_OPERATE_SET_KEY        "set"
#define MCM_OPERATE_ADD_KEY        "add"
#define MCM_OPERATE_DEL_KEY        "del"
#define MCM_OPERATE_DEL_ALL_KEY    "delall"
#define MCM_OPERATE_MAX_COUNT_KEY  "maxcount"
#define MCM_OPERATE_COUNT_KEY      "count"
#define MCM_OPERATE_USABLE_KEY_KEY "usablekey"
#define MCM_OPERATE_RUN_KEY        "run"
#define MCM_OPERATE_UPDATE_KEY     "update"
#define MCM_OPERATE_SAVE_KEY       "save"
#define MCM_OPERATE_SHUTDOWN_KEY   "shutdown"

// save 指令的參數 (編號).
enum MCM_SAVE_METHOD_INDEX
{
    MCM_SAVE_CHECK_INDEX = 0,
    MCM_SAVE_FORCE_INDEX
};
// save 指令的參數 (對映的字串).
#define MCM_SAVE_CHECK_KEY "check"
#define MCM_SAVE_FORCE_KEY "force"




struct mcm_command_t
{
    // 指令內容.
    char *cmd_con;
    // 指令的控制碼類型 (MCM_OPERATE_PARAMETER_INDEX).
    MCM_DTYPE_LIST_TD cmd_operate;
    // 指令是否含有路徑.
    MCM_DTYPE_BOOL_TD with_path;
    // 指令的路徑.
    char *cmd_path;
    // 指令是否含有資料.
    MCM_DTYPE_BOOL_TD with_data;
    // 指令的資料.
    char *cmd_data;
    // MCM_SAVE_METHOD_INDEX.
    MCM_DTYPE_LIST_TD save_method;
};




int do_get(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info);

int do_set(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info);

int do_add(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info);

int do_del(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info);

int do_del_all(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info);

int do_max_count(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info);

int do_count(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info);

int do_usable_key(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info);

int do_run(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info);

int do_update(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info);

int do_save(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info);

int do_shutdown(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info);




struct mcm_permission_type_map_t
{
    char *permission_key;
    MCM_DTYPE_LIST_TD permission_index;
};
struct mcm_permission_type_map_t mcm_permission_type_map_info[] =
{
    {MCM_PERMISSION_RO_KEY, MCM_SPERMISSION_RO},
    {MCM_PERMISSION_RW_KEY, MCM_SPERMISSION_RW},
    {NULL, 0}
};

struct mcm_operate_type_map_t
{
    char *operate_key;
    MCM_DTYPE_LIST_TD operate_index;
    MCM_DTYPE_BOOL_TD need_path;
    MCM_DTYPE_BOOL_TD need_data;
    int (*handle_cb)(struct mcm_lulib_lib_t *this_lulib, struct mcm_command_t *command_info);
};
struct mcm_operate_type_map_t mcm_operate_type_map_info[] =
{
    {MCM_OPERATE_GET_KEY,        MCM_OPERATE_GET_INDEX,        1, 0, do_get},
    {MCM_OPERATE_SET_KEY,        MCM_OPERATE_SET_INDEX,        1, 1, do_set},
    {MCM_OPERATE_ADD_KEY,        MCM_OPERATE_ADD_INDEX,        1, 1, do_add},
    {MCM_OPERATE_DEL_KEY,        MCM_OPERATE_DEL_INDEX,        1, 0, do_del},
    {MCM_OPERATE_DEL_ALL_KEY,    MCM_OPERATE_DEL_ALL_INDEX,    1, 0, do_del_all},
    {MCM_OPERATE_MAX_COUNT_KEY,  MCM_OPERATE_MAX_COUNT_INDEX,  1, 0, do_max_count},
    {MCM_OPERATE_COUNT_KEY,      MCM_OPERATE_COUNT_INDEX,      1, 0, do_count},
    {MCM_OPERATE_USABLE_KEY_KEY, MCM_OPERATE_USABLE_KEY_INDEX, 1, 0, do_usable_key},
    {MCM_OPERATE_RUN_KEY,        MCM_OPERATE_RUN_INDEX,        1, 0, do_run},
    {MCM_OPERATE_UPDATE_KEY,     MCM_OPERATE_UPDATE_INDEX,     0, 0, do_update},
    {MCM_OPERATE_SAVE_KEY,       MCM_OPERATE_SAVE_INDEX,       0, 1, do_save},
    {MCM_OPERATE_SHUTDOWN_KEY,   MCM_OPERATE_SHUTDOWN_INDEX,   0, 0, do_shutdown},
    {NULL, 0, 0, 0, NULL}
};

struct mcm_save_method_map_t
{
    char *method_key;
    MCM_DTYPE_LIST_TD method_index;
};
struct mcm_save_method_map_t mcm_save_method_map_info[] =
{
    {MCM_SAVE_CHECK_KEY, MCM_SAVE_CHECK_INDEX},
    {MCM_SAVE_FORCE_KEY, MCM_SAVE_FORCE_INDEX},
    {NULL, 0}
};




// 找出指令的控制碼.
int mcm_find_command_operate(
    struct mcm_command_t *command_info)
{
    char *tmp_con;
    MCM_DTYPE_USIZE_TD iidx, oidx;


    tmp_con = command_info->cmd_con;

    // 找到指令中第一個 "." (後面接路徑) 或 "=" (後面接資料), 分離指令的控制碼.
    for(iidx = 0; tmp_con[iidx] != '\0'; iidx++)
        if((tmp_con[iidx] == MCM_SPROFILE_PATH_SPLIT_KEY) || (tmp_con[iidx] == MCM_DATA_SPLIT_KEY))
            break;
    if(tmp_con[iidx] == MCM_SPROFILE_PATH_SPLIT_KEY)
        command_info->with_path = 1;
    else
    if(tmp_con[iidx] == MCM_DATA_SPLIT_KEY)
        command_info->with_data = 1;
    tmp_con[iidx] = '\0';

    // 比對是哪種控制碼.
    for(oidx = 0; mcm_operate_type_map_info[oidx].operate_key != NULL; oidx++)
        if(strcmp(tmp_con, mcm_operate_type_map_info[oidx].operate_key) == 0)
            break;
    if(mcm_operate_type_map_info[oidx].operate_key == NULL)
    {
        MCM_EMSG("invalid, unknown operate [%s]", tmp_con);
        return MCM_RCODE_COMMAND_INTERNAL_ERROR;
    }
    command_info->cmd_operate = mcm_operate_type_map_info[oidx].operate_index;
    MCM_CMDMSG("operate [%s]",
               mcm_operate_type_map_info[command_info->cmd_operate].operate_key);

    // 將指令的內容往後移動, 跳過控制碼部分.
    command_info->cmd_con += iidx + 1;

    return MCM_RCODE_PASS;
}

// 找出指令的路徑.
int mcm_find_command_path(
    struct mcm_command_t *command_info)
{
    char *tmp_con;
    MCM_DTYPE_USIZE_TD iidx;


    tmp_con = command_info->cmd_con;

    if(command_info->with_path == 0)
    {
        if(mcm_operate_type_map_info[command_info->cmd_operate].need_path != 0)
        {
            MCM_EMSG("invalid, empty path [%s]", tmp_con);
            return MCM_RCODE_COMMAND_INTERNAL_ERROR;
        }
    }
    else
    {
        if(mcm_operate_type_map_info[command_info->cmd_operate].need_path == 0)
        {
            MCM_EMSG("invalid, this operate not need path [%s]",
                     mcm_operate_type_map_info[command_info->cmd_operate].operate_key);
            return MCM_RCODE_COMMAND_INTERNAL_ERROR;
        }

        // 找到 "=", 分離路徑.
        for(iidx = 0; tmp_con[iidx] != '\0'; iidx++)
            if(tmp_con[iidx] == MCM_DATA_SPLIT_KEY)
            {
                command_info->with_data = 1;
                break;
            }
        if(iidx == 0)
            if((tmp_con[iidx] == '\0') || (tmp_con[iidx] == MCM_DATA_SPLIT_KEY))
                if(mcm_operate_type_map_info[command_info->cmd_operate].need_path != 0)
                {
                    MCM_EMSG("invalid, empty path [%s]", tmp_con);
                    return MCM_RCODE_COMMAND_INTERNAL_ERROR;
                }
        tmp_con[iidx] = '\0';
        // 紀錄路徑.
        command_info->cmd_path = tmp_con;

        // 將指令的內容往後移動, 跳過路徑部分.
        command_info->cmd_con += iidx + (command_info->with_data == 0 ? 0 : 1);
    }
    MCM_CMDMSG("path [%s]", command_info->cmd_path);

    return MCM_RCODE_PASS;
}

// 指令的資料.
int mcm_find_command_data(
    struct mcm_command_t *command_info)
{
    if(command_info->with_data == 0)
    {
        if(mcm_operate_type_map_info[command_info->cmd_operate].need_data != 0)
        {
            MCM_EMSG("invalid, lose data [%s][%s]",
                     mcm_operate_type_map_info[command_info->cmd_operate].operate_key,
                     command_info->cmd_path);
            return MCM_RCODE_COMMAND_INTERNAL_ERROR;
        }
    }
    else
    {
        if(mcm_operate_type_map_info[command_info->cmd_operate].need_data == 0)
        {
            MCM_EMSG("invalid, this operate not need data [%s][%s]",
                     mcm_operate_type_map_info[command_info->cmd_operate].operate_key,
                     command_info->cmd_path);
            return MCM_RCODE_COMMAND_INTERNAL_ERROR;
        }
        // 紀錄資料.
        command_info->cmd_data = command_info->cmd_con;
    }
    MCM_CMDMSG("data [%s]", command_info->cmd_data);

    return MCM_RCODE_PASS;
}

// 檢查指令.
int mcm_check_command(
    struct mcm_command_t *command_info)
{
    MCM_DTYPE_USIZE_TD iidx;


    switch(command_info->cmd_operate)
    {
        case MCM_OPERATE_GET_INDEX:
            break;

        case MCM_OPERATE_SET_INDEX:
            break;

        case MCM_OPERATE_ADD_INDEX:
            break;

        case MCM_OPERATE_DEL_INDEX:
            break;

        case MCM_OPERATE_DEL_ALL_INDEX:
            break;

        case MCM_OPERATE_MAX_COUNT_INDEX:
            break;

        case MCM_OPERATE_COUNT_INDEX:
            break;

        case MCM_OPERATE_USABLE_KEY_INDEX:
            break;

        case MCM_OPERATE_SAVE_INDEX:
            for(iidx = 0; mcm_save_method_map_info[iidx].method_key != NULL; iidx++)
                if(strcmp(command_info->cmd_data, mcm_save_method_map_info[iidx].method_key) == 0)
                    break;
            if(mcm_save_method_map_info[iidx].method_key == NULL)
            {
                MCM_EMSG("invalid, unknown save method [%s]", command_info->cmd_data);
                return MCM_RCODE_COMMAND_INTERNAL_ERROR;
            }
            command_info->save_method = mcm_save_method_map_info[iidx].method_index;
            break;

        case MCM_OPERATE_RUN_INDEX:
            break;

        case MCM_OPERATE_SHUTDOWN_INDEX:
            break;
    }

    return MCM_RCODE_PASS;
}

// 分析指令.
int mcm_parse_command(
    char **command_con,
    MCM_DTYPE_USIZE_TD command_cnt,
    struct mcm_command_t **command_list_buf)
{
    int fret = MCM_RCODE_COMMAND_INTERNAL_ERROR;
    struct mcm_command_t *tmp_command_list, *each_command;
    MCM_DTYPE_USIZE_TD cidx;


    tmp_command_list = (struct mcm_command_t *) calloc(command_cnt, sizeof(struct mcm_command_t));
    if(tmp_command_list == NULL)
    {
        MCM_EMSG("call malloc() fail [%s]", strerror(errno));
        goto FREE_01;
    }
    MCM_CMDMSG("alloc command_list[" MCM_DTYPE_USIZE_PF "][%p]", command_cnt, tmp_command_list);

    // 記錄每條指令的內容.
    for(cidx = 0; cidx < command_cnt; cidx++)
        tmp_command_list[cidx].cmd_con = (char *) command_con[cidx];

    // 分析每條指令.
    for(cidx = 0; cidx < command_cnt; cidx++)
    {
        each_command = tmp_command_list + cidx;
        MCM_CMDMSG("[%s]", each_command->cmd_con);

        if(mcm_find_command_operate(each_command) < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_find_command_operate() fail");
            goto FREE_02;
        }

        if(mcm_find_command_path(each_command) < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_find_command_path() fail");
            goto FREE_02;
        }

        if(mcm_find_command_data(each_command) < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_find_command_data() fail");
            goto FREE_02;
        }

        if(mcm_check_command(each_command) < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_check_command() fail");
            goto FREE_02;
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

int do_get(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info)
{
    int fret;
    MCM_DTYPE_LIST_TD tmp_type;
    void *tmp_data = NULL;
    MCM_DTYPE_USIZE_TD tmp_len;
#if MCM_SUPPORT_DTYPE_B
    MCM_DTYPE_B_TD *tmp_b;
    MCM_DTYPE_USIZE_TD bidx;
#endif


#define MCM_SHOW_NUM_VALUE(type_def, type_fmt) \
        printf(type_fmt "\n", *((type_def *) tmp_data))


    fret = mcm_lulib_get_with_type_alone(this_lulib, command_info->cmd_path, &tmp_type,
                                         (void **) &tmp_data, &tmp_len);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_lulib_get_with_type_alone(%s) fail", command_info->cmd_path);
        return fret;
    }

    switch(tmp_type)
    {
        case MCM_DTYPE_EK_INDEX:
            MCM_SHOW_NUM_VALUE(MCM_DTYPE_EK_TD, MCM_DTYPE_EK_PF);
            break;
#if MCM_SUPPORT_DTYPE_RK
        case MCM_DTYPE_RK_INDEX:
            MCM_SHOW_NUM_VALUE(MCM_DTYPE_RK_TD, MCM_DTYPE_RK_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISC
        case MCM_DTYPE_ISC_INDEX:
            MCM_SHOW_NUM_VALUE(MCM_DTYPE_ISC_TD, MCM_DTYPE_ISC_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IUC
        case MCM_DTYPE_IUC_INDEX:
            MCM_SHOW_NUM_VALUE(MCM_DTYPE_IUC_TD, MCM_DTYPE_IUC_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISS
        case MCM_DTYPE_ISS_INDEX:
            MCM_SHOW_NUM_VALUE(MCM_DTYPE_ISS_TD, MCM_DTYPE_ISS_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IUS
        case MCM_DTYPE_IUS_INDEX:
            MCM_SHOW_NUM_VALUE(MCM_DTYPE_IUS_TD, MCM_DTYPE_IUS_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISI
        case MCM_DTYPE_ISI_INDEX:
            MCM_SHOW_NUM_VALUE(MCM_DTYPE_ISI_TD, MCM_DTYPE_ISI_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IUI
        case MCM_DTYPE_IUI_INDEX:
            MCM_SHOW_NUM_VALUE(MCM_DTYPE_IUI_TD, MCM_DTYPE_IUI_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISLL
        case MCM_DTYPE_ISLL_INDEX:
            MCM_SHOW_NUM_VALUE(MCM_DTYPE_ISLL_TD, MCM_DTYPE_ISLL_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IULL
        case MCM_DTYPE_IULL_INDEX:
            MCM_SHOW_NUM_VALUE(MCM_DTYPE_IULL_TD, MCM_DTYPE_IULL_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_FF
        case MCM_DTYPE_FF_INDEX:
            MCM_SHOW_NUM_VALUE(MCM_DTYPE_FF_TD, MCM_DTYPE_FF_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_FD
        case MCM_DTYPE_FD_INDEX:
            MCM_SHOW_NUM_VALUE(MCM_DTYPE_FD_TD, MCM_DTYPE_FD_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_FLD
        case MCM_DTYPE_FLD_INDEX:
            MCM_SHOW_NUM_VALUE(MCM_DTYPE_FLD_TD, MCM_DTYPE_FLD_PF);
            break;
#endif
#if MCM_SUPPORT_DTYPE_S
        case MCM_DTYPE_S_INDEX:
            printf(MCM_DTYPE_S_PF "\n", (MCM_DTYPE_S_TD *) tmp_data);
            break;
#endif
#if MCM_SUPPORT_DTYPE_B
        case MCM_DTYPE_B_INDEX:
            tmp_b = (MCM_DTYPE_B_TD *) tmp_data;
            for(bidx = 0; bidx < tmp_len; bidx++)
                printf(MCM_DTYPE_B_PF, tmp_b[bidx] & 0xFF);
            printf("\n");
            break;
#endif
    }

    free(tmp_data);
    return fret;
}

int do_set(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info)
{
    int fret;


    fret = mcm_lulib_set_any_type_alone(this_lulib, command_info->cmd_path,
                                        command_info->cmd_data);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_lulib_set_any_type_alone([%s][%s]) fail",
                 command_info->cmd_path, command_info->cmd_data);
        return fret;
    }

    return fret;
}

int do_add(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info)
{
    int fret;


    fret = mcm_lulib_add_entry(this_lulib, command_info->cmd_path,
                               command_info->cmd_data, NULL, 0);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_lulib_add_entry([%s][%s]) fail",
                 command_info->cmd_path, command_info->cmd_data);
        return fret;
    }

    return fret;
}

int do_del(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info)
{
    int fret;


    fret = mcm_lulib_del_entry(this_lulib, command_info->cmd_path);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_lulib_del_entry(%s) fail", command_info->cmd_path);
        return fret;
    }

    return fret;
}

int do_del_all(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info)
{
    int fret;


    fret = mcm_lulib_del_all_entry(this_lulib, command_info->cmd_path);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_lulib_del_all_entry(%s) fail", command_info->cmd_path);
        return fret;
    }

    return fret;
}

int do_max_count(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info)
{
    int fret;
    MCM_DTYPE_EK_TD tmp_count;


    fret = mcm_lulib_get_max_count(this_lulib, command_info->cmd_path, &tmp_count);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_lulib_get_max_count(%s) fail", command_info->cmd_path);
        return fret;
    }

    printf(MCM_DTYPE_EK_PF "\n", tmp_count);

    return fret;
}

int do_count(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info)
{
    int fret;
    MCM_DTYPE_EK_TD tmp_count;


    fret = mcm_lulib_get_count(this_lulib, command_info->cmd_path, &tmp_count);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_lulib_get_count(%s) fail", command_info->cmd_path);
        return fret;
    }

    printf(MCM_DTYPE_EK_PF "\n", tmp_count);

    return fret;
}

int do_usable_key(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info)
{
    int fret;
    MCM_DTYPE_EK_TD tmp_key;


    fret = mcm_lulib_get_usable_key(this_lulib, command_info->cmd_path, &tmp_key);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_lulib_get_usable_key(%s) fail", command_info->cmd_path);
        return fret;
    }

    printf(MCM_DTYPE_EK_PF "\n", tmp_key);

    return fret;
}

int do_run(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info)
{
    int fret;
    void *tmp_buf;


    fret = mcm_lulib_run(this_lulib, command_info->cmd_path, NULL, 0, &tmp_buf, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_lulib_run(%s) fail", command_info->cmd_path);
        return fret;
    }

    if(tmp_buf != NULL)
    {
        printf("%s\n", (char *) tmp_buf);
        free(tmp_buf);
    }

    return fret;
}

int do_update(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info)
{
    int fret;


    fret = mcm_lulib_update(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_lulib_update() fail");
        return fret;
    }

    return fret;
}

int do_save(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info)
{
    int fret;


    fret = mcm_lulib_save(this_lulib, command_info->save_method);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_lulib_save(" MCM_DTYPE_LIST_PF ") fail", command_info->save_method);
        return fret;
    }

    return fret;
}

int do_shutdown(
    struct mcm_lulib_lib_t *this_lulib,
    struct mcm_command_t *command_info)
{
    int fret;


    fret = mcm_lulib_shutdown(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_lulib_shutdown() fail");
        return fret;
    }

    return fret;
}

int main(
    int argc,
    char **argv)
{
    int fret = MCM_RCODE_COMMAND_INTERNAL_ERROR, opt_ch;
    struct mcm_lulib_lib_t self_lulib;
    char *arg_socket_path = NULL, *arg_permission = NULL, *arg_stack_size = NULL;
    MCM_DTYPE_USIZE_TD ccnt, cidx, pidx;
    struct mcm_command_t *command_list, *each_command;


    while((opt_ch = getopt(argc , argv, "a:p:s:")) != -1)
        switch(opt_ch)
        {
            case 'a':
                arg_socket_path = optarg;
                break;
            case 'p':
                arg_permission = optarg;
                break;
            case 's':
                arg_stack_size = optarg;
                break;
            default:
                MCM_EMSG("unknown argument [%d]", opt_ch);
                goto FREE_HELP;
        }

    if(arg_socket_path == NULL)
    {
        MCM_EMSG("miss socket-path");
        goto FREE_HELP;
    }
    if(arg_permission == NULL)
    {
        MCM_EMSG("miss session-permission");
        goto FREE_HELP;
    }

    // 計算指令的數目和在參數列表中的位置.
    ccnt = argc - optind;
    cidx = optind;
    if(ccnt <= 0)
    {
        MCM_EMSG("no any command");
        goto FREE_HELP;
    }

    memset(&self_lulib, 0, sizeof(struct mcm_lulib_lib_t));
    self_lulib.socket_path = arg_socket_path;
    self_lulib.call_from = MCM_CFROM_USER;

    for(pidx = 0; mcm_permission_type_map_info[pidx].permission_key != NULL; pidx++)
        if(strcmp(arg_permission, mcm_permission_type_map_info[pidx].permission_key) == 0)
            break;
    if(mcm_permission_type_map_info[pidx].permission_key == NULL)
    {
        MCM_EMSG("unknown permission [%s]", arg_permission);
        goto FREE_HELP;
    }
    self_lulib.session_permission = mcm_permission_type_map_info[pidx].permission_index;

    self_lulib.session_stack_size = arg_stack_size != NULL ?
                                    MCM_DTYPE_USIZE_SB(arg_stack_size, NULL, 10) : 0;

    // 分析指令.
    if(mcm_parse_command(argv + cidx, ccnt, &command_list) < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_parse_command() fail");
        goto FREE_01;
    }

    fret = mcm_lulib_init(&self_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_EMSG("call mcm_lulib_init() fail");
        goto FREE_02;
    }

    // 執行指令.
    for(cidx = 0; cidx < ccnt; cidx++)
    {
        each_command = command_list + cidx;
        MCM_CMDMSG("do [%s][%s][%s]",
                   mcm_operate_type_map_info[each_command->cmd_operate].operate_key,
                   each_command->cmd_path, each_command->cmd_data);
        fret = mcm_operate_type_map_info[each_command->cmd_operate].
                   handle_cb(&self_lulib, each_command);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_EMSG("command [%s][%s][%s] fail",
                     mcm_operate_type_map_info[each_command->cmd_operate].operate_key,
                     each_command->cmd_path, each_command->cmd_data);
            goto FREE_03;
        }
    }

FREE_03:
    mcm_lulib_exit(&self_lulib);
FREE_02:
    free(command_list);
FREE_01:
    return fret;
FREE_HELP:
    printf("mcm <-a> <-p> [-s] <command-lists ...>\n");
    printf("  -a : <-a socket-path>\n");
    printf("    server socket path.\n");
    printf("  -p : <-p session-permission>\n");
    printf("    session permission.\n");
    printf("    session-permission = <ro | rw>\n");
    printf("  -s : [-s session-stack-size]\n");
    printf("    session stack size.\n");
    printf("  <command-list ...> :\n");
    printf("    get.<full-path> :\n");
    printf("      get alone.\n");
    printf("    set.<full-path>=<data> :\n");
    printf("      set alone.\n");
    printf("    add.<full-path>=<insert-path> :\n");
    printf("      add entry.\n");
    printf("    del.<full-path> :\n");
    printf("      delete entry.\n");
    printf("    delall.<mix-path> :\n");
    printf("      delete all entry.\n");
    printf("    maxcount.<mask-path> :\n");
    printf("      get max entry count.\n");
    printf("    count.<mix-path> :\n");
    printf("      get entry count.\n");
    printf("    usablekey.<mix-path> :\n");
    printf("      get usable key.\n");
    printf("    run.<module-function> :\n");
    printf("      run module function.\n");
    printf("    update :\n");
    printf("      update store.\n");
    printf("    save=<method> :\n");
    printf("      save store to file.\n");
    printf("      method = <check | force>.\n");
    printf("    shutdown :\n");
    printf("      shutdown mcm_daemon.\n");
    return 0;
}

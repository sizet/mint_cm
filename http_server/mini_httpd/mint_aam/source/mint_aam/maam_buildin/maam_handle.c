/*
Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
This file is part of the MintAAM.
Some rights reserved. See README.
*/

#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <arpa/inet.h>
#include "maam_local.h"
#include "maam_custom.h"
#include "maam_handle.h"




// POST 路徑 (登入, 登出, 檢查 session 是否有效).
#define MAAM_ACCOUNT_LOGIN_KEY  "/account-login"
#define MAAM_ACCOUNT_LOGOUT_KEY "/account-logout"
#define MAAM_ACCOUNT_CHECK_KEY  "/account-check"

// 網頁端傳送帳號名稱的資料名稱.
#define MAAM_LOGIN_VERIFY_NAME_KEY                      "verify_name"
// 網頁端傳送帳號密碼的資料名稱.
#define MAAM_LOGIN_VERIFY_PASSWORD_KEY                  "verify_password"
// 網頁端傳送如何處理同時超過一個帳號登入系統的資料名稱.
#define MAAM_LOGIN_MULTIPLE_USER_ACTIVE_RULE_KEY        "multiple_user_active_rule"
// 網頁端傳送如何處理同一個帳號被多個使用者同時登入的資料名稱.
#define MAAM_LOGIN_SAME_ACCOUT_MULTIPLE_ACTIVE_RULE_KEY "same_accout_multiple_active_rule"
// 分離資料的符號.
#define MAAM_CONTENT_PARAMETER_SPLIT_KEY                '&'
#define MAAM_CONTENT_VALUE_SPLIT_KEY                    '='
// cookie 中記錄 session_key 的資料名稱.
#define MAAM_COOKIE_SESSION_KEY_KEY        "session_key"
// cookie 中記錄 account_name 的資料名稱.
#define MAAM_COOKIE_ACCOUNT_NAME_KEY       "account_name"
// cookie 中記錄 account_permission 的資料名稱.
#define MAAM_COOKIE_ACCOUNT_PERMISSION_KEY "account_permission"
// 清除 cookie 數值時填入的內容.
#define MAAM_COOKIE_EMPTY_VALUE_KEY        "!"
// cookie 中記錄存放路徑的資料名稱.
#define MAAM_COOKIE_PATH_NAME_KEY          "path"
// 存放路徑為根目錄.
#define MAAM_COOKIE_PATH_ROOT_KEY          "/"
// 分離資料的符號.
#define MAAM_COOKIE_PARAMETER_SPLIT_KEY    ';'
#define MAAM_COOKIE_SPACE_KEY              ' '
#define MAAM_COOKIE_VALUE_SPLIT_KEY        '='

// 比對權限, 值越小權限越高.
#define MAAM_PERMISSION_HEIGHT_EQUAL(a, b) ((a) <= (b))
#define MAAM_PERMISSION_EQUAL(a, b) ((a) == (b))
#define MAAM_PERMISSION_LOW(a, b) ((a) > (b))

// 計算最後活動到現在經過的時間差.
#define MAAM_CALCULATE_INTERVAL(interval_value, system_uptime, session_uptime, sysmax_uptime) \
    do                                                                          \
    {                                                                           \
        if(system_uptime >= session_uptime)                                     \
        {                                                                       \
            interval_value = system_uptime - session_uptime;                    \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            /* 如果系統 uptime < 最後活動, 表示系統 uptime 溢位重新計時,     */ \
            /* auth_sys_info->max_uptime 會紀錄系統 uptime 能記錄的的最大值, */ \
            /* 使用 (最大值 - 最後活動時間) + 目前時間 計算.                 */ \
            interval_value = (sysmax_uptime - session_uptime) + system_uptime;  \
        }                                                                       \
    }                                                                           \
    while(0)

#define MAAM_FILL_STRING(msg_buf, msg_size, msg_len, msg_fmt, msg_args...) \
    do                                                                  \
    {                                                                   \
        msg_len += snprintf(msg_buf + msg_len, msg_size - msg_len,      \
                            msg_fmt, ##msg_args);                       \
        if(msg_len >= msg_size)                                         \
        {                                                               \
            MAAM_EMSG("buffer too small [%zu/%zu]", msg_len, msg_size); \
            return MAAM_RCODE_INTERNAL_ERROR;                           \
        }                                                               \
    }                                                                   \
    while(0)

// 登入的參數在 maam_login_para_list[] 中的索引值.
enum MAAM_LOGIN_PARAMETER_INDEX
{
    MAAM_LOGIN_VERIFY_NAME_INDEX = 0,
    MAAM_LOGIN_VERIFY_PASSWORD_INDEX,
    MAAM_LOGIN_MULTIPLE_USER_ACTIVE_RULE_INDEX,
    MAAM_LOGIN_SAME_ACCOUT_MULTIPLE_ACTIVE_RULE_INDEX
};

// session_key 在 maam_cookie_para_list[] 中的索引值.
enum MAAM_COOKIE_PARAMETER_INDEX
{
    MAAM_COOKIE_SESSION_KEY_INDEX = 0
};

// 尋找 session 的比對方式.
enum MAAM_COMPARE_SESSION_METHOD
{
    // 找到帳號名稱相同的.
    MAAM_CSESSION_EQUAL_ACCOUNT_NAME = 0,
    // 找到 session_key 相同的.
    MAAM_CSESSION_EQUAL_SESSION_KEY,
    // 找到權限高於等於的.
    MAAM_CSESSION_HEIGHT_EQUAL_ACCOUNT_PERMISSION
};

// 比對存取路徑和查詢後要如何處理.
enum MAAM_PROCESS_ACCESS_METHOD
{
    // 允許的存取.
    MAAM_PACCESS_ALLOW = 0,
    // 禁止的存取, 回傳檔案不存在.
    MAAM_PACCESS_LOSE,
    // 禁止的存取, 重導向頁面.
    MAAM_PACCESS_REDIRECT
};

// 填充 cookie 的方式.
enum MAAM_FILL_COOKIE_METHOD
{
    // 加入 session_key.
    MAAM_FCOOKIE_ADD = 0,
    // 移除 session_key.
    MAAM_FCOOKIE_DEL
};




// 紀錄取出的資料名稱和資料值.
struct maam_para_t
{
    char *data_name;
    char *data_value;
    int miss_error_code;
};
// 登入的資料.
struct maam_para_t maam_login_para_list[] =
{
    {MAAM_LOGIN_VERIFY_NAME_KEY,                      NULL, MAAM_RCODE_MISS_VERIFY_NAME},
    {MAAM_LOGIN_VERIFY_PASSWORD_KEY,                  NULL, MAAM_RCODE_MISS_VERIFY_PASSWORD},
    {MAAM_LOGIN_MULTIPLE_USER_ACTIVE_RULE_KEY,        NULL, MAAM_RCODE_MISS_MULTIPLE_USER_ACTIVE_RULE},
    {MAAM_LOGIN_SAME_ACCOUT_MULTIPLE_ACTIVE_RULE_KEY, NULL, MAAM_RCODE_MISS_SAME_ACCOUT_MULTIPLE_ACTIVE_RULE},
    {NULL, NULL, 0}
};
// cookie 的資料.
struct maam_para_t maam_cookie_para_list[] =
{
    {MAAM_COOKIE_SESSION_KEY_KEY, NULL, MAAM_RCODE_MISS_SESSION_KEY},
    {NULL, NULL, 0}
};

// cookkie 的資料.
struct maam_cookie_t
{
    char *session_key;
};




// 取出 POST 時的 content 資料.
// 需要找到 :
// verify_name, verify_password, multiple_user_active_rule, same_accout_multiple_active_rule.
static int maam_parse_content(
    char **req_content_con,
    size_t *req_content_len,
    char **data_name_buf,
    char **data_value_buf)
{
    char *data_con;
    size_t data_len, didx1, didx2;
    int remain_data = 0;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    data_con = *req_content_con;
    data_len = *req_content_len;

    if(data_len <= 0)
        return -1;

    // 找到 '&', 分離各各參數.
    for(didx1 = 0; didx1 < data_len; didx1++)
        if(data_con[didx1] == MAAM_CONTENT_PARAMETER_SPLIT_KEY)
        {
            data_con[didx1] = '\0';
            remain_data = 1;
            break;
        }

    // 找到 '=', 分離資料名稱和資料數值.
    for(didx2 = 0; didx2 < didx1; didx2++)
        if(data_con[didx2] == MAAM_CONTENT_VALUE_SPLIT_KEY)
        {
            data_con[didx2] = '\0';
            break;
        }

    *data_name_buf = didx2 < didx1 ? data_con : data_con + didx1;
    *data_value_buf = didx2 < didx1 ? data_con + didx2 + 1 : data_con;

    // 回傳剩下的內容.
    didx1 += remain_data != 0 ? 1 : 0;
    *req_content_con = data_con + didx1;
    *req_content_len = data_len - didx1;

    return 0;
}

// 取出 cookie 內的資料 (主要找到 session_key).
static int maam_parse_cookie(
    char **req_cookie_con,
    size_t *req_cookie_len,
    char **data_name_buf,
    char **data_value_buf)
{
    char *data_con, *tmp_name, *tmp_value;
    size_t data_len, didx1, didx2;
    int remain_data = 0;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    data_con = *req_cookie_con;
    data_len = *req_cookie_len;

    if(data_len <= 0)
        return -1;

    // 找到 ';', 分離參數.
    for(didx1 = 0; didx1 < data_len; didx1++)
        if(data_con[didx1] == MAAM_COOKIE_PARAMETER_SPLIT_KEY)
        {
            data_con[didx1] = '\0';
            remain_data = 1;
            break;
        }

    // 找到 '=', 分離資料名稱和資料數值.
    for(didx2 = 0; didx2 < didx1; didx2++)
        if(data_con[didx2] == MAAM_COOKIE_VALUE_SPLIT_KEY)
        {
            data_con[didx2] = '\0';
            break;
        }

    // 剔除前面的空格.
    tmp_name = didx2 < didx1 ? data_con : data_con + didx1;
    for(; (*tmp_name == MAAM_COOKIE_SPACE_KEY) || (*tmp_name == '\0'); tmp_name++);
    // 剔除前面的空格.
    tmp_value = didx2 < didx1 ? data_con + didx2 + 1 : data_con;
    for(; (*tmp_value == MAAM_COOKIE_SPACE_KEY) || (*tmp_value == '\0'); tmp_value++);


    *data_name_buf = tmp_name;
    *data_value_buf = tmp_value;

    // 回傳剩下的內容.
    didx1 += remain_data != 0 ? 1 : 0;
    *req_cookie_con = data_con + didx1;
    *req_cookie_len = data_len - didx1;

    return 0;
}

// 找出登入要用的資料.
static int maam_get_login_para(
    struct maam_req_data_t *req_data_info,
    struct maam_login_t *login_info)
{
    struct maam_para_t *each_para;
    char *tmp_name, *tmp_value;
    unsigned int aidx;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    while(1)
    {
        if(maam_parse_content(&req_data_info->req_con, &req_data_info->req_len,
                              &tmp_name, &tmp_value) < 0)
        {
            break;
        }
        MAAM_BDMSG("parse-content [%s][%s]", tmp_name, tmp_value);

        // 比對資料的名稱.
        for(aidx = 0; maam_login_para_list[aidx].data_name != NULL; aidx++)
        {
            each_para = maam_login_para_list + aidx;
            if(each_para->data_value == NULL)
                if(strcmp(each_para->data_name, tmp_name) == 0)
                {
                    // 紀錄資料數值.
                    each_para->data_value = tmp_value;
                    break;
                }
        }
    }
    for(aidx = 0; maam_login_para_list[aidx].data_name != NULL; aidx++)
    {
        each_para = maam_login_para_list + aidx;
        if(each_para->data_value == NULL)
        {
            MAAM_EMSG("miss login data [%s]", each_para->data_name);
            return each_para->miss_error_code;
        }
    }

    // 取得 verify_name.
    login_info->verify_name =
        maam_login_para_list[MAAM_LOGIN_VERIFY_NAME_INDEX].data_value;
    // 取得 verify_password.
    login_info->verify_password =
        maam_login_para_list[MAAM_LOGIN_VERIFY_PASSWORD_INDEX].data_value;
    // 取得 multiple_user_active_rule.
    login_info->multiple_user_active_rule =
        strtol(maam_login_para_list[MAAM_LOGIN_MULTIPLE_USER_ACTIVE_RULE_INDEX].data_value,
               NULL, 10);
    // 取得 same_accout_multiple_active_rule.
    login_info->same_accout_multiple_active_rule =
        strtol(maam_login_para_list[MAAM_LOGIN_SAME_ACCOUT_MULTIPLE_ACTIVE_RULE_INDEX].data_value,
               NULL, 10);

    login_info->client_addr = req_data_info->req_addr;

    return MAAM_RCODE_PASS;
}

// 找出 cookie 內要的資料.
static int maam_get_cookie_para(
    struct maam_req_data_t *req_data_info,
    struct maam_cookie_t *cookie_data_buf)
{
    int fret = MAAM_RCODE_PASS;
    struct maam_para_t *each_para;
    char *cookie_buf = NULL, *cookie_con, *tmp_name, *tmp_value;
    size_t cookie_len;
    unsigned int aidx;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    // 會修改到 cookie 的內容, 複製一份避免改到原本的影響到其他程式使用.
    cookie_len = strlen(req_data_info->req_cookie) + 1;
    cookie_buf = malloc(cookie_len);
    if(cookie_buf == NULL)
    {
        MAAM_EMSG("call malloc() fail [%s]", strerror(errno));
        fret = MAAM_RCODE_INTERNAL_ERROR;
        goto FREE_01;
    }
    memcpy(cookie_buf, req_data_info->req_cookie, cookie_len);
    cookie_con = cookie_buf;
    cookie_len--;

    while(1)
    {
        if(maam_parse_cookie(&cookie_con, &cookie_len, &tmp_name, &tmp_value) < 0)
            break;
        MAAM_BDMSG("parse-cookie [%s][%s]", tmp_name, tmp_value);

        // 比對資料的名稱.
        for(aidx = 0; maam_cookie_para_list[aidx].data_name != NULL; aidx++)
        {
            each_para = maam_cookie_para_list + aidx;
            if(each_para->data_value == NULL)
                if(strcmp(each_para->data_name, tmp_name) == 0)
                {
                    // 紀錄資料數值.
                    each_para->data_value = tmp_value;
                    break;
                }
        }
    }
    for(aidx = 0; maam_cookie_para_list[aidx].data_name != NULL; aidx++)
    {
        each_para = maam_cookie_para_list + aidx;
        if(each_para->data_value == NULL)
        {
            MAAM_EMSG("miss cookie data [%s]", each_para->data_name);
            fret = each_para->miss_error_code;
            goto FREE_02;
        }
    }

    // 取得 session_key.
    cookie_data_buf->session_key =
        maam_cookie_para_list[MAAM_COOKIE_SESSION_KEY_INDEX].data_value;

FREE_02:
    free(cookie_buf);
FREE_01:
    return fret;
}

// 增加 session.
static int maam_add_session(
    struct maam_auth_sys_t *auth_sys_info,
    struct maam_session_t *session_info)
{
    struct maam_session_t *each_session;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    MAAM_BDMSG("add session [%s][%s]", session_info->account_name, session_info->session_key);

    if(auth_sys_info->session_usable == -1)
    {
        MAAM_EMSG("session table full");
        return MAAM_RCODE_SESSION_TABLE_FULL;
    }

    each_session = auth_sys_info->session_info + auth_sys_info->session_usable;
    MAAM_BDMSG("alloc [%d]", each_session->session_index);

    auth_sys_info->session_usable =
        auth_sys_info->session_info[each_session->session_index].empty_session;

    session_info->session_index = each_session->session_index;
    memcpy(each_session, session_info, sizeof(struct maam_session_t));

    each_session->prev_session = each_session->next_session = each_session->empty_session = -1;

    if(auth_sys_info->session_head == -1)
    {
        auth_sys_info->session_head = auth_sys_info->session_tail = each_session->session_index;
        MAAM_BDMSG("link [head %d][tail %d]",
                   auth_sys_info->session_head, auth_sys_info->session_tail);
    }
    else
    {
        each_session->prev_session = auth_sys_info->session_tail;
        auth_sys_info->session_info[auth_sys_info->session_tail].next_session =
            each_session->session_index;
        MAAM_BDMSG("link [neighbor %d][->%d]",
                   auth_sys_info->session_tail,
                   auth_sys_info->session_info[auth_sys_info->session_tail].next_session);
        MAAM_BDMSG("link [tail %d>>%d]",
                   auth_sys_info->session_tail, each_session->session_index); 
        auth_sys_info->session_tail = each_session->session_index;
    }

    MAAM_BDMSG("link [self %d][%d<-][->%d]",
               each_session->session_index, each_session->prev_session, each_session->next_session);

    MAAM_BDMSG("count [%d>>%d]", auth_sys_info->session_count, auth_sys_info->session_count + 1);
    auth_sys_info->session_count++;

    return MAAM_RCODE_PASS;
}

// 刪除 session.
static int maam_del_session(
    struct maam_auth_sys_t *auth_sys_info,
    struct maam_session_t *session_info)
{
    MAAM_BDMSG("=> %s", __FUNCTION__);

    MAAM_BDMSG("del session [%s][%s]", session_info->account_name, session_info->session_key);

    MAAM_BDMSG("link [self %d][%d<-][->%d]",
               session_info->session_index, session_info->prev_session, session_info->next_session);

    if(session_info->prev_session != -1)
    {
        MAAM_BDMSG("link [neighbor %d][->%d>>%d]",
                   session_info->prev_session,
                   auth_sys_info->session_info[session_info->prev_session].next_session,
                   session_info->next_session);
        auth_sys_info->session_info[session_info->prev_session].next_session =
            session_info->next_session;
    }

    if(session_info->next_session != -1)
    {
        MAAM_BDMSG("link [neighbor %d][%d>>%d<-]",
                   session_info->next_session,
                   auth_sys_info->session_info[session_info->next_session].prev_session,
                   session_info->prev_session);
        auth_sys_info->session_info[session_info->next_session].prev_session =
            session_info->prev_session;
    }

    if(auth_sys_info->session_head == session_info->session_index)
    {
        MAAM_BDMSG("link [head %d>>%d]", auth_sys_info->session_head, session_info->next_session);
        auth_sys_info->session_head = session_info->next_session;
    }

    if(auth_sys_info->session_tail == session_info->session_index)
    {
        MAAM_BDMSG("link [tail %d>>%d]", auth_sys_info->session_tail, session_info->prev_session);
        auth_sys_info->session_tail = session_info->prev_session;
    }

    MAAM_BDMSG("free [%d]", session_info->session_index);
    session_info->empty_session = auth_sys_info->session_usable;
    auth_sys_info->session_usable = session_info->session_index;

    MAAM_BDMSG("count [%d>>%d]", auth_sys_info->session_count, auth_sys_info->session_count - 1);
    auth_sys_info->session_count--;

    return MAAM_RCODE_PASS;
}

// 尋找 session.
static int maam_compare_session(
    struct maam_auth_sys_t *auth_sys_info,
    int compare_method,
    void *compare_data,
    struct maam_session_t **session_buf)
{
    struct maam_session_t *each_session = NULL;
    int each_index;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    for(each_index = auth_sys_info->session_head;
        each_index != -1;
        each_index = each_session->next_session)
    {
        each_session = auth_sys_info->session_info + each_index;

        // 找到帳號名稱和 compare_data 相同的.
        if(compare_method == MAAM_CSESSION_EQUAL_ACCOUNT_NAME)
        {
            if(strcmp(each_session->account_name, (char *) compare_data) == 0)
                break;
        }
        else
        // 找到 session_key 和 compare_data 相同的.
        if(compare_method == MAAM_CSESSION_EQUAL_SESSION_KEY)
        {
            if(strcmp(each_session->session_key, compare_data) == 0)
                break;
        }
        else
        // 找到權限高於等於 compare_data 的.
        if(compare_method == MAAM_CSESSION_HEIGHT_EQUAL_ACCOUNT_PERMISSION)
        {
            if(MAAM_PERMISSION_HEIGHT_EQUAL(each_session->account_permission,
                                            *((int *) compare_data)))
            {
                break;
            }
        }
    }

    if(session_buf != NULL)
        *session_buf = each_index == -1 ? NULL : each_session;

    return each_index == -1 ? -1 : 0;
}

// 刪除所有的 session.
static int maam_clear_all(
    struct maam_auth_sys_t *auth_sys_info)
{
    int fret = MAAM_RCODE_PASS, each_index, next_index;
    struct maam_session_t *each_session;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    for(each_index = auth_sys_info->session_head, next_index = -1; each_index != -1;
        each_index = next_index)
    {
        each_session = auth_sys_info->session_info + each_index;
        next_index = each_session->next_session;

        fret = maam_del_session(auth_sys_info, each_session);
        if(fret < MAAM_RCODE_PASS)
        {
            MAAM_EMSG("call maam_del_session() fail");
            return fret;
        }
    }

    return fret;
}

// 刪除閒置過久的 session.
static int maam_clear_idle(
    struct maam_auth_sys_t *auth_sys_info)
{
    int fret = MAAM_RCODE_PASS, each_index, next_index;
    struct sysinfo sys_info;
    struct maam_session_t *each_session;
    long interval_uptime;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    // 取得系統 uptime.
    if(sysinfo(&sys_info) == -1)
    {
        MAAM_EMSG("call sysinfo() fail [%s]", strerror(errno));
        return MAAM_RCODE_INTERNAL_ERROR;
    }

    for(each_index = auth_sys_info->session_head, next_index = -1; each_index != -1;
        each_index = next_index)
    {
        each_session = auth_sys_info->session_info + each_index;
        next_index = each_session->next_session;

        // 計算最後活動到現在經過的時間差.
        MAAM_CALCULATE_INTERVAL(interval_uptime, sys_info.uptime, each_session->last_access_uptime,
                                auth_sys_info->max_uptime);

        MAAM_BDMSG("clear idle-timeout [%s][%s][%ld/%ld][%s]",
                   each_session->account_name, each_session->session_key,
                   interval_uptime, each_session->idle_timeout,
                   interval_uptime > each_session->idle_timeout ? "timeout" : "keep");

        // 比對閒置時間是否超過 idle_timeout.
        if(interval_uptime > each_session->idle_timeout)
        {
            fret = maam_del_session(auth_sys_info, each_session);
            if(fret < MAAM_RCODE_PASS)
            {
                MAAM_EMSG("call maam_del_session() fail");
                return fret;
            }
        }
    }

    return fret;
}

// 處理禁止同時超過一個帳號登入系統.
static int maam_check_multiple_user_active(
    struct maam_auth_sys_t *auth_sys_info,
    struct maam_login_t *login_info,
    int *check_result_buf)
{
    int fret = MAAM_RCODE_PASS, cret = MAAM_RCODE_PASS, tmp_code;
    struct maam_session_t *each_session;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    if(auth_sys_info->session_count > 0)
    {
        if(login_info->multiple_user_active_rule == MAAM_LMULTIPLE_DENY_FORCE_KICK_INSIDE)
        {
            MAAM_BDMSG("deny multiple user login (force kick inside)");
            fret = maam_clear_all(auth_sys_info);
            if(fret < MAAM_RCODE_PASS)
            {
                MAAM_EMSG("call maam_clear_all() fail");
                return fret;
            }
        }
        else
        if(login_info->multiple_user_active_rule == MAAM_LMULTIPLE_DENY_FORCE_KICK_OUTSIDE)
        {
            MAAM_BDMSG("deny multiple user login (force kick outside)");
            cret = MAAM_RCODE_OTHER_USER_HAS_LOGIN;
        }
        else
        {
            // 使用權限處理, 找出權限高於等於登入中的帳號的.
            tmp_code = maam_compare_session(auth_sys_info,
                                            MAAM_CSESSION_HEIGHT_EQUAL_ACCOUNT_PERMISSION,
                                            &login_info->account_permission, &each_session);
            // 有找到.
            if(tmp_code == 0)
            {
                // 如果權限一樣.
                if(MAAM_PERMISSION_EQUAL(login_info->account_permission,
                                         each_session->account_permission))
                {
                    if(login_info->multiple_user_active_rule ==
                       MAAM_LMULTIPLE_DENY_PERMISSION_KICK_INSIDE)
                    {
                        MAAM_BDMSG("deny multiple user login (equal priority kick inside)");
                        fret = maam_clear_all(auth_sys_info);
                        if(fret < MAAM_RCODE_PASS)
                        {
                            MAAM_EMSG("call maam_clear_all() fail");
                            return fret;
                        }
                    }
                    else
                    if(login_info->multiple_user_active_rule ==
                       MAAM_LMULTIPLE_DENY_PERMISSION_KICK_OUTSIDE)
                    {
                        MAAM_BDMSG("deny multiple user login (equal priority kick outside)");
                        cret = MAAM_RCODE_OTHER_USER_HAS_LOGIN;
                    }
                }
                // 如果有 session 權限高於登入中的帳號.
                else
                {
                    MAAM_BDMSG("deny multiple user login (low priority kick outside)");
                    cret = MAAM_RCODE_OTHER_USER_HAS_LOGIN;
                }
            }
            // 沒找到, 所有 session 的權限都低於登入中的帳號.
            else
            {
                MAAM_BDMSG("deny multiple user login (height priority kick inside)");
                fret = maam_clear_all(auth_sys_info);
                if(fret < MAAM_RCODE_PASS)
                {
                    MAAM_EMSG("call maam_clear_all() fail");
                    return fret;
                }
            }
        }
    }

    *check_result_buf = cret;

    return fret;
}

// 處理禁止同一個帳號被多個使用者同時登入.
static int maam_check_same_accout_multiple_active(
    struct maam_auth_sys_t *auth_sys_info,
    struct maam_login_t *login_info,
    int *check_result_buf)
{
    int fret = MAAM_RCODE_PASS, cret = MAAM_RCODE_PASS, tmp_code;
    struct maam_session_t *each_session;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    do
    {
        // 找到相同帳號名稱的 session.
        tmp_code = maam_compare_session(auth_sys_info, MAAM_CSESSION_EQUAL_ACCOUNT_NAME,
                                        login_info->account_name, &each_session);
        if(tmp_code == 0)
        {
            if(login_info->same_accout_multiple_active_rule ==
               MAAM_LMULTIPLE_DENY_FORCE_KICK_INSIDE)
            {
                MAAM_BDMSG("deny user multiple login (force kick inside)");
                fret = maam_del_session(auth_sys_info, each_session);
                if(fret < MAAM_RCODE_PASS)
                {
                    MAAM_EMSG("call maam_del_session() fail");
                    return fret;
                }
            }
            else
            if(login_info->same_accout_multiple_active_rule ==
               MAAM_LMULTIPLE_DENY_FORCE_KICK_OUTSIDE)
            {
                MAAM_BDMSG("deny user multiple login (force kick outside)");
                cret = MAAM_RCODE_SAME_ACCOUNT_HAS_LOGIN;
                break;
            }
            else
            {
                if(MAAM_PERMISSION_EQUAL(login_info->account_permission,
                                         each_session->account_permission))
                {
                    if(login_info->same_accout_multiple_active_rule ==
                       MAAM_LMULTIPLE_DENY_PERMISSION_KICK_INSIDE)
                    {
                        MAAM_BDMSG("deny user multiple login (equal priority kick inside)");
                        fret = maam_del_session(auth_sys_info, each_session);
                        if(fret < MAAM_RCODE_PASS)
                        {
                            MAAM_EMSG("call maam_del_session() fail");
                            return fret;
                        }
                    }
                    else
                    if(login_info->same_accout_multiple_active_rule ==
                       MAAM_LMULTIPLE_DENY_PERMISSION_KICK_OUTSIDE)
                    {
                        MAAM_BDMSG("deny user multiple login (equal priority kick outside)");
                        cret = MAAM_RCODE_SAME_ACCOUNT_HAS_LOGIN;
                        break;
                    }
                }
                else
                if(MAAM_PERMISSION_LOW(login_info->account_permission,
                                       each_session->account_permission))
                {
                    MAAM_BDMSG("deny user multiple login (low priority kick outside)");
                    cret = MAAM_RCODE_SAME_ACCOUNT_HAS_LOGIN;
                    break;
                }
                else
                {
                    MAAM_BDMSG("deny user multiple login (height priority kick inside)");
                    fret = maam_del_session(auth_sys_info, each_session);
                    if(fret < MAAM_RCODE_PASS)
                    {
                        MAAM_EMSG("call maam_del_session() fail");
                        return fret;
                    }
                }
            }
        }
    }
    while(tmp_code == 0);

    *check_result_buf = cret;

    return fret;
}

// 檢查 session 是否閒置超過限定時間.
static int maam_check_idle(
    struct maam_auth_sys_t *auth_sys_info,
    struct maam_session_t *session_info,
    int *timeout_result_buf)
{
    struct sysinfo sys_info;
    long interval_uptime;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    if(sysinfo(&sys_info) == -1)
    {
        MAAM_EMSG("call sysinfo() fail [%s]", strerror(errno));
        return MAAM_RCODE_INTERNAL_ERROR;
    }

    MAAM_CALCULATE_INTERVAL(interval_uptime, sys_info.uptime, session_info->last_access_uptime,
                            auth_sys_info->max_uptime);

    MAAM_BDMSG("check idle-timeout [%s][%s][%ld/%ld][%s]",
               session_info->account_name, session_info->session_key,
               interval_uptime, session_info->idle_timeout,
               interval_uptime > session_info->idle_timeout ? "timeout" : "keep");

    if(interval_uptime > session_info->idle_timeout)
    {
        *timeout_result_buf = MAAM_RCODE_IDLE_TIMEOUT;
    }
    else
    {
        session_info->last_access_uptime = sys_info.uptime;
        *timeout_result_buf = MAAM_RCODE_PASS;
    }

    return MAAM_RCODE_PASS;
}

// 比對路徑和比對查詢.
static int maam_match_path_query(
    char *path_con,
    size_t path_len,
    char *query_con,
    size_t query_len,
    struct maam_access_t *access_info)
{
    char *data_con, *cmp_con;
    size_t data_len, cmp_len;
    int cmp_method, cmp_step, cmp_ret = -1;


    for(cmp_step = 0; cmp_step < 2; cmp_step++)
    {
        if(cmp_step == 0)
        {
            data_con = path_con;
            data_len = path_len;
            cmp_con = access_info->cmp_path_con;
            cmp_len = access_info->cmp_path_len;
            cmp_method = access_info->cmp_path_method;
        }
        else
        if(cmp_step == 1)
        {
            if(access_info->cmp_query_con == NULL)
                break;
            data_con = query_con;
            data_len = query_len;
            cmp_con = access_info->cmp_query_con;
            cmp_len = access_info->cmp_query_len;
            cmp_method = access_info->cmp_query_method;
        }

        cmp_ret = -1;
        if(cmp_method == MAAM_CACCESS_NONE)
        {
            cmp_ret = 0;
            MAAM_BDMSG("[%s][%s][%s][%d]", "none", data_con, cmp_con, cmp_ret);
        }
        else
        if(cmp_method == MAAM_CACCESS_ALL)
        {
            if(strcmp(data_con, cmp_con) == 0)
                cmp_ret = 0;
            MAAM_BDMSG("[%s][%s][%s][%d]", "all", data_con, cmp_con, cmp_ret);
        }
        else
        if(cmp_method == MAAM_CACCESS_PREFIX)
        {
            if(data_len >= cmp_len)
                if(strncmp(data_con, cmp_con, cmp_len) == 0)
                    cmp_ret = 0;
            MAAM_BDMSG("[%s][%s][%s][%d]", "prefix", data_con, cmp_con, cmp_ret);
        }
        else
        if(cmp_method == MAAM_CACCESS_POSTFIX)
        {
            if(data_len >= cmp_len)
                if(strncmp(data_con + (data_len - cmp_len), cmp_con, cmp_len) == 0)
                    cmp_ret = 0;
            MAAM_BDMSG("[%s][%s][%s][%d]", "postfix", data_con, cmp_con, cmp_ret);
        }
        else
        if(cmp_method == MAAM_CACCESS_SUB)
        {
            if(data_len >= cmp_len)
                if(strstr(data_con, cmp_con) != NULL)
                    cmp_ret = 0;
            MAAM_BDMSG("[%s][%s][%s][%d]", "sub", data_con, cmp_con, cmp_ret);
        }

        if(cmp_ret != 0)
            break;
    }

    return cmp_ret;
}

// 檢查要存取的路徑是否允許.
static int maam_check_access(
    char *path_con,
    char *query_con,
    int access_method)
{
    size_t plen, qlen;
    struct maam_access_t *each_access;
    int is_html = 0;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    plen = strlen(path_con);
    qlen = strlen(query_con);

    if(access_method == METHOD_GET)
    {
        MAAM_BDMSG("get [%s]", path_con);

        if(plen > maam_html_len)
            if(strncmp(path_con + (plen - maam_html_len), maam_html_key, maam_html_len) == 0)
                is_html = 1;

        // 檢查方式 :
        // 對於網頁檔,
        //   登入頁面或其他指定的頁面可以不用登入也能存取,
        //   存取其他需要登入的頁面導向到登入頁面.
        // 對於非網頁檔,
        //   除了指定的檔案都是不用登入也能存取,
        //   存取需要登入才能存取的檔案回傳 HTPP 404.

        // 如果是網頁檔.
        if(is_html != 0)
        {
            // 是否是登入頁面或其他指定的, 可以不用登入也能存取.
            for(each_access = maam_access_allow_get_html_list;
                each_access->cmp_path_con != NULL;
                each_access++)
            {
                if(maam_match_path_query(path_con, plen, query_con, qlen, each_access) == 0)
                {
                    MAAM_BDMSG("match allow get html [%s][%s], allow",
                               path_con, each_access->cmp_path_con);
                    return MAAM_PACCESS_ALLOW;
                }
            }

            // 不是登入頁面或其他指定的.
            MAAM_BDMSG("deny get html [%s][%s], redirect", path_con, maam_html_key);
            return MAAM_PACCESS_REDIRECT;
        }
        // 不是網頁檔.
        else
        {
            // 是否是指定要登入才能存取的.
            for(each_access = maam_access_deny_get_other_list;
                each_access->cmp_path_con != NULL;
                each_access++)
            {
                if(maam_match_path_query(path_con, plen, query_con, qlen, each_access) == 0)
                {
                    MAAM_BDMSG("match deny get other [%s][%s], lose",
                               path_con, each_access->cmp_path_con);
                    return MAAM_PACCESS_LOSE;
                }
            }

            MAAM_BDMSG("alow get other [%s], allow", path_con);
            return 0;
        }
    }
    else
    if(access_method == METHOD_POST)
    {
        MAAM_BDMSG("post [%s]", path_con);

        // 如果是 POST, 除了指定的可以不用登入也能存取, 其他都需要登入才能存取.
        for(each_access = maam_access_allow_post_list;
            each_access->cmp_path_con != NULL;
            each_access++)
        {
            if(maam_match_path_query(path_con, plen, query_con, qlen, each_access) == 0)
            {
                MAAM_BDMSG("match allow post [%s][%s], allow", path_con, each_access->cmp_path_con);
                return MAAM_PACCESS_ALLOW;
            }
        }

        MAAM_BDMSG("deny post [%s], redirect", path_con);
        return MAAM_PACCESS_REDIRECT;
    }

    return MAAM_PACCESS_LOSE;
}

// 填充 session 內容.
static int maam_fill_session(
    struct maam_auth_sys_t *auth_sys_info,
    struct maam_login_t *login_info,
    struct maam_session_t *session_info)
{
    struct sysinfo sys_info;
    int kidx, klen, kcode;
#if MAAM_BDMODE
    char dbg_haddr[64];
#endif


    MAAM_BDMSG("=> %s", __FUNCTION__);

    srand(time(NULL));

    if(sysinfo(&sys_info) == -1)
    {
        MAAM_EMSG("call sysinfo() fail [%s]", strerror(errno));
        return MAAM_RCODE_INTERNAL_ERROR;
    }

    memcpy(&session_info->client_addr, login_info->client_addr, sizeof(session_info->client_addr));
#if MAAM_BDMODE
    MAAM_IP_NTOH(session_info->client_addr, dbg_haddr, sizeof(dbg_haddr));
    MAAM_BDMSG("client_addr = %s", dbg_haddr);
#endif

    snprintf(session_info->account_name, sizeof(session_info->account_name), "%s",
             login_info->account_name);
    MAAM_BDMSG("account_name = %s", session_info->account_name);

    session_info->account_permission = login_info->account_permission;
    MAAM_BDMSG("account_permission = %d", session_info->account_permission);

    // 計算 session_key.
    klen = sizeof(session_info->session_key) - 1;
    while(1)
    {
        // 亂數產生 0 ~ 9, A ~ Z, a ~ z 組合的字串.
        for(kidx = 0; kidx < klen; kidx++)
        {
            kcode = rand() % 62;
            if(kcode < 10)
                kcode = kcode + 48;
            else
            if((10 <= kcode) && (kcode < 36))
                kcode = (kcode - 10) + 65;
            else
            if((36 <= kcode) && (kcode < 62))
                kcode = (kcode - 36) + 97;
            session_info->session_key[kidx] = kcode & 0xFF;
        }
        session_info->session_key[kidx] = '\0';

        // 如果有重複, 重新產生.
        if(maam_compare_session(auth_sys_info, MAAM_CSESSION_EQUAL_SESSION_KEY,
                                session_info->session_key, NULL) < 0)
        {
            break;
        }
    }
    MAAM_BDMSG("session_key = %s", session_info->session_key);

    session_info->idle_timeout = login_info->account_idle_timeout;
    MAAM_BDMSG("idle_timeout = %ld", session_info->idle_timeout);

    session_info->create_uptime = sys_info.uptime;
    MAAM_BDMSG("create_uptime = %ld", session_info->create_uptime);

    session_info->last_access_uptime = sys_info.uptime;
    MAAM_BDMSG("last_access_uptime = %ld", session_info->last_access_uptime);

    return MAAM_RCODE_PASS;
}

// 填充回應的 cookie.
static int maam_fill_cookie(
    int fill_method,
    struct maam_session_t *session_info,
    struct maam_rep_data_t *rep_data_info)
{
    size_t cidx, mlen;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    // cookie 的數目不可超過 MAAM_REP_COOKIE_MAX_COUNT.

    // cookie, 填入 session_key.
    cidx = 0;
    mlen = 0;
    MAAM_FILL_STRING(rep_data_info->rep_cookie_buf[cidx], MAAM_BSIZE_REP_COOKIE, mlen,
                     "%s%c%s"   // session_key=$(session_key)
                     "%c%c"     // ;
                     "%s%c%s",  // path=/
                     MAAM_COOKIE_SESSION_KEY_KEY, MAAM_COOKIE_VALUE_SPLIT_KEY,
                     fill_method == MAAM_FCOOKIE_ADD ?
                     session_info->session_key : MAAM_COOKIE_EMPTY_VALUE_KEY,
                     MAAM_COOKIE_PARAMETER_SPLIT_KEY, MAAM_COOKIE_SPACE_KEY,
                     MAAM_COOKIE_PATH_NAME_KEY, MAAM_COOKIE_VALUE_SPLIT_KEY,
                     MAAM_COOKIE_PATH_ROOT_KEY);
    MAAM_BDMSG("fill-cookie [%s]", rep_data_info->rep_cookie_buf[cidx]);

    // cookie, 填入 account_name.
    cidx++;
    mlen = 0;
    if(fill_method == MAAM_FCOOKIE_ADD)
    {
        MAAM_FILL_STRING(rep_data_info->rep_cookie_buf[cidx], MAAM_BSIZE_REP_COOKIE, mlen,
                         "%s%c%s",  // account_name=$(account_name)
                         MAAM_COOKIE_ACCOUNT_NAME_KEY, MAAM_COOKIE_VALUE_SPLIT_KEY,
                         session_info->account_name);
    }
    else
    {
        MAAM_FILL_STRING(rep_data_info->rep_cookie_buf[cidx], MAAM_BSIZE_REP_COOKIE, mlen,
                         "%s%c%s",  // account_name=!
                         MAAM_COOKIE_ACCOUNT_NAME_KEY, MAAM_COOKIE_VALUE_SPLIT_KEY,
                         MAAM_COOKIE_EMPTY_VALUE_KEY);
    }
    MAAM_FILL_STRING(rep_data_info->rep_cookie_buf[cidx], MAAM_BSIZE_REP_COOKIE, mlen,
                     "%c%c"        // ;
                     "%s%c%s",     // path=/
                     MAAM_COOKIE_PARAMETER_SPLIT_KEY, MAAM_COOKIE_SPACE_KEY,
                     MAAM_COOKIE_PATH_NAME_KEY, MAAM_COOKIE_VALUE_SPLIT_KEY,
                     MAAM_COOKIE_PATH_ROOT_KEY);
    MAAM_BDMSG("fill-cookie [%s]", rep_data_info->rep_cookie_buf[cidx]);

    // cookie, 填入 account_permission.
    cidx++;
    mlen = 0;
    if(fill_method == MAAM_FCOOKIE_ADD)
    {
        MAAM_FILL_STRING(rep_data_info->rep_cookie_buf[cidx], MAAM_BSIZE_REP_COOKIE, mlen,
                         "%s%c%d",  // account_permission=$(account_permission)
                         MAAM_COOKIE_ACCOUNT_PERMISSION_KEY, MAAM_COOKIE_VALUE_SPLIT_KEY,
                         session_info->account_permission);
    }
    else
    {
        MAAM_FILL_STRING(rep_data_info->rep_cookie_buf[cidx], MAAM_BSIZE_REP_COOKIE, mlen,
                         "%s%c%s",  // account_permission=!
                         MAAM_COOKIE_ACCOUNT_PERMISSION_KEY, MAAM_COOKIE_VALUE_SPLIT_KEY,
                         MAAM_COOKIE_EMPTY_VALUE_KEY);
    }
    MAAM_FILL_STRING(rep_data_info->rep_cookie_buf[cidx], MAAM_BSIZE_REP_COOKIE, mlen,
                     "%c%c"        // ;
                     "%s%c%s",     // path=/
                     MAAM_COOKIE_PARAMETER_SPLIT_KEY, MAAM_COOKIE_SPACE_KEY,
                     MAAM_COOKIE_PATH_NAME_KEY, MAAM_COOKIE_VALUE_SPLIT_KEY,
                     MAAM_COOKIE_PATH_ROOT_KEY);
    MAAM_BDMSG("fill-cookie [%s]", rep_data_info->rep_cookie_buf[cidx]);

    return MAAM_RCODE_PASS;
}

// 填充使用 GET 時的頁面導向處理.
static int maam_fill_redirect_get(
    char *redirect_page,
    struct maam_rep_data_t *rep_data_info)
{
    size_t mlen = 0;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    // 存取要登入才能存取的非網頁檔案, 回傳 HTTP 404.
    if(rep_data_info->rep_handle == MAAM_RHANDLE_LOSE)
    {
                snprintf(rep_data_info->rep_body_buf, sizeof(rep_data_info->rep_body_buf),
                         "<html>"
                         "<head></head>"
                         "<body>File Not Found</body>"
                         "</html>");
    }
    else
    // 使用 script 方式, 填入空頁面並使用 javascript 導向.
    // 不使用 HTTP 302 模式是避免網頁端使用 frame 架構時只有子框架導向.
    if(rep_data_info->rep_handle == MAAM_RHANDLE_SCRIPT)
    {
        MAAM_FILL_STRING(rep_data_info->rep_body_buf, sizeof(rep_data_info->rep_body_buf), mlen,
                         "<html>"
                         "<head>"
                         "<script type=\"text/javascript\">"
                         "window.top.location.href=\"%s\";"
                         "</script>"
                         "</head>"
                         "<body></body>"
                         "</html>",
                         redirect_page);
    }
    else
    // 使用 rewrite 方式, mini_httpd 會改讀取指定的網頁內容.
    if(rep_data_info->rep_handle == MAAM_RHANDLE_REWRITE)
    {
        MAAM_FILL_STRING(rep_data_info->rep_body_buf, sizeof(rep_data_info->rep_body_buf), mlen,
                         "%s", redirect_page);
    }
    MAAM_BDMSG("fill-body-get [%s][%s]",
               rep_data_info->rep_handle == MAAM_RHANDLE_SCRIPT ? "script" : "rewrite",
               rep_data_info->rep_body_buf);

    return MAAM_RCODE_PASS;
}

// 填充使用 POST 時的頁面導向處理.
static int maam_fill_redirect_post(
    int result_code,
    char *redirect_page,
    struct maam_rep_data_t *rep_data_info)
{
    size_t mlen = 0;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    // 使用 script 方式, 在 登入成功, 登出, 一般 POST (閒置超時) 會使用此方法.
    if(rep_data_info->rep_handle == MAAM_RHANDLE_SCRIPT)
    {
        MAAM_FILL_STRING(rep_data_info->rep_body_buf, sizeof(rep_data_info->rep_body_buf), mlen,
                         "%d:window.top.location.href=\"%s\";", result_code, redirect_page);
    }
    else
    // 使用 message 方式, 回傳代碼.
    if(rep_data_info->rep_handle == MAAM_RHANDLE_MESSAGE)
    {
        MAAM_FILL_STRING(rep_data_info->rep_body_buf, sizeof(rep_data_info->rep_body_buf), mlen,
                         "%d", result_code);
    }
    MAAM_BDMSG("fill-body-post [%s][%s]",
               rep_data_info->rep_handle == MAAM_RHANDLE_SCRIPT ? "script" : "message",
               rep_data_info->rep_body_buf);

    return MAAM_RCODE_PASS;
}

// 取得 share memory 使用權.
static int maam_auth_sys_wait(
    int sm_id,
    char *sm_mutex_path,
    struct maam_auth_sys_t **auth_sys_info_buf,
    int *auth_sys_mutex_buf)
{
    int fret = MAAM_RCODE_PASS, file_fd;
    void *sm_addr;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    sm_addr = shmat(sm_id, NULL, 0);
    if(sm_addr == (void *) -1)
    {
        MAAM_EMSG("call shmat() fail [%s]", strerror(errno));
        fret = MAAM_RCODE_INTERNAL_ERROR;
        goto FREE_01;
    }

    file_fd = open(sm_mutex_path, O_RDWR);
    if(file_fd == -1)
    {
        MAAM_EMSG("call open(%s) fail [%s]", sm_mutex_path, strerror(errno));
        fret = MAAM_RCODE_INTERNAL_ERROR;
        goto FREE_02;
    }

    if(flock(file_fd, LOCK_EX) == -1)
    {
        MAAM_EMSG("call flock() fail [%s]", strerror(errno));
        fret = MAAM_RCODE_INTERNAL_ERROR;
        goto FREE_03;
    }

    *auth_sys_mutex_buf = file_fd;
    *auth_sys_info_buf = (struct maam_auth_sys_t *) sm_addr;

    return MAAM_RCODE_PASS;
FREE_03:
    close(file_fd);
FREE_02:
    shmdt(sm_addr);
FREE_01:
    return fret;
}

// 釋放 share memory 使用權.
static int maam_auth_sys_post(
    struct maam_auth_sys_t *auth_sys_info,
    int auth_sys_mutex)
{
    MAAM_BDMSG("=> %s", __FUNCTION__);


    flock(auth_sys_mutex, LOCK_UN);
    close(auth_sys_mutex);
    shmdt(auth_sys_info);

    return MAAM_RCODE_PASS;
}

// 登入.
static int maam_process_login(
    char *sm_mutex_path,
    int sm_id,
    struct maam_req_data_t *req_data_info,
    struct maam_rep_data_t *rep_data_info)
{
    int fret, cret = MAAM_RCODE_PASS, auth_sys_mutex, tmp_code;
    struct maam_auth_sys_t *auth_sys_info;
    struct maam_login_t login_data;
    struct maam_session_t session_data;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    memset(&login_data, 0, sizeof(struct maam_login_t));

    // 預設為認證失敗.
    rep_data_info->rep_handle = MAAM_RHANDLE_MESSAGE;

    // 取出驗證資料.
    fret = maam_get_login_para(req_data_info, &login_data);
    if(fret < MAAM_RCODE_PASS)
    {
        MAAM_EMSG("call maam_get_login_para() fail");
        goto FREE_01;
    }

    // 驗證帳號.
    fret = maam_verify_account(&login_data, &cret);
    if(fret < MAAM_RCODE_PASS)
    {
        MAAM_EMSG("call maam_verify_account() fail");
        goto FREE_01;
    }
    if(cret < MAAM_RCODE_PASS)
        goto FREE_01;

    fret = maam_auth_sys_wait(sm_id, sm_mutex_path, &auth_sys_info, &auth_sys_mutex);
    if(fret < MAAM_RCODE_PASS)
    {
        MAAM_EMSG("call maam_auth_sys_wait() fail");
        goto FREE_01;
    }

    // 填入 session 資料.
    fret = maam_fill_session(auth_sys_info, &login_data, &session_data);
    if(fret < MAAM_RCODE_PASS)
    {
        MAAM_EMSG("call maam_fill_session() fail");
        goto FREE_02;
    }

    // 清除閒置超時的 session, 騰出空間.
    fret = maam_clear_idle(auth_sys_info);
    if(fret < MAAM_RCODE_PASS)
    {
        MAAM_EMSG("call maam_clear_idle() fail");
        goto FREE_02;
    }

    // 如果不允許同時超過一個帳號登入系統活動, 檢查.
    if(login_data.multiple_user_active_rule != MAAM_LMULTIPLE_ALLOW)
    {
        fret = maam_check_multiple_user_active(auth_sys_info, &login_data, &cret);
        if(fret < MAAM_RCODE_PASS)
        {
            MAAM_EMSG("call maam_check_multiple_user_active() fail");
            goto FREE_02;
        }
        if(cret < MAAM_RCODE_PASS)
            goto FREE_02;
    }

    // 如果不允許同一個帳號被多個使用者同時登入, 撿查.
    if(login_data.same_accout_multiple_active_rule != MAAM_LMULTIPLE_ALLOW)
    {
        fret = maam_check_same_accout_multiple_active(auth_sys_info, &login_data, &cret);
        if(fret < MAAM_RCODE_PASS)
        {
            MAAM_EMSG("call maam_check_same_accout_multiple_active() fail");
            goto FREE_02;
        }
        if(cret < MAAM_RCODE_PASS)
            goto FREE_02;
    }

    // 驗證成功, 加入到 session 表.
    fret = maam_add_session(auth_sys_info, &session_data);
    if(fret < MAAM_RCODE_PASS)
    {
        MAAM_EMSG("call maam_add_session() fail");
        goto FREE_02;
    }

    // 驗證成功, 導向到主頁面.
    rep_data_info->rep_handle = MAAM_RHANDLE_SCRIPT;

FREE_02:
    maam_auth_sys_post(auth_sys_info, auth_sys_mutex);
FREE_01:
    // 如果驗證失敗, 清除 session_key, 否則填入正確的 session_key.
    tmp_code = (fret < MAAM_RCODE_PASS) || (cret < MAAM_RCODE_PASS) ?
               MAAM_FCOOKIE_DEL : MAAM_FCOOKIE_ADD;
    maam_fill_cookie(tmp_code, &session_data, rep_data_info);

    // 如果驗證失敗, 回傳錯誤代碼, 否則導向到主頁面.
    if(fret < MAAM_RCODE_PASS)
        tmp_code = fret;
    else
    if(cret < MAAM_RCODE_PASS)
        tmp_code = cret;
    else
        tmp_code = MAAM_RCODE_PASS;
    maam_fill_redirect_post(tmp_code, MAAM_PAGE_ROOT, rep_data_info);

    return fret;
}

// 登出.
static int maam_process_logout(
    char *sm_mutex_path,
    int sm_id,
    struct maam_req_data_t *req_data_info,
    struct maam_rep_data_t *rep_data_info)
{
    int fret = MAAM_RCODE_PASS, cret = MAAM_RCODE_PASS, auth_sys_mutex, tmp_code;
    struct maam_auth_sys_t *auth_sys_info;
    struct maam_cookie_t cookie_data;
    struct maam_session_t *each_session;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    memset(&cookie_data, 0, sizeof(struct maam_cookie_t));

    // 不管驗證失敗還成功, 都導向到登入頁面.
    rep_data_info->rep_handle = MAAM_RHANDLE_SCRIPT;

    // 沒有 cookie, 有問題的存取.
    if(req_data_info->req_have_cookie == 0)
    {
        MAAM_BDMSG("empty cookie");
        cret = MAAM_RCODE_INVALID_SESSION_KEY;
        goto FREE_01;
    }

    // 取出 session_key.
    cret = maam_get_cookie_para(req_data_info, &cookie_data);
    if(cret < MAAM_RCODE_PASS)
    {
        MAAM_EMSG("call maam_get_cookie_para() fail");
        goto FREE_01;
    }

    // 空的 session_key, 有問題的存取.
    if(strcmp(cookie_data.session_key, MAAM_COOKIE_EMPTY_VALUE_KEY) == 0)
    {
        MAAM_BDMSG("empty session [%s]", MAAM_COOKIE_EMPTY_VALUE_KEY);
        cret = MAAM_RCODE_INVALID_SESSION_KEY;
        goto FREE_01;
    }

    fret = maam_auth_sys_wait(sm_id, sm_mutex_path, &auth_sys_info, &auth_sys_mutex);
    if(fret < MAAM_RCODE_PASS)
    {
        MAAM_EMSG("call maam_auth_sys_wait() fail");
        goto FREE_01;
    }

    // 找到對應的 session, 不存在表示有問題的存取.
    tmp_code = maam_compare_session(auth_sys_info, MAAM_CSESSION_EQUAL_SESSION_KEY,
                                    cookie_data.session_key, &each_session);
    if(tmp_code < 0)
    {
        MAAM_BDMSG("unknown session [%s]", cookie_data.session_key);
        cret = MAAM_RCODE_INVALID_SESSION_KEY;
        goto FREE_02;
    }

    // 刪除 session.
    fret = maam_del_session(auth_sys_info, each_session);
    if(fret < MAAM_RCODE_PASS)
    {
        MAAM_EMSG("call maam_del_session() fail");
        goto FREE_02;
    }

FREE_02:
    maam_auth_sys_post(auth_sys_info, auth_sys_mutex);
FREE_01:
    // 清除 session_key.
    maam_fill_cookie(MAAM_FCOOKIE_DEL, NULL, rep_data_info);

    // 導向到登入頁面.
    if(fret < MAAM_RCODE_PASS)
        tmp_code = fret;
    else
    if(cret < MAAM_RCODE_PASS)
        tmp_code = cret;
    else
        tmp_code = MAAM_RCODE_PASS;
    maam_fill_redirect_post(tmp_code, MAAM_PAGE_ROOT, rep_data_info);

    return fret;
}

// 一般存取.
static int maam_process_access(
    char *sm_mutex_path,
    int sm_id,
    struct maam_req_data_t *req_data_info,
    struct maam_rep_data_t *rep_data_info)
{
    int fret = MAAM_RCODE_PASS, cret = MAAM_RCODE_PASS, auth_sys_mutex, tmp_code;
    struct maam_auth_sys_t *auth_sys_info;
    struct maam_cookie_t cookie_data;
    struct maam_session_t *each_session;
    char *tmp_path;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    memset(&cookie_data, 0, sizeof(struct maam_cookie_t));

    // 假設為已登入的.
    rep_data_info->rep_handle = MAAM_RHANDLE_ALLOW;

    // 沒有 cookie, 有問題的存取.
    if(req_data_info->req_have_cookie == 0)
    {
        MAAM_BDMSG("empty cookie");
        cret = MAAM_RCODE_INVALID_SESSION_KEY;
        goto FREE_01;
    }

    // 取出 session_key.
    cret = maam_get_cookie_para(req_data_info, &cookie_data);
    if(cret < MAAM_RCODE_PASS)
    {
        MAAM_EMSG("call maam_get_cookie_para() fail");
        goto FREE_01;
    }

    // 空的 session_key, 有問題的存取.
    if(strcmp(cookie_data.session_key, MAAM_COOKIE_EMPTY_VALUE_KEY) == 0)
    {
        MAAM_BDMSG("empty session [%s]", MAAM_COOKIE_EMPTY_VALUE_KEY);
        cret = MAAM_RCODE_INVALID_SESSION_KEY;
        goto FREE_01;
    }

    fret = maam_auth_sys_wait(sm_id, sm_mutex_path, &auth_sys_info, &auth_sys_mutex);
    if(fret < MAAM_RCODE_PASS)
    {
        MAAM_EMSG("call maam_auth_sys_wait() fail");
        goto FREE_01;
    }

    // 找到對應的 session, 不存在表示有問題的存取.
    tmp_code = maam_compare_session(auth_sys_info, MAAM_CSESSION_EQUAL_SESSION_KEY,
                                    cookie_data.session_key, &each_session);
    if(tmp_code < 0)
    {
        MAAM_BDMSG("unknown session [%s]", cookie_data.session_key);
        cret = MAAM_RCODE_INVALID_SESSION_KEY;
        goto FREE_02;
    }

    // 檢查是否閒置超時.
    fret = maam_check_idle(auth_sys_info, each_session, &cret);
    if(fret < MAAM_RCODE_PASS)
    {
        MAAM_EMSG("call maam_check_idle() fail");
        goto FREE_03;
    }
    if(cret < MAAM_RCODE_PASS)
        goto FREE_03;

FREE_03:
    // 如果閒置超時, 刪除 session.
    if((fret < MAAM_RCODE_PASS) || (cret < MAAM_RCODE_PASS))
    {
        fret = maam_del_session(auth_sys_info, each_session);
        if(fret < MAAM_RCODE_PASS)
        {
            MAAM_EMSG("call maam_del_session() fail");
            goto FREE_02;
        }
    }
FREE_02:
    maam_auth_sys_post(auth_sys_info, auth_sys_mutex);
FREE_01:
    // 如果是有問題 (有問題的存取, 被要求踢掉, 閒置超時), 導向到登入頁面.
    if((fret < MAAM_RCODE_PASS) || (cret < MAAM_RCODE_PASS))
    {
        // 預設使用 script 方式導向.
        rep_data_info->rep_handle = MAAM_RHANDLE_SCRIPT;

        // 清除 session_key.
        maam_fill_cookie(MAAM_FCOOKIE_DEL, NULL, rep_data_info);

        if(req_data_info->req_method == METHOD_GET)
        {
            // 如果存取首頁, 當作是在存取登入頁面.
            tmp_path = req_data_info->access_root == 0 ? req_data_info->req_path : maam_page_login;
            // 檢查是不是存取不用登入也可以存取的頁面, 或是存取要登入才能存取的非網頁檔案.
            tmp_code = maam_check_access(tmp_path, req_data_info->req_query,
                                         req_data_info->req_method);
            if(tmp_code == MAAM_PACCESS_ALLOW)
            {
                // 如果是存取不用不用登入也可以存取的項目,
                // access_root == 0, 不是存取首頁, 不理會.
                // access_root != 0, 存取首頁, 使用 rewrite 方式回傳登入頁面.
                rep_data_info->rep_handle = req_data_info->access_root == 0 ?
                                            MAAM_RHANDLE_ALLOW : MAAM_RHANDLE_REWRITE;
            }
            else
            if(tmp_code == MAAM_PACCESS_LOSE)
            {
                // 存取要登入才能存取的非網頁檔案, 回傳 HTTP 404.
                rep_data_info->rep_handle = MAAM_RHANDLE_LOSE;
            }

            // 填入導向資料.
            if((rep_data_info->rep_handle == MAAM_RHANDLE_SCRIPT) ||
               (rep_data_info->rep_handle == MAAM_RHANDLE_REWRITE))
            {
                // 如果是使用 script 做導向,
                // 1. 先讓瀏覽器導向到首頁.
                // 2. 瀏覽器存取首頁時使用 rewrite 方式回傳登入頁面.
                tmp_path = rep_data_info->rep_handle == MAAM_RHANDLE_SCRIPT ?
                           MAAM_PAGE_ROOT : maam_page_login;
                maam_fill_redirect_get(tmp_path, rep_data_info);
            }
        }
        else
        if(req_data_info->req_method == METHOD_POST)
        {
            // 檢查是不是存取不用登入也可以存取的路徑, 是的話不做導向.
            tmp_code = maam_check_access(req_data_info->req_path, req_data_info->req_query,
                                         req_data_info->req_method);
            if(tmp_code == MAAM_PACCESS_ALLOW)
                rep_data_info->rep_handle = MAAM_RHANDLE_ALLOW;

            // 填入導向資料.
            if(rep_data_info->rep_handle != MAAM_RHANDLE_ALLOW)
                maam_fill_redirect_post(cret, MAAM_PAGE_ROOT, rep_data_info);
        }
    }
    // 如果沒有問題, 是有效的存取.
    else
    {
        if(req_data_info->req_method == METHOD_GET)
        {
            // 如果是存取首頁, 使用 rewrite 方式回傳主要頁面.
            if(req_data_info->access_root != 0)
            {
                rep_data_info->rep_handle = MAAM_RHANDLE_REWRITE;
                maam_fill_redirect_get(maam_page_index, rep_data_info);
            }
        }
        else
        if(req_data_info->req_method == METHOD_POST)
        {
            // 如果是檢查 session 是否有效, 回傳有效的代碼.
            if(req_data_info->req_action == MAAM_RACTION_CHECK)
            {
                rep_data_info->rep_handle = MAAM_RHANDLE_MESSAGE;
                maam_fill_redirect_post(cret, MAAM_PAGE_ROOT, rep_data_info);
            }
        }
    }

    return fret;
}

// 初始化資料.
int maam_init(
    int sm_id,
    char *sm_mutex_path)
{
    int fret = MAAM_RCODE_PASS, auth_sys_mutex, midx, mlen;
    struct maam_auth_sys_t *auth_sys_info;
    long tmp_uptime;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    fret = maam_auth_sys_wait(sm_id, sm_mutex_path, &auth_sys_info, &auth_sys_mutex);
    if(fret < MAAM_RCODE_PASS)
    {
        MAAM_EMSG("call maam_auth_sys_wait() fail");
        goto FREE_01;
    }

    memset(auth_sys_info, 0, sizeof(struct maam_auth_sys_t));

    // 計算系統 uptime 能記錄的最大值 (MAAM_CALCULATE_INTERVAL() 會需要使用).
    mlen = (sizeof(tmp_uptime) * 8) - 2;
    for(tmp_uptime = 1, midx = 0; midx < mlen; midx++)
        tmp_uptime |= (tmp_uptime << 1);
    auth_sys_info->max_uptime = tmp_uptime;

    auth_sys_info->session_head = auth_sys_info->session_tail = -1;

    for(midx = 0; midx < MAAM_MAX_SESSION; midx++)
    {
        auth_sys_info->session_info[midx].session_index = midx;
        auth_sys_info->session_info[midx].prev_session = -1;
        auth_sys_info->session_info[midx].next_session = -1;
        auth_sys_info->session_info[midx].empty_session = -1;
    }

    auth_sys_info->session_usable = 0;

    for(midx = 0; midx < (MAAM_MAX_SESSION - 1); midx++)
    {
        auth_sys_info->session_info[midx].empty_session =
            auth_sys_info->session_info[midx + 1].session_index;
    }

    maam_auth_sys_post(auth_sys_info, auth_sys_mutex);
FREE_01:
    return fret;
}

// 檢查是要做哪種處理.
int maam_action(
    char *req_path)
{
    MAAM_BDMSG("=> %s", __FUNCTION__);

    if(strcmp(req_path, MAAM_ACCOUNT_LOGIN_KEY) == 0)
        return MAAM_RACTION_LOGIN;
    else
    if(strcmp(req_path, MAAM_ACCOUNT_LOGOUT_KEY) == 0)
        return MAAM_RACTION_LOGOUT;
    else
    if(strcmp(req_path, MAAM_ACCOUNT_CHECK_KEY) == 0)
        return MAAM_RACTION_CHECK;

    return MAAM_RACTION_NONE;
}

// 主要處理函式.
int maam_handle(
    int sm_id,
    char *sm_mutex_path,
    struct maam_req_data_t *req_data_info,
    struct maam_rep_data_t *rep_data_info)
{
    int fret;


    MAAM_BDMSG("=> %s", __FUNCTION__);

    if(req_data_info->req_action == MAAM_RACTION_LOGIN)
    {
        fret = maam_process_login(sm_mutex_path, sm_id, req_data_info, rep_data_info);
        if(fret < MAAM_RCODE_PASS)
        {
            MAAM_EMSG("call maam_process_login() fail");
            return fret;
        }
    }
    else
    if(req_data_info->req_action == MAAM_RACTION_LOGOUT)
    {
        fret = maam_process_logout(sm_mutex_path, sm_id, req_data_info, rep_data_info);
        if(fret < MAAM_RCODE_PASS)
        {
            MAAM_EMSG("call maam_process_logout() fail");
            return fret;
        }
    }
    else
    {
        fret = maam_process_access(sm_mutex_path, sm_id, req_data_info, rep_data_info);
        if(fret < MAAM_RCODE_PASS)
        {
            MAAM_EMSG("call maam_process_access() fail");
            return fret;
        }
    }

    return MAAM_RCODE_PASS;
}

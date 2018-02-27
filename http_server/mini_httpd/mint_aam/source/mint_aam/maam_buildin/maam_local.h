/*
Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
This file is part of the MintAAM.
Some rights reserved. See README.
*/

#ifndef _MAAM_LOCAL_H_
#define _MAAM_LOCAL_H_




#include "maam_lib/maam_lulib/maam_common.h"




// 首頁.
#define MAAM_PAGE_ROOT "/"

// copy from mini_httpd.c
#define METHOD_UNKNOWN 0
#define METHOD_GET     1
#define METHOD_HEAD    2
#define METHOD_POST    3

// 要求的動作.
enum MAAM_REQUEST_ACTION
{
    // 一般網頁存取.
    MAAM_RACTION_NONE = 0,
    // 要求登入.
    MAAM_RACTION_LOGIN,
    // 要求登出.
    MAAM_RACTION_LOGOUT,
    // 要求檢查 session 是否有效.
    MAAM_RACTION_CHECK
};

// 多人登入或重複登入的處理.
enum MAAM_LIMIT_MULTIPLE_ACTIVE
{
    // 允許多人登入或重複登入.
    MAAM_LMULTIPLE_ALLOW = 0,
    // 禁止多人登入或重複登入, 發生時踢掉已登入的使用者.
    MAAM_LMULTIPLE_DENY_FORCE_KICK_INSIDE,
    // 禁止多人登入或重複登入, 發生時踢掉登入中的使用者.
    MAAM_LMULTIPLE_DENY_FORCE_KICK_OUTSIDE,

    // 以下二種方式處理方式 :
    // 使用帳號的權限判定, 號權高的保留, 號權低的踢掉, 號權一樣使用下面方式判定.

    // 禁止多人登入或重複登入, 發生時如果二邊號權一樣踢掉已登入的使用者.
    MAAM_LMULTIPLE_DENY_PERMISSION_KICK_INSIDE,
    // 禁止多人登入或重複登入, 發生時如果二邊號權一樣踢掉登入中的使用者.
    MAAM_LMULTIPLE_DENY_PERMISSION_KICK_OUTSIDE
};

// 回應的處理方法.
enum MAAM_RESPONSE_HANDLE
{
    // 允許存取.
    MAAM_RHANDLE_ALLOW = 0,
    // 禁止存取, 回傳 HTTP 404.
    MAAM_RHANDLE_LOSE,
    // 禁止存取, 回傳代碼.
    MAAM_RHANDLE_MESSAGE,
    // 禁止存取, 回傳 javascript 導向指定的頁面.
    MAAM_RHANDLE_SCRIPT,
    // 禁止存取, 修改原始路徑讓 mini_httpd 回傳指定的頁面的內容.
    MAAM_RHANDLE_REWRITE
};

// 比對存取路徑和查詢的方式.
enum MAAM_COMPARE_ACCESS_METHOD
{
    // 不比對, 直接認定匹配.
    MAAM_CACCESS_NONE = 0,
    // 完全比對.
    MAAM_CACCESS_ALL,
    // 從前面往後比對 N 個字元相同.
    MAAM_CACCESS_PREFIX,
    // 從後面往前比對 N 個字元相同.
    MAAM_CACCESS_POSTFIX,
    // 子字串.
    MAAM_CACCESS_SUB
};




// 客戶端的請求資料.
struct maam_req_data_t
{
    // 客戶端的位址.
    usockaddr *req_addr;
    // 請求的方法 (METHOD_GET, METHOD_POST, ...).
    int req_method;
    // 請求的動作 (MAAM_REQUEST_ACTION).
    int req_action;
    // 請求的路徑.
    char *req_path;
    // 請求的參數.
    char *req_query;
    // 請求的 cookie 資料.
    char *req_cookie;
    // 是否帶有 cookie 資料.
    int req_have_cookie;
    // 請求的 content 資料.
    char *req_con;
    // 請求的 content 資料的長度.
    size_t req_len;
    // 是否存取首頁 ("/").
    int access_root;
};

// 回應的資料.
struct maam_rep_data_t
{
    // 是否網頁導向 (MAAM_RESPONSE_HANDLE).
    int rep_handle;
    // 回應的 cookie.
    char rep_cookie_buf[MAAM_REP_COOKIE_MAX_COUNT][MAAM_BSIZE_REP_COOKIE];
    // 每個 cookie 的位址.
    char *rep_cookie_loc[MAAM_REP_COOKIE_MAX_COUNT];
    // 網頁導向的內容.
    char rep_body_buf[MAAM_BSIZE_REP_BODY];
};

// 使用者登入時帶的資料.
struct maam_login_t
{
    // 客戶端的位址.
    usockaddr *client_addr;
    // 帳號名稱.
    char *verify_name;
    // 帳號密碼.
    char *verify_password;
    // 如何處理同時超過一個帳號登入系統.
    int multiple_user_active_rule;
    // 如何處理同一個帳號被多個使用者同時登入.
    int same_accout_multiple_active_rule;
    // 驗證成功後取得的帳號的名稱.
    char account_name[MAAM_BSIZE_ACCOUNT_NAME];
    // 驗證成功後取得的帳號的權限.
    int account_permission;
    // 驗證成功後取得的帳號的閒置超時時間.
    long account_idle_timeout;
};

// 紀錄哪些路徑在未登入時也可以存取.
struct maam_access_t
{
    // 要和路徑部分比對的字串和長度和比對方法 (MAAM_COMPARE_ACCESS_METHOD).
    char *cmp_path_con;
    size_t cmp_path_len;
    int cmp_path_method;
    // 要和查詢部分比對的字串和長度和比對方法 (MAAM_COMPARE_ACCESS_METHOD).
    char *cmp_query_con;
    size_t cmp_query_len;
    int cmp_query_method;
};




#endif

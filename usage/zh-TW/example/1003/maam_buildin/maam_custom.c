/*
Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
This file is part of the MintAAM.
Some rights reserved. See README.
*/

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "maam_local.h"
#include "maam_md5.h"
#include "maam_custom.h"
#include "mcm_lib/mcm_lheader/mcm_type.h"
#include "mcm_lib/mcm_lheader/mcm_size.h"
#include "mcm_lib/mcm_lheader/mcm_connect.h"
#include "mcm_lib/mcm_lheader/mcm_return.h"
#include "mcm_lib/mcm_lheader/mcm_data_exinfo_auto.h"
#include "mcm_lib/mcm_lulib/mcm_lulib_api.h"




// 網頁檔案的副檔名的值和長度.
#define MAAM_HTML_KEY ".html"
#define MAAM_HTML_LEN 5

// 登入網頁和主要網頁的路徑 (以 mini_httpd.conf 中 dir=... 的位置為基準).
#define MAAM_PAGE_LOGIN "/web_app_1003_login.html"
#define MAAM_PAGE_INDEX "/web_app_1003_index.html"




char *maam_html_key = MAAM_HTML_KEY;
size_t maam_html_len = MAAM_HTML_LEN;

char *maam_page_login = MAAM_PAGE_LOGIN;
char *maam_page_index = MAAM_PAGE_INDEX;

// 在 GET 時哪些網頁在未登入時也可以存取.
struct maam_access_t maam_access_allow_get_html_list[] =
{
    // 登入頁面.
    {MAAM_PAGE_LOGIN, 0, MAAM_CACCESS_ALL, NULL, 0, 0},
    {NULL, 0, 0, NULL, 0, 0}
};

// 在 GET 時哪些非網頁檔在登入後才能存取.
struct maam_access_t maam_access_deny_get_other_list[] =
{
    {NULL, 0, 0, NULL, 0, 0}
};

// 在 POST 時哪些在未登入時也可以存取.
struct maam_access_t maam_access_allow_post_list[] =
{
    // 使用 "/cgi/mcm_cgi_config.cgi" 存取設定時, 如果查詢字串的尾端有 "anyone" 關鍵字就允許使用.
    {"/cgi/mcm_cgi_config.cgi", 0, MAAM_CACCESS_ALL, "anyone", 6, MAAM_CACCESS_POSTFIX},
    {NULL, 0, 0, NULL, 0, 0}
};




// 驗證帳號.
int maam_verify_account(
    struct maam_login_t *login_info,
    int *verify_result_buf)
{
    int fret = MAAM_RCODE_PASS, cret = MAAM_RCODE_INTERNAL_ERROR;
    char hash_code[MAAM_BSIZE_MD5_HASH];
    char *path1, path2[MCM_PATH_MAX_LENGTH];
    struct mcm_lulib_lib_t self_lulib;
    struct mcm_dv_device_web_account_t account_v;
    MCM_DTYPE_EK_TD account_count, i;


    // 使用 MintCM.
    // device.web.account.* 紀錄帳號的資料.
    self_lulib.socket_path = "@mintcm";
    self_lulib.call_from = MCM_CFROM_USER;
    self_lulib.session_permission = MCM_SPERMISSION_RO;
    self_lulib.session_stack_size = 0;
    if(mcm_lulib_init(&self_lulib) < MCM_RCODE_PASS)
    {
        MAAM_EMSG("call mcm_lulib_init() fail");
        fret = MAAM_RCODE_INTERNAL_ERROR;
        goto FREE_01;
    }

    // 讀出 device.web.account.* 的資料筆數.
    path1 = "device.web.account.*";
    if(mcm_lulib_get_count(&self_lulib, path1, &account_count) < MCM_RCODE_PASS)
    {
        MAAM_EMSG("call mcm_lulib_get_count(%s) fail", path1);
        fret = MAAM_RCODE_INTERNAL_ERROR;
        goto FREE_02;
    }
    MAAM_BDMSG("[count] %s = " MCM_DTYPE_EK_PF, path1, account_count);

    for(i = 0; i < account_count; i++)
    {
        // 讀出帳號資料.
        snprintf(path2, sizeof(path2), "device.web.account.@" MCM_DTYPE_EK_PF, i + 1);
        if(mcm_lulib_get_entry(&self_lulib, path2, &account_v) < MCM_RCODE_PASS)
        {
            MAAM_EMSG("call mcm_lulib_get_entry(%s) fail", path2);
            fret = MAAM_RCODE_INTERNAL_ERROR;
            goto FREE_02;
        }
        MAAM_BDMSG("[get-entry] %s.name = " MCM_DTYPE_S_PF,
                   path2, account_v.name);
        MAAM_BDMSG("[get-entry] %s.password = " MCM_DTYPE_S_PF,
                   path2, account_v.password);
        MAAM_BDMSG("[get-entry] %s.permission = " MCM_DTYPE_ISI_PF,
                   path2, account_v.permission);
        MAAM_BDMSG("[get-entry] %s.idle_timeout = " MCM_DTYPE_ISI_PF,
                   path2, account_v.idle_timeout);

        // 使用 md5 處理帳號名稱並比對.
        maam_md5_hash(account_v.name, strlen(account_v.name),
                      hash_code, sizeof(hash_code));
        MAAM_BDMSG("name md5[%s] = [%s] ? [%s] : [%s]",
                   account_v.name, hash_code, login_info->verify_name,
                   strcmp(login_info->verify_name, hash_code) == 0 ? "match" : "not match");
        if(strcmp(login_info->verify_name, hash_code) == 0)
        {
            // 使用 md5 處理帳號密碼並比對.
            maam_md5_hash(account_v.password, strlen(account_v.password),
                          hash_code, sizeof(hash_code));
            MAAM_BDMSG("password md5[%s] = [%s] ? [%s] : [%s]",
                       account_v.password, hash_code, login_info->verify_password,
                       strcmp(login_info->verify_password, hash_code) == 0 ? "match" : "not match");
            if(strcmp(login_info->verify_password, hash_code) == 0)
            {
                // 帳號名稱和帳號密碼都正確.
                MAAM_BDMSG("login pass [%s]", account_v.name);
                break;
            }
            else
            {
                // 帳號密碼錯誤.
                MAAM_BDMSG("login fail [%s], invalid password", account_v.name);
                cret = MAAM_RCODE_INVALID_ACCOUNT_PASSWORD;
                goto FREE_02;
            }
        }
    }
    if(i >= account_count)
    {
        // 帳號名稱錯誤.
        MAAM_BDMSG("login fail, invalid name");
        cret = MAAM_RCODE_INVALID_ACCOUNT_NAME;
        goto FREE_02;
    }

    // 驗證成功, 填充 session 需要的資料.
    snprintf(login_info->account_name, sizeof(login_info->account_name), "%s", account_v.name);
    login_info->account_permission = account_v.permission;
    login_info->account_idle_timeout = account_v.idle_timeout;

    // 設定帳號驗證正確.
    cret = MAAM_RCODE_PASS;
FREE_02:
    mcm_lulib_exit(&self_lulib);
FREE_01:
    *verify_result_buf = cret;
    return fret;
}

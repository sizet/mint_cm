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




// 網頁檔案的副檔名的值和長度.
#define MAAM_HTML_KEY ".html"
#define MAAM_HTML_LEN 5

// 登入網頁和主要網頁的路徑 (以 mini_httpd.conf 中 dir=... 的位置為基準).
#define MAAM_PAGE_LOGIN "/login.html"
#define MAAM_PAGE_INDEX "/index.html"




char *maam_html_key = MAAM_HTML_KEY;
size_t maam_html_len = MAAM_HTML_LEN;

char *maam_page_login = MAAM_PAGE_LOGIN;
char *maam_page_index = MAAM_PAGE_INDEX;

// 在 GET 時哪些網頁在未登入時也可以存取.
struct maam_access_t maam_access_allow_get_html_list[] =
{
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
    {NULL, 0, 0, NULL, 0, 0}
};




// 驗證帳號.
int maam_verify_account(
    struct maam_login_t *login_info,
    int *verify_result_buf)
{
    int fret = MAAM_RCODE_INTERNAL_ERROR, cret = MAAM_RCODE_INTERNAL_ERROR;

    *verify_result_buf = cret;
    return fret;
}

// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../mcm_lib/mcm_lheader/mcm_type.h"
#include "../mcm_lib/mcm_lheader/mcm_return.h"
#include "../mcm_lib/mcm_lheader/mcm_debug.h"
#include "mcm_cgi_common.h"
#include "mcm_cgi_upload_custom.h"
#include "mcm_cgi_upload_handle.h"




#if MCM_CGIEMODE | MCM_CUHDMODE
    int dbg_tty_fd_h;
    char dbg_msg_buf_h[MCM_DBG_BUFFER_SIZE];
#endif

#if MCM_CGIEMODE
    #define MCM_CEMSG(msg_fmt, msg_args...) \
        MCM_CGI_TTY_MSG(dbg_tty_fd_h, dbg_msg_buf_h, msg_fmt, ##msg_args)
#else
    #define MCM_CEMSG(msg_fmt, msg_args...)
#endif

#if MCM_CUHDMODE
    #define MCM_CUHDMSG(msg_fmt, msg_args...) \
        MCM_CGI_TTY_MSG(dbg_tty_fd_h, dbg_msg_buf_h, msg_fmt, ##msg_args)
#else
    #define MCM_CUHDMSG(msg_fmt, msg_args...)
#endif

#define MCM_MULTIPART_KEY           "multipart/form-data"
#define MCM_MULTIPART_LEN           19
#define MCM_BOUNDARY_KEY            "boundary"
#define MCM_BOUNDARY_LEN            8
#define MCM_CONTENT_DISPOSITION_KEY "Content-Disposition"
#define MCM_CONTENT_DISPOSITION_LEN 19
#define MCM_NAME_KEY                "name"
#define MCM_NAME_LEN                4
#define MCM_FILENAME_KEY            "filename"
#define MCM_FILENAME_LEN            8
#define MCM_PART_SPLIT_KEY          "--"
#define MCM_PART_SPLIT_LEN          2
#define MCM_CRLF_KEY                "\r\n"
#define MCM_CRLF_LEN                2
#define MCM_HEADER_TOKEN_KEY        ':'
#define MCM_PARAMETER_SPLIT_KEY     ';'
#define MCM_PARAMETER_TOKEN_KEY     '='
#define MCM_VALUE_BORDER_KEY        '\"'
#define MCM_CALLBACK_KEY            "callback"




char *boundary_key = NULL;
MCM_DTYPE_USIZE_TD boundary_len = 0;
char crlf_key[MCM_CRLF_LEN + 1], part_split_key[MCM_PART_SPLIT_LEN + 1];
char *callback_name = NULL;
struct part_info_t *part_list_head = NULL, *part_list_tail = NULL;




int mcm_find_boundary(
    char *content_con,
    MCM_DTYPE_USIZE_TD content_len);

int mcm_process_post(
    char *post_con,
    MCM_DTYPE_USIZE_TD post_len);




int main(
    int arg_cnt,
    char **arg_list)
{
    char *env_loc;
    char *post_buf;
    MCM_DTYPE_USIZE_TD post_len, rcnt;
    ssize_t rlen;


#if MCM_CGIEMODE | MCM_CUHDMODE
    dbg_tty_fd_h = open(MCM_DBG_DEV_TTY, O_WRONLY);
    if(dbg_tty_fd_h == -1)
    {
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0,
                      "call open() fail [%s]\\n[%s]", strerror(errno), MCM_DBG_DEV_TTY);
        goto FREE_01;
    }
#endif

    snprintf(crlf_key, sizeof(crlf_key), MCM_CRLF_KEY);
    snprintf(part_split_key, sizeof(part_split_key), MCM_PART_SPLIT_KEY);

    // 確認是 POST 方法.
    env_loc = getenv(MCM_CGI_REQUEST_METHOD_KEY);
    if(env_loc == NULL)
    {
        MCM_CEMSG("invalid, not find %s", MCM_CGI_REQUEST_METHOD_KEY);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0,
                      "invalid, not find %s", MCM_CGI_REQUEST_METHOD_KEY);
        goto FREE_01;
    }
    if(strcmp(env_loc, MCM_CGI_POST_KEY) != 0)
    {
        MCM_CEMSG("invalid, %s must be %s [%s]",
                  MCM_CGI_REQUEST_METHOD_KEY, MCM_CGI_POST_KEY, env_loc);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0,
                      "invalid, %s must be %s\\n[%s]",
                      MCM_CGI_REQUEST_METHOD_KEY, MCM_CGI_POST_KEY, env_loc);
        goto FREE_01;
    }

    // 讀出 HTTP 的 CONTENT_TYPE.
    // 例如 :
    // [CONTENT_TYPE: multipart/form-data; boundary=---------------------------24548616531490].
    // 會得到 [multipart/form-data; boundary=---------------------------24548616531490].
    env_loc = getenv(MCM_CGI_CONTENT_TYPE_KEY);
    if(env_loc == NULL)
    {
        MCM_CEMSG("invalid, not find %s", MCM_CGI_CONTENT_TYPE_KEY);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0,
                      "invalid, not find %s", MCM_CGI_CONTENT_TYPE_KEY);
        goto FREE_01;
    }
    // 取出 boundary 資料.
    if(mcm_find_boundary(env_loc, strlen(env_loc)) < MCM_RCODE_PASS)
    {
        MCM_CEMSG("call mcm_find_boundary() fail");
        goto FREE_01;
    }
    MCM_CUHDMSG("boundary[" MCM_DTYPE_USIZE_PF "][%s]", boundary_len, boundary_key);

    // 取得 POST 的資料的長度.
    env_loc = getenv(MCM_CGI_CONTENT_LENGTH_KRY);
    if(env_loc == NULL)
    {
        MCM_CEMSG("invalid, not find %s", MCM_CGI_CONTENT_LENGTH_KRY);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0,
                      "invalid, not find %s", MCM_CGI_CONTENT_LENGTH_KRY);
        goto FREE_01;
    }
    post_len = MCM_DTYPE_USIZE_SB(env_loc, NULL, 10);
    if(post_len == 0)
    {
        MCM_CEMSG("invalid, %s can not be 0", MCM_CGI_CONTENT_LENGTH_KRY);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0,
                      "invalid, %s can not be 0", MCM_CGI_CONTENT_LENGTH_KRY);
        goto FREE_01;
    }

    post_buf = (char *) malloc(post_len + 1);
    if(post_buf == NULL)
    {
        MCM_CEMSG("call malloc() fail [%s]", strerror(errno));
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0,
                      "call malloc() fail\\n[%s]", strerror(errno));
        goto FREE_01;
    }
    MCM_CUHDMSG("alloc post_buf[" MCM_DTYPE_USIZE_PF "][%p]", post_len + 1, post_buf);
    post_buf[post_len] = '\0';

    // 讀取 POST 資料.
    for(rcnt = 0; rcnt < post_len; rcnt += rlen)
    {
        rlen = read(STDIN_FILENO, post_buf + rcnt, post_len - rcnt);
        if(rlen == -1)
        {
            MCM_CEMSG("call read(STDIN_FILENO) fail [%s]", strerror(errno));
            MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0,
                          "call read(STDIN_FILENO) fail\\n[%s]", strerror(errno));
            goto FREE_01;
        }
    }

    if(mcm_process_post(post_buf, post_len) < MCM_RCODE_PASS)
    {
        MCM_CEMSG("call mcm_process_post() fail");
        goto FREE_01;
    }

    MCM_CUHDMSG("free post_buf[%p]", post_buf);
    free(post_buf);
FREE_01:
#if MCM_CGIEMODE | MCM_CUHDMODE
    if(dbg_tty_fd_h != -1)
        close(dbg_tty_fd_h);
#endif
    return MCM_RCODE_PASS;
}

// 比對字串, 並回傳剩下的字串.
int mcm_compare_key_ncmp(
    char **base_con,
    MCM_DTYPE_USIZE_TD *base_len,
    char *target_key,
    MCM_DTYPE_USIZE_TD target_len,
    char token_key)
{
    char *base_dcon;
    MCM_DTYPE_USIZE_TD base_dlen;


    base_dcon = *base_con;
    base_dlen = *base_len;

    // 例如 :
    // base_dcon = [multipart/form-data; boundary=---------------------------24548616531490].
    // target_key = [multipart/form-data].
    // token_key = [;].
    // 比對 base_dcon 和 target_key, 比對長度是 target_len,
    // 符合的話再檢查 base_dcon[target_len] 是否等於 token_key,
    // 都符合回傳 base_dcon 中剔除 target_key 和 token_key 的字串內容和長度,
    // 也就是 [ boundary=---------------------------24548616531490].

    // 比對 base_dcon 和 target_key, 比對長度是 target_len.
    // 符合的話再檢查 base_dcon[target_len] 是否等於 token_key.
    if(strncmp(base_dcon, target_key, target_len) != 0)
        return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
    else
        if(base_dcon[target_len] != token_key)
            return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;

    // 剔除 token_key.
    target_len++;

    // 剔除 target_key.
    *base_con = base_dcon + target_len;
    *base_len = base_dlen - target_len;

    return MCM_RCODE_PASS;
}

// 檢查是否在字串內, 並回傳剩下的字串.
int mcm_compare_key_str(
    char **base_con,
    MCM_DTYPE_USIZE_TD *base_len,
    char *target_key,
    MCM_DTYPE_USIZE_TD target_len,
    char token_key)
{
    char *base_dcon, *tmp_loc;
    MCM_DTYPE_USIZE_TD base_dlen;


    base_dcon = *base_con;
    base_dlen = *base_len;

    // 例如 :
    // base_dcon = [boundary=---------------------------24548616531490].
    // target_key = [boundary].
    // token_key = [=].
    // 檢查 target_key 是否在 base_dcon 內,
    // 符合的話再檢查 base_dcon[target_len] 是否等於 token_key,
    // 都符合回傳 base_dcon 中剔除 target_key 和 token_key 的字串內容和長度,
    // 也就是 [---------------------------24548616531490].

    // 檢查 target_key 是否在 base_dcon 內.
    // 符合的話再檢查 base_dcon[target_len] 是否等於 token_key.
    if((tmp_loc = strstr(base_dcon, target_key)) == NULL)
        return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
    else
        if(tmp_loc[target_len] != token_key)
            return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;

    // 剔除 token_key.
    target_len++;

    // 剔除 target_key.
    *base_con = tmp_loc + target_len;
    *base_len = base_dlen - ((tmp_loc + target_len) - base_dcon);

    return MCM_RCODE_PASS;
}

// 從 CONTENT_TYPE 中取出 boundary 資料.
int mcm_find_boundary(
    char *content_con,
    MCM_DTYPE_USIZE_TD content_len)
{
    int fret;


    MCM_CUHDMSG("=> %s", __FUNCTION__);

    // 例如 :
    // CONTENT_TYPE = [multipart/form-data; boundary=---------------------------24548616531490].
    // 先檢查上傳的資料是不是 [multipart/form-data] 類型,
    // 是的話再檢查是否有 [boundary] 關鍵字,
    // 有的話取出 boundary 資料.

    // 先檢查上傳的資料是不是 [multipart/form-data] 類型.
    fret = mcm_compare_key_ncmp(&content_con, &content_len, MCM_MULTIPART_KEY, MCM_MULTIPART_LEN,
                                MCM_PARAMETER_SPLIT_KEY);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CEMSG("invalid, enctype must be [%s] [%s]", MCM_MULTIPART_KEY, content_con);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0,
                      "invalid, enctype must be [%s]", MCM_MULTIPART_KEY);
        return fret;
    }

    // 再檢查是否有 [boundary] 關鍵字, 有的話取得 boundary 資料.
    fret = mcm_compare_key_str(&content_con, &content_len, MCM_BOUNDARY_KEY, MCM_BOUNDARY_LEN,
                               MCM_PARAMETER_TOKEN_KEY);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CEMSG("invalid, not find boundary [%s]", content_con);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0, "invalid, not find boundary");
        return fret;
    }
    if(content_len == 0)
    {
        MCM_CEMSG("invalid, empty boundary [%s]", content_con);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0, "invalid, empty boundary");
        return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
    }

    boundary_key = content_con;
    boundary_len = content_len;

    return fret;
}

// 取出字串開頭到找到 [\r\n] 的字串內容和長度, 並將 [\r\n] 改為空字元 [\0\0].
int mcm_read_line(
    char **post_con,
    MCM_DTYPE_USIZE_TD *post_len,
    char **read_con_buf,
    MCM_DTYPE_USIZE_TD *read_len_buf)
{
    char *post_dcon;
    MCM_DTYPE_USIZE_TD post_dlen, clen, cidx;


    MCM_CUHDMSG("=> %s", __FUNCTION__);

    post_dcon = *post_con;
    post_dlen = *post_len;

    if(post_dlen < MCM_CRLF_LEN)
        return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;

    clen = post_dlen - MCM_CRLF_LEN;
    for(cidx = 0; cidx <= clen; cidx++)
        if(memcmp(post_dcon + cidx, crlf_key, MCM_CRLF_LEN) == 0)
            break;
    if(cidx > clen)
        return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;

    memset(post_dcon + cidx, '\0', MCM_CRLF_LEN);

    *read_con_buf = post_dcon;
    *read_len_buf = cidx;

    cidx += MCM_CRLF_LEN;
    *post_con = post_dcon + cidx;
    *post_len = post_dlen - cidx;

    return MCM_RCODE_PASS;
}

// 取出參數的數值部份.
int mcm_parse_parameter(
    char **base_con,
    MCM_DTYPE_USIZE_TD *base_len,
    char *target_key,
    MCM_DTYPE_USIZE_TD target_len,
    char **data_con_buf)
{
    int fret;
    char *base_dcon;
    MCM_DTYPE_USIZE_TD base_dlen, cidx, dcnt;


    MCM_CUHDMSG("=> %s", __FUNCTION__);

    base_dcon = *base_con;
    base_dlen = *base_len;

    // 例如 :
    // [Content-Disposition: form-data; name="${input-name}"; filename="${file-name}"].
    // 如果要取出 [name="${input-name}"] 的 ["${input-name}"],
    // 先找到 [name],
    // 再找到 "${input-name}" 結尾的 ["], 取出 ["${input-name}"].

    // 先找到要取出的目標.
    fret = mcm_compare_key_str(&base_dcon, &base_dlen, target_key, target_len,
                               MCM_PARAMETER_TOKEN_KEY);
    if(fret < MCM_RCODE_PASS)
        return fret;

    // 找到 "${...}", 的結尾 ["], 並移除頭尾的 ["].
    for(cidx = dcnt = 0; cidx < base_dlen; cidx++)
        if(base_dcon[cidx] == MCM_VALUE_BORDER_KEY)
        {
            base_dcon[cidx] = '\0';
            // == 2 : 表示頭尾 ["] 都找到.
            if((++dcnt) == 2)
                break;
        }
    if(cidx == base_dlen)
    {
        MCM_CEMSG("invalid, not find %s \"...\"", target_key);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0,
                      "invalid, not find %s \"...\"", target_key);
        return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
    }

    // 跳過開頭的 ["].
    base_dcon++;
    base_dlen--;
    MCM_CUHDMSG("[%s][" MCM_DTYPE_USIZE_PF "][%s]", target_key, cidx - 1, base_dcon);

    // 回傳 [${...}].
    *data_con_buf = base_dcon[0] != '\0' ? base_dcon : NULL;

    // 回傳剩下的資料內容和長度.
    *base_con = base_dcon + cidx;
    *base_len = base_dlen - cidx;

    return fret;
}

// 取出 [Content-Disposition] 後面的參數.
int mcm_parse_disposition(
    char *base_con,
    MCM_DTYPE_USIZE_TD base_len,
    char **name_buf,
    char **filename_buf)
{
    int fret;
    char *tmp_name = NULL, *tmp_filename = NULL;


    MCM_CUHDMSG("=> %s", __FUNCTION__);

    // 取出 [name="${input-name}"] 的 ${input-name}.
    fret = mcm_parse_parameter(&base_con, &base_len, MCM_NAME_KEY, MCM_NAME_LEN, &tmp_name);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CEMSG("call mcm_parse_parameter(%s) fail", MCM_NAME_KEY);
        return fret;
    }
    MCM_CUHDMSG("[%s][%s]", MCM_NAME_KEY, tmp_name);

    if(base_len > 0)
    {
        // 取出 [filename="${file-name}"] 的 ${file-name}.
        fret = mcm_parse_parameter(&base_con, &base_len, MCM_FILENAME_KEY, MCM_FILENAME_LEN,
                                   &tmp_filename);
#if MCM_CUHDMODE
        if(fret >= MCM_RCODE_PASS)
        {
            MCM_CUHDMSG("[%s][%s]", MCM_FILENAME_KEY, tmp_filename);
        }
#endif
        fret = MCM_RCODE_PASS;
    }

    *name_buf = tmp_name;
    *filename_buf = tmp_filename;

    return fret;
}

int mcm_parse_data(
    char **post_con,
    MCM_DTYPE_USIZE_TD *post_len,
    char **data_con_buf,
    MCM_DTYPE_USIZE_TD *data_len_buf,
    MCM_DTYPE_BOOL_TD *last_part_buf)
{
    char *post_dcon;
    MCM_DTYPE_USIZE_TD post_dlen, clen, cidx, tidx;
    MCM_DTYPE_BOOL_TD end_type = 0;


    MCM_CUHDMSG("=> %s", __FUNCTION__);

    post_dcon = *post_con;
    post_dlen = *post_len;

    // [data] 部分 :
    //
    // 不是最後一個 [part] :
    // [raw data] + [\r\n] + [--] + [boundary] + [\r\n]
    //
    // 最後一個 [part] :
    // [raw data] + [\r\n] + [--] + [boundary] + [--]

    // 找到 [boundary], 比對範圍 : 0 ~ 資料長度 - [boundary].
    if(post_dlen < (MCM_CRLF_LEN + MCM_PART_SPLIT_LEN + boundary_len))
    {
        MCM_CEMSG("invalid, not find split boundary");
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0, "invalid, not find split boundary");
        return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
    }
    clen = post_dlen - boundary_len;
    for(cidx = 0; cidx <= clen; cidx++)
        if(memcmp(post_dcon + cidx, boundary_key, boundary_len) == 0)
            break;
    if(cidx > clen)
    {
        MCM_CEMSG("invalid, not find split boundary");
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0, "invalid, not find split boundary");
        return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
    }

    // 檢查前面的 [--] 是否存在.
    if(cidx < MCM_PART_SPLIT_LEN)
    {
        MCM_CEMSG("invalid, not find data --");
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0, "invalid, not find data --");
        return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
    }
    tidx = cidx - MCM_PART_SPLIT_LEN;
    if(memcmp(post_dcon + tidx, part_split_key, MCM_PART_SPLIT_LEN) != 0)
    {
        MCM_CEMSG("invalid, not find data --");
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0, "invalid, not find data --");
        return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
    }

    // 檢查前面的 [\r\n] 是否存在, 並改為空字元.
    if(cidx < (MCM_CRLF_LEN + MCM_PART_SPLIT_LEN))
    {
        MCM_CEMSG("invalid, not find data CRLF");
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0, "invalid, not find data CRLF");
        return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
    }
    tidx = cidx - (MCM_CRLF_LEN + MCM_PART_SPLIT_LEN);
    if(memcmp(post_dcon + tidx, crlf_key, MCM_CRLF_LEN) != 0)
    {
        MCM_CEMSG("invalid, not find data CRLF");
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0, "invalid, not find data CRLF");
        return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
    }
    memset(post_dcon + tidx, 0, MCM_CRLF_LEN);

    // 檢查後面的 [\r\n] 或 [--] 是否存在
    tidx = cidx + boundary_len;

    if((tidx + MCM_CRLF_LEN) <= post_dlen)
        end_type = memcmp(post_dcon + tidx, crlf_key, MCM_CRLF_LEN) == 0 ? 1 : 0;

    if(end_type == 0)
        if((tidx + MCM_PART_SPLIT_LEN) <= post_dlen)
            end_type = memcmp(post_dcon + tidx, part_split_key, MCM_PART_SPLIT_LEN) == 0 ? 2 : 0;

    if(end_type == 0)
    {
        MCM_CEMSG("invalid, not find boundary CRLF or --");
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0,
                      "invalid, not find boundary CRLF or --");
        return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
    }

    tidx += end_type == 1 ? MCM_CRLF_LEN : MCM_PART_SPLIT_LEN;

    // 回傳取出的資料.
    *data_con_buf = post_dcon;
    *data_len_buf = cidx - (MCM_CRLF_LEN + MCM_PART_SPLIT_LEN);
    MCM_CUHDMSG("data[" MCM_DTYPE_USIZE_PF "][%p]", *data_len_buf, *data_con_buf);

    *last_part_buf = end_type == 2 ? 1 : 0;

    *post_con = post_dcon + tidx;
    *post_len = post_dlen - tidx;

    return MCM_RCODE_PASS;
}

int mcm_find_part(
    char **post_con,
    MCM_DTYPE_USIZE_TD *post_len,
    MCM_DTYPE_BOOL_TD *last_part_buf)
{
    int fret;
    char *post_dcon, *read_con, *name_con, *filename_con, *data_con;
    MCM_DTYPE_USIZE_TD post_dlen, read_len, data_len;
    struct part_info_t *tmp_part;


    MCM_CUHDMSG("=> %s", __FUNCTION__);

    post_dcon = *post_con;
    post_dlen = *post_len;

    // [part] 的內容 :
    //
    // 檔案類型 :
    // [Content-Disposition: form-data; name="${input-name}"; filename="${file-name}"] + [\r\n] +
    // [Content-Type: ${MIME-Type}] + [\r\n] +
    // [\r\n] +
    // [data]
    //
    // 其他類型 :
    // [Content-Disposition: form-data; name="${input-name}"] + [\r\n] +
    // [\r\n] +
    // [data]

    // 取出一行資料.
    fret = mcm_read_line(&post_dcon, &post_dlen, &read_con, &read_len);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CEMSG("invalid, not find CRLF");
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0, "invalid, not find CRLF");
        return fret;
    }
    MCM_CUHDMSG("read[" MCM_DTYPE_USIZE_PF "][%s]", read_len, read_con);
    // 檢查是不是 [Content-Disposition: ...] 部分.
    fret = mcm_compare_key_ncmp(&read_con, &read_len, MCM_CONTENT_DISPOSITION_KEY,
                                MCM_CONTENT_DISPOSITION_LEN, MCM_HEADER_TOKEN_KEY);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CEMSG("invalid, not find %s", MCM_CONTENT_DISPOSITION_KEY);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0,
                      "invalid, not find %s", MCM_CONTENT_DISPOSITION_KEY);
        return fret;
    }
    // 取出 [Content-Disposition: ...] 裡面的參數.
    fret = mcm_parse_disposition(read_con, read_len, &name_con, &filename_con);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CEMSG("call mcm_parse_disposition() fail");
        return fret;
    }

    // 取出一行一行資料, 直到 [data] 之前的 [\r\n].
    while(1)
    {
        fret = mcm_read_line(&post_dcon, &post_dlen, &read_con, &read_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_CEMSG("invalid, not find CRLF");
            MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0, "invalid, not find CRLF");
            return fret;
        }
        if(read_len == 0)
            break;
    }

    // 取出 [data].
    fret = mcm_parse_data(&post_dcon, &post_dlen, &data_con, &data_len, last_part_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CEMSG("call mcm_parse_disposition() fail");
        return fret;
    }

    if(strcmp(name_con, MCM_CALLBACK_KEY) == 0)
    {
        // 取出 callback 函式名稱.
        if(data_con[0] == '\0')
        {
            MCM_CEMSG("invalid, empty %s function name", MCM_CALLBACK_KEY);
            MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0,
                          "invalid, empty %s function name", MCM_CALLBACK_KEY);
            return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
        }
        else
        {
            if(callback_name != NULL)
            {
                MCM_CEMSG("invalid, duplic %s tag", MCM_CALLBACK_KEY);
                MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0,
                              "invalid, duplic %s tag", MCM_CALLBACK_KEY);
                return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
            }

            callback_name = data_con;
            MCM_CUHDMSG("find callback[%s]", callback_name);
        }
    }
    else
    {
        // 加入新的 part.
        tmp_part = calloc(1, sizeof(struct part_info_t));
        if(tmp_part == NULL)
        {
            MCM_CEMSG("call mcalloc() fail");
            MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0, "call mcalloc() fail");
            return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
        }
        MCM_CUHDMSG("alloc tmp_part[%zu][%p]", sizeof(struct part_info_t), tmp_part);

        tmp_part->name_tag = name_con;
        tmp_part->filename_tag = filename_con;
        tmp_part->data_con = data_con;
        tmp_part->data_len = data_len;
        MCM_CUHDMSG("new part");
        MCM_CUHDMSG("name[%s]", tmp_part->name_tag);
        MCM_CUHDMSG("filename[%s]", tmp_part->filename_tag);
        MCM_CUHDMSG("data[" MCM_DTYPE_USIZE_PF "][%p]", tmp_part->data_len, tmp_part->data_con);

        if(part_list_head == NULL)
        {
            part_list_head = part_list_tail = tmp_part;
        }
        else
        {
            part_list_tail->next_part = tmp_part;
            tmp_part->prev_part = part_list_tail;
            part_list_tail = tmp_part;
        }
    }

    *post_con = post_dcon;
    *post_len = post_dlen;

    return fret;
}

int mcm_find_multipart(
    char *post_con,
    MCM_DTYPE_USIZE_TD post_len)
{
    int fret;
    char *read_con;
    MCM_DTYPE_USIZE_TD read_len;
    MCM_DTYPE_BOOL_TD last_part = 0;


    MCM_CUHDMSG("=> %s", __FUNCTION__);

    //  multipart 的資料內容 :
    // [--] + [boundary] + [\r\n] +
    // [part 1] +
    // [part 2] +
    // [part N] + [\r\n]

    // 取出一行資料.
    fret = mcm_read_line(&post_con, &post_len, &read_con, &read_len);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CEMSG("invalid, not find first CRLF");
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0, "invalid, not find first CRLF");
        return fret;
    }
    MCM_CUHDMSG("read[" MCM_DTYPE_USIZE_PF "][%s]", read_len, read_con);
    // 檢查是否有第一個 [--].
    if(read_len < MCM_PART_SPLIT_LEN)
    {
        MCM_CEMSG("invalid, not find first --");
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0, "invalid, not find first --");
        return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
    }
    if(memcmp(read_con, part_split_key, MCM_PART_SPLIT_LEN) != 0)
    {
        MCM_CEMSG("invalid, not find first --");
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0, "invalid, not find first --");
        return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
    }
    // 檢查是否有第一個 [boundary].
    if(strcmp(read_con + MCM_PART_SPLIT_LEN, boundary_key) != 0)
    {
        MCM_CEMSG("invalid, not find first boundary");
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0, "invalid, not find first boundary");
        return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
    }
    MCM_CUHDMSG("find first boundary");

    // 處理每一個 [part].
    while(last_part == 0)
    {
        fret = mcm_find_part(&post_con, &post_len, &last_part);
        if(fret < 0)
        {
            MCM_CEMSG("call mcm_find_part() fail");
            return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
        }
    }

    return fret;
}

int mcm_process_post(
    char *post_con,
    MCM_DTYPE_USIZE_TD post_len)
{
    int fret;
    MCM_DTYPE_USIZE_TD cidx;


    MCM_CUHDMSG("=> %s", __FUNCTION__);

    fret = mcm_find_multipart(post_con, post_len);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_CEMSG("call mcm_find_multipart() fail");
        goto FREE_01;
    }

    if(callback_name == NULL)
    {
        MCM_CEMSG("invalid, not find callback function name");
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 0,
                      "invalid, not find callback function name");
        fret = MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
        goto FREE_01;
    }

    for(cidx = 0; mcm_upload_callback_list[cidx].cb_name[0] != '\0'; cidx++)
        if(strcmp(callback_name, mcm_upload_callback_list[cidx].cb_name) == 0)
        {
            if(mcm_upload_callback_list[cidx].cb_function != NULL)
                mcm_upload_callback_list[cidx].cb_function(part_list_head);
            break;
        }
    if(mcm_upload_callback_list[cidx].cb_name[0] == '\0')
    {
        MCM_CEMSG("invalid, unknown callback [%s]", callback_name);
        MCM_CGI_AEMSG(MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR, 1,
                      "invalid, unknown callback [%s]", callback_name);
        return MCM_RCODE_CGI_UPLOAD_INTERNAL_ERROR;
    }

FREE_01:
    for(part_list_tail = part_list_head; part_list_tail != NULL; part_list_tail = part_list_head)
    {
        part_list_head = part_list_head->next_part;
        MCM_CUHDMSG("free part_list[%p]", part_list_tail);
        free(part_list_tail);
    }
    return fret;
}

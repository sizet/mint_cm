// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "../mcm_lheader/mcm_type.h"
#include "../mcm_lheader/mcm_control.h"
#include "../mcm_lheader/mcm_request.h"
#include "../mcm_lheader/mcm_connect.h"
#include "../mcm_lheader/mcm_return.h"
#include "../mcm_lheader/mcm_debug.h"
#include "mcm_lulib_api.h"




// 預設的封包緩衝大小.
#define MCM_USER_PACKET_BUFFER_SIZE 512




#define MCM_BUILD_BASE_REQ_01(tmp_offset, this_lulib, req_code) \
    do                                                              \
    {                                                               \
        /* T */                                                     \
        tmp_offset = this_lulib->pkt_buf;                           \
        *((MCM_DTYPE_USIZE_TD *) tmp_offset) = this_lulib->pkt_len; \
        tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);                   \
        /* REQ */                                                   \
        *((MCM_DTYPE_LIST_TD *) tmp_offset) = req_code;             \
        tmp_offset += sizeof(MCM_DTYPE_LIST_TD);                    \
        /* ... */                                                   \
    }                                                               \
    while(0)

#define MCM_BUILD_BASE_REQ_02(tmp_offset, this_lulib, req_code, req_path, req_plen) \
    do                                                           \
    {                                                            \
        /* T + REQ */                                            \
        MCM_BUILD_BASE_REQ_01(tmp_offset, this_lulib, req_code); \
        /* PC */                                                 \
        memcpy(tmp_offset, req_path, req_plen);                  \
        tmp_offset += req_plen;                                  \
        /* ... */                                                \
    }                                                            \
    while(0)

#define MCM_BUILD_BASE_REQ_03(tmp_offset, this_lulib, req_code, req_path, req_plen) \
    do                                                           \
    {                                                            \
        /* T + REQ */                                            \
        MCM_BUILD_BASE_REQ_01(tmp_offset, this_lulib, req_code); \
        /* PL */                                                 \
        *((MCM_DTYPE_USIZE_TD *) tmp_offset) = req_plen;         \
        tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);                \
        /* PC */                                                 \
        memcpy(tmp_offset, req_path, req_plen);                  \
        tmp_offset += req_plen;                                  \
        /* ... */                                                \
    }                                                            \
    while(0)

#define MCM_PARSE_BASE_REP(tmp_offset, this_lulib, tmp_code) \
    do                                                                 \
    {                                                                  \
        /* T */                                                        \
        tmp_offset = this_lulib->pkt_buf + sizeof(MCM_DTYPE_USIZE_TD); \
        /* REP */                                                      \
        tmp_code = *((MCM_DTYPE_LIST_TD *) tmp_offset);                \
        if(mcm_lulib_show_msg != 0)                                    \
        {                                                              \
            MCM_LUDMSG("tmp_code[" MCM_DTYPE_LIST_PF "]", tmp_code);   \
        }                                                              \
        tmp_offset += sizeof(MCM_DTYPE_LIST_TD);                       \
    }                                                                  \
    while(0)




// 是否使用 printf 顯示訊息 (MCM_EMSG, MCM_ECTMSG, MCM_LUDMSG).
// 1 : 是.
// 0 : 否.
// 如果輸出被重定向到其他非 console 裝置 (如 CGI),
// 則需要在使用此函式庫的程式內將值設為 0.
MCM_DTYPE_BOOL_TD mcm_lulib_show_msg = 1;




// 增加封包的緩衝大小.
// this_lulib (I) :
//   要增加緩衝的結構.
// new_size (I) :
//   要設定的大小.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_realloc_buf_lib(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_USIZE_TD new_size)
{
    void *tmp_buf;


    tmp_buf = realloc(this_lulib->pkt_buf, new_size);
    if(tmp_buf == NULL)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call realloc() fail [%s]", strerror(errno));
        }
        return MCM_RCODE_LULIB_ALLOC_FAIL;
    }
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("realloc [" MCM_DTYPE_USIZE_PF "][%p] -> [" MCM_DTYPE_USIZE_PF "][%p]",
                   this_lulib->pkt_size, this_lulib->pkt_buf, new_size, tmp_buf);
    }

    this_lulib->pkt_buf = tmp_buf;
    this_lulib->pkt_size = new_size;

    return MCM_RCODE_PASS;
}

// 傳送封包.
// this_lklib (I) :
//   連線資訊.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_send_req(
    struct mcm_lulib_lib_t *this_lulib)
{
    if(send(this_lulib->sock_fd, this_lulib->pkt_buf, this_lulib->pkt_len, 0) == -1)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call send() fail [%s]", strerror(errno));
        }
        return MCM_RCODE_LULIB_SOCKET_ERROR;
    }

    return MCM_RCODE_PASS;
}

// 接收封包.
// this_lulib (I) :
//   連線資訊.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_recv_rep(
    struct mcm_lulib_lib_t *this_lulib)
{
    int fret;
    void *bloc;
    MCM_DTYPE_USIZE_TD bsize;
    ssize_t rlen;
    MCM_DTYPE_USIZE_TD tlen, clen;


    bloc = this_lulib->pkt_buf;
    bsize = this_lulib->pkt_size;
    tlen = clen = 0;
    while(1)
    {
        rlen = recv(this_lulib->sock_fd, bloc, bsize, 0);
        if(rlen == -1)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call recv() fail [%s]", strerror(errno));
            }
            return errno == EINTR ? MCM_RCODE_LULIB_INTERRUPT : MCM_RCODE_LULIB_SOCKET_ERROR;
        }

        // 找到此封包應有的長度.
        if(tlen == 0)
        {
            if(rlen < sizeof(MCM_DTYPE_USIZE_TD))
            {
                if(mcm_lulib_show_msg != 0)
                {
                    MCM_EMSG("recv fail (packet too small [%zd])", rlen);
                }
                return MCM_RCODE_LULIB_SOCKET_ERROR;
            }

            // 如果緩衝不夠, 調整大小.
            tlen = *((MCM_DTYPE_USIZE_TD *) bloc);
            if(tlen > this_lulib->pkt_size)
            {
                fret = mcm_realloc_buf_lib(this_lulib, tlen);
                if(fret < MCM_RCODE_PASS)
                {
                    if(mcm_lulib_show_msg != 0)
                    {
                        MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
                    }
                    return fret;
                }
            }
        }

        clen += rlen;
        bloc = this_lulib->pkt_buf + clen;
        bsize = this_lulib->pkt_size - clen;

        if(clen >= tlen)
        {
            this_lulib->pkt_len = tlen;
            break;
        }
    }
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("rep[" MCM_DTYPE_USIZE_PF "]", this_lulib->pkt_len);
    }

    return MCM_RCODE_PASS;
}

// 資料初始化.
// this_lulib (I) :
//   連線資訊.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_lulib_init(
    struct mcm_lulib_lib_t *this_lulib)
{
    int fret;
    struct sockaddr_un sock_addr;
    socklen_t addr_len;
    struct mcm_connect_option_t connect_data;
    char notify_usable;


    fret = mcm_realloc_buf_lib(this_lulib, MCM_USER_PACKET_BUFFER_SIZE);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
        }
        goto FREE_01;
    }

    memset(&sock_addr, 0, sizeof(struct sockaddr_un));
    sock_addr.sun_family = AF_UNIX;
    snprintf(sock_addr.sun_path, sizeof(sock_addr.sun_path), "%s", this_lulib->socket_path);
    sock_addr.sun_path[0] = '\0';
    addr_len = strlen(this_lulib->socket_path) + offsetof(struct sockaddr_un, sun_path);

    this_lulib->sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(this_lulib->sock_fd == -1)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call socket() fail [%s]", strerror(errno));
        }
        fret = MCM_RCODE_LULIB_SOCKET_ERROR;
        goto FREE_02;
    }

    if((connect(this_lulib->sock_fd, (struct sockaddr *) &sock_addr, addr_len)) == -1)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call connect() fail [%s]", strerror(errno));
        }
        fret = errno == EINTR ? MCM_RCODE_LULIB_INTERRUPT : MCM_RCODE_LULIB_SOCKET_ERROR;
        goto FREE_03;
    }

    memset(&connect_data, 0, sizeof(struct mcm_connect_option_t));
    connect_data.call_from = this_lulib->call_from;
    connect_data.session_permission = this_lulib->session_permission;
    connect_data.session_stack_size = this_lulib->session_stack_size;
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("connect[" MCM_DTYPE_LIST_PF "][%s][" MCM_DTYPE_USIZE_PF "]",
                   connect_data.call_from, MCM_DBG_SPERMISSION(connect_data.session_permission),
                   connect_data.session_stack_size);
    }

    if(send(this_lulib->sock_fd, &connect_data, sizeof(struct mcm_connect_option_t), 0) == -1)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call send() fail [%s]", strerror(errno));
        }
        fret = MCM_RCODE_LULIB_SOCKET_ERROR;
        goto FREE_03;
    }

    if(recv(this_lulib->sock_fd, &notify_usable, sizeof(notify_usable), 0) == -1)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call recv() fail [%s]", strerror(errno));
        }
        fret = errno == EINTR ? MCM_RCODE_LULIB_INTERRUPT : MCM_RCODE_LULIB_SOCKET_ERROR;
        goto FREE_03;
    }

    return MCM_RCODE_PASS;
FREE_03:
    close(this_lulib->sock_fd);
FREE_02:
    free(this_lulib->pkt_buf);
FREE_01:
    return fret;
}

// 結束使用.
// this_lulib (I) :
//   連線資訊.
// return :
//   MCM_RCODE_PASS.
int mcm_lulib_exit(
    struct mcm_lulib_lib_t *this_lulib)
{
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("close sock_fd[%d]", this_lulib->sock_fd);
    }
    close(this_lulib->sock_fd);

    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("free pkt_buf[%p]", this_lulib->pkt_buf);
    }
    free(this_lulib->pkt_buf);

    return MCM_RCODE_PASS;
}

int mcm_lulib_get_alone(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void *data_buf)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(full_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PC.
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lulib, MCM_SREQUEST_GET_ALONE, full_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | DL | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + DL + DC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的資料的長度.
    // DC  [binary].
    //     紀錄回應的資料.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call fail (%d)", fret);
            }
        goto FREE_01;
    }
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    }
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(xlen > 0)
        memcpy(data_buf, tmp_offset, xlen);

FREE_01:
    return fret;
}

int mcm_lulib_set_alone(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(full_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen +
                          sizeof(MCM_DTYPE_USIZE_TD) + data_len + 1;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC | DL | DC | \0 |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PL + PC + DL + DC + \0.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄要傳送的資料的長度.
    // DC  [binary].
    //     紀錄要傳送的資料.

    // T + REQ + PL + PC.
    MCM_BUILD_BASE_REQ_03(tmp_offset, this_lulib, MCM_SREQUEST_SET_ALONE, full_path, xlen);
    // DL.
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = data_len;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    memcpy(tmp_offset, data_con, data_len);
    tmp_offset += data_len;
    // \0.
    *((unsigned char *) tmp_offset) = 0;

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call fail (%d)", fret);
            }
        goto FREE_01;
    }

FREE_01:
    return fret;
}

int mcm_lulib_get_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void *data_buf)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(full_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PC.
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lulib, MCM_SREQUEST_GET_ENTRY, full_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | DL | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + DL + DC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的資料的長度.
    // DC  [binary].
    //     紀錄回應的資料.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call fail (%d)", fret);
            }
        goto FREE_01;
    }
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    }
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(xlen > 0)
        memcpy(data_buf, tmp_offset, xlen);

FREE_01:
    return fret;
}

int mcm_lulib_set_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(full_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen +
                          data_len;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PL + PC + DC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.
    // DC  [binary].
    //     紀錄要傳送的資料.

    // T + REQ + PL + PC.
    MCM_BUILD_BASE_REQ_03(tmp_offset, this_lulib, MCM_SREQUEST_SET_ENTRY, full_path, xlen);
    // DC.
    memcpy(tmp_offset, data_con, data_len);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call fail (%d)", fret);
            }
        goto FREE_01;
    }

FREE_01:
    return fret;
}

int mcm_lulib_add_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    char *insert_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen, ilen;
    void *tmp_offset;


    xlen = strlen(full_path) + 1;
    ilen = insert_path == NULL ? 1 : strlen(insert_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen +
                          sizeof(MCM_DTYPE_USIZE_TD) + ilen +
                          sizeof(MCM_DTYPE_USIZE_TD) + data_len;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC | IL | IC | DL | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PL + PC + IL + IC + DL + DC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.
    // IL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的插入路徑長度 (包括最後的 \0).
    // IC  [binary].
    //     紀錄請求的插入路徑.
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄要傳送的資料的長度.
    // DC  [binary].
    //     紀錄要傳送的資料.

    // T + REQ + PL + PC.
    MCM_BUILD_BASE_REQ_03(tmp_offset, this_lulib, MCM_SREQUEST_ADD_ENTRY, full_path, xlen);
    // IL.
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = ilen;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // IC.
    if(insert_path == NULL)
        *((char *) tmp_offset) = '\0';
    else
        memcpy(tmp_offset, insert_path, ilen);
    tmp_offset += ilen;
    // DL.
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = data_len;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(data_con != NULL)
        memcpy(tmp_offset, data_con, data_len);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call fail (%d)", fret);
            }
        goto FREE_01;
    }

FREE_01:
    return fret;
}

int mcm_lulib_del_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(full_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PC.
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lulib, MCM_SREQUEST_DEL_ENTRY, full_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call fail (%d)", fret);
            }
        goto FREE_01;
    }

FREE_01:
    return fret;
}

int mcm_lulib_get_all_key(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path,
    MCM_DTYPE_EK_TD **key_buf,
    MCM_DTYPE_EK_TD *count_buf)
{
    int fret;
    MCM_DTYPE_EK_TD tmp_count;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset, *tmp_buf = NULL;


    *key_buf = NULL;

    xlen = strlen(mix_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PC.
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lulib, MCM_SREQUEST_GET_ALL_KEY, mix_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | EC | DL | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + EC + DL + DC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // EC  [MCM_DTYPE_EK_TD].
    //     紀錄回應的資料 (資料筆數).
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的資料的長度.
    // DC  [binary].
    //     紀錄回應的資料 (資料內容).

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call fail (%d)", fret);
            }
        goto FREE_01;
    }
    // EC.
    tmp_count = *((MCM_DTYPE_EK_TD *) tmp_offset);
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("count[" MCM_DTYPE_EK_PF "]", tmp_count);
    }
    tmp_offset += sizeof(MCM_DTYPE_EK_TD);
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    }
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(xlen > 0)
    {
        tmp_buf = malloc(xlen);
        if(tmp_buf == NULL)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call malloc() fail [%s]", strerror(errno));
            }
            fret = MCM_RCODE_LULIB_ALLOC_FAIL;
            goto FREE_01;
        }
        memcpy(tmp_buf, tmp_offset, xlen);
    }

    *key_buf = tmp_buf;
    *count_buf = tmp_count;

FREE_01:
    return fret;
}

int mcm_lulib_get_all_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path,
    void **data_buf,
    MCM_DTYPE_EK_TD *count_buf)
{
    int fret;
    MCM_DTYPE_EK_TD tmp_count;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset, *tmp_buf = NULL;


    *data_buf = NULL;

    xlen = strlen(mix_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PC.
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lulib, MCM_SREQUEST_GET_ALL_ENTRY, mix_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | EC | DL | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + EC + DL + DC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // EC  [MCM_DTYPE_EK_TD].
    //     紀錄回應的資料 (資料筆數).
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的資料的長度.
    // DC  [binary].
    //     紀錄回應的資料 (資料內容).

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call fail (%d)", fret);
            }
        goto FREE_01;
    }
    // EC.
    tmp_count = *((MCM_DTYPE_EK_TD *) tmp_offset);
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("count[" MCM_DTYPE_EK_PF "]", tmp_count);
    }
    tmp_offset += sizeof(MCM_DTYPE_EK_TD);
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    }
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(xlen > 0)
    {
        tmp_buf = malloc(xlen);
        if(tmp_buf == NULL)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call malloc() fail [%s]", strerror(errno));
            }
            fret = MCM_RCODE_LULIB_ALLOC_FAIL;
            goto FREE_01;
        }
        memcpy(tmp_buf, tmp_offset, xlen);
    }

    *data_buf = tmp_buf;
    *count_buf = tmp_count;

FREE_01:
    return fret;
}

int mcm_lulib_del_all_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(mix_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PC.
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lulib, MCM_SREQUEST_DEL_ALL_ENTRY, mix_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call fail (%d)", fret);
            }
        goto FREE_01;
    }

FREE_01:
    return fret;
}

int mcm_lulib_get_max_count(
    struct mcm_lulib_lib_t *this_lulib,
    char *mask_path,
    MCM_DTYPE_EK_TD *count_buf)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(mask_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PC.
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lulib, MCM_SREQUEST_GET_MAX_COUNT, mask_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | EC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + EC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // EC  [MCM_DTYPE_EK_TD].
    //     紀錄回應的資料.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }
    // EC.
    *count_buf = *((MCM_DTYPE_EK_TD *) tmp_offset);
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("max_count[" MCM_DTYPE_EK_PF "]", *count_buf);
    }

FREE_01:
    return fret;
}

int mcm_lulib_get_count(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path,
    MCM_DTYPE_EK_TD *count_buf)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(mix_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PC.
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lulib, MCM_SREQUEST_GET_COUNT, mix_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | EC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + EC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // EC  [MCM_DTYPE_EK_TD].
    //     紀錄回應的資料.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call fail (%d)", fret);
            }
        goto FREE_01;
    }
    // EC.
    *count_buf = *((MCM_DTYPE_EK_TD *) tmp_offset);
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("count[" MCM_DTYPE_EK_PF "]", *count_buf);
    }

FREE_01:
    return fret;
}

int mcm_lulib_get_usable_key(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path,
    MCM_DTYPE_EK_TD *key_buf)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(mix_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PC.
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lulib, MCM_SREQUEST_GET_USABLE_KEY, mix_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | EK |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + EK.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // EK  [MCM_DTYPE_EK_TD].
    //     紀錄回應的資料.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call fail (%d)", fret);
            }
        goto FREE_01;
    }
    // EK.
    *key_buf = *((MCM_DTYPE_EK_TD *) tmp_offset);
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("usable_key[" MCM_DTYPE_EK_PF "]", *key_buf);
    }

FREE_01:
    return fret;
}

int mcm_lulib_update(
    struct mcm_lulib_lib_t *this_lulib)
{
    int fret;
    void *tmp_offset;


    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD);

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.

    // T + REQ.
    MCM_BUILD_BASE_REQ_01(tmp_offset, this_lulib, MCM_SREQUEST_UPDATE);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }

FREE_01:
    return fret;
}

int mcm_lulib_save(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_BOOL_TD force_save)
{
    int fret;
    void *tmp_offset;


    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_BOOL_TD);

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + DC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // DC  [MCM_DTYPE_BOOL_TD].
    //     紀錄要傳送的資料.

    // T + REQ.
    MCM_BUILD_BASE_REQ_01(tmp_offset, this_lulib, MCM_SREQUEST_SAVE);
    // DC.
    memcpy(tmp_offset, &force_save, sizeof(MCM_DTYPE_BOOL_TD));

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }

FREE_01:
    return fret;
}

int mcm_lulib_run(
    struct mcm_lulib_lib_t *this_lulib,
    char *module_function,
    void *req_data_con,
    MCM_DTYPE_USIZE_TD req_data_len,
    void **rep_data_buf,
    MCM_DTYPE_USIZE_TD *rep_data_len_buf)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset, *tmp_buf;


    if(rep_data_buf != NULL)
        *rep_data_buf = NULL;
    if(rep_data_len_buf != NULL)
        *rep_data_len_buf = 0;

    xlen = strlen(module_function) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen +
                          sizeof(MCM_DTYPE_USIZE_TD) + req_data_len;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC | DL | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PL + PC + DL + DC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄要傳送的資料的長度.
    // DC  [binary].
    //     紀錄要傳送的資料.

    // T + REQ + PL + PC.
    MCM_BUILD_BASE_REQ_03(tmp_offset, this_lulib, MCM_SREQUEST_RUN, module_function, xlen);
    // DL.
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = req_data_len;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(req_data_con != NULL)
        memcpy(tmp_offset, req_data_con, req_data_len);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | DL | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + DL + DC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的資料的長度.
    // DC  [binary].
    //     紀錄回應的資料.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call fail (%d)", fret);
        }
    }
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    }
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if((xlen > 0) && (rep_data_buf != NULL))
    {
        tmp_buf = malloc(xlen);
        if(tmp_buf == NULL)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call malloc() fail [%s]", strerror(errno));
            }
            fret = MCM_RCODE_LULIB_ALLOC_FAIL;
            goto FREE_01;
        }
        memcpy(tmp_buf, tmp_offset, xlen);

        *rep_data_buf = tmp_buf;
        if(rep_data_len_buf != NULL)
            *rep_data_len_buf = xlen;
    }

FREE_01:
    return fret;
}

int mcm_lulib_shutdown(
    struct mcm_lulib_lib_t *this_lulib)
{
    int fret;
    void *tmp_offset;


    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD);

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.

    // T + REQ.
    MCM_BUILD_BASE_REQ_01(tmp_offset, this_lulib, MCM_SREQUEST_SHUTDOWN);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }

FREE_01:
    return fret;
}

int mcm_lulib_check_store_file(
    struct mcm_lulib_lib_t *this_lulib,
    char *file_path,
    MCM_DTYPE_LIST_TD *store_result_buf,
    char *store_version_buf,
    MCM_DTYPE_USIZE_TD store_version_size)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen, clen;
    void *tmp_offset;


    xlen = strlen(file_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄檢查的檔案的路徑.

    // T + REQ + PC.
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lulib, MCM_SREQUEST_CHECK_STORE_FILE, file_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | SR | VL | VC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + + SR + VL + VC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // SR  [MCM_DTYPE_LIST_TD].
    //     紀錄檔案的檢查結果.
    // VL  [MCM_DTYPE_USIZE_TD].
    //     紀錄檔案的版本資訊的長度 (包含最後的 \0).
    // VC  [binary].
    //     紀錄檔案的版本資訊.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }
    // SR.
    *store_result_buf = *((MCM_DTYPE_LIST_TD *) tmp_offset);
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("store_result[" MCM_DTYPE_LIST_PF "]", *store_result_buf);
    }
    tmp_offset += sizeof(MCM_DTYPE_LIST_TD);
    // VL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("version_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    }
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // VC.
    if(xlen > 0)
    {
        clen = store_version_size - 1;
        clen = clen > xlen ? xlen : clen;
        memcpy(store_version_buf, tmp_offset, clen);
        store_version_buf[clen] = '\0';
        if(mcm_lulib_show_msg != 0)
        {
            MCM_LUDMSG("version_con[%s]", store_version_buf);
        }
    }

FREE_01:
    return fret;
}

int mcm_lulib_check_mask_path(
    struct mcm_lulib_lib_t *this_lulib,
    char *mask_path)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(mask_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PC.
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lulib, MCM_SREQUEST_CHECK_MASK_PATH, mask_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }

FREE_01:
    return fret;
}

int mcm_lulib_get_path_max_length(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_USIZE_TD *max_len_buf)
{
    int fret;
    void *tmp_offset;


    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD);

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.

    // T + REQ.
    MCM_BUILD_BASE_REQ_01(tmp_offset, this_lulib, MCM_SREQUEST_GET_PATH_MAX_LENGTH);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | PML |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + PML.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // PML [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的資料.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }
    // PML.
    *max_len_buf = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("path_max_length[" MCM_DTYPE_USIZE_PF "]", *max_len_buf);
    }

FREE_01:
    return fret;
}

int mcm_lulib_get_list_name(
    struct mcm_lulib_lib_t *this_lulib,
    char *mask_path,
    void **data_buf_buf,
    MCM_DTYPE_USIZE_TD *data_len_buf)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset, *tmp_buf;


    xlen = strlen(mask_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PC.
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lulib, MCM_SREQUEST_GET_LIST_NAME, mask_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | DL | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + DL + DC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的資料的長度.
    // DC  [binary].
    //     紀錄回應的資料.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    }
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(xlen > 0)
    {
        tmp_buf = malloc(xlen);
        if(tmp_buf == NULL)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call malloc() fail [%s]", strerror(errno));
            }
            fret = MCM_RCODE_LULIB_ALLOC_FAIL;
            goto FREE_01;
        }
        memcpy(tmp_buf, tmp_offset, xlen);
        *data_buf_buf = tmp_buf;
        *data_len_buf = xlen;
    }

FREE_01:
    return fret;
}

int mcm_lulib_get_list_type(
    struct mcm_lulib_lib_t *this_lulib,
    char *mask_path,
    void **data_buf_buf,
    MCM_DTYPE_USIZE_TD *data_len_buf)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset, *tmp_buf;


    xlen = strlen(mask_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PC.
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lulib, MCM_SREQUEST_GET_LIST_TYPE, mask_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | DL | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + DL + DC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的資料的長度.
    // DC  [binary].
    //     紀錄回應的資料.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    }
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(xlen > 0)
    {
        tmp_buf = malloc(xlen);
        if(tmp_buf == NULL)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call malloc() fail [%s]", strerror(errno));
            }
            fret = MCM_RCODE_LULIB_ALLOC_FAIL;
            goto FREE_01;
        }
        memcpy(tmp_buf, tmp_offset, xlen);
        *data_buf_buf = tmp_buf;
        *data_len_buf = xlen;
    }

FREE_01:
    return fret;
}

int mcm_lulib_get_list_value(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void **data_buf_buf,
    MCM_DTYPE_USIZE_TD *data_len_buf)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset, *tmp_buf;


    xlen = strlen(full_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PC.
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lulib, MCM_SREQUEST_GET_LIST_VALUE, full_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | DL | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + DL + DC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的資料的長度.
    // DC  [binary].
    //     紀錄回應的資料.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call fail (%d)", fret);
            }
        goto FREE_01;
    }
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    }
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(xlen > 0)
    {
        tmp_buf = malloc(xlen);
        if(tmp_buf == NULL)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call malloc() fail [%s]", strerror(errno));
            }
            fret = MCM_RCODE_LULIB_ALLOC_FAIL;
            goto FREE_01;
        }
        memcpy(tmp_buf, tmp_offset, xlen);
        *data_buf_buf = tmp_buf;
        *data_len_buf = xlen;
    }

FREE_01:
    return fret;
}

int mcm_lulib_set_any_type_alone(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    char *data_con)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen, dlen;
    void *tmp_offset;


    xlen = strlen(full_path) + 1;
    dlen = strlen(data_con);
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen +
                          sizeof(MCM_DTYPE_USIZE_TD) + dlen + 1;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC | DL | DC | \0 |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PL + PC + DL + DC + \0.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄要傳送的資料的長度.
    // DC  [binary].
    //     紀錄要傳送的資料.

    // T + REQ + PL + PC.
    MCM_BUILD_BASE_REQ_03(tmp_offset, this_lulib, MCM_SREQUEST_SET_ANY_TYPE_ALONE,
                          full_path, xlen);
    // DL.
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = dlen;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    memcpy(tmp_offset, data_con, dlen);
    tmp_offset += dlen;
    // \0.
    *((unsigned char *) tmp_offset) = 0;

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call fail (%d)", fret);
            }
        goto FREE_01;
    }

FREE_01:
    return fret;
}

int mcm_lulib_get_with_type_alone(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    MCM_DTYPE_LIST_TD *type_buf,
    void **data_buf_buf,
    MCM_DTYPE_USIZE_TD *data_len_buf)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;
    MCM_DTYPE_LIST_TD tmp_type;
    MCM_DTYPE_USIZE_TD tmp_len;
    void *tmp_data;


    xlen = strlen(full_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PC.
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lulib, MCM_SREQUEST_GET_WITH_TYPE_ALONE,
                          full_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_ECTMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | DT | DL | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + DT + DL + DC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // DT  [MCM_DTYPE_LIST_TD]
    //     紀錄回應的資料的類型.
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的字串資料的長度.
    // DC  [binary].
    //     紀錄回應的字串資料.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lulib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
            if(mcm_lulib_show_msg != 0)
            {
                MCM_ECTMSG("call fail (%d)", fret);
            }
        goto FREE_01;
    }
    // DT.
    tmp_type = *((MCM_DTYPE_LIST_TD *) tmp_offset);
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("data_type[" MCM_DTYPE_LIST_PF "]", tmp_type);
    }
    tmp_offset += sizeof(MCM_DTYPE_LIST_TD);
    // DL.
    tmp_len = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", tmp_len);
    }
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    //DC.
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("data_con[%s]", (char *) tmp_offset);
    }

    if(type_buf != NULL)
        *type_buf = tmp_type;

    if(data_buf_buf != NULL)
    {
        tmp_data = malloc(tmp_len);
        if(tmp_data == NULL)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call malloc() fail [%s]", strerror(errno));
            }
            fret = MCM_RCODE_LULIB_ALLOC_FAIL;
            goto FREE_01;
        }
        memcpy(tmp_data, tmp_offset, tmp_len);
        *data_buf_buf = tmp_data;
    }

    if(data_len_buf != NULL)
        *data_len_buf = tmp_len;

FREE_01:
    return fret;
}

int mcm_lulib_do_get_alone(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void *data_buf)
{
    int fret;


    fret = mcm_lulib_init(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_init() fail");
        return fret;
    }

    fret = mcm_lulib_get_alone(this_lulib, full_path, data_buf);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_lulib_get_alone() fail");
        }
    }

    mcm_lulib_exit(this_lulib);

    return fret;
}

int mcm_lulib_do_set_alone(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    int fret;


    fret = mcm_lulib_init(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_init() fail");
        return fret;
    }

    fret = mcm_lulib_set_alone(this_lulib, full_path, data_con, data_len);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_lulib_set_alone() fail");
        }
    }

    mcm_lulib_exit(this_lulib);

    return fret;
}

int mcm_lulib_do_get_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void *data_buf)
{
    int fret;


    fret = mcm_lulib_init(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_init() fail");
        return fret;
    }

    fret = mcm_lulib_get_entry(this_lulib, full_path, data_buf);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_lulib_get_entry() fail");
        }
    }

    mcm_lulib_exit(this_lulib);

    return fret;
}

int mcm_lulib_do_set_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    int fret;


    fret = mcm_lulib_init(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_init() fail");
        return fret;
    }

    fret = mcm_lulib_set_entry(this_lulib, full_path, data_con, data_len);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_lulib_set_entry() fail");
        }
    }

    mcm_lulib_exit(this_lulib);

    return fret;
}

int mcm_lulib_do_add_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    char *insert_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    int fret;


    fret = mcm_lulib_init(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_init() fail");
        return fret;
    }

    fret = mcm_lulib_add_entry(this_lulib, full_path, insert_path, data_con, data_len);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_lulib_add_entry() fail");
        }
    }

    mcm_lulib_exit(this_lulib);

    return fret;
}

int mcm_lulib_do_del_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path)
{
    int fret;


    fret = mcm_lulib_init(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_init() fail");
        return fret;
    }

    fret = mcm_lulib_del_entry(this_lulib, full_path);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_lulib_del_entry() fail");
        }
    }

    mcm_lulib_exit(this_lulib);

    return fret;
}

int mcm_lulib_do_get_all_key(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path,
    MCM_DTYPE_EK_TD **key_buf,
    MCM_DTYPE_EK_TD *count_buf)
{
    int fret;


    fret = mcm_lulib_init(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_init() fail");
        return fret;
    }

    fret = mcm_lulib_get_all_key(this_lulib, mix_path, key_buf, count_buf);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_lulib_get_all_key() fail");
        }
    }

    mcm_lulib_exit(this_lulib);

    return fret;
}

int mcm_lulib_do_get_all_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path,
    void **data_buf,
    MCM_DTYPE_EK_TD *count_buf)
{
    int fret;


    fret = mcm_lulib_init(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_init() fail");
        return fret;
    }

    fret = mcm_lulib_get_all_entry(this_lulib, mix_path, data_buf, count_buf);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_lulib_get_all_entry() fail");
        }
    }

    mcm_lulib_exit(this_lulib);

    return fret;
}

int mcm_lulib_do_del_all_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path)
{
    int fret;


    fret = mcm_lulib_init(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_init() fail");
        return fret;
    }

    fret = mcm_lulib_del_all_entry(this_lulib, mix_path);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_lulib_del_all_entry() fail");
        }
    }

    mcm_lulib_exit(this_lulib);

    return fret;
}

int mcm_lulib_do_get_max_count(
    struct mcm_lulib_lib_t *this_lulib,
    char *mask_path,
    MCM_DTYPE_EK_TD *count_buf)
{
    int fret;


    fret = mcm_lulib_init(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_init() fail");
        return fret;
    }

    fret = mcm_lulib_get_max_count(this_lulib, mask_path, count_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_get_max_count() fail");
    }

    mcm_lulib_exit(this_lulib);

    return fret;
}

int mcm_lulib_do_get_count(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path,
    MCM_DTYPE_EK_TD *count_buf)
{
    int fret;


    fret = mcm_lulib_init(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_init() fail");
        return fret;
    }

    fret = mcm_lulib_get_count(this_lulib, mix_path, count_buf);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_lulib_get_count() fail");
        }
    }

    mcm_lulib_exit(this_lulib);

    return fret;
}

int mcm_lulib_do_get_usable_key(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path,
    MCM_DTYPE_EK_TD *key_buf)
{
    int fret;


    fret = mcm_lulib_init(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_init() fail");
        return fret;
    }

    fret = mcm_lulib_get_usable_key(this_lulib, mix_path, key_buf);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_lulib_get_usable_key() fail");
        }
    }

    mcm_lulib_exit(this_lulib);

    return fret;
}

int mcm_lulib_do_update(
    struct mcm_lulib_lib_t *this_lulib)
{
    int fret;


    fret = mcm_lulib_init(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_init() fail");
        return fret;
    }

    fret = mcm_lulib_update(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_update() fail");
    }

    mcm_lulib_exit(this_lulib);

    return fret;
}

int mcm_lulib_do_save(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_BOOL_TD force_save)
{
    int fret;


    fret = mcm_lulib_init(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_init() fail");
        return fret;
    }

    fret = mcm_lulib_save(this_lulib, force_save);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_save() fail");
    }

    mcm_lulib_exit(this_lulib);

    return fret;
}

int mcm_lulib_do_run(
    struct mcm_lulib_lib_t *this_lulib,
    char *module_function,
    void *req_data_con,
    MCM_DTYPE_USIZE_TD req_data_len,
    void **rep_data_buf,
    MCM_DTYPE_USIZE_TD *rep_data_len_buf)
{
    int fret;


    fret = mcm_lulib_init(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_init() fail");
        return fret;
    }

    fret = mcm_lulib_run(this_lulib, module_function, req_data_con, req_data_len,
                         rep_data_buf, rep_data_len_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_run() fail");
    }

    mcm_lulib_exit(this_lulib);

    return fret;
}

int mcm_lulib_do_shutdown(
    struct mcm_lulib_lib_t *this_lulib)
{
    int fret;


    fret = mcm_lulib_init(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_init() fail");
        return fret;
    }

    fret = mcm_lulib_shutdown(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_shutdown() fail");
    }

    mcm_lulib_exit(this_lulib);

    return fret;
}

int mcm_lulib_do_check_store_file(
    struct mcm_lulib_lib_t *this_lulib,
    char *file_path,
    MCM_DTYPE_LIST_TD *store_result_buf,
    char *store_version_buf,
    MCM_DTYPE_USIZE_TD store_version_size)
{
    int fret;


    fret = mcm_lulib_init(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_init() fail");
        return fret;
    }

    fret = mcm_lulib_check_store_file(this_lulib, file_path, store_result_buf,
                                      store_version_buf, store_version_size);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_lulib_check_store_file() fail");
    }

    mcm_lulib_exit(this_lulib);

    return fret;
}

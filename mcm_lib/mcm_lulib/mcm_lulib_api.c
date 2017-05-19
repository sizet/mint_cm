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




// 是否使用 printf 顯示訊息 (MCM_EMSG, MCM_LUDMSG).
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
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("realloc [" MCM_DTYPE_USIZE_PF "][%p] -> [" MCM_DTYPE_USIZE_PF "][%p]",
                   this_lulib->pkt_size, this_lulib->pkt_buf, new_size, tmp_buf);
    }
#endif
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

// 接收回傳的封包.
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
                        MCM_EMSG("call mcm_realloc_buf_lib() fail");
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
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("rep[" MCM_DTYPE_USIZE_PF "]", this_lulib->pkt_len);
    }
#endif

    return MCM_RCODE_PASS;
}

int mcm_build_base_req(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_LIST_TD req_code,
    MCM_DTYPE_BOOL_TD add_path,
    char *req_path,
    MCM_DTYPE_USIZE_TD req_plen)
{
    void *tmp_offset;

    // 封包格式 :
    // | T | REQ | PL | PC | ... |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PL + PC + ...
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.
    // ...
    //     剩餘的資料.

    // T.
    tmp_offset = this_lulib->pkt_buf;
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = this_lulib->pkt_len;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // REQ.
    *((MCM_DTYPE_LIST_TD *) tmp_offset) = req_code;
    tmp_offset += sizeof(MCM_DTYPE_LIST_TD);
    // PC + PC.
    if(add_path != 0)
    {
        // PL.
        *((MCM_DTYPE_USIZE_TD *) tmp_offset) = req_plen;
        tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
        // PC.
        memcpy(tmp_offset, req_path, req_plen);
        tmp_offset += req_plen;
    }
    // ...

    this_lulib->pkt_offset = tmp_offset;

    return 0;
}

int mcm_parse_base_rep(
    struct mcm_lulib_lib_t *this_lulib)
{
    void *tmp_offset;


    // 封包格式 :
    // | T | REP | ML | MC | ... |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC + ...
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.
    // ...
    //     剩餘的資料.

    // T.
    tmp_offset = this_lulib->pkt_buf;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // REP.
    this_lulib->rep_code = *((MCM_DTYPE_LIST_TD *) tmp_offset);
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("rep_code[" MCM_DTYPE_LIST_PF "]", this_lulib->rep_code);
    }
#endif
    tmp_offset += sizeof(MCM_DTYPE_LIST_TD);
    // ML.
    this_lulib->rep_msg_len = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // MC.
    this_lulib->rep_msg_con = tmp_offset;
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("rep_msg[" MCM_DTYPE_USIZE_PF "][%s]",
                   this_lulib->rep_msg_len, this_lulib->rep_msg_con);
    }
#endif
    tmp_offset += this_lulib->rep_msg_len;
    // ...
    this_lulib->pkt_offset = tmp_offset;

    if(this_lulib->rep_code < MCM_RCODE_PASS)
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("report fail [" MCM_DTYPE_LIST_PF "][%s]",
                     this_lulib->rep_code, this_lulib->rep_msg_con);
        }

    return this_lulib->rep_code;
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
            MCM_EMSG("call mcm_realloc_buf_lib() fail");
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
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("connect[" MCM_DTYPE_LIST_PF "][%s][" MCM_DTYPE_USIZE_PF "]",
                   connect_data.call_from, MCM_DBG_SPERMISSION(connect_data.session_permission),
                   connect_data.session_stack_size);
    }
#endif

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
    this_lulib->rep_code = fret;
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
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("close sock_fd[%d]", this_lulib->sock_fd);
    }
#endif
    close(this_lulib->sock_fd);

#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("free pkt_buf[%p]", this_lulib->pkt_buf);
    }
#endif
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


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    xlen = strlen(full_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PL + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑的長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PL + PC.
    mcm_build_base_req(this_lulib, MCM_SREQUEST_GET_ALONE, 1, full_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC | DL | DC | \0 |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC + DL + DC + \0.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的資料的長度.
    // DC  [binary].
    //     紀錄回應的資料.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }
    tmp_offset = this_lulib->pkt_offset;
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    }
#endif
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(xlen > 0)
        memcpy(data_buf, tmp_offset, xlen);

FREE_01:
    this_lulib->rep_code = fret;
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


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

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
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lulib, MCM_SREQUEST_SET_ALONE, 1, full_path, xlen);
    tmp_offset = this_lulib->pkt_offset;
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
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }

FREE_01:
    this_lulib->rep_code = fret;
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


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    xlen = strlen(full_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PL + PC + DS.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑的長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PL + PC.
    mcm_build_base_req(this_lulib, MCM_SREQUEST_GET_ENTRY, 1, full_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC | DL | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC + DL + DC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的資料的長度.
    // DC  [binary].
    //     紀錄回應的資料.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }
    tmp_offset = this_lulib->pkt_offset;
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    }
#endif
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(xlen > 0)
        memcpy(data_buf, tmp_offset, xlen);

FREE_01:
    this_lulib->rep_code = fret;
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


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    xlen = strlen(full_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen +
                          sizeof(MCM_DTYPE_USIZE_TD) + data_len;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lulib, MCM_SREQUEST_SET_ENTRY, 1, full_path, xlen);
    tmp_offset = this_lulib->pkt_offset;
    // DL.
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = data_len;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    memcpy(tmp_offset, data_con, data_len);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }

FREE_01:
    this_lulib->rep_code = fret;
    return fret;
}

int mcm_lulib_add_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    xlen = strlen(full_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen +
                          sizeof(MCM_DTYPE_USIZE_TD) + data_len;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lulib, MCM_SREQUEST_ADD_ENTRY, 1, full_path, xlen);
    tmp_offset = this_lulib->pkt_offset;
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
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }

FREE_01:
    this_lulib->rep_code = fret;
    return fret;
}

int mcm_lulib_del_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *full_path)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    xlen = strlen(full_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PL + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PL + PC.
    mcm_build_base_req(this_lulib, MCM_SREQUEST_DEL_ENTRY, 1, full_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }

FREE_01:
    this_lulib->rep_code = fret;
    return fret;
}

int mcm_lulib_get_all_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path,
    MCM_DTYPE_EK_TD *count_buf,
    void **data_buf)
{
    int fret;
    MCM_DTYPE_EK_TD tmp_count;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset, *tmp_buf = NULL;


    *data_buf = NULL;

    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    xlen = strlen(mix_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PL + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PL + PC.
    mcm_build_base_req(this_lulib, MCM_SREQUEST_GET_ALL_ENTRY, 1, mix_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC | EC | DL | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC + EC + DL + DC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.
    // EC  [MCM_DTYPE_EK_TD].
    //     紀錄回應的資料 (資料筆數).
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的資料的長度.
    // DC  [binary].
    //     紀錄回應的資料 (資料內容).

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }
    tmp_offset = this_lulib->pkt_offset;
    // EC.
    tmp_count = *((MCM_DTYPE_EK_TD *) tmp_offset);
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("count[" MCM_DTYPE_EK_PF "]", tmp_count);
    }
#endif
    tmp_offset += sizeof(MCM_DTYPE_EK_TD);
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    }
#endif
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

    *count_buf = tmp_count;
    *data_buf = tmp_buf;

FREE_01:
    this_lulib->rep_code = fret;
    return fret;
}

int mcm_lulib_del_all_entry(
    struct mcm_lulib_lib_t *this_lulib,
    char *mix_path)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    xlen = strlen(mix_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PL + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PL + PC.
    mcm_build_base_req(this_lulib, MCM_SREQUEST_DEL_ALL_ENTRY, 1, mix_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }

FREE_01:
    this_lulib->rep_code = fret;
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


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    xlen = strlen(mask_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PL + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PL + PC.
    mcm_build_base_req(this_lulib, MCM_SREQUEST_GET_MAX_COUNT, 1, mask_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC | EC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC + EC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.
    // EC  [MCM_DTYPE_EK_TD].
    //     紀錄回應的資料.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }
    tmp_offset = this_lulib->pkt_offset;
    // EC.
    *count_buf = *((MCM_DTYPE_EK_TD *) tmp_offset);
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("max_count[" MCM_DTYPE_EK_PF "]", *count_buf);
    }
#endif

FREE_01:
    this_lulib->rep_code = fret;
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


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    xlen = strlen(mix_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PL + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PL + PC.
    mcm_build_base_req(this_lulib, MCM_SREQUEST_GET_COUNT, 1, mix_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC | EC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC + EC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.
    // EC  [MCM_DTYPE_EK_TD].
    //     紀錄回應的資料.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }
    tmp_offset = this_lulib->pkt_offset;
    // EC.
    *count_buf = *((MCM_DTYPE_EK_TD *) tmp_offset);
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("count[" MCM_DTYPE_EK_PF "]", *count_buf);
    }
#endif

FREE_01:
    this_lulib->rep_code = fret;
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


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    xlen = strlen(mix_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PL + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PL + PC.
    mcm_build_base_req(this_lulib, MCM_SREQUEST_GET_USABLE_KEY, 1, mix_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC | EK |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC + EK.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.
    // EK  [MCM_DTYPE_EK_TD].
    //     紀錄回應的資料.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }
    tmp_offset = this_lulib->pkt_offset;
    // EK.
    *key_buf = *((MCM_DTYPE_EK_TD *) tmp_offset);
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("usable_key[" MCM_DTYPE_EK_PF "]", *key_buf);
    }
#endif

FREE_01:
    this_lulib->rep_code = fret;
    return fret;
}

int mcm_lulib_update(
    struct mcm_lulib_lib_t *this_lulib)
{
    int fret;


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD);

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lulib, MCM_SREQUEST_UPDATE, 0, NULL, 0);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }

FREE_01:
    this_lulib->rep_code = fret;
    return fret;
}

int mcm_lulib_save(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_BOOL_TD force_save)
{
    int fret;
    void *tmp_offset;


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

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
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lulib, MCM_SREQUEST_SAVE, 0, NULL, 0);
    tmp_offset = this_lulib->pkt_offset;
    // DC.
    memcpy(tmp_offset, &force_save, sizeof(MCM_DTYPE_BOOL_TD));

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }

FREE_01:
    this_lulib->rep_code = fret;
    return fret;
}

int mcm_lulib_run(
    struct mcm_lulib_lib_t *this_lulib,
    char *module_function)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    xlen = strlen(module_function) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PL + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PL + PC.
    mcm_build_base_req(this_lulib, MCM_SREQUEST_RUN, 1, module_function, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }

FREE_01:
    this_lulib->rep_code = fret;
    return fret;
}

int mcm_lulib_shutdown(
    struct mcm_lulib_lib_t *this_lulib)
{
    int fret;


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD);

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lulib, MCM_SREQUEST_SHUTDOWN, 0, NULL, 0);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }

FREE_01:
    this_lulib->rep_code = fret;
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


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    xlen = strlen(file_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PL + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄檢查的檔案的路徑長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄檢查的檔案的路徑.

    // T + REQ + PL + PC.
    mcm_build_base_req(this_lulib, MCM_SREQUEST_CHECK_STORE_FILE, 1, file_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC | SR | VL | VC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.
    // SR  [MCM_DTYPE_LIST_TD].
    //     紀錄檔案的檢查結果.
    // VL  [MCM_DTYPE_USIZE_TD].
    //     紀錄檔案的版本資訊的長度 (包含最後的 \0).
    // VC  [binary].
    //     紀錄檔案的版本資訊.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }
    tmp_offset = this_lulib->pkt_offset;
    // SR.
    *store_result_buf = *((MCM_DTYPE_LIST_TD *) tmp_offset);
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("store_result[" MCM_DTYPE_LIST_PF "]", *store_result_buf);
    }
#endif
    tmp_offset += sizeof(MCM_DTYPE_LIST_TD);
    // VL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("version_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    }
#endif
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // VC.
    if(xlen > 0)
    {
        clen = store_version_size - 1;
        clen = clen > xlen ? xlen : clen;
        memcpy(store_version_buf, tmp_offset, clen);
        store_version_buf[clen] = '\0';
#if MCM_LUDMODE
        if(mcm_lulib_show_msg != 0)
        {
            MCM_LUDMSG("version_con[%s]", store_version_buf);
        }
#endif
    }

FREE_01:
    this_lulib->rep_code = fret;
    return fret;
}

int mcm_lulib_check_mask_path(
    struct mcm_lulib_lib_t *this_lulib,
    char *mask_path)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    xlen = strlen(mask_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PL + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑的長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PL + PC.
    mcm_build_base_req(this_lulib, MCM_SREQUEST_CHECK_MASK_PATH, 1, mask_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }

FREE_01:
    this_lulib->rep_code = fret;
    return fret;
}

int mcm_lulib_get_path_max_length(
    struct mcm_lulib_lib_t *this_lulib,
    MCM_DTYPE_USIZE_TD *max_len_buf)
{
    int fret;
    void *tmp_offset;


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD);

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lulib, MCM_SREQUEST_GET_PATH_MAX_LENGTH, 0, NULL, 0);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC | PML |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC + PML.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.
    // PML [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的資料.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }
    tmp_offset = this_lulib->pkt_offset;
    // PML.
    *max_len_buf = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("path_max_length[" MCM_DTYPE_USIZE_PF "]", *max_len_buf);
    }
#endif

FREE_01:
    this_lulib->rep_code = fret;
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


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    xlen = strlen(mask_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PL + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑的長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PL + PC.
    mcm_build_base_req(this_lulib, MCM_SREQUEST_GET_LIST_NAME, 1, mask_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC | DL | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC + DL + DC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的資料的長度.
    // DC  [binary].
    //     紀錄回應的資料.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }
    tmp_offset = this_lulib->pkt_offset;
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    }
#endif
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
    this_lulib->rep_code = fret;
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


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    xlen = strlen(mask_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PL + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑的長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PL + PC.
    mcm_build_base_req(this_lulib, MCM_SREQUEST_GET_LIST_TYPE, 1, mask_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC | DL | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC + DL + DC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的資料的長度.
    // DC  [binary].
    //     紀錄回應的資料.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }
    tmp_offset = this_lulib->pkt_offset;
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    }
#endif
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
    this_lulib->rep_code = fret;
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


    this_lulib->rep_msg_con = NULL;
    this_lulib->rep_msg_len = 0;

    xlen = strlen(full_path) + 1;
    this_lulib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lulib->pkt_len > this_lulib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lulib, this_lulib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            if(mcm_lulib_show_msg != 0)
            {
                MCM_EMSG("call mcm_realloc_buf_lib() fail");
            }
            goto FREE_01;
        }
    }

    // 封包格式 :
    // | T | REQ | PL | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PL + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PL  [MCM_DTYPE_USIZE_TD].
    //     紀錄請求的路徑的長度 (包括最後的 \0).
    // PC  [binary].
    //     紀錄請求的路徑.

    // T + REQ + PL + PC.
    mcm_build_base_req(this_lulib, MCM_SREQUEST_GET_LIST_VALUE, 1, full_path, xlen);

    fret = mcm_send_req(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_send_req() fail");
        }
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_recv_rep() fail");
        }
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC | DL | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC + DL + DC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的資料的長度.
    // DC  [binary].
    //     紀錄回應的資料.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lulib);
    if(fret < MCM_RCODE_PASS)
    {
        if(mcm_lulib_show_msg != 0)
        {
            MCM_EMSG("call mcm_parse_base_rep() fail");
        }
        goto FREE_01;
    }
    tmp_offset = this_lulib->pkt_offset;
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
#if MCM_LUDMODE
    if(mcm_lulib_show_msg != 0)
    {
        MCM_LUDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    }
#endif
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
    this_lulib->rep_code = fret;
    return fret;
}

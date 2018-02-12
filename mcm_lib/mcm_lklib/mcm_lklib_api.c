// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/socket.h>
#include <linux/un.h>
#include <net/sock.h>
#include "../mcm_lheader/mcm_type.h"
#include "../mcm_lheader/mcm_control.h"
#include "../mcm_lheader/mcm_request.h"
#include "../mcm_lheader/mcm_connect.h"
#include "../mcm_lheader/mcm_return.h"
#include "../mcm_lheader/mcm_debug.h"
#include "mcm_lklib_api.h"




// 預設的封包緩衝大小.
#define MCM_KERNEL_PACKET_BUFFER_SIZE 512




#define MCM_BUILD_BASE_REQ_01(tmp_offset, this_lklib, req_code) \
    do                                                              \
    {                                                               \
        /* T */                                                     \
        tmp_offset = this_lklib->pkt_buf;                           \
        *((MCM_DTYPE_USIZE_TD *) tmp_offset) = this_lklib->pkt_len; \
        tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);                   \
        /* REQ */                                                   \
        *((MCM_DTYPE_LIST_TD *) tmp_offset) = req_code;             \
        tmp_offset += sizeof(MCM_DTYPE_LIST_TD);                    \
        /* ... */                                                   \
    }                                                               \
    while(0)

#define MCM_BUILD_BASE_REQ_02(tmp_offset, this_lklib, req_code, req_path, req_plen) \
    do                                                           \
    {                                                            \
        /* T + REQ */                                            \
        MCM_BUILD_BASE_REQ_01(tmp_offset, this_lklib, req_code); \
        /* PC */                                                 \
        memcpy(tmp_offset, req_path, req_plen);                  \
        tmp_offset += req_plen;                                  \
        /* ... */                                                \
    }                                                            \
    while(0)

#define MCM_BUILD_BASE_REQ_03(tmp_offset, this_lklib, req_code, req_path, req_plen) \
    do                                                           \
    {                                                            \
        /* T + REQ */                                            \
        MCM_BUILD_BASE_REQ_01(tmp_offset, this_lklib, req_code); \
        /* PL */                                                 \
        *((MCM_DTYPE_USIZE_TD *) tmp_offset) = req_plen;         \
        tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);                \
        /* PC */                                                 \
        memcpy(tmp_offset, req_path, req_plen);                  \
        tmp_offset += req_plen;                                  \
        /* ... */                                                \
    }                                                            \
    while(0)

#define MCM_PARSE_BASE_REP(tmp_offset, this_lklib, tmp_code) \
    do                                                                 \
    {                                                                  \
        /* T */                                                        \
        tmp_offset = this_lklib->pkt_buf + sizeof(MCM_DTYPE_USIZE_TD); \
        /* REP */                                                      \
        tmp_code = *((MCM_DTYPE_LIST_TD *) tmp_offset);                \
        MCM_LKDMSG("tmp_code[" MCM_DTYPE_LIST_PF "]", tmp_code);       \
        tmp_offset += sizeof(MCM_DTYPE_LIST_TD);                       \
    }                                                                  \
    while(0)




// 增加封包的緩衝大小.
// this_lklib (I) :
//   要增加緩衝的結構.
// new_size  (I) :
//   要設定的大小.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_realloc_buf_lib(
    struct mcm_lklib_lib_t *this_lklib,
    MCM_DTYPE_USIZE_TD new_size)
{
    void *tmp_buf;


    tmp_buf = kmalloc(new_size, GFP_KERNEL);
    if(tmp_buf == NULL)
    {
        MCM_KEMSG("call kmalloc() fail");
        return MCM_RCODE_LKLIB_ALLOC_FAIL;
    }

    MCM_LKDMSG("realloc [" MCM_DTYPE_USIZE_PF "][%p] -> [" MCM_DTYPE_USIZE_PF "][%p]",
               this_lklib->pkt_size, this_lklib->pkt_buf, new_size, tmp_buf);

    if(this_lklib->pkt_buf != NULL)
    {
        memcpy(tmp_buf, this_lklib->pkt_buf, this_lklib->pkt_size);
        kfree(this_lklib->pkt_buf);
    }

    this_lklib->pkt_buf = tmp_buf;
    this_lklib->pkt_size = new_size;

    return MCM_RCODE_PASS;
}

// 傳送封包.
// this_lklib (I) :
//   連線資訊.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_send_req(
    struct mcm_lklib_lib_t *this_lklib)
{
    int slen;
    struct msghdr sock_msg;
    struct kvec sock_iov;


    memset(&sock_msg, 0, sizeof(struct msghdr));
    sock_iov.iov_base = this_lklib->pkt_buf;
    sock_iov.iov_len = this_lklib->pkt_len;
    slen = kernel_sendmsg(this_lklib->sock_fp, &sock_msg, &sock_iov, 1, sock_iov.iov_len);
    if(slen < 0)
    {
        MCM_KEMSG("call sock_sendmsg() fail [%d]", slen);
        return MCM_RCODE_LKLIB_SOCKET_ERROR;
    }

    return MCM_RCODE_PASS;
}

// 接收封包.
// this_lklib (I) :
//   連線資訊.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_recv_rep(
    struct mcm_lklib_lib_t *this_lklib)
{
    int fret, rlen;
    void *bloc;
    MCM_DTYPE_USIZE_TD bsize;
    MCM_DTYPE_USIZE_TD tlen, clen;
    struct msghdr sock_msg;
    struct kvec sock_iov;


    bloc = this_lklib->pkt_buf;
    bsize = this_lklib->pkt_size;
    tlen = clen = 0;
    while(1)
    {
        memset(&sock_msg, 0, sizeof(struct msghdr));
        sock_iov.iov_base = bloc;
        sock_iov.iov_len = bsize;
        rlen = kernel_recvmsg(this_lklib->sock_fp, &sock_msg, &sock_iov, 1, sock_iov.iov_len, 0);
        if(rlen < 0)
        {
            MCM_KEMSG("call sock_recvmsg() fail [%d]", rlen);
            return MCM_RCODE_LKLIB_SOCKET_ERROR;
        }

        if(tlen == 0)
        {
            if(rlen < sizeof(MCM_DTYPE_USIZE_TD))
            {
                MCM_KEMSG("recv fail (packet too small [%d])", rlen);
                return MCM_RCODE_LKLIB_SOCKET_ERROR;
            }

            tlen = *((MCM_DTYPE_USIZE_TD *) bloc);
            if(tlen > this_lklib->pkt_size)
            {
                fret = mcm_realloc_buf_lib(this_lklib, tlen);
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
                    return fret;
                }
            }
        }

        clen += rlen;
        bloc = this_lklib->pkt_buf + clen;
        bsize = this_lklib->pkt_size - clen;

        if(clen >= tlen)
        {
            this_lklib->pkt_len = tlen;
            break;
        }
    }

    MCM_LKDMSG("rep[" MCM_DTYPE_USIZE_PF "]", this_lklib->pkt_len);

    return MCM_RCODE_PASS;
}

// 資料初始化.
// this_lklib (I) :
//   連線資訊.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_lklib_init(
    struct mcm_lklib_lib_t *this_lklib)
{
    int fret, xlen;
    struct sockaddr_un *sock_addr;
    size_t addr_len;
    struct mcm_connect_option_t connect_data;
    struct msghdr sock_msg;
    struct kvec sock_iov;
    char notify_usable = 0;


    fret = mcm_realloc_buf_lib(this_lklib, MCM_KERNEL_PACKET_BUFFER_SIZE);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
        goto FREE_01;
    }

    sock_addr = (struct sockaddr_un *) kmalloc(sizeof(struct sockaddr_un), GFP_KERNEL);
    if(sock_addr == NULL)
    {
        MCM_KEMSG("call kmalloc() fail");
        fret = MCM_RCODE_LKLIB_ALLOC_FAIL;
        goto FREE_02;
    }
    memset(sock_addr, 0, sizeof(struct sockaddr_un));
    sock_addr->sun_family = AF_UNIX;
    snprintf(sock_addr->sun_path, sizeof(sock_addr->sun_path), this_lklib->socket_path);
    sock_addr->sun_path[0] = '\0';
    addr_len = strlen(this_lklib->socket_path) + offsetof(struct sockaddr_un, sun_path);

    fret = sock_create(AF_UNIX, SOCK_STREAM, 0, &this_lklib->sock_fp);
    if(fret < 0)
    {
        MCM_KEMSG("call sock_create() fail [%d]", fret);
        fret = MCM_RCODE_LKLIB_SOCKET_ERROR;
        goto FREE_03;
    }

    fret = this_lklib->sock_fp->ops->connect(this_lklib->sock_fp, (struct sockaddr *) sock_addr,
                                             addr_len, 0);
    if(fret < 0)
    {
        MCM_KEMSG("call connect() fail [%d]", fret);
        fret = MCM_RCODE_LKLIB_SOCKET_ERROR;
        goto FREE_04;
    }

    memset(&connect_data, 0, sizeof(struct mcm_connect_option_t));
    connect_data.call_from = this_lklib->call_from;
    connect_data.session_permission = this_lklib->session_permission;
    connect_data.session_stack_size = this_lklib->session_stack_size;
    MCM_LKDMSG("connect[" MCM_DTYPE_LIST_PF "][%s][" MCM_DTYPE_USIZE_PF "]",
               connect_data.call_from, MCM_DBG_SPERMISSION(connect_data.session_permission),
               connect_data.session_stack_size);

    memset(&sock_msg, 0, sizeof(struct msghdr));
    sock_iov.iov_base = &connect_data;
    sock_iov.iov_len = sizeof(struct mcm_connect_option_t);
    xlen = kernel_sendmsg(this_lklib->sock_fp, &sock_msg, &sock_iov, 1, sock_iov.iov_len);
    if(xlen < 0)
    {
        MCM_KEMSG("call sock_sendmsg() fail [%d]", xlen);
        fret = MCM_RCODE_LKLIB_SOCKET_ERROR;
        goto FREE_04;
    }

    memset(&sock_msg, 0, sizeof(struct msghdr));
    sock_iov.iov_base = &notify_usable;
    sock_iov.iov_len = sizeof(notify_usable);
    xlen = kernel_recvmsg(this_lklib->sock_fp, &sock_msg, &sock_iov, 1, sock_iov.iov_len, 0);
    if(xlen < 0)
    {
        MCM_KEMSG("call sock_recvmsg() fail [%d]", xlen);
        fret = MCM_RCODE_LKLIB_SOCKET_ERROR;
        goto FREE_04;
    }

    kfree(sock_addr);
    return MCM_RCODE_PASS;
FREE_04:
    sock_release(this_lklib->sock_fp);
FREE_03:
    kfree(sock_addr);
FREE_02:
    kfree(this_lklib->pkt_buf);
FREE_01:
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_init);

// 結束使用.
// this_lklib (I) :
//   連線資訊.
// return :
//   MCM_RCODE_PASS.
int mcm_lklib_exit(
    struct mcm_lklib_lib_t *this_lklib)
{
    MCM_LKDMSG("close sock_fp[%p]", this_lklib->sock_fp);
    sock_release(this_lklib->sock_fp);

    MCM_LKDMSG("free pkt_buf[%p]", this_lklib->pkt_buf);
    kfree(this_lklib->pkt_buf);

    return MCM_RCODE_PASS;
}
EXPORT_SYMBOL(mcm_lklib_exit);

int mcm_lklib_get_alone(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path,
    void *data_buf)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(full_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
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
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lklib, MCM_SREQUEST_GET_ALONE, full_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_recv_rep() fail");
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
    MCM_PARSE_BASE_REP(tmp_offset, this_lklib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    MCM_LKDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(xlen > 0)
        memcpy(data_buf, tmp_offset, xlen);

FREE_01:
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_get_alone);

int mcm_lklib_set_alone(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(full_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen +
                          sizeof(MCM_DTYPE_USIZE_TD) + data_len + 1;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
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
    MCM_BUILD_BASE_REQ_03(tmp_offset, this_lklib, MCM_SREQUEST_SET_ALONE, full_path, xlen);
    // DL.
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = data_len;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    memcpy(tmp_offset, data_con, data_len);
    tmp_offset += data_len;
    // \0.
    *((unsigned char *) tmp_offset) = 0;

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_recv_rep() fail");
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lklib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }

FREE_01:
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_set_alone);

int mcm_lklib_get_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path,
    void *data_buf)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(full_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
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
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lklib, MCM_SREQUEST_GET_ENTRY, full_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_recv_rep() fail");
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
    MCM_PARSE_BASE_REP(tmp_offset, this_lklib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    MCM_LKDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(xlen > 0)
        memcpy(data_buf, tmp_offset, xlen);

FREE_01:
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_get_entry);

int mcm_lklib_set_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(full_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen +
                          data_len;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
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
    MCM_BUILD_BASE_REQ_03(tmp_offset, this_lklib, MCM_SREQUEST_SET_ENTRY, full_path, xlen);
    // DC.
    memcpy(tmp_offset, data_con, data_len);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_recv_rep() fail");
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lklib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }

FREE_01:
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_set_entry);

int mcm_lklib_add_entry(
    struct mcm_lklib_lib_t *this_lklib,
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
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen +
                          sizeof(MCM_DTYPE_USIZE_TD) + ilen +
                          sizeof(MCM_DTYPE_USIZE_TD) + data_len;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
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
    MCM_BUILD_BASE_REQ_03(tmp_offset, this_lklib, MCM_SREQUEST_ADD_ENTRY, full_path, xlen);
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

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_recv_rep() fail");
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lklib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }

FREE_01:
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_add_entry);

int mcm_lklib_del_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(full_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
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
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lklib, MCM_SREQUEST_DEL_ENTRY, full_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_recv_rep() fail");
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lklib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }

FREE_01:
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_del_entry);

int mcm_lklib_get_all_key(
    struct mcm_lklib_lib_t *this_lklib,
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
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
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
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lklib, MCM_SREQUEST_GET_ALL_KEY, mix_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_recv_rep() fail");
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
    MCM_PARSE_BASE_REP(tmp_offset, this_lklib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }
    // EC.
    tmp_count = *((MCM_DTYPE_EK_TD *) tmp_offset);
    MCM_LKDMSG("count[" MCM_DTYPE_EK_PF "]", tmp_count);
    tmp_offset += sizeof(MCM_DTYPE_EK_TD);
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    MCM_LKDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(xlen > 0)
    {
        tmp_buf = kmalloc(xlen, GFP_KERNEL);
        if(tmp_buf == NULL)
        {
            MCM_KEMSG("call kmalloc() fail");
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
EXPORT_SYMBOL(mcm_lklib_get_all_key);

int mcm_lklib_get_all_entry(
    struct mcm_lklib_lib_t *this_lklib,
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
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
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
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lklib, MCM_SREQUEST_GET_ALL_ENTRY, mix_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_recv_rep() fail");
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
    MCM_PARSE_BASE_REP(tmp_offset, this_lklib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }
    // EC.
    tmp_count = *((MCM_DTYPE_EK_TD *) tmp_offset);
    MCM_LKDMSG("count[" MCM_DTYPE_EK_PF "]", tmp_count);
    tmp_offset += sizeof(MCM_DTYPE_EK_TD);
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    MCM_LKDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(xlen > 0)
    {
        tmp_buf = kmalloc(xlen, GFP_KERNEL);
        if(tmp_buf == NULL)
        {
            MCM_KEMSG("call kmalloc() fail");
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
EXPORT_SYMBOL(mcm_lklib_get_all_entry);

int mcm_lklib_del_all_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *mix_path)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(mix_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
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
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lklib, MCM_SREQUEST_DEL_ALL_ENTRY, mix_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_recv_rep() fail");
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lklib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }

FREE_01:
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_del_all_entry);

int mcm_lklib_get_max_count(
    struct mcm_lklib_lib_t *this_lklib,
    char *mask_path,
    MCM_DTYPE_EK_TD *count_buf)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(mask_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
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
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lklib, MCM_SREQUEST_GET_MAX_COUNT, mask_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_recv_rep() fail");
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
    MCM_PARSE_BASE_REP(tmp_offset, this_lklib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call fail (%d)", fret);
        goto FREE_01;
    }
    // EC.
    *count_buf = *((MCM_DTYPE_EK_TD *) tmp_offset);
    MCM_LKDMSG("max_count[" MCM_DTYPE_EK_PF "]", *count_buf);

FREE_01:
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_get_max_count);

int mcm_lklib_get_count(
    struct mcm_lklib_lib_t *this_lklib,
    char *mix_path,
    MCM_DTYPE_EK_TD *count_buf)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(mix_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
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
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lklib, MCM_SREQUEST_GET_COUNT, mix_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_recv_rep() fail");
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
    MCM_PARSE_BASE_REP(tmp_offset, this_lklib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }
    // EC.
    *count_buf = *((MCM_DTYPE_EK_TD *) tmp_offset);
    MCM_LKDMSG("count[" MCM_DTYPE_EK_PF "]", *count_buf);

FREE_01:
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_get_count);

int mcm_lklib_get_usable_key(
    struct mcm_lklib_lib_t *this_lklib,
    char *mix_path,
    MCM_DTYPE_EK_TD *key_buf)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    xlen = strlen(mix_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
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
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lklib, MCM_SREQUEST_GET_USABLE_KEY, mix_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_recv_rep() fail");
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
    MCM_PARSE_BASE_REP(tmp_offset, this_lklib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call fail (%d)", fret);
        }
        goto FREE_01;
    }
    // EK.
    *key_buf = *((MCM_DTYPE_EK_TD *) tmp_offset);
    MCM_LKDMSG("usable_key[" MCM_DTYPE_EK_PF "]", *key_buf);

FREE_01:
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_get_usable_key);

int mcm_lklib_update(
    struct mcm_lklib_lib_t *this_lklib)
{
    int fret;
    void *tmp_offset;


    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD);

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
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
    MCM_BUILD_BASE_REQ_01(tmp_offset, this_lklib, MCM_SREQUEST_UPDATE);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_recv_rep() fail");
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lklib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call fail (%d)", fret);
        goto FREE_01;
    }

FREE_01:
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_update);

int mcm_lklib_save(
    struct mcm_lklib_lib_t *this_lklib,
    MCM_DTYPE_BOOL_TD force_save)
{
    int fret;
    void *tmp_offset;


    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_BOOL_TD);

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
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
    MCM_BUILD_BASE_REQ_01(tmp_offset, this_lklib, MCM_SREQUEST_SAVE);
    // DC.
    memcpy(tmp_offset, &force_save, sizeof(MCM_DTYPE_BOOL_TD));

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_recv_rep() fail");
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lklib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call fail (%d)", fret);
        goto FREE_01;
    }

FREE_01:
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_save);

int mcm_lklib_run(
    struct mcm_lklib_lib_t *this_lklib,
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
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen +
                          sizeof(MCM_DTYPE_USIZE_TD) + req_data_len;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
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
    MCM_BUILD_BASE_REQ_03(tmp_offset, this_lklib, MCM_SREQUEST_RUN, module_function, xlen);
    // DL.
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = req_data_len;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(req_data_con != NULL)
        memcpy(tmp_offset, req_data_con, req_data_len);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_recv_rep() fail");
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
    MCM_PARSE_BASE_REP(tmp_offset, this_lklib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call fail (%d)", fret);
    }
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    MCM_LKDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if((xlen > 0) && (rep_data_buf != NULL))
    {
        tmp_buf = kmalloc(xlen, GFP_KERNEL);
        if(tmp_buf == NULL)
        {
            MCM_KEMSG("call kmalloc() fail");
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
EXPORT_SYMBOL(mcm_lklib_run);

int mcm_lklib_shutdown(
    struct mcm_lklib_lib_t *this_lklib)
{
    int fret;
    void *tmp_offset;


    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD);

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
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
    MCM_BUILD_BASE_REQ_01(tmp_offset, this_lklib, MCM_SREQUEST_SHUTDOWN);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_recv_rep() fail");
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lklib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call fail (%d)", fret);
        goto FREE_01;
    }

FREE_01:
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_shutdown);

int mcm_lklib_check_store_file(
    struct mcm_lklib_lib_t *this_lklib,
    char *file_path,
    MCM_DTYPE_LIST_TD *store_result_buf,
    char *store_version_buf,
    MCM_DTYPE_USIZE_TD store_version_size)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen, clen;
    void *tmp_offset;


    xlen = strlen(file_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KECTMSG("call mcm_realloc_buf_lib() fail");
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
    MCM_BUILD_BASE_REQ_02(tmp_offset, this_lklib, MCM_SREQUEST_CHECK_STORE_FILE, file_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_recv_rep() fail");
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | SR | VL | VC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + SR + VL + VC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // SR  [MCM_DTYPE_LIST_TD].
    //     紀錄檔案的檢查結果.
    // VL  [MCM_DTYPE_USIZE_TD].
    //     紀錄檔案的版本資訊的長度 (包含最後的 \0).
    // VC  [binary].
    //     紀錄檔案的版本資訊.

    // T + REP.
    MCM_PARSE_BASE_REP(tmp_offset, this_lklib, fret);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call fail (%d)", fret);
        goto FREE_01;
    }
    // SR.
    *store_result_buf = *((MCM_DTYPE_LIST_TD *) tmp_offset);
    MCM_LKDMSG("store_result[" MCM_DTYPE_LIST_PF "]", *store_result_buf);
    tmp_offset += sizeof(MCM_DTYPE_LIST_TD);
    // VL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    MCM_LKDMSG("version_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // VC.
    if(xlen > 0)
    {
        clen = store_version_size - 1;
        clen = clen > xlen ? xlen : clen;
        memcpy(store_version_buf, tmp_offset, clen);
        store_version_buf[clen] = '\0';
        MCM_LKDMSG("version_con[%s]", store_version_buf);
    }

FREE_01:
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_check_store_file);

int mcm_lklib_do_get_alone(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path,
    void *data_buf)
{
    int fret;


    fret = mcm_lklib_init(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_init() fail");
        return fret;
    }

    fret = mcm_lklib_get_alone(this_lklib, full_path, data_buf);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call mcm_lklib_get_alone() fail");
        }
    }

    mcm_lklib_exit(this_lklib);

    return fret;
}
EXPORT_SYMBOL(mcm_lklib_do_get_alone);

int mcm_lklib_do_set_alone(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    int fret;


    fret = mcm_lklib_init(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_init() fail");
        return fret;
    }

    fret = mcm_lklib_set_alone(this_lklib, full_path, data_con, data_len);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call mcm_lklib_set_alone() fail");
        }
    }

    mcm_lklib_exit(this_lklib);

    return fret;
}
EXPORT_SYMBOL(mcm_lklib_do_set_alone);

int mcm_lklib_do_get_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path,
    void *data_buf)
{
    int fret;


    fret = mcm_lklib_init(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_init() fail");
        return fret;
    }

    fret = mcm_lklib_get_entry(this_lklib, full_path, data_buf);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call mcm_lklib_get_entry() fail");
        }
    }

    mcm_lklib_exit(this_lklib);

    return fret;
}
EXPORT_SYMBOL(mcm_lklib_do_get_entry);

int mcm_lklib_do_set_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    int fret;


    fret = mcm_lklib_init(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_init() fail");
        return fret;
    }

    fret = mcm_lklib_set_entry(this_lklib, full_path, data_con, data_len);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call mcm_lklib_set_entry() fail");
        }
    }

    mcm_lklib_exit(this_lklib);

    return fret;
}
EXPORT_SYMBOL(mcm_lklib_do_set_entry);

int mcm_lklib_do_add_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path,
    char *insert_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    int fret;


    fret = mcm_lklib_init(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_init() fail");
        return fret;
    }

    fret = mcm_lklib_add_entry(this_lklib, full_path, insert_path, data_con, data_len);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call mcm_lklib_add_entry() fail");
        }
    }

    mcm_lklib_exit(this_lklib);

    return fret;
}
EXPORT_SYMBOL(mcm_lklib_do_add_entry);

int mcm_lklib_do_del_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path)
{
    int fret;


    fret = mcm_lklib_init(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_init() fail");
        return fret;
    }

    fret = mcm_lklib_del_entry(this_lklib, full_path);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call mcm_lklib_del_entry() fail");
        }
    }

    mcm_lklib_exit(this_lklib);

    return fret;
}
EXPORT_SYMBOL(mcm_lklib_do_del_entry);

int mcm_lklib_do_get_all_key(
    struct mcm_lklib_lib_t *this_lklib,
    char *mix_path,
    MCM_DTYPE_EK_TD **key_buf,
    MCM_DTYPE_EK_TD *count_buf)
{
    int fret;


    fret = mcm_lklib_init(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_init() fail");
        return fret;
    }

    fret = mcm_lklib_get_all_key(this_lklib, mix_path, key_buf, count_buf);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call mcm_lklib_get_all_key() fail");
        }
    }

    mcm_lklib_exit(this_lklib);

    return fret;
}
EXPORT_SYMBOL(mcm_lklib_do_get_all_key);

int mcm_lklib_do_get_all_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *mix_path,
    void **data_buf,
    MCM_DTYPE_EK_TD *count_buf)
{
    int fret;


    fret = mcm_lklib_init(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_init() fail");
        return fret;
    }

    fret = mcm_lklib_get_all_entry(this_lklib, mix_path, data_buf, count_buf);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call mcm_lklib_get_all_entry() fail");
        }
    }

    mcm_lklib_exit(this_lklib);

    return fret;
}
EXPORT_SYMBOL(mcm_lklib_do_get_all_entry);

int mcm_lklib_do_del_all_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *mix_path)
{
    int fret;


    fret = mcm_lklib_init(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_init() fail");
        return fret;
    }

    fret = mcm_lklib_del_all_entry(this_lklib, mix_path);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call mcm_lklib_del_all_entry() fail");
        }
    }

    mcm_lklib_exit(this_lklib);

    return fret;
}
EXPORT_SYMBOL(mcm_lklib_do_del_all_entry);

int mcm_lklib_do_get_max_count(
    struct mcm_lklib_lib_t *this_lklib,
    char *mask_path,
    MCM_DTYPE_EK_TD *count_buf)
{
    int fret;


    fret = mcm_lklib_init(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_init() fail");
        return fret;
    }

    fret = mcm_lklib_get_max_count(this_lklib, mask_path, count_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_get_max_count() fail");
    }

    mcm_lklib_exit(this_lklib);

    return fret;
}
EXPORT_SYMBOL(mcm_lklib_do_get_max_count);

int mcm_lklib_do_get_count(
    struct mcm_lklib_lib_t *this_lklib,
    char *mix_path,
    MCM_DTYPE_EK_TD *count_buf)
{
    int fret;


    fret = mcm_lklib_init(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_init() fail");
        return fret;
    }

    fret = mcm_lklib_get_count(this_lklib, mix_path, count_buf);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call mcm_lklib_get_count() fail");
        }
    }

    mcm_lklib_exit(this_lklib);

    return fret;
}
EXPORT_SYMBOL(mcm_lklib_do_get_count);

int mcm_lklib_do_get_usable_key(
    struct mcm_lklib_lib_t *this_lklib,
    char *mix_path,
    MCM_DTYPE_EK_TD *key_buf)
{
    int fret;


    fret = mcm_lklib_init(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_init() fail");
        return fret;
    }

    fret = mcm_lklib_get_usable_key(this_lklib, mix_path, key_buf);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_KECTMSG("call mcm_lklib_get_usable_key() fail");
        }
    }

    mcm_lklib_exit(this_lklib);

    return fret;
}
EXPORT_SYMBOL(mcm_lklib_do_get_usable_key);

int mcm_lklib_do_update(
    struct mcm_lklib_lib_t *this_lklib)
{
    int fret;


    fret = mcm_lklib_init(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_init() fail");
        return fret;
    }

    fret = mcm_lklib_update(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_update() fail");
    }

    mcm_lklib_exit(this_lklib);

    return fret;
}
EXPORT_SYMBOL(mcm_lklib_do_update);

int mcm_lklib_do_save(
    struct mcm_lklib_lib_t *this_lklib,
    MCM_DTYPE_BOOL_TD force_save)
{
    int fret;


    fret = mcm_lklib_init(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_init() fail");
        return fret;
    }

    fret = mcm_lklib_save(this_lklib, force_save);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_save() fail");
    }

    mcm_lklib_exit(this_lklib);

    return fret;
}
EXPORT_SYMBOL(mcm_lklib_do_save);

int mcm_lklib_do_run(
    struct mcm_lklib_lib_t *this_lklib,
    char *module_function,
    void *req_data_con,
    MCM_DTYPE_USIZE_TD req_data_len,
    void **rep_data_buf,
    MCM_DTYPE_USIZE_TD *rep_data_len_buf)
{
    int fret;


    fret = mcm_lklib_init(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_init() fail");
        return fret;
    }

    fret = mcm_lklib_run(this_lklib, module_function, req_data_con, req_data_len,
                         rep_data_buf, rep_data_len_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_run() fail");
    }

    mcm_lklib_exit(this_lklib);

    return fret;
}
EXPORT_SYMBOL(mcm_lklib_do_run);

int mcm_lklib_do_shutdown(
    struct mcm_lklib_lib_t *this_lklib)
{
    int fret;


    fret = mcm_lklib_init(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_init() fail");
        return fret;
    }

    fret = mcm_lklib_shutdown(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_shutdown() fail");
    }

    mcm_lklib_exit(this_lklib);

    return fret;
}
EXPORT_SYMBOL(mcm_lklib_do_shutdown);

int mcm_lklib_do_check_store_file(
    struct mcm_lklib_lib_t *this_lklib,
    char *file_path,
    MCM_DTYPE_LIST_TD *store_result_buf,
    char *store_version_buf,
    MCM_DTYPE_USIZE_TD store_version_size)
{
    int fret;


    fret = mcm_lklib_init(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_init() fail");
        return fret;
    }

    fret = mcm_lklib_check_store_file(this_lklib, file_path, store_result_buf,
                                      store_version_buf, store_version_size);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KECTMSG("call mcm_lklib_check_store_file() fail");
    }

    mcm_lklib_exit(this_lklib);

    return fret;
}
EXPORT_SYMBOL(mcm_lklib_do_check_store_file);

static int __init init_mcm_lklib_api(void)
{
    MCM_LKDMSG("init_mcm_lklib_api");

    return 0;
}

static void __exit exit_mcm_lklib_api(void)
{
    MCM_LKDMSG("exit_mcm_lklib_api");

    return;
}




module_init(init_mcm_lklib_api);
module_exit(exit_mcm_lklib_api);




MODULE_LICENSE("LGPLv3");
MODULE_AUTHOR("Che-Wei Hsu");
MODULE_DESCRIPTION("MintCM");

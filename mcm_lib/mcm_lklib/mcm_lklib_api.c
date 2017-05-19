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
    int fret;
    struct iovec sock_iov;
    struct msghdr sock_msg;
    mm_segment_t tmp_kfs;
    __kernel_size_t xlen;


    memset(&sock_iov, 0, sizeof(struct iovec));
    sock_iov.iov_base = this_lklib->pkt_buf;
    sock_iov.iov_len = this_lklib->pkt_len;
    memset(&sock_msg, 0, sizeof(struct msghdr));
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
    sock_msg.msg_iov = &sock_iov;
    sock_msg.msg_iovlen = 1;
#else
    sock_msg.msg_iter.type = ITER_IOVEC;
    sock_msg.msg_iter.count = sock_iov.iov_len;
    sock_msg.msg_iter.iov = &sock_iov;
    sock_msg.msg_iter.nr_segs = 1;
#endif

    fret = MCM_RCODE_PASS;
    tmp_kfs = get_fs();
    set_fs(KERNEL_DS);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)
    xlen = sock_sendmsg(this_lklib->sock_fp, &sock_msg, sock_iov.iov_len);
#else
    xlen = sock_sendmsg(this_lklib->sock_fp, &sock_msg);
#endif
    if(xlen < 0)
    {
        MCM_KEMSG("call sock_sendmsg() fail [%zd]", xlen);
        fret = MCM_RCODE_LKLIB_SOCKET_ERROR;
    }
    set_fs(tmp_kfs);

    return fret;
}

// 接收回傳的封包.
// this_lklib (I) :
//   連線資訊.
// return :
//   >= MCM_RCODE_PASS : 成功.
//   <  MCM_RCODE_PASS : 錯誤.
int mcm_recv_rep(
    struct mcm_lklib_lib_t *this_lklib)
{
    int fret;
    void *bloc;
    MCM_DTYPE_USIZE_TD bsize;
    __kernel_size_t rlen;
    MCM_DTYPE_USIZE_TD tlen, clen;
    struct iovec sock_iov;
    struct msghdr sock_msg;
    mm_segment_t tmp_kfs;


    bloc = this_lklib->pkt_buf;
    bsize = this_lklib->pkt_size;
    tlen = clen = 0;
    while(1)
    {
        memset(&sock_iov, 0, sizeof(struct iovec));
        sock_iov.iov_base = bloc;
        sock_iov.iov_len = bsize;
        memset(&sock_msg, 0, sizeof(struct msghdr));
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
        sock_msg.msg_iov = &sock_iov;
        sock_msg.msg_iovlen = 1;
#else
        sock_msg.msg_iter.type = ITER_IOVEC;
        sock_msg.msg_iter.count = sock_iov.iov_len;
        sock_msg.msg_iter.iov = &sock_iov;
        sock_msg.msg_iter.nr_segs = 1;
#endif

        fret = MCM_RCODE_PASS;
        tmp_kfs = get_fs();
        set_fs(KERNEL_DS);
        rlen = sock_recvmsg(this_lklib->sock_fp, &sock_msg, sock_iov.iov_len, 0);
        if(rlen < 0)
        {
            MCM_KEMSG("call sock_recvmsg() fail");
            fret = MCM_RCODE_LKLIB_SOCKET_ERROR;
        }
        set_fs(tmp_kfs);
        if(fret < MCM_RCODE_PASS)
            return fret;

        if(tlen == 0)
        {
            if(rlen < sizeof(MCM_DTYPE_USIZE_TD))
            {
                MCM_KEMSG("recv fail (packet too small [%zd])", rlen);
                return MCM_RCODE_LKLIB_SOCKET_ERROR;
            }

            tlen = *((MCM_DTYPE_USIZE_TD *) bloc);
            if(tlen > this_lklib->pkt_size)
            {
                fret = mcm_realloc_buf_lib(this_lklib, tlen);
                if(fret < MCM_RCODE_PASS)
                {
                    MCM_KEMSG("call mcm_realloc_buf_lib() fail");
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

int mcm_build_base_req(
    struct mcm_lklib_lib_t *this_lklib,
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
    tmp_offset = this_lklib->pkt_buf;
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = this_lklib->pkt_len;
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

    this_lklib->pkt_offset = tmp_offset;

    return 0;
}

int mcm_parse_base_rep(
    struct mcm_lklib_lib_t *this_lklib)
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
    tmp_offset = this_lklib->pkt_buf;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // REP.
    this_lklib->rep_code = *((MCM_DTYPE_LIST_TD *) tmp_offset);
    MCM_LKDMSG("rep_code[" MCM_DTYPE_LIST_PF "]", this_lklib->rep_code);
    tmp_offset += sizeof(MCM_DTYPE_LIST_TD);
    // ML.
    this_lklib->rep_msg_len = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // MC.
    this_lklib->rep_msg_con = tmp_offset;
    MCM_LKDMSG("rep_msg[" MCM_DTYPE_USIZE_PF "][%s]",
               this_lklib->rep_msg_len, this_lklib->rep_msg_con);
    tmp_offset += this_lklib->rep_msg_len;
    // ...
    this_lklib->pkt_offset = tmp_offset;

    if(this_lklib->rep_code < MCM_RCODE_PASS)
    {
        MCM_KEMSG("report fail [" MCM_DTYPE_LIST_PF "][%s]",
                  this_lklib->rep_code, this_lklib->rep_msg_con);
        return this_lklib->rep_code;
    }

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
    int fret;
    struct sockaddr_un *sock_addr;
    __kernel_size_t addr_len, xlen;
    struct mcm_connect_option_t connect_data;
    struct iovec sock_iov;
    struct msghdr sock_msg;
    mm_segment_t tmp_kfs;
    char notify_usable = 0;


    fret = mcm_realloc_buf_lib(this_lklib, MCM_KERNEL_PACKET_BUFFER_SIZE);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_realloc_buf_lib() fail");
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

    memset(&sock_iov, 0, sizeof(struct iovec));
    sock_iov.iov_base = &connect_data;
    sock_iov.iov_len = sizeof(struct mcm_connect_option_t);
    memset(&sock_msg, 0, sizeof(struct msghdr));
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
    sock_msg.msg_iov = &sock_iov;
    sock_msg.msg_iovlen = 1;
#else
    sock_msg.msg_iter.type = ITER_IOVEC;
    sock_msg.msg_iter.count = sock_iov.iov_len;
    sock_msg.msg_iter.iov = &sock_iov;
    sock_msg.msg_iter.nr_segs = 1;
#endif

    fret = MCM_RCODE_PASS;
    tmp_kfs = get_fs();
    set_fs(KERNEL_DS);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)
    xlen = sock_sendmsg(this_lklib->sock_fp, &sock_msg, sock_iov.iov_len);
#else
    xlen = sock_sendmsg(this_lklib->sock_fp, &sock_msg);
#endif
    if(xlen < 0)
    {
        MCM_KEMSG("call sock_sendmsg() fail [%zd]", xlen);
        fret = MCM_RCODE_LKLIB_SOCKET_ERROR;
    }
    set_fs(tmp_kfs);
    if(fret < MCM_RCODE_PASS)
        goto FREE_04;

    memset(&sock_iov, 0, sizeof(struct iovec));
    sock_iov.iov_base = &notify_usable;
    sock_iov.iov_len = sizeof(notify_usable);
    memset(&sock_msg, 0, sizeof(struct msghdr));
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
    sock_msg.msg_iov = &sock_iov;
    sock_msg.msg_iovlen = 1;
#else
    sock_msg.msg_iter.type = ITER_IOVEC;
    sock_msg.msg_iter.count = sock_iov.iov_len;
    sock_msg.msg_iter.iov = &sock_iov;
    sock_msg.msg_iter.nr_segs = 1;
#endif

    fret = MCM_RCODE_PASS;
    tmp_kfs = get_fs();
    set_fs(KERNEL_DS);
    xlen = sock_recvmsg(this_lklib->sock_fp, &sock_msg, sock_iov.iov_len, 0);
    if(xlen < 0)
    {
        MCM_KEMSG("call sock_recvmsg() fail [%zd]", xlen);
        fret = MCM_RCODE_LKLIB_SOCKET_ERROR;
    }
    set_fs(tmp_kfs);
    if(fret < MCM_RCODE_PASS)
        goto FREE_04;

    kfree(sock_addr);
    return MCM_RCODE_PASS;
FREE_04:
     sock_release(this_lklib->sock_fp);
FREE_03:
    kfree(sock_addr);
FREE_02:
    kfree(this_lklib->pkt_buf);
FREE_01:
    this_lklib->rep_code = fret;
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


    this_lklib->rep_msg_con = NULL;
    this_lklib->rep_msg_len = 0;

    xlen = strlen(full_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KEMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lklib, MCM_SREQUEST_GET_ALONE, 1, full_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_recv_rep() fail");
        goto FREE_01;
    }

    // 封包格式 :
    // | T | REP | ML | MC | DT | DL | DC | \0 |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC + DT + DL + DC + \0.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息.
    // DT  [MCM_DTYPE_LIST_TD].
    //     紀錄回應的資料的類型.
    // DL  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的資料的長度.
    // DC  [binary].
    //     紀錄回應的資料.

    // T + REP + ML + MC.
    fret = mcm_parse_base_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_parse_base_rep() fail");
        goto FREE_01;
    }
    tmp_offset = this_lklib->pkt_offset;
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    MCM_LKDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(xlen > 0)
        memcpy(data_buf, tmp_offset, xlen);

FREE_01:
    this_lklib->rep_code = fret;
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


    this_lklib->rep_msg_con = NULL;
    this_lklib->rep_msg_len = 0;

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
            MCM_KEMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lklib, MCM_SREQUEST_SET_ALONE, 1, full_path, xlen);
    tmp_offset = this_lklib->pkt_offset;
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
        MCM_KEMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_recv_rep() fail");
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
    fret = mcm_parse_base_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_parse_base_rep() fail");
        goto FREE_01;
    }

FREE_01:
    this_lklib->rep_code = fret;
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


    this_lklib->rep_msg_con = NULL;
    this_lklib->rep_msg_len = 0;

    xlen = strlen(full_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KEMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lklib, MCM_SREQUEST_GET_ENTRY, 1, full_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_recv_rep() fail");
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
    fret = mcm_parse_base_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_parse_base_rep() fail");
        goto FREE_01;
    }
    tmp_offset = this_lklib->pkt_offset;
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    MCM_LKDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(xlen > 0)
        memcpy(data_buf, tmp_offset, xlen);

FREE_01:
    this_lklib->rep_code = fret;
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


    this_lklib->rep_msg_con = NULL;
    this_lklib->rep_msg_len = 0;

    xlen = strlen(full_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen +
                          sizeof(MCM_DTYPE_USIZE_TD) + data_len;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KEMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lklib, MCM_SREQUEST_SET_ENTRY, 1, full_path, xlen);
    tmp_offset = this_lklib->pkt_offset;
    // DL.
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = data_len;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    memcpy(tmp_offset, data_con, data_len);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_recv_rep() fail");
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
    fret = mcm_parse_base_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_parse_base_rep() fail");
        goto FREE_01;
    }

FREE_01:
    this_lklib->rep_code = fret;
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_set_entry);

int mcm_lklib_add_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset;


    this_lklib->rep_msg_con = NULL;
    this_lklib->rep_msg_len = 0;

    xlen = strlen(full_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen +
                          sizeof(MCM_DTYPE_USIZE_TD) + data_len;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KEMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lklib, MCM_SREQUEST_ADD_ENTRY, 1, full_path, xlen);
    tmp_offset = this_lklib->pkt_offset;
    // DL.
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = data_len;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    if(data_con != NULL)
        memcpy(tmp_offset, data_con, data_len);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_recv_rep() fail");
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
    fret = mcm_parse_base_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_parse_base_rep() fail");
        goto FREE_01;
    }

FREE_01:
    this_lklib->rep_code = fret;
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_add_entry);

int mcm_lklib_del_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *full_path)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;


    this_lklib->rep_msg_con = NULL;
    this_lklib->rep_msg_len = 0;

    xlen = strlen(full_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KEMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lklib, MCM_SREQUEST_DEL_ENTRY, 1, full_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_recv_rep() fail");
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
    fret = mcm_parse_base_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_parse_base_rep() fail");
        goto FREE_01;
    }

FREE_01:
    this_lklib->rep_code = fret;
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_del_entry);

int mcm_lklib_get_all_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *mix_path,
    MCM_DTYPE_EK_TD *count_buf,
    void **data_buf)
{
    int fret;
    MCM_DTYPE_EK_TD tmp_count;
    MCM_DTYPE_USIZE_TD xlen;
    void *tmp_offset, *tmp_buf = NULL;


    *data_buf = NULL;

    this_lklib->rep_msg_con = NULL;
    this_lklib->rep_msg_len = 0;

    xlen = strlen(mix_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KEMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lklib, MCM_SREQUEST_GET_ALL_ENTRY, 1, mix_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_recv_rep() fail");
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
    fret = mcm_parse_base_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_parse_base_rep() fail");
        goto FREE_01;
    }
    tmp_offset = this_lklib->pkt_offset;
    // EC.
    tmp_count = *((MCM_DTYPE_EK_TD *) tmp_offset);
#if MCM_LUDMODE
    MCM_LKDMSG("count[" MCM_DTYPE_EK_PF "]", tmp_count);
#endif
    tmp_offset += sizeof(MCM_DTYPE_EK_TD);
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
#if MCM_LUDMODE
    MCM_LKDMSG("data_len[" MCM_DTYPE_USIZE_PF "]", xlen);
#endif
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

    *count_buf = tmp_count;
    *data_buf = tmp_buf;

FREE_01:
    this_lklib->rep_code = fret;
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_get_all_entry);

int mcm_lklib_del_all_entry(
    struct mcm_lklib_lib_t *this_lklib,
    char *mix_path)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;


    this_lklib->rep_msg_con = NULL;
    this_lklib->rep_msg_len = 0;

    xlen = strlen(mix_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KEMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lklib, MCM_SREQUEST_DEL_ALL_ENTRY, 1, mix_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_recv_rep() fail");
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
    fret = mcm_parse_base_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_parse_base_rep() fail");
        goto FREE_01;
    }

FREE_01:
    this_lklib->rep_code = fret;
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


    this_lklib->rep_msg_con = NULL;
    this_lklib->rep_msg_len = 0;

    xlen = strlen(mask_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KEMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lklib, MCM_SREQUEST_GET_MAX_COUNT, 1, mask_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_recv_rep() fail");
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
    fret = mcm_parse_base_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_parse_base_rep() fail");
        goto FREE_01;
    }
    tmp_offset = this_lklib->pkt_offset;
    // EC.
    *count_buf = *((MCM_DTYPE_EK_TD *) tmp_offset);
    MCM_LKDMSG("max_count[" MCM_DTYPE_EK_PF "]", *count_buf);

FREE_01:
    this_lklib->rep_code = fret;
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


    this_lklib->rep_msg_con = NULL;
    this_lklib->rep_msg_len = 0;

    xlen = strlen(mix_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KEMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lklib, MCM_SREQUEST_GET_COUNT, 1, mix_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_recv_rep() fail");
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
    fret = mcm_parse_base_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_parse_base_rep() fail");
        goto FREE_01;
    }
    tmp_offset = this_lklib->pkt_offset;
    // EC.
    *count_buf = *((MCM_DTYPE_EK_TD *) tmp_offset);
    MCM_LKDMSG("count[" MCM_DTYPE_EK_PF "]", *count_buf);

FREE_01:
    this_lklib->rep_code = fret;
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


    this_lklib->rep_msg_con = NULL;
    this_lklib->rep_msg_len = 0;

    xlen = strlen(mix_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KEMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lklib, MCM_SREQUEST_GET_USABLE_KEY, 1, mix_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_recv_rep() fail");
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
    fret = mcm_parse_base_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_parse_base_rep() fail");
        goto FREE_01;
    }
    tmp_offset = this_lklib->pkt_offset;
    // EK.
    *key_buf = *((MCM_DTYPE_EK_TD *) tmp_offset);
    MCM_LKDMSG("usable_key[" MCM_DTYPE_EK_PF "]", *key_buf);

FREE_01:
    this_lklib->rep_code = fret;
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_get_usable_key);

int mcm_lklib_update(
    struct mcm_lklib_lib_t *this_lklib)
{
    int fret;


    this_lklib->rep_msg_con = NULL;
    this_lklib->rep_msg_len = 0;

    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD);

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KEMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lklib, MCM_SREQUEST_UPDATE, 0, NULL, 0);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_recv_rep() fail");
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
    fret = mcm_parse_base_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_parse_base_rep() fail");
        goto FREE_01;
    }

FREE_01:
    this_lklib->rep_code = fret;
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_update);

int mcm_lklib_save(
    struct mcm_lklib_lib_t *this_lklib,
    MCM_DTYPE_BOOL_TD force_save)
{
    int fret;
    void *tmp_offset;


    this_lklib->rep_msg_con = NULL;
    this_lklib->rep_msg_len = 0;

    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_BOOL_TD);

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KEMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lklib, MCM_SREQUEST_SAVE, 0, NULL, 0);
    tmp_offset = this_lklib->pkt_offset;
    // DC.
    memcpy(tmp_offset, &force_save, sizeof(MCM_DTYPE_BOOL_TD));

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_recv_rep() fail");
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
    fret = mcm_parse_base_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_parse_base_rep() fail");
        goto FREE_01;
    }

FREE_01:
    this_lklib->rep_code = fret;
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_save);

int mcm_lklib_run(
    struct mcm_lklib_lib_t *this_lklib,
    char *module_function)
{
    int fret;
    MCM_DTYPE_USIZE_TD xlen;


    this_lklib->rep_msg_con = NULL;
    this_lklib->rep_msg_len = 0;

    xlen = strlen(module_function) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KEMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lklib, MCM_SREQUEST_RUN, 1, module_function, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_recv_rep() fail");
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
    fret = mcm_parse_base_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_parse_base_rep() fail");
        goto FREE_01;
    }

FREE_01:
    this_lklib->rep_code = fret;
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_run);

int mcm_lklib_shutdown(
    struct mcm_lklib_lib_t *this_lklib)
{
    int fret;


    this_lklib->rep_msg_con = NULL;
    this_lklib->rep_msg_len = 0;

    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD);

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KEMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lklib, MCM_SREQUEST_SHUTDOWN, 0, NULL, 0);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_recv_rep() fail");
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
    fret = mcm_parse_base_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_parse_base_rep() fail");
        goto FREE_01;
    }

FREE_01:
    this_lklib->rep_code = fret;
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


    this_lklib->rep_msg_con = NULL;
    this_lklib->rep_msg_len = 0;

    xlen = strlen(file_path) + 1;
    this_lklib->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                          sizeof(MCM_DTYPE_LIST_TD) +
                          sizeof(MCM_DTYPE_USIZE_TD) + xlen;

    if(this_lklib->pkt_len > this_lklib->pkt_size)
    {
        fret = mcm_realloc_buf_lib(this_lklib, this_lklib->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_KEMSG("call mcm_realloc_buf_lib() fail");
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
    mcm_build_base_req(this_lklib, MCM_SREQUEST_CHECK_STORE_FILE, 1, file_path, xlen);

    fret = mcm_send_req(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_send_req() fail");
        goto FREE_01;
    }

    fret = mcm_recv_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_recv_rep() fail");
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
    fret = mcm_parse_base_rep(this_lklib);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_KEMSG("call mcm_parse_base_rep() fail");
        goto FREE_01;
    }
    tmp_offset = this_lklib->pkt_offset;
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
    this_lklib->rep_code = fret;
    return fret;
}
EXPORT_SYMBOL(mcm_lklib_check_store_file);

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

// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "../mcm_lib/mcm_lheader/mcm_type.h"
#include "../mcm_lib/mcm_lheader/mcm_size.h"
#include "../mcm_lib/mcm_lheader/mcm_control.h"
#include "../mcm_lib/mcm_lheader/mcm_request.h"
#include "../mcm_lib/mcm_lheader/mcm_connect.h"
#include "../mcm_lib/mcm_lheader/mcm_return.h"
#include "../mcm_lib/mcm_lheader/mcm_debug.h"
#include "mcm_daemon_extern.h"
#include "mcm_config_handle_extern.h"
#include "mcm_service_handle_extern.h"




// service 執行緒的堆疊大小.
#define MCM_SERVICE_STACK_SIZE 16384
// service 的 listen() 列隊上限.
#define MCM_SOCKET_LISTEN_SIZE 32
// session 預設的封包緩衝大小.
#define MCM_PACKET_BUFFER_SIZE 512
// session 預設的資料緩衝大小.
#define MCM_CACHE_BUFFER_SIZE  512

#define MCM_CHECK_SHUTDOWN_01(this_flag) \
    do                                        \
    {                                         \
        mcm_daemon_get_shutdown(&this_flag);  \
        if(this_flag != 0)                    \
        {                                     \
            MCM_SVDMSG("daemon shutdown");    \
            return MCM_RCODE_COMMON_SHUTDOWN; \
        }                                     \
    }                                         \
    while(0)

#define MCM_CHECK_SHUTDOWN_02(this_flag, this_ret, this_label) \
    do                                            \
    {                                             \
        mcm_daemon_get_shutdown(&this_flag);      \
        if(this_flag != 0)                        \
        {                                         \
            MCM_SVDMSG("daemon shutdown");        \
            this_ret = MCM_RCODE_COMMON_SHUTDOWN; \
            goto this_label;                      \
        }                                         \
    }                                             \
    while(0)

#define MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code) \
    do                                                                \
    {                                                                 \
        /* T */                                                       \
        tmp_offset = this_session->pkt_buf;                           \
        *((MCM_DTYPE_USIZE_TD *) tmp_offset) = this_session->pkt_len; \
        tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);                     \
        /* REP */                                                     \
        *((MCM_DTYPE_LIST_TD *) tmp_offset) = rep_code;               \
        tmp_offset += sizeof(MCM_DTYPE_LIST_TD);                      \
        /* ... */                                                     \
    }                                                                 \
    while(0)

#define MCM_BUILD_BASE_REP_02(tmp_offset, this_session, rep_code, rep_msg, rep_mlen) \
    do                                                             \
    {   /* T + REP */                                              \
        MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code); \
        /* ML */                                                   \
        *((MCM_DTYPE_USIZE_TD *) tmp_offset) = rep_mlen;           \
        tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);                  \
        /* MC */                                                   \
        memcpy(tmp_offset, rep_msg, rep_mlen);                     \
        tmp_offset += rep_mlen;                                    \
        /* ... */                                                  \
    }                                                              \
    while(0)




enum MCM_REALLOC_MEMORY_OBJECT
{
    MCM_RMEMORY_PACKET = 0,
    MCM_RMEMORY_CACHE
};




// session 的預設堆疊大小.
MCM_DTYPE_USIZE_TD mcm_session_default_stack_size = 0;
// 是否限制同時執行的 session 數目.
// 0 : 否.
// 1 : 是.
MCM_DTYPE_BOOL_TD mcm_session_limit_count = 0;
// 目前可用的 session 的數目.
sem_t mcm_session_idle_pool;
// 目前執行中的 session 的數目.
sem_t mcm_session_busy_pool;
// 開啟服務的 socket.
int mcm_service_socket = 0;
// 開始接收連線的鎖.
sem_t mcm_service_mutex_run;
// service 的 thread id.
pthread_t mcm_service_thread_id;
// 存取 session 的鎖.
sem_t mcm_session_mutex_list;
// session 串列.
struct mcm_service_session_t *mcm_session_head = NULL, *mcm_session_tail = NULL;
// 需要存取權限的 session 串列.
struct mcm_service_session_t *mcm_session_access_head = NULL;
// 目前共有幾個 session 正在存取資料 (取得權限).
MCM_DTYPE_USIZE_TD mcm_session_access_use = 0;
// 是否限制 session 存取資料.
// 0 : 不限制, 目前正在存取資料的 session 是 MCM_SPERMISSION_RO (唯讀) 模式,
//     允許其他使用 MCM_SPERMISSION_RO 模式共享存取.
// 1 : 有限制, 目前正在存取資料的 session 是 MCM_SPERMISSION_RW (讀/寫) 模式,
//     禁止其他 session 共享存取.
MCM_DTYPE_BOOL_TD mcm_session_access_limit = 0;




int mcm_req_get_alone(
    struct mcm_service_session_t *this_session);

int mcm_req_set_alone(
    struct mcm_service_session_t *this_session);

int mcm_req_get_entry(
    struct mcm_service_session_t *this_session);

int mcm_req_set_entry(
    struct mcm_service_session_t *this_session);

int mcm_req_add_entry(
    struct mcm_service_session_t *this_session);

int mcm_req_del_entry(
    struct mcm_service_session_t *this_session);

int mcm_req_get_all_key(
    struct mcm_service_session_t *this_session);

int mcm_req_get_all_entry(
    struct mcm_service_session_t *this_session);

int mcm_req_del_all_entry(
    struct mcm_service_session_t *this_session);

int mcm_req_get_max_count(
    struct mcm_service_session_t *this_session);

int mcm_req_get_count(
    struct mcm_service_session_t *this_session);

int mcm_req_get_usable_key(
    struct mcm_service_session_t *this_session);

int mcm_req_run(
    struct mcm_service_session_t *this_session);

int mcm_req_update(
    struct mcm_service_session_t *this_session);

int mcm_req_save(
    struct mcm_service_session_t *this_session);

int mcm_req_shutdown(
    struct mcm_service_session_t *this_session);

int mcm_req_check_store_file(
    struct mcm_service_session_t *this_session);

int mcm_req_check_mask_path(
    struct mcm_service_session_t *this_session);

int mcm_req_get_path_max_length(
    struct mcm_service_session_t *this_session);

int mcm_req_get_list_name(
    struct mcm_service_session_t *this_session);

int mcm_req_get_list_type(
    struct mcm_service_session_t *this_session);

int mcm_req_get_list_value(
    struct mcm_service_session_t *this_session);

int mcm_req_set_any_type_alone(
    struct mcm_service_session_t *this_session);

int mcm_req_get_with_type_alone(
    struct mcm_service_session_t *this_session);




struct mcm_req_cb_t
{
    int (*handle_cb)(struct mcm_service_session_t *this_session);
};
// 函式的順序參考 mcm_lib/mcm_lheader/mcm_request.h 的 MCM_SERVICE_REQUEST.
struct mcm_req_cb_t mcm_req_cb_list[] =
{
    {mcm_req_get_alone},
    {mcm_req_set_alone},
    {mcm_req_get_entry},
    {mcm_req_set_entry},
    {mcm_req_add_entry},
    {mcm_req_del_entry},
    {mcm_req_get_all_key},
    {mcm_req_get_all_entry},
    {mcm_req_del_all_entry},
    {mcm_req_get_max_count},
    {mcm_req_get_count},
    {mcm_req_get_usable_key},
    {mcm_req_update},
    {mcm_req_save},
    {mcm_req_run},
    {mcm_req_shutdown},
    {mcm_req_check_store_file},
    {mcm_req_check_mask_path},
    {mcm_req_get_path_max_length},
    {mcm_req_get_list_name},
    {mcm_req_get_list_type},
    {mcm_req_get_list_value},
    {mcm_req_set_any_type_alone},
    {mcm_req_get_with_type_alone}
};




int mcm_realloc_buf_service(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD target_type,
    MCM_DTYPE_USIZE_TD new_size)
{
    void *tmp_buf;
    void **target_buf;
    MCM_DTYPE_USIZE_TD *target_size;


    if(target_type == MCM_RMEMORY_PACKET)
    {
        target_buf = &this_session->pkt_buf;
        target_size = &this_session->pkt_size;
    }
    else
    {
        target_buf = &this_session->cache_buf;
        target_size = &this_session->cache_size;
    }

    tmp_buf = realloc(*target_buf, new_size);
    if(tmp_buf == NULL)
    {
        MCM_EMSG("call realloc() fail [%s] (%s)",
                 strerror(errno), target_type == MCM_RMEMORY_PACKET ? "PACKET" : "CACHE");
        return MCM_RCODE_SERVICE_ALLOC_FAIL;
    }
    MCM_SVDMSG("realloc[" MCM_DTYPE_USIZE_PF "][%p] -> [" MCM_DTYPE_USIZE_PF "][%p]",
               *target_size, *target_buf, new_size, tmp_buf);

    *target_buf = tmp_buf;
    *target_size = new_size;

    return MCM_RCODE_PASS;
}

int mcm_socket_accept_check(
    int socket_fd)
{
    MCM_DTYPE_BOOL_TD shutdown_flag;
    fd_set fdset_fd;


    do
    {
        FD_ZERO(&fdset_fd);
        FD_SET(socket_fd, &fdset_fd);

        if(select(socket_fd + 1, &fdset_fd, NULL, NULL, NULL) == -1)
        {
            MCM_EMSG("call select() fail [%s]", strerror(errno));
            if(errno == EINTR)
            {
                MCM_CHECK_SHUTDOWN_01(shutdown_flag);
                continue;
            }
            return MCM_RCODE_SERVICE_INTERNAL_ERROR;
        }
    }
    while(0);

    if(FD_ISSET(socket_fd, &fdset_fd) == 0)
    {
        MCM_EMSG("call select() fail (FD_ISSET() return nothing)");
        return MCM_RCODE_SERVICE_INTERNAL_ERROR;
    }

    return MCM_RCODE_PASS;
}

int mcm_socket_accept_new(
    int socket_fd,
    int *socket_cfd_buf)
{
    MCM_DTYPE_BOOL_TD shutdown_flag;
    struct sockaddr_in sock_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int tmp_socket;


    do
    {
        tmp_socket = accept(socket_fd, (struct sockaddr *) &sock_addr, &addr_len);
        if(tmp_socket == -1)
        {
            MCM_EMSG("call accept() fail [%s]", strerror(errno));
            if(errno == EINTR)
            {
                MCM_CHECK_SHUTDOWN_01(shutdown_flag);
                continue;
            }
            return MCM_RCODE_SERVICE_INTERNAL_ERROR;
        }
        else
        {
            *socket_cfd_buf = tmp_socket;
        }
    }
    while(0);

    return MCM_RCODE_PASS;
}

int mcm_socket_send(
    struct mcm_service_session_t *this_session,
    void *data_con,
    MCM_DTYPE_USIZE_TD data_len,
    ssize_t *send_len_buf)
{
    MCM_DTYPE_BOOL_TD shutdown_flag;
    ssize_t tmp_len;


    do
    {
        tmp_len = send(this_session->socket_fd, data_con, data_len, 0);
        if(tmp_len == -1)
        {
            MCM_EMSG("call send() fail [%s] <%lu>", strerror(errno), this_session->thread_id);
            if(errno == EINTR)
            {
                MCM_CHECK_SHUTDOWN_01(shutdown_flag);
                continue;
            }
            return MCM_RCODE_SERVICE_INTERNAL_ERROR;
        }
        else
        {
           *send_len_buf = tmp_len;
        }
    }
    while(0);

    return MCM_RCODE_PASS;
}

int mcm_socket_recv(
    struct mcm_service_session_t *this_session,
    void *data_buf,
    MCM_DTYPE_USIZE_TD data_size,
    ssize_t *recv_len_buf)
{
    MCM_DTYPE_BOOL_TD shutdown_flag;
    ssize_t tmp_len;


    do
    {
        tmp_len = recv(this_session->socket_fd, data_buf, data_size, 0);
        if(tmp_len == -1)
        {
            MCM_EMSG("call recv() fail [%s] <%lu>", strerror(errno), this_session->thread_id);
            if(errno == EINTR)
            {
                MCM_CHECK_SHUTDOWN_01(shutdown_flag);
                continue;
            }
            return MCM_RCODE_SERVICE_INTERNAL_ERROR;
        }
        else
        {
            *recv_len_buf = tmp_len;
        }
    }
    while(0);

    return MCM_RCODE_PASS;
}

int mcm_build_fail_rep(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code,
    MCM_DTYPE_BOOL_TD with_msg)
{
    int fret;
    void *tmp_offset;


    if(with_msg == 0)
    {
        this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                                sizeof(MCM_DTYPE_LIST_TD);
    }
    else
    {
        this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                                sizeof(MCM_DTYPE_LIST_TD) +
                                sizeof(MCM_DTYPE_USIZE_TD) + 1;
    }

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            return fret;
        }
    }

    // 封包格式 (不帶有回應訊息, with_msg == 0) :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // 封包格式 (帶有回應訊息, with_msg != 0) :
    // | T | REP | ML | MC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP + ML + MC.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // ML  [MCM_DTYPE_USIZE_TD].
    //     紀錄回應的訊息的長度 (包含最後的 \0).
    // MC  [binary].
    //     紀錄回應的訊息 (空字串).

    if(with_msg == 0)
    {
        MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);
    }
    else
    {
        MCM_BUILD_BASE_REP_02(tmp_offset, this_session, rep_code, "", 1);
    }

    return MCM_RCODE_PASS;
}

void mcm_parse_get_alone(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // PC.
    this_session->req_path = (char *) this_session->pkt_offset;
    MCM_SVDMSG("req_path[%s]", this_session->req_path);
}

int mcm_build_get_alone(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code,
    void *rep_data_con,
    MCM_DTYPE_USIZE_TD rep_data_len)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD) +
                            sizeof(MCM_DTYPE_USIZE_TD) + rep_data_len;

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
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
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);
    // DL.
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = rep_data_len;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    memcpy(tmp_offset, rep_data_con, rep_data_len);
    tmp_offset += rep_data_len;

    return MCM_RCODE_PASS;
}

int mcm_req_get_alone(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_model_member_t *self_model_member;
    struct mcm_config_store_t *self_store;
    MCM_DTYPE_USIZE_TD tmp_len = 0;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_get_alone(this_session);

    fret = mcm_config_anysis_path(this_session, MCM_APATH_ALONE_FULL,
                                  this_session->req_path, 0,
                                  0, 0, NULL,
                                  MCM_PLIMIT_BOTH, MCM_PLIMIT_BOTH, 1,
                                  &self_model_group, &self_model_member,
                                  &self_store, NULL,
                                  NULL, NULL, NULL, NULL, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_config_anysis_path() fail");
        }
        goto FREE_01;
    }

    if(self_model_member->member_size > this_session->cache_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_CACHE,
                                       self_model_member->member_size);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            goto FREE_01;
        }
    }

    fret = mcm_config_get_alone_by_info(this_session, self_model_group, self_model_member,
                                        self_store, MCM_DACCESS_AUTO, this_session->cache_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_get_alone_by_info() fail");
        goto FREE_01;
    }

    switch(self_model_member->member_type)
    {
        case MCM_DTYPE_EK_INDEX:
#if MCM_SUPPORT_DTYPE_RK
        case MCM_DTYPE_RK_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISC
        case MCM_DTYPE_ISC_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUC
        case MCM_DTYPE_IUC_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISS
        case MCM_DTYPE_ISS_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUS
        case MCM_DTYPE_IUS_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISI
        case MCM_DTYPE_ISI_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUI
        case MCM_DTYPE_IUI_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISLL
        case MCM_DTYPE_ISLL_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IULL
        case MCM_DTYPE_IULL_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FF
        case MCM_DTYPE_FF_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FD
        case MCM_DTYPE_FD_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FLD
        case MCM_DTYPE_FLD_INDEX:
#endif
            tmp_len = self_model_member->member_size;
            break;
#if MCM_SUPPORT_DTYPE_S
        case MCM_DTYPE_S_INDEX:
            tmp_len = strlen(this_session->cache_buf) + 1;
            break;
#endif
#if MCM_SUPPORT_DTYPE_B
        case MCM_DTYPE_B_INDEX:
            tmp_len = self_model_member->member_size;
            break;
#endif
    }

FREE_01:
    cret = mcm_build_get_alone(this_session, fret, this_session->cache_buf,
                               fret < MCM_RCODE_PASS ? 0 : tmp_len);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_set_alone(
    struct mcm_service_session_t *this_session)
{
    void *tmp_offset;
    MCM_DTYPE_USIZE_TD plen;


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

    // PL.
    tmp_offset = this_session->pkt_offset;
    plen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // PC.
    this_session->req_path = (char *) tmp_offset;
    MCM_SVDMSG("req_path[" MCM_DTYPE_USIZE_PF "][%s]", plen, this_session->req_path);
    tmp_offset += plen;
    // DL.
    this_session->req_data_len = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    this_session->req_data_con = this_session->req_data_len > 0 ? tmp_offset : NULL;
    MCM_SVDMSG("req_config_info[" MCM_DTYPE_USIZE_PF "][%p]",
               this_session->req_data_len, this_session->req_data_con);
}

int mcm_build_set_alone(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD);

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);

    return MCM_RCODE_PASS;
}

int mcm_req_set_alone(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_model_member_t *self_model_member;
    struct mcm_config_store_t *self_store;
    void *tmp_con = NULL;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_set_alone(this_session);

    fret = mcm_config_anysis_path(this_session, MCM_APATH_ALONE_FULL,
                                  this_session->req_path, 0,
                                  0, 0, NULL,
                                  MCM_PLIMIT_BOTH, MCM_PLIMIT_BOTH, 1,
                                  &self_model_group, &self_model_member,
                                  &self_store, NULL,
                                  NULL, NULL, NULL, NULL, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_config_anysis_path() fail");
        }
        goto FREE_01;
    }

    switch(self_model_member->member_type)
    {
        case MCM_DTYPE_EK_INDEX:
#if MCM_SUPPORT_DTYPE_RK
        case MCM_DTYPE_RK_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISC
        case MCM_DTYPE_ISC_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUC
        case MCM_DTYPE_IUC_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISS
        case MCM_DTYPE_ISS_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUS
        case MCM_DTYPE_IUS_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISI
        case MCM_DTYPE_ISI_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUI
        case MCM_DTYPE_IUI_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISLL
        case MCM_DTYPE_ISLL_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IULL
        case MCM_DTYPE_IULL_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FF
        case MCM_DTYPE_FF_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FD
        case MCM_DTYPE_FD_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FLD
        case MCM_DTYPE_FLD_INDEX:
#endif
            tmp_con = this_session->req_data_con;
            break;
#if MCM_SUPPORT_DTYPE_S
        case MCM_DTYPE_S_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_B
        case MCM_DTYPE_B_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_S || MCM_SUPPORT_DTYPE_B
            tmp_con = this_session->req_data_con != NULL ? this_session->req_data_con : "";
            break;
#endif
    }

    fret = mcm_config_set_alone_by_info(this_session, self_model_group, self_model_member,
                                        self_store, MCM_DACCESS_NEW, tmp_con,
                                        this_session->req_data_len);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_set_alone_by_info() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_set_alone(this_session, fret);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_get_entry(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // PC.
    this_session->req_path = (char *) this_session->pkt_offset;
    MCM_SVDMSG("req_path[%s]", this_session->req_path);
}

int mcm_build_get_entry(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code,
    void *rep_data_con,
    MCM_DTYPE_USIZE_TD rep_data_len)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD) +
                            sizeof(MCM_DTYPE_USIZE_TD) + rep_data_len;

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
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
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);
    // DL.
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = rep_data_len;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    memcpy(tmp_offset, rep_data_con, rep_data_len);

    return MCM_RCODE_PASS;
}

int mcm_req_get_entry(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *self_store;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_get_entry(this_session);

    fret = mcm_config_anysis_path(this_session, MCM_APATH_ENTRY_FULL,
                                  this_session->req_path, 0,
                                  0, 0, NULL,
                                  MCM_PLIMIT_BOTH, MCM_PLIMIT_BOTH, 1,
                                  &self_model_group, NULL,
                                  &self_store, NULL,
                                  NULL, NULL, NULL, NULL, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_config_anysis_path() fail");
        }
        goto FREE_01;
    }

    if(self_model_group->data_value_size > this_session->cache_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_CACHE,
                                       self_model_group->data_value_size);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            goto FREE_01;
        }
    }

    fret = mcm_config_get_entry_by_info(this_session, self_model_group, self_store,
                                        MCM_DACCESS_AUTO, this_session->cache_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_get_entry_by_info() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_get_entry(this_session, fret, this_session->cache_buf,
                               fret < MCM_RCODE_PASS ? 0 : self_model_group->data_value_size);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_set_entry(
    struct mcm_service_session_t *this_session)
{
    void *tmp_offset;
    MCM_DTYPE_USIZE_TD plen;


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

    // PL.
    tmp_offset = this_session->pkt_offset;
    plen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // PC.
    this_session->req_path = (char *) tmp_offset;
    MCM_SVDMSG("req_path[" MCM_DTYPE_USIZE_PF "][%s]", plen, this_session->req_path);
    tmp_offset += plen;
    // DC.
    this_session->req_data_con = tmp_offset;
    MCM_SVDMSG("req_config_info[%p]", this_session->req_data_con);
}

int mcm_build_set_entry(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD);

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    //     紀錄回應的訊息.

    // T + REP.
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);

    return MCM_RCODE_PASS;
}

int mcm_req_set_entry(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *self_store;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_set_entry(this_session);

    fret = mcm_config_anysis_path(this_session, MCM_APATH_ENTRY_FULL,
                                  this_session->req_path, 0,
                                  0, 0, NULL,
                                  MCM_PLIMIT_BOTH, MCM_PLIMIT_BOTH, 1,
                                  &self_model_group, NULL,
                                  &self_store, NULL,
                                  NULL, NULL, NULL, NULL, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_config_anysis_path() fail");
        }
        goto FREE_01;
    }

    fret = mcm_config_set_entry_by_info(this_session, self_model_group, self_store,
                                        MCM_DACCESS_NEW, this_session->req_data_con);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_set_entry_by_info() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_set_entry(this_session, fret);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_add_entry(
    struct mcm_service_session_t *this_session)
{
    void *tmp_offset;
    MCM_DTYPE_USIZE_TD xlen;


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

    // PL.
    tmp_offset = this_session->pkt_offset;
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // PC.
    this_session->req_path = (char *) tmp_offset;
    MCM_SVDMSG("req_path[" MCM_DTYPE_USIZE_PF "][%s]", xlen, this_session->req_path);
    tmp_offset += xlen;
    // IL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // IC.
    this_session->req_other_path = (char *) tmp_offset;
    MCM_SVDMSG("req_other_path[" MCM_DTYPE_USIZE_PF "][%s]", xlen, this_session->req_other_path);
    tmp_offset += xlen;
    // DL.
    xlen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    this_session->req_data_con = xlen > 0 ? tmp_offset : NULL;
    MCM_SVDMSG("req_config_info[" MCM_DTYPE_USIZE_PF "][%p]", xlen, this_session->req_data_con);
}

int mcm_build_add_entry(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD);

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);

    return MCM_RCODE_PASS;
}

int mcm_req_add_entry(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    MCM_DTYPE_LIST_TD path_limit;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *self_store, *parent_store, *insert_store;
    MCM_DTYPE_EK_TD self_key;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_add_entry(this_session);

    path_limit = this_session->call_from == MCM_CFROM_WEB ? MCM_PLIMIT_KEY : MCM_PLIMIT_BOTH;

    fret = mcm_config_anysis_path(this_session, MCM_APATH_ENTRY_FULL,
                                  this_session->req_path, 0,
                                  0, 0, NULL,
                                  path_limit, MCM_PLIMIT_KEY, 0,
                                  &self_model_group, NULL,
                                  &self_store, &self_key,
                                  NULL, NULL, NULL, NULL, &parent_store);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_config_anysis_path() fail");
        }
        goto FREE_01;
    }

    fret = mcm_config_find_entry_use_ik(this_session, self_model_group, parent_store,
                                        this_session->req_other_path, path_limit, &insert_store);
    if(fret < MCM_RCODE_PASS)
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_config_find_entry_use_ik() fail");
            goto FREE_01;
        }

    fret = mcm_config_add_entry_by_info(this_session, self_model_group, parent_store, self_key,
                                        insert_store, MCM_DACCESS_NEW, 
                                        this_session->req_data_con, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_add_entry_by_info() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_add_entry(this_session, fret);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_del_entry(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // PC.
    this_session->req_path = (char *) this_session->pkt_offset;
    MCM_SVDMSG("req_path[%s]", this_session->req_path);
}

int mcm_build_del_entry(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD);

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);

    return MCM_RCODE_PASS;
}

int mcm_req_del_entry(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    MCM_DTYPE_LIST_TD path_limit;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *self_store;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_del_entry(this_session);

    path_limit = this_session->call_from == MCM_CFROM_WEB ? MCM_PLIMIT_KEY : MCM_PLIMIT_BOTH;

    fret = mcm_config_anysis_path(this_session, MCM_APATH_ENTRY_FULL,
                                  this_session->req_path, 0,
                                  0, 0, NULL,
                                  path_limit, path_limit, 1,
                                  &self_model_group, NULL,
                                  &self_store, NULL,
                                  NULL, NULL, NULL, NULL, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_config_anysis_path() fail");
        }
        goto FREE_01;
    }

    fret = mcm_config_del_entry_by_info(this_session, self_model_group, self_store,
                                        MCM_DACCESS_NEW);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_del_entry_by_info() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_del_entry(this_session, fret);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_get_all_key(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // PC.
    this_session->req_path = (char *) this_session->pkt_offset;
    MCM_SVDMSG("req_path[%s]", this_session->req_path);
}

int mcm_build_get_all_key(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code,
    MCM_DTYPE_EK_TD rep_count,
    void *rep_data_con,
    MCM_DTYPE_USIZE_TD rep_data_len)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD) +
                            sizeof(MCM_DTYPE_EK_TD) +
                            sizeof(MCM_DTYPE_USIZE_TD) + rep_data_len;

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
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
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);
    // EC.
    *((MCM_DTYPE_EK_TD *) tmp_offset) = rep_count;
    tmp_offset += sizeof(MCM_DTYPE_EK_TD);
    // DL.
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = rep_data_len;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    memcpy(tmp_offset, rep_data_con, rep_data_len);

    return MCM_RCODE_PASS;
}

int mcm_req_get_all_key(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *parent_store;
    MCM_DTYPE_EK_TD self_count = 0, self_size = 0;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_get_all_key(this_session);

    fret = mcm_config_anysis_path(this_session, MCM_APATH_ENTRY_MIX,
                                  this_session->req_path, 0,
                                  0, 0, NULL,
                                  MCM_PLIMIT_BOTH, 0, -1,
                                  &self_model_group, NULL,
                                  NULL, NULL,
                                  NULL, NULL, NULL, NULL, &parent_store);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_config_anysis_path() fail");
        }
        goto FREE_01;
    }

    fret = mcm_config_get_count_by_info(this_session, self_model_group, parent_store, &self_count);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_get_count_by_info() fail");
        goto FREE_01;
    }

    self_size = sizeof(MCM_DTYPE_EK_TD) * self_count;
    if(self_size > this_session->cache_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_CACHE, self_size);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            goto FREE_01;
        }
    }

    fret = mcm_config_get_all_key_by_info(this_session, self_model_group, parent_store,
                                          &self_count,
                                          (MCM_DTYPE_EK_TD **) &this_session->cache_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_get_all_key_by_info() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_get_all_key(this_session, fret,
                                 fret < MCM_RCODE_PASS ? 0 : self_count, this_session->cache_buf,
                                 fret < MCM_RCODE_PASS ? 0 : self_size);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_get_all_entry(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // PC.
    this_session->req_path = (char *) this_session->pkt_offset;
    MCM_SVDMSG("req_path[%s]", this_session->req_path);
}

int mcm_build_get_all_entry(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code,
    MCM_DTYPE_EK_TD rep_count,
    void *rep_data_con,
    MCM_DTYPE_USIZE_TD rep_data_len)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD) +
                            sizeof(MCM_DTYPE_EK_TD) +
                            sizeof(MCM_DTYPE_USIZE_TD) + rep_data_len;

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
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
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);
    // EC.
    *((MCM_DTYPE_EK_TD *) tmp_offset) = rep_count;
    tmp_offset += sizeof(MCM_DTYPE_EK_TD);
    // DL.
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = rep_data_len;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    memcpy(tmp_offset, rep_data_con, rep_data_len);

    return MCM_RCODE_PASS;
}

int mcm_req_get_all_entry(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *parent_store;
    MCM_DTYPE_EK_TD self_count = 0, self_size = 0;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_get_all_entry(this_session);

    fret = mcm_config_anysis_path(this_session, MCM_APATH_ENTRY_MIX,
                                  this_session->req_path, 0,
                                  0, 0, NULL,
                                  MCM_PLIMIT_BOTH, 0, -1,
                                  &self_model_group, NULL,
                                  NULL, NULL,
                                  NULL, NULL, NULL, NULL, &parent_store);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_config_anysis_path() fail");
        }
        goto FREE_01;
    }

    fret = mcm_config_get_count_by_info(this_session, self_model_group, parent_store, &self_count);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_get_count_by_info() fail");
        goto FREE_01;
    }

    self_size = self_model_group->data_value_size * self_count;
    if(self_size > this_session->cache_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_CACHE, self_size);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            goto FREE_01;
        }
    }

    fret = mcm_config_get_all_entry_by_info(this_session, self_model_group, parent_store,
                                            MCM_DACCESS_AUTO, &self_count,
                                            &this_session->cache_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_get_all_entry_by_info() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_get_all_entry(this_session, fret,
                                   fret < MCM_RCODE_PASS ? 0 : self_count, this_session->cache_buf,
                                   fret < MCM_RCODE_PASS ? 0 : self_size);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_del_all_entry(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // PC.
    this_session->req_path = (char *) this_session->pkt_offset;
    MCM_SVDMSG("req_path[%s]", this_session->req_path);
}

int mcm_build_del_all_entry(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD);

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);

    return MCM_RCODE_PASS;
}

int mcm_req_del_all_entry(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    MCM_DTYPE_LIST_TD path_limit;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *parent_store;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_del_all_entry(this_session);

    path_limit = this_session->call_from == MCM_CFROM_WEB ? MCM_PLIMIT_KEY : MCM_PLIMIT_BOTH;

    fret = mcm_config_anysis_path(this_session, MCM_APATH_ENTRY_MIX,
                                  this_session->req_path, 0,
                                  0, 0, NULL,
                                  path_limit, 0, -1,
                                  &self_model_group, NULL,
                                  NULL, NULL,
                                  NULL, NULL, NULL, NULL, &parent_store);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_config_anysis_path() fail");
        }
        goto FREE_01;
    }

    fret = mcm_config_del_all_entry_by_info(this_session, self_model_group, parent_store,
                                            MCM_DACCESS_NEW);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_del_all_entry_by_info() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_del_all_entry(this_session, fret);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_get_max_count(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // PC.
    this_session->req_path = (char *) this_session->pkt_offset;
    MCM_SVDMSG("req_path[%s]", this_session->req_path);
}

int mcm_build_get_max_count(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code,
    MCM_DTYPE_EK_TD rep_count)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD) +
                            sizeof(MCM_DTYPE_EK_TD);

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
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
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);
    // EC.
    *((MCM_DTYPE_EK_TD *) tmp_offset) = rep_count;

    return MCM_RCODE_PASS;
}

int mcm_req_get_max_count(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    MCM_DTYPE_EK_TD self_count = 0;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_get_max_count(this_session);

    fret = mcm_config_get_max_count_by_path(this_session, this_session->req_path, &self_count);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_get_max_count_by_path() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_get_max_count(this_session, fret, self_count);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_get_count(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // PC.
    this_session->req_path = (char *) this_session->pkt_offset;
    MCM_SVDMSG("req_path[%s]", this_session->req_path);
}

int mcm_build_get_count(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code,
    MCM_DTYPE_EK_TD rep_count)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD) +
                            sizeof(MCM_DTYPE_EK_TD);

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
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
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);
    // EC.
    *((MCM_DTYPE_EK_TD *) tmp_offset) = rep_count;

    return MCM_RCODE_PASS;
}

int mcm_req_get_count(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    MCM_DTYPE_EK_TD self_count = 0;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_get_count(this_session);

    fret = mcm_config_get_count_by_path(this_session, this_session->req_path, &self_count);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_config_get_count_by_path() fail");
        }
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_get_count(this_session, fret, self_count);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_get_usable_key(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // PC.
    this_session->req_path = (char *) this_session->pkt_offset;
    MCM_SVDMSG("req_path[%s]", this_session->req_path);
}

int mcm_build_get_usable_key(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code,
    MCM_DTYPE_EK_TD rep_usable_key)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD) +
                            sizeof(MCM_DTYPE_EK_TD);

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
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
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);
    // EK.
    *((MCM_DTYPE_EK_TD *) tmp_offset) = rep_usable_key;

    return MCM_RCODE_PASS;
}

int mcm_req_get_usable_key(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    MCM_DTYPE_EK_TD self_key = 0;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_get_usable_key(this_session);

    fret = mcm_config_get_usable_key_by_path(this_session, this_session->req_path, &self_key);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_config_get_usable_key_by_path() fail");
        }
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_get_usable_key(this_session, fret, self_key);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_update(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
}

int mcm_build_update(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD);

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);

    return MCM_RCODE_PASS;
}

int mcm_req_update(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_update(this_session);

    fret = mcm_config_update(this_session, MCM_DUPDATE_SYNC);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_update() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_update(this_session, fret);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_save(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ | DC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + DC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // DC  [MCM_DTYPE_BOOL_TD].
    //     紀錄要傳送的資料.

    // DC.
    this_session->req_data_con = this_session->pkt_offset;
    MCM_SVDMSG("req_config_info[%p]", this_session->req_data_con);
}

int mcm_build_save(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD);

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);

    return MCM_RCODE_PASS;
}

int mcm_req_save(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    MCM_DTYPE_BOOL_TD tmp_force;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_save(this_session);

    tmp_force = *((MCM_DTYPE_BOOL_TD *) this_session->req_data_con);

    fret = mcm_config_save(this_session, MCM_DUPDATE_SYNC, 0, tmp_force);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_save() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_save(this_session, fret);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_run(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // PC.
    this_session->req_path = (char *) this_session->pkt_offset;
    MCM_SVDMSG("req_path[%s]", this_session->req_path);
}

int mcm_build_run(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code,
    char *rep_msg)
{
    int fret;
    MCM_DTYPE_USIZE_TD mlen;
    void *tmp_offset;


    if(rep_msg == NULL)
    {
        rep_msg = "";
        mlen = 1;
    }
    else
    {
        mlen = strlen(rep_msg) + 1;
    }
    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD) +
                            sizeof(MCM_DTYPE_USIZE_TD) + mlen;

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 1);
            return fret;
        }
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
    MCM_BUILD_BASE_REP_02(tmp_offset, this_session, rep_code, rep_msg, mlen);

    return MCM_RCODE_PASS;
}

int mcm_req_run(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_MODULE_INTERNAL_ERROR, cret;
    char *dl_err;
    int (*module_cb)(struct mcm_service_session_t *this_session);


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_run(this_session);

    if(this_session->req_path[0] == '\0')
    {
        MCM_EMSG("empty function name");
        goto FREE_01;
    }

    module_cb = dlsym(mcm_config_module_fp, this_session->req_path);
    dl_err = dlerror();
    if(dl_err != NULL)
    {
        MCM_EMSG("call dlsym(%s) fail [%s]", this_session->req_path, dl_err);
        goto FREE_01;
    }

    fret = module_cb(this_session);
    if(fret < MCM_RCODE_PASS)
        goto FREE_01;

FREE_01:
    cret = mcm_build_run(this_session, fret, this_session->rep_msg_buf);
    mcm_service_response_exit(this_session);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_shutdown(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
}

int mcm_build_shutdown(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD);

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);

    return MCM_RCODE_PASS;
}

int mcm_req_shutdown(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_shutdown(this_session);

    fret = mcm_config_shutdown(this_session);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_shutdown() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_shutdown(this_session, fret);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_check_store_file(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄檢查的檔案的路徑.

    // PC.
    this_session->req_path = (char *) this_session->pkt_offset;
    MCM_SVDMSG("req_path[%s]", this_session->req_path);
}

int mcm_build_check_store_file(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code,
    MCM_DTYPE_LIST_TD store_result,
    char *version_con,
    MCM_DTYPE_USIZE_TD version_len)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD) +
                            sizeof(MCM_DTYPE_LIST_TD) +
                            sizeof(MCM_DTYPE_USIZE_TD) + version_len;

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
    }

    // 封包格式 :
    // | T | REP | SR | VL | VC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.
    // SR  [MCM_DTYPE_LIST_TD].
    //     紀錄檔案的檢查結果.
    // VL  [MCM_DTYPE_USIZE_TD].
    //     紀錄檔案的版本資訊的長度 (包含最後的 \0).
    // VC  [binary].
    //     紀錄檔案的版本資訊.

    // T + REP.
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);
    // SR.
    *((MCM_DTYPE_LIST_TD *) tmp_offset) = store_result;
    tmp_offset += sizeof(MCM_DTYPE_LIST_TD);
    // VL.
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = version_len;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // VC.
    memcpy(tmp_offset, version_con, version_len);

    return MCM_RCODE_PASS;
}

int mcm_req_check_store_file(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    MCM_DTYPE_LIST_TD store_result = MCM_RCODE_CONFIG_INTERNAL_ERROR;
    char store_version[MCM_BASE_VERSION_BUFFER_SIZE] = {0};
    MCM_DTYPE_USIZE_TD vlen;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_check_store_file(this_session);

    fret = mcm_config_check_store_file(this_session, this_session->req_path, &store_result,
                                       store_version, sizeof(store_version));
    vlen = strlen(store_version) + 1;
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_check_store_file() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_check_store_file(this_session, fret, store_result, store_version, vlen);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_check_mask_path(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // PC.
    this_session->req_path = (char *) this_session->pkt_offset;
    MCM_SVDMSG("req_path[%s]", this_session->req_path);
}

int mcm_build_check_mask_path(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD);

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);

    return MCM_RCODE_PASS;
}

int mcm_req_check_mask_path(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_check_mask_path(this_session);

    fret = mcm_config_find_group_by_mask(this_session, this_session->req_path, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_find_group_by_mask() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_check_mask_path(this_session, fret);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_get_path_max_length(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REQ.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
}

int mcm_build_get_path_max_length(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code,
    MCM_DTYPE_USIZE_TD rep_max_length)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD) +
                            sizeof(MCM_DTYPE_EK_TD);

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
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
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);
    // PML.
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = rep_max_length;

    return MCM_RCODE_PASS;
}

int mcm_req_get_path_max_length(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    MCM_DTYPE_USIZE_TD max_len = 0;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_get_path_max_length(this_session);

    fret = mcm_config_get_path_max_length(this_session, &max_len);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_get_path_max_length() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_get_path_max_length(this_session, fret, max_len);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_get_list_name(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // PC.
    this_session->req_path = (char *) this_session->pkt_offset;
    MCM_SVDMSG("req_path[%s]", this_session->req_path);
}

int mcm_layout_get_list_name(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_USIZE_TD rep_data_len,
    void **rep_data_buf)
{
    int fret;
    MCM_DTYPE_USIZE_TD len_offset, buf_offset;


    // 預先將資料放置位置設定到 this_session->pkt_buf 內,
    // 避免從 this_session->cache_buf 拷貝資料的動作.

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
    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD);
    // 紀錄資料長度要放到封包的位置.
    len_offset = this_session->pkt_len;
    // DL.
    this_session->pkt_len += sizeof(MCM_DTYPE_USIZE_TD);
    // 紀錄資料內容要放到封包的位置.
    buf_offset = this_session->pkt_len;
    // DC.
    this_session->pkt_len += rep_data_len;

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            return fret;
        }
    }

    // 預先設定 DL.
    *((MCM_DTYPE_USIZE_TD *) (this_session->pkt_buf + len_offset)) = rep_data_len;
    // 回傳 DC 位置.
    *rep_data_buf = this_session->pkt_buf + buf_offset;

    return MCM_RCODE_PASS;
}

int mcm_build_get_list_name(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code)
{
    void *tmp_offset;


    // 封包格式在 mcm_layout_get_list_name().

    // T + REP.
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);
    // DL + DC 部分已經放入封包.

    return MCM_RCODE_PASS;
}

int mcm_req_get_list_name(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    struct mcm_config_model_group_t *self_model_group;
    MCM_DTYPE_USIZE_TD tmp_size;
    void *tmp_buf;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_get_list_name(this_session);

    fret = mcm_config_find_group_by_mask(this_session, this_session->req_path, &self_model_group);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_find_group_by_mask() fail");
        goto FREE_01;
    }

    fret = mcm_config_get_list_name_size(this_session, self_model_group, &tmp_size);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_get_list_name_size() fail");
        goto FREE_01;
    }

    fret = mcm_layout_get_list_name(this_session, tmp_size, &tmp_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_layout_get_list_name() fail");
        goto FREE_01;
    }

    fret = mcm_config_get_list_name_data(this_session, self_model_group, tmp_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_get_list_name_data() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_get_list_name(this_session, fret);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_get_list_type(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // PC.
    this_session->req_path = (char *) this_session->pkt_offset;
    MCM_SVDMSG("req_path[%s]", this_session->req_path);
}

int mcm_layout_get_list_type(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_USIZE_TD rep_data_len,
    void **rep_data_buf)
{
    int fret;
    MCM_DTYPE_USIZE_TD len_offset, buf_offset;


    // 預先將資料放置位置設定到 this_session->pkt_buf 內,
    // 避免從 this_session->cache_buf 拷貝資料的動作.

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
    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD);
    // 紀錄資料長度要放到封包的位置.
    len_offset = this_session->pkt_len;
    // DL.
    this_session->pkt_len += sizeof(MCM_DTYPE_USIZE_TD);
    // 紀錄資料內容要放到封包的位置.
    buf_offset = this_session->pkt_len;
    // DC.
    this_session->pkt_len += rep_data_len;

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            return fret;
        }
    }

    // 預先設定 DL.
    *((MCM_DTYPE_USIZE_TD *) (this_session->pkt_buf + len_offset)) = rep_data_len;
    // 回傳 DC 位置.
    *rep_data_buf = this_session->pkt_buf + buf_offset;

    return MCM_RCODE_PASS;
}

int mcm_build_get_list_type(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code)
{
    void *tmp_offset;


    // 封包格式在 mcm_layout_get_list_type().

    // T + REP.
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);
    // DL + DC 部分已經放入封包.

    return MCM_RCODE_PASS;
}

int mcm_req_get_list_type(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    struct mcm_config_model_group_t *self_model_group;
    MCM_DTYPE_USIZE_TD tmp_size;
    void *tmp_buf;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_get_list_type(this_session);

    fret = mcm_config_find_group_by_mask(this_session, this_session->req_path, &self_model_group);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_find_group_by_mask() fail");
        goto FREE_01;
    }

    fret = mcm_config_get_list_type_size(this_session, self_model_group, &tmp_size);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_get_list_type_size() fail");
        goto FREE_01;
    }

    fret = mcm_layout_get_list_type(this_session, tmp_size, &tmp_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_layout_get_list_type() fail");
        goto FREE_01;
    }

    fret = mcm_config_get_list_type_data(this_session, self_model_group, tmp_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_get_list_type_data() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_get_list_type(this_session, fret);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_get_list_value(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // PC.
    this_session->req_path = (char *) this_session->pkt_offset;
    MCM_SVDMSG("req_path[%s]", this_session->req_path);
}

int mcm_layout_get_list_value(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_USIZE_TD rep_data_len,
    void **rep_data_buf)
{
    int fret;
    MCM_DTYPE_USIZE_TD len_offset, buf_offset;


    // 預先將資料放置位置設定到 this_session->pkt_buf 內,
    // 避免從 this_session->cache_buf 拷貝資料的動作.

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
    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD);
    // 紀錄資料長度要放到封包的位置.
    len_offset = this_session->pkt_len;
    // DL.
    this_session->pkt_len += sizeof(MCM_DTYPE_USIZE_TD);
    // 紀錄資料內容要放到封包的位置.
    buf_offset = this_session->pkt_len;
    // DC.
    this_session->pkt_len += rep_data_len;

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            return fret;
        }
    }

    // 預先設定 DL.
    *((MCM_DTYPE_USIZE_TD *) (this_session->pkt_buf + len_offset)) = rep_data_len;
    // 回傳 DC 位置.
    *rep_data_buf = this_session->pkt_buf + buf_offset;

    return MCM_RCODE_PASS;
}

int mcm_build_get_list_value(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code)
{
    void *tmp_offset;


    // 封包格式在 mcm_layout_get_list_value().

    // T + REP.
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);
    // DL + DC 部分已經放入封包.

    return MCM_RCODE_PASS;
}

int mcm_req_get_list_value(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_store_t *self_store;
    MCM_DTYPE_USIZE_TD tmp_size;
    void *tmp_buf;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_get_list_value(this_session);


    fret = mcm_config_anysis_path(this_session, MCM_APATH_ENTRY_FULL,
                                  this_session->req_path, 0,
                                  0, 0, NULL,
                                  MCM_PLIMIT_BOTH, MCM_PLIMIT_BOTH, 1,
                                  &self_model_group, NULL,
                                  &self_store, NULL,
                                  NULL, NULL, NULL, NULL, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_config_anysis_path() fail");
        }
        goto FREE_01;
    }

    fret = mcm_config_get_list_value_size(this_session, self_model_group, self_store, &tmp_size);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_get_list_value_size() fail");
        goto FREE_01;
    }

    fret = mcm_layout_get_list_value(this_session, tmp_size, &tmp_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_layout_get_list_value() fail");
        goto FREE_01;
    }

    fret = mcm_config_get_list_value_data(this_session, self_model_group, self_store, tmp_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_get_list_value_data() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_get_list_value(this_session, fret);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_set_any_type_alone(
    struct mcm_service_session_t *this_session)
{
    void *tmp_offset;
    MCM_DTYPE_USIZE_TD plen;


    // 封包格式 :
    // | T | REQ | PL | PC | DL | DC | \0 |.
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

    // PL.
    tmp_offset = this_session->pkt_offset;
    plen = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // PC.
    this_session->req_path = (char *) tmp_offset;
    MCM_SVDMSG("req_path[" MCM_DTYPE_USIZE_PF "][%s]", plen, this_session->req_path);
    tmp_offset += plen;
    // DL.
    this_session->req_data_len = *((MCM_DTYPE_USIZE_TD *) tmp_offset);
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    this_session->req_data_con = this_session->req_data_len > 0 ? tmp_offset : NULL;
    MCM_SVDMSG("req_config_info[" MCM_DTYPE_USIZE_PF "][%p]",
               this_session->req_data_len, this_session->req_data_con);
}

int mcm_build_set_any_type_alone(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD);

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
    }

    // 封包格式 :
    // | T | REP |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包的總長度, 內容 = T + REP.
    // REP [MCM_DTYPE_LIST_TD].
    //     紀錄回應的類型.

    // T + REP.
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);

    return MCM_RCODE_PASS;
}

int mcm_req_set_any_type_alone(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    MCM_DTYPE_LIST_TD path_limit;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_model_member_t *self_model_member;
    struct mcm_config_store_t *self_store;
    void *tmp_con;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_set_any_type_alone(this_session);

    path_limit = this_session->call_from == MCM_CFROM_WEB ? MCM_PLIMIT_KEY : MCM_PLIMIT_BOTH;

    fret = mcm_config_anysis_path(this_session, MCM_APATH_ALONE_FULL,
                                  this_session->req_path, 0,
                                  0, 0, NULL,
                                  path_limit, path_limit, 1,
                                  &self_model_group, &self_model_member,
                                  &self_store, NULL,
                                  NULL, NULL, NULL, NULL, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_config_anysis_path() fail");
        }
        goto FREE_01;
    }

    tmp_con = this_session->req_data_con != NULL ? this_session->req_data_con : "";

    fret = mcm_config_set_any_type_alone_by_info(this_session, self_model_group, self_model_member,
                                                 self_store, MCM_DACCESS_NEW, tmp_con,
                                                 this_session->req_data_len);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_set_any_type_alone_by_info() fail");
        goto FREE_01;
    }

FREE_01:
    cret = mcm_build_set_any_type_alone(this_session, fret);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

void mcm_parse_get_with_type_alone(
    struct mcm_service_session_t *this_session)
{
    // 封包格式 :
    // | T | REQ | PC |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = T + REQ + PC.
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // PC  [binary].
    //     紀錄請求的路徑.

    // PC.
    this_session->req_path = (char *) this_session->pkt_offset;
    MCM_SVDMSG("req_path[%s]", this_session->req_path);
}

int mcm_build_get_with_type_alone(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_LIST_TD rep_code,
    MCM_DTYPE_LIST_TD rep_data_type,
    void *rep_data_con,
    MCM_DTYPE_USIZE_TD rep_data_len)
{
    int fret;
    void *tmp_offset;


    this_session->pkt_len = sizeof(MCM_DTYPE_USIZE_TD) +
                            sizeof(MCM_DTYPE_LIST_TD) +
                            sizeof(MCM_DTYPE_LIST_TD) +
                            sizeof(MCM_DTYPE_USIZE_TD) + rep_data_len;

    if(this_session->pkt_len > this_session->pkt_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, this_session->pkt_len);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            mcm_build_fail_rep(this_session, fret, 0);
            return fret;
        }
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
    MCM_BUILD_BASE_REP_01(tmp_offset, this_session, rep_code);
    // DT.
    *((MCM_DTYPE_LIST_TD *) tmp_offset) = rep_data_type;
    tmp_offset += sizeof(MCM_DTYPE_LIST_TD);
    // DL.
    *((MCM_DTYPE_USIZE_TD *) tmp_offset) = rep_data_len;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // DC.
    memcpy(tmp_offset, rep_data_con, rep_data_len);
    tmp_offset += rep_data_len;

    return MCM_RCODE_PASS;
}

int mcm_req_get_with_type_alone(
    struct mcm_service_session_t *this_session)
{
    int fret, cret;
    struct mcm_config_model_group_t *self_model_group;
    struct mcm_config_model_member_t *self_model_member;
    struct mcm_config_store_t *self_store;
    MCM_DTYPE_USIZE_TD tmp_len = 0;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_parse_get_with_type_alone(this_session);

    fret = mcm_config_anysis_path(this_session, MCM_APATH_ALONE_FULL,
                                  this_session->req_path, 0,
                                  0, 0, NULL,
                                  MCM_PLIMIT_BOTH, MCM_PLIMIT_BOTH, 1,
                                  &self_model_group, &self_model_member,
                                  &self_store, NULL,
                                  NULL, NULL, NULL, NULL, NULL);
    if(fret < MCM_RCODE_PASS)
    {
        if(fret != MCM_RCODE_CONFIG_NOT_FIND_STORE)
        {
            MCM_ECTMSG("call mcm_config_anysis_path() fail");
        }
        goto FREE_01;
    }

    if(self_model_member->member_size > this_session->cache_size)
    {
        fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_CACHE,
                                       self_model_member->member_size);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_realloc_buf_service() fail");
            goto FREE_01;
        }
    }

    fret = mcm_config_get_alone_by_info(this_session, self_model_group, self_model_member,
                                        self_store, MCM_DACCESS_AUTO, this_session->cache_buf);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_get_alone_by_info() fail");
        goto FREE_01;
    }

    switch(self_model_member->member_type)
    {
        case MCM_DTYPE_EK_INDEX:
#if MCM_SUPPORT_DTYPE_RK
        case MCM_DTYPE_RK_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISC
        case MCM_DTYPE_ISC_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUC
        case MCM_DTYPE_IUC_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISS
        case MCM_DTYPE_ISS_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUS
        case MCM_DTYPE_IUS_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISI
        case MCM_DTYPE_ISI_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUI
        case MCM_DTYPE_IUI_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISLL
        case MCM_DTYPE_ISLL_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IULL
        case MCM_DTYPE_IULL_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FF
        case MCM_DTYPE_FF_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FD
        case MCM_DTYPE_FD_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FLD
        case MCM_DTYPE_FLD_INDEX:
#endif
            tmp_len = self_model_member->member_size;
            break;
#if MCM_SUPPORT_DTYPE_S
        case MCM_DTYPE_S_INDEX:
            tmp_len = strlen(this_session->cache_buf) + 1;
            break;
#endif
#if MCM_SUPPORT_DTYPE_B
        case MCM_DTYPE_B_INDEX:
            tmp_len = self_model_member->member_size;
            break;
#endif
    }

FREE_01:
    cret = mcm_build_get_with_type_alone(this_session, fret,
                                         fret < MCM_RCODE_PASS ?
                                         0 : self_model_member->member_type,
                                         this_session->cache_buf,
                                         fret < MCM_RCODE_PASS ? 0 : tmp_len);
    return fret < MCM_RCODE_PASS ? fret : cret;
}

int mcm_req_handle(
    struct mcm_service_session_t *this_session)
{
    int fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
    void *tmp_offset;


    // 封包格式 :
    // | T | REQ | ... |.
    // T   [MCM_DTYPE_USIZE_TD].
    //     紀錄封包總長度, 內容 = A + REQ + ...
    // REQ [MCM_DTYPE_LIST_TD].
    //     紀錄請求類型.
    // ...
    //     紀錄剩餘的資料.

    // T.
    tmp_offset = this_session->pkt_buf;
    tmp_offset += sizeof(MCM_DTYPE_USIZE_TD);
    // REQ.
    this_session->req_type = *((MCM_DTYPE_LIST_TD *) tmp_offset);
    MCM_SVDMSG("req_type[" MCM_DTYPE_LIST_PF "]", this_session->req_type);
    tmp_offset += sizeof(MCM_DTYPE_LIST_TD);
    // ...
    this_session->pkt_offset = tmp_offset;

    if((0 <= this_session->req_type) && (this_session->req_type < MCM_SREQUEST_MAX))
    {
        fret = mcm_req_cb_list[this_session->req_type].handle_cb(this_session);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_req_cb_list[" MCM_DTYPE_USIZE_PF "].handle_cb fail",
                       this_session->req_type);
        }
    }
    else
    {
        MCM_EMSG("unknown request type [" MCM_DTYPE_USIZE_PF "]", this_session->req_type);
        mcm_build_fail_rep(this_session, MCM_RCODE_SERVICE_INVALID_REQ, 0);
    }

    return fret;
}

int mcm_session_init(
    struct mcm_service_session_t *this_session)
{
    int fret;
    MCM_DTYPE_BOOL_TD is_allow = 0, has_post = 0, shutdown_flag;


    MCM_SVDMSG("=> %s <%lu>", __FUNCTION__, this_session->thread_id);

    if(sem_init(&this_session->access_mutex, 0, 0) == -1)
    {
        MCM_EMSG("call sem_init() fail [%s] <%lu>", strerror(errno), this_session->thread_id);
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
        goto FREE_01;
    }

    fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_CACHE, MCM_CACHE_BUFFER_SIZE);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_realloc_buf_service() fail <%lu>", this_session->thread_id);
        goto FREE_02;
    }

    fret = mcm_realloc_buf_service(this_session, MCM_RMEMORY_PACKET, MCM_PACKET_BUFFER_SIZE);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_realloc_buf_service() fail <%lu>", this_session->thread_id);
        goto FREE_03;
    }

    do
    {
        if(sem_wait(&mcm_session_mutex_list) == -1)
        {
            MCM_EMSG("call sem_wait() fail [%s] <%lu>", strerror(errno), this_session->thread_id);
            if(errno == EINTR)
            {
                MCM_CHECK_SHUTDOWN_02(shutdown_flag, fret, FREE_04);
                continue;
            }
            fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
            goto FREE_04;
        }
    }
    while(0);

    if(mcm_session_head == NULL)
    {
        mcm_session_head = mcm_session_tail = this_session;
    }
    else
    {
        mcm_session_tail->next_session = this_session;
        this_session->prev_session = mcm_session_tail;
        mcm_session_tail = this_session;
    }

    // 沒其他 session 使用, 取得權限.
    if(mcm_session_access_use == 0)
    {
        // 使用的 session 數增加.
        mcm_session_access_use++;
        // 如果 session 使用 MCM_SPERMISSION_RW 模式, 設定不共享存取.
        if(this_session->session_permission == MCM_SPERMISSION_RW)
            mcm_session_access_limit = 1;
        MCM_SVDMSG("access, wait[get][empty][%s][" MCM_DTYPE_USIZE_PF "] <%lu>",
                   MCM_DBG_SPERMISSION(this_session->session_permission), mcm_session_access_use,
                   this_session->thread_id);
    }
    // 有其他 session 使用.
    else
    {
        // 如果沒有其他 session 在排隊等待.
        // 如果正在使用的 session 是 MCM_SPERMISSION_RO 模式, 讀共享.
        // 如果要取得權限的 session 也是 MCM_SPERMISSION_RO 模式, 允許取得權限.
        if(mcm_session_access_head == NULL)
            if(mcm_session_access_limit == 0)
                if(this_session->session_permission == MCM_SPERMISSION_RO)
                    is_allow = 1;

        // 允許取得權限.
        if(is_allow != 0)
        {
            mcm_session_access_use++;
            MCM_SVDMSG("access, wait[get][share][%s][" MCM_DTYPE_USIZE_PF "] <%lu>",
                       MCM_DBG_SPERMISSION(this_session->session_permission),
                       mcm_session_access_use, this_session->thread_id);
        }
        // 禁止取得權限, 排隊.
        else
        {
            MCM_SVDMSG("access, wait[queue][%s] <%lu>",
                       MCM_DBG_SPERMISSION(this_session->session_permission),
                       this_session->thread_id);

            // 設定排隊的列隊開頭.
            if(mcm_session_access_head == NULL)
                mcm_session_access_head = this_session;

            has_post = 1;
            if(sem_post(&mcm_session_mutex_list) == -1)
            {
                MCM_EMSG("call sem_post() fail [%s] <%lu>",
                         strerror(errno), this_session->thread_id);
                goto FREE_04;
            }

            // 等待使用完畢的 session 喚醒.
            do
            {
                if(sem_wait(&this_session->access_mutex) == -1)
                {
                    MCM_EMSG("call sem_wait() fail [%s] <%lu>",
                             strerror(errno), this_session->thread_id);
                    if(errno == EINTR)
                        continue;
                    goto FREE_04;
                }
            }
            while(0);

            // 取得使用權.
            MCM_SVDMSG("access, wait[get][wake][%s] <%lu>",
                       MCM_DBG_SPERMISSION(this_session->session_permission),
                       this_session->thread_id);
        }
    }

    if(has_post == 0)
        if(sem_post(&mcm_session_mutex_list) == -1)
        {
            MCM_EMSG("call sem_post() fail [%s] <%lu>", strerror(errno), this_session->thread_id);
            fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
            goto FREE_04;
        }

    return MCM_RCODE_PASS;
FREE_04:
    free(this_session->pkt_buf);
FREE_03:
    free(this_session->cache_buf);
FREE_02:
    sem_destroy(&this_session->access_mutex);
FREE_01:
    return fret;
}

int mcm_session_exit(
    struct mcm_service_session_t *this_session)
{
    struct mcm_service_session_t *each_session;


    MCM_SVDMSG("=> %s <%lu>", __FUNCTION__, this_session->thread_id);

    do
    {
        if(sem_wait(&mcm_session_mutex_list) == -1)
        {
            MCM_EMSG("call sem_wait() fail [%s] <%lu>", strerror(errno), this_session->thread_id);
            if(errno == EINTR)
                continue;
        }
    }
    while(0);

    // 使用的 session 數減少.
    mcm_session_access_use--;
    MCM_SVDMSG("access, post[%s][" MCM_DTYPE_USIZE_PF "] <%lu>",
               MCM_DBG_SPERMISSION(this_session->session_permission),
               mcm_session_access_use, this_session->thread_id);

    // 最後一個使用完的 session 要喚醒列隊上等待的 session.
    if(mcm_session_access_use == 0)
    {
        // 清除限制.
        mcm_session_access_limit = 0;

        // 喚醒列隊上等待的 session.
        for(each_session = mcm_session_access_head; each_session != NULL;
            each_session = mcm_session_access_head)
        {
            // 步驟-04 :
            // 如果不是列隊上的第一個 session.
            // 如果 session 也是 MCM_SPERMISSION_RO 模式, 繼續喚醒.
            if(mcm_session_access_use != 0)
                if(mcm_session_access_head->session_permission == MCM_SPERMISSION_RW)
                    break;

            // 步驟-01 : 從列隊上移除被喚醒的 session.
            mcm_session_access_head = mcm_session_access_head->next_session;

            // 步驟-02 : 喚醒 session.
            mcm_session_access_use++;
            MCM_SVDMSG("access, notify[%lu][%s][" MCM_DTYPE_USIZE_PF "] <%lu>",
                       each_session->thread_id,
                       MCM_DBG_SPERMISSION(each_session->session_permission),
                       mcm_session_access_use, this_session->thread_id);
            if(sem_post(&each_session->access_mutex) == -1)
            {
                MCM_EMSG("call sem_post() fail [%s] <%lu>",
                         strerror(errno), this_session->thread_id);
            }
            // 步驟-03a : 如果 session 使用 MCM_SPERMISSION_RW 模式, 不共享存取.
            if(each_session->session_permission == MCM_SPERMISSION_RW)
            {
                mcm_session_access_limit = 1;
                break;
            }

            // 步驟-03b : 如果 session 使用 MCM_SPERMISSION_RO 模式, 讀共享,
            // 如果下一個 session 也是 MCM_SPERMISSION_RO 模式, 繼續喚醒下一個 session.
        }
    }

    if(this_session->prev_session != NULL)
        this_session->prev_session->next_session = this_session->next_session;
    if(this_session->next_session != NULL)
        this_session->next_session->prev_session = this_session->prev_session;
    if(mcm_session_head == this_session)
        mcm_session_head = this_session->next_session;
    if(mcm_session_tail == this_session)
        mcm_session_tail = this_session->prev_session;
    if(mcm_session_access_head == this_session)
        mcm_session_access_head = mcm_session_access_head->next_session;

    if(sem_post(&mcm_session_mutex_list) == -1)
    {
        MCM_EMSG("call sem_post() fail [%s] <%lu>", strerror(errno), this_session->thread_id);
    }

    MCM_SVDMSG("free pkt_buf[%p] <%lu>", this_session->pkt_buf, this_session->thread_id);
    free(this_session->pkt_buf);

    MCM_SVDMSG("free cache_buf[%p] <%lu>", this_session->cache_buf, this_session->thread_id);
    free(this_session->cache_buf);

    sem_destroy(&this_session->access_mutex);

    return MCM_RCODE_PASS;
}

int mcm_session_idle_inc(
    struct mcm_service_session_t *this_session)
{
    MCM_SVDMSG("=> %s <%lu>", __FUNCTION__, this_session == NULL ? 0 : this_session->thread_id);

    if(mcm_session_limit_count != 0)
        if(sem_post(&mcm_session_idle_pool) == -1)
        {
            MCM_EMSG("call sem_post() fail [%s] <%lu>",
                     strerror(errno), this_session == NULL ? 0 : this_session->thread_id);
            return MCM_RCODE_SERVICE_INTERNAL_ERROR;
        }

    return MCM_RCODE_PASS;
}

int mcm_session_idle_dec(
    struct mcm_service_session_t *this_session)
{
    MCM_DTYPE_BOOL_TD shutdown_flag;


    MCM_SVDMSG("=> %s <%lu>", __FUNCTION__, this_session == NULL ? 0 : this_session->thread_id);

    if(mcm_session_limit_count != 0)
        do
        {
            if(sem_wait(&mcm_session_idle_pool) == -1)
            {
                MCM_EMSG("call sem_wait() fail [%s] <%lu>",
                         strerror(errno), this_session == NULL ? 0 : this_session->thread_id);
                if(errno == EINTR)
                {
                    MCM_CHECK_SHUTDOWN_01(shutdown_flag);
                    continue;
                }
                return MCM_RCODE_SERVICE_INTERNAL_ERROR;
            }
        }
        while(0);

    return MCM_RCODE_PASS;
}

int mcm_session_busy_inc(
    struct mcm_service_session_t *this_session)
{
    MCM_SVDMSG("=> %s <%lu>", __FUNCTION__, this_session == NULL ? 0 : this_session->thread_id);

    if(sem_post(&mcm_session_busy_pool) == -1)
    {
        MCM_EMSG("call sem_post() fail [%s] <%lu>",
                 strerror(errno), this_session == NULL ? 0 : this_session->thread_id);
        return MCM_RCODE_SERVICE_INTERNAL_ERROR;
    }

    return MCM_RCODE_PASS;
}

int mcm_session_busy_dec(
    struct mcm_service_session_t *this_session)
{
    MCM_SVDMSG("=> %s <%lu>", __FUNCTION__, this_session == NULL ? 0 : this_session->thread_id);

    do
    {
        if(sem_wait(&mcm_session_busy_pool) == -1)
        {
            MCM_EMSG("call sem_wait() fail [%s] <%lu>",
                     strerror(errno), this_session == NULL ? 0 : this_session->thread_id);
            if(errno == EINTR)
                continue;
            return MCM_RCODE_SERVICE_INTERNAL_ERROR;
        }
    }
    while(0);

    return MCM_RCODE_PASS;
}

void *mcm_session_thread_handle(
    void *session_data)
{
    int fret, cret;
    struct mcm_service_session_t *self_session = (struct mcm_service_session_t *) session_data;
    MCM_DTYPE_BOOL_TD shutdown_flag = 0;
    char notify_usable = 0;
    ssize_t xlen;
    void *bloc;
    MCM_DTYPE_USIZE_TD bsize, tlen, clen;
    MCM_DTYPE_LIST_TD update_method;


    self_session->thread_id = pthread_self();

    MCM_SVDMSG("=> %s <%lu>", __FUNCTION__, self_session->thread_id);

    cret = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    if(cret != 0)
    {
        MCM_EMSG("call pthread_setcancelstate() fail [%s] <%lu>",
                 strerror(cret), self_session->thread_id);
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
        goto FREE_01;
    }

    // 初始化以及取得使用權限.
    fret = mcm_session_init(self_session);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_session_init() fail <%lu>", self_session->thread_id);
        goto FREE_01;
    }

    // 允許對方使用前確認是否發生結束執行的中斷.
    MCM_CHECK_SHUTDOWN_02(shutdown_flag, fret, FREE_02);

    // 通知對方可以使用.
    fret = mcm_socket_send(self_session, &notify_usable, sizeof(notify_usable), &xlen);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_socket_send() fail <%lu>", self_session->thread_id);
        goto FREE_02;
    }

    while(1)
    {
        // 在使用期間發生結束執行的中斷的話, 結束連接.
        MCM_CHECK_SHUTDOWN_02(shutdown_flag, fret, EXIT_CONNECT);

        // bloc 紀錄收到的資料要放在的何處.
        bloc = self_session->pkt_buf;
        // bsize 紀錄目前剩餘的緩衝大小.
        bsize = self_session->pkt_size;
        // self_session->pkt_len, tlen 紀錄資料的總長度.
        // clen 紀錄目前收的資料長度.
        self_session->pkt_len = tlen = clen = 0;
        while(self_session->pkt_len == 0)
        {
            // 接收資料.
            fret = mcm_socket_recv(self_session, bloc, bsize, &xlen);
            if(fret < MCM_RCODE_PASS)
            {
                MCM_ECTMSG("call mcm_socket_recv() fail <%lu>", self_session->thread_id);
                goto EXIT_CONNECT;
            }
            // 對方關閉連接, 結束連接.
            if(xlen == 0)
                goto EXIT_CONNECT;

            // == 0 表示讀取新的請求.
            if(tlen == 0)
            {
                // 前面 N byte (sizeof(MCM_DTYPE_USIZE_TD)) 是紀錄資料總長度,
                // 資料長度太小表是傳送有問題.
                if(xlen < sizeof(MCM_DTYPE_USIZE_TD))
                {
                    MCM_EMSG("recv fail (packet too small [%zd]) <%lu>",
                             xlen, self_session->thread_id);
                    fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
                    goto EXIT_CONNECT;
                }
                // 調整緩衝大小符合資料的長度.
                tlen = *((MCM_DTYPE_USIZE_TD *) bloc);
                if(tlen > self_session->pkt_size)
                {
                    fret = mcm_realloc_buf_service(self_session, MCM_RMEMORY_PACKET, tlen);
                    if(fret < MCM_RCODE_PASS)
                    {
                        MCM_ECTMSG("call mcm_realloc_buf_service() fail <%lu>",
                                   self_session->thread_id);
                        goto EXIT_CONNECT;
                    }
                }
            }

            // 目前收到的資料總長度.
            clen += xlen;
            // 調整下一段資料要從哪邊開始存放.
            bloc = self_session->pkt_buf + clen;
            // 調整剩餘的緩衝長度.
            bsize = self_session->pkt_size - clen;

            // 接收完畢.
            if(clen >= tlen)
                self_session->pkt_len = tlen;
        }

        // 處理請求.
        cret = mcm_req_handle(self_session);
        if(cret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_req_handle() fail <%lu>", self_session->thread_id);
        }

        // 傳送處理結果.
        fret = mcm_socket_send(self_session, self_session->pkt_buf, self_session->pkt_len, &xlen);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_socket_recv() fail <%lu>", self_session->thread_id);
            goto EXIT_CONNECT;
        }

        // 如果請求內容有錯誤, 結束連接.
        if(cret < MCM_RCODE_PASS)
            goto EXIT_CONNECT;
    }

EXIT_CONNECT:

    close(self_session->socket_fd);
    self_session->socket_fd = 0;

    // 更新資料.
    if(self_session->session_permission == MCM_SPERMISSION_RW)
    {
        update_method = (fret < MCM_RCODE_PASS) || (cret < MCM_RCODE_PASS) ||
                        (shutdown_flag != 0) ? MCM_DUPDATE_DROP : MCM_DUPDATE_SYNC;
        fret = mcm_config_save(self_session, update_method, 1, 0);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_config_save() fail <%lu>", self_session->thread_id);
            goto FREE_02;
        }
    }

    if(self_session->need_shutdown != 0)
    {
        fret = mcm_daemon_shutdown();
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_daemon_shutdown() fail <%lu>", self_session->thread_id);
            goto FREE_02;
        }
    }

FREE_02:
    // 釋放使用權限和資源.
    mcm_session_exit(self_session);
FREE_01:
    if(self_session->socket_fd > 0)
        close(self_session->socket_fd);

    mcm_session_busy_dec(self_session);

    mcm_session_idle_inc(self_session);

    MCM_SVDMSG("free session[%p] <%lu>", self_session, self_session->thread_id);
    free(self_session);

    pthread_exit(NULL);
}

int mcm_session_create(
    void)
{
    int fret, cret;
    struct mcm_service_session_t *each_session;
    struct mcm_connect_option_t connect_data;
    ssize_t xlen;
    pthread_attr_t tattr;
    pthread_t ttid;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    fret = mcm_session_idle_dec(NULL);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_session_idle_dec() fail");
        goto FREE_01;
    }

    each_session = calloc(1, sizeof(struct mcm_service_session_t));
    if(each_session == NULL)
    {
        MCM_EMSG("call calloc() fail [%s]", strerror(errno));
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
        goto FREE_01;
    }
    MCM_SVDMSG("alloc session [%zu][%p]", sizeof(struct mcm_service_session_t), each_session);

    fret = mcm_socket_accept_new(mcm_service_socket, &each_session->socket_fd);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_socket_accept_new() fail");
        goto FREE_02;
    }

    fret = mcm_socket_recv(each_session, &connect_data, sizeof(struct mcm_connect_option_t), &xlen);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_socket_accept_new() fail");
        goto FREE_03;
    }
    if(xlen != sizeof(struct mcm_connect_option_t))
    {
        MCM_EMSG("connect option not match [%zd/%zu]", xlen, sizeof(struct mcm_connect_option_t));
        goto FREE_03;
    }

    each_session->call_from = connect_data.call_from;
    each_session->session_permission = connect_data.session_permission;
    each_session->session_stack_size = connect_data.session_stack_size;
    if(each_session->session_stack_size == 0)
        each_session->session_stack_size = mcm_session_default_stack_size;
    else
        if(each_session->session_stack_size < PTHREAD_STACK_MIN)
            each_session->session_stack_size = PTHREAD_STACK_MIN;
    MCM_SVDMSG("accept[" MCM_DTYPE_LIST_PF "][%s][" MCM_DTYPE_USIZE_PF "]",
               each_session->call_from, MCM_DBG_SPERMISSION(each_session->session_permission),
               each_session->session_stack_size);

    fret = mcm_session_busy_inc(NULL);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_session_busy_inc() fail");
        goto FREE_03;
    }

    cret = pthread_attr_init(&tattr);
    if(cret != 0)
    {
        MCM_EMSG("call pthread_attr_init() fail [%s]", strerror(cret));
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
        goto FREE_04;
    }

    cret = pthread_attr_setstacksize(&tattr, each_session->session_stack_size);
    if(cret != 0)
    {
        MCM_EMSG("call pthread_attr_setstacksize() fail [%s]", strerror(cret));
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
        goto FREE_05;
    }

    cret = pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    if(cret != 0)
    {
        MCM_EMSG("call pthread_attr_setdetachstate() fail [%s]", strerror(cret));
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
        goto FREE_05;
    }

    cret = pthread_create(&ttid, &tattr, mcm_session_thread_handle, each_session);
    if(cret != 0)
    {
        MCM_EMSG("call pthread_create() fail [%s]", strerror(cret));
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
        goto FREE_05;
    }

    fret = MCM_RCODE_PASS;
FREE_05:
    pthread_attr_destroy(&tattr);
FREE_04:
    if(fret < MCM_RCODE_PASS)
        mcm_session_busy_dec(NULL);
FREE_03:
    if(fret < MCM_RCODE_PASS)
        close(each_session->socket_fd);
FREE_02:
    if(fret < MCM_RCODE_PASS)
        free(each_session);
FREE_01:
    return fret;
}

int mcm_session_shutdown(
    void)
{
    int cret;
    struct mcm_service_session_t *each_session;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    do
    {
        if(sem_wait(&mcm_session_mutex_list) == -1)
        {
            MCM_EMSG("call sem_wait() fail [%s]", strerror(errno));
            if(errno == EINTR)
                continue;
        }
    }
    while(0);

    for(each_session = mcm_session_head; each_session != NULL;
        each_session = each_session->next_session)
    {
        MCM_SVDMSG("notify session[%lu]", each_session->thread_id);
        if(each_session->thread_id != 0)
        {
            cret = pthread_kill(each_session->thread_id, SIGUSR1);
            if(cret != 0)
            {
                MCM_EMSG("call pthread_kill() fail [%s]", strerror(cret));
            }
        }
        else
        {
            MCM_EMSG("zero session[%ld]", each_session->thread_id);
        }
    }

    if(sem_post(&mcm_session_mutex_list) == -1)
    {
        MCM_EMSG("call sem_post() fail [%s]", strerror(errno));
    }

    return MCM_RCODE_PASS;
}

void *mcm_service_thread_handle(
    void *data)
{
    int fret, cret, busy_count;
    sigset_t sig_block;
    MCM_DTYPE_BOOL_TD shutdown_flag = 0;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    cret = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    if(cret != 0)
    {
        MCM_EMSG("call pthread_setcancelstate() fail [%s]", strerror(cret));
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
        goto EXIT_ACCEPT;
    }

    // service 部分使用異步模式處理主執行緒傳送的 MCM_SERVICE_SHUTDOWN_SIGNAL, 解除阻塞.
    sigfillset(&sig_block);
    sigdelset(&sig_block, MCM_SERVICE_SHUTDOWN_SIGNAL);
    cret = pthread_sigmask(SIG_SETMASK, &sig_block, NULL);
    if(cret != 0)
    {
        MCM_EMSG("call pthread_sigmask() fail [%s]", strerror(cret));
        fret = MCM_RCODE_DAEMON_INTERNAL_ERROR;
        goto EXIT_ACCEPT;
    }

    // 等待 mcm_action_boot_other_run() 執行完畢.
    fret = mcm_service_run_wait();
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_service_run_wait() fail");
        goto EXIT_ACCEPT;
    }

    while(1)
    {
        MCM_CHECK_SHUTDOWN_02(shutdown_flag, fret, EXIT_ACCEPT);

        fret = mcm_socket_accept_check(mcm_service_socket);
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_socket_accept_check() fail");
            goto EXIT_ACCEPT;
        }

        fret = mcm_session_create();
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_session_create() fail");
            goto EXIT_ACCEPT;
        }
    }

EXIT_ACCEPT:

    // 離開 service 迴圈不是因為發生結束執行的中斷, 表示是發生錯誤而離開迴圈,
    // 通知 daemon 結束執行.
    if(shutdown_flag == 0)
    {
        // 通知.
        fret = mcm_daemon_shutdown();
        if(fret < MCM_RCODE_PASS)
        {
            MCM_ECTMSG("call mcm_daemon_shutdown() fail");
        }
        // 等待確認結束執行.
        while(shutdown_flag == 0)
        {
            mcm_daemon_get_shutdown(&shutdown_flag);
            if(shutdown_flag == 0)
                usleep(100000);
        }
    }

    // 通知所有 session 結束執行.
    fret = mcm_session_shutdown();
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_session_shutdown() fail");
    }

    // 等待所有 session 結束.
    do
    {
        if(sem_getvalue(&mcm_session_busy_pool, &busy_count) == -1)
        {
            MCM_EMSG("call sem_getvalue() fail [%s]", strerror(errno));
            break;
        }
        if(busy_count > 0)
            usleep(100000);
    }
    while(busy_count > 0);

    pthread_exit(NULL);
}

int mcm_service_init(
    char *socket_path,
    MCM_DTYPE_USIZE_TD max_session_count)
{
    int fret, cret;
    struct rlimit sys_limit;
    struct sockaddr_un sock_addr;
    socklen_t addr_len;
    pthread_attr_t tattr;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    if(getrlimit(RLIMIT_STACK, &sys_limit) == -1)
    {
        MCM_EMSG("call getrlimit(RLIMIT_STACK) fail [%s]", strerror(errno));
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
        goto FREE_01;
    }
    mcm_session_default_stack_size = sys_limit.rlim_cur;
    MCM_SVDMSG("stack[default = " MCM_DTYPE_USIZE_PF "][min = " MCM_DTYPE_USIZE_PF "]",
               mcm_session_default_stack_size, PTHREAD_STACK_MIN);

    // 如果有限制最大的 session 數, 使用信號量處理,
    // 初始化時可用的資源數目設為 max_session_count.
    if(max_session_count > 0)
    {
        mcm_session_limit_count = 1;
        if(sem_init(&mcm_session_idle_pool, 0, max_session_count) == -1)
        {
            MCM_EMSG("call sem_init() fail [%s]", strerror(errno));
            fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
            goto FREE_01;
        }
    }

    if(sem_init(&mcm_session_busy_pool, 0, 0) == -1)
    {
        MCM_EMSG("call sem_init() fail [%s]", strerror(errno));
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
        goto FREE_02;
    }

    if(sem_init(&mcm_session_mutex_list, 0, 1) == -1)
    {
        MCM_EMSG("call sem_init() fail [%s]", strerror(errno));
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
        goto FREE_03;
    }

    memset(&sock_addr, 0, sizeof(struct sockaddr_un));
    sock_addr.sun_family = AF_UNIX;
    snprintf(sock_addr.sun_path, sizeof(sock_addr.sun_path), "%s", socket_path);
    sock_addr.sun_path[0] = '\0';
    addr_len = strlen(socket_path) + offsetof(struct sockaddr_un, sun_path);

    mcm_service_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if(mcm_service_socket == -1)
    {
        MCM_EMSG("call socket() fail [%s]", strerror(errno));
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
        goto FREE_04;
    }

    if((bind(mcm_service_socket, (struct sockaddr *) &sock_addr, addr_len)) == -1)
    {
        MCM_EMSG("call bind() fail [%s]", strerror(errno));
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
        goto FREE_05;
    }

    if(listen(mcm_service_socket, MCM_SOCKET_LISTEN_SIZE) == -1)
    {
        MCM_EMSG("call listen() fail [%s]", strerror(errno));
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
        goto FREE_05;
    }

    if(sem_init(&mcm_service_mutex_run, 0, 0) == -1)
    {
        MCM_EMSG("call sem_init() fail [%s]", strerror(errno));
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
        goto FREE_05;
    }

    cret = pthread_attr_init(&tattr);
    if(cret != 0)
    {
        MCM_EMSG("call pthread_attr_init() fail [%s]", strerror(cret));
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
        goto FREE_06;
    }

    cret = pthread_attr_setstacksize(&tattr,
                                     MCM_SERVICE_STACK_SIZE > PTHREAD_STACK_MIN ?
                                     MCM_SERVICE_STACK_SIZE : PTHREAD_STACK_MIN);
    if(cret != 0)
    {
        MCM_EMSG("call pthread_attr_setstacksize() fail [%s]", strerror(cret));
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
        goto FREE_07;
    }

    cret = pthread_create(&mcm_service_thread_id, &tattr, mcm_service_thread_handle, NULL);
    if(cret != 0)
    {
        MCM_EMSG("call pthread_create() fail [%s]", strerror(cret));
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
        goto FREE_07;
    }

    fret = MCM_RCODE_PASS;
FREE_07:
    pthread_attr_destroy(&tattr);
FREE_06:
    if(fret < MCM_RCODE_PASS)
        sem_destroy(&mcm_service_mutex_run);
FREE_05:
    if(fret < MCM_RCODE_PASS)
        close(mcm_service_socket);
FREE_04:
    if(fret < MCM_RCODE_PASS)
        sem_destroy(&mcm_session_mutex_list);
FREE_03:
    if(fret < MCM_RCODE_PASS)
        sem_destroy(&mcm_session_busy_pool);
FREE_02:
    if(fret < MCM_RCODE_PASS)
        if(mcm_session_limit_count != 0)
            sem_destroy(&mcm_session_idle_pool);
FREE_01:
    return fret;
}

int mcm_service_exit(
    void)
{
    int cret;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    cret = pthread_join(mcm_service_thread_id, NULL);
    if(cret != 0)
    {
        MCM_EMSG("call pthread_join() fail [%s]", strerror(cret));
    }

    sem_destroy(&mcm_service_mutex_run);

    close(mcm_service_socket);

    sem_destroy(&mcm_session_mutex_list);

    sem_destroy(&mcm_session_busy_pool);

    if(mcm_session_limit_count != 0)
        sem_destroy(&mcm_session_idle_pool);

    return MCM_RCODE_PASS;
}

int mcm_service_shutdown(
    void)
{
    int fret = MCM_RCODE_PASS, cret;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    MCM_SVDMSG("notify service[%lu]", mcm_service_thread_id);
    cret = pthread_kill(mcm_service_thread_id, MCM_SERVICE_SHUTDOWN_SIGNAL);
    if(cret != 0)
    {
        MCM_EMSG("call pthread_kill() fail [%s]", strerror(cret));
        fret = MCM_RCODE_SERVICE_INTERNAL_ERROR;
    }

    return fret;
}

int mcm_service_run_wait(
    void)
{
    MCM_SVDMSG("=> %s", __FUNCTION__);

    do
    {
        if(sem_wait(&mcm_service_mutex_run) == -1)
        {
            MCM_EMSG("call sem_wait() fail [%s]", strerror(errno));
            if(errno == EINTR)
                continue;
            return MCM_RCODE_SERVICE_INTERNAL_ERROR;
        }
    }
    while(0);

    return MCM_RCODE_PASS;
}

int mcm_service_run_post(
    void)
{
    MCM_SVDMSG("=> %s", __FUNCTION__);

    if(sem_post(&mcm_service_mutex_run) == -1)
    {
        MCM_EMSG("call sem_post() fail [%s]", strerror(errno));
        return MCM_RCODE_SERVICE_INTERNAL_ERROR;
    }

    return MCM_RCODE_PASS;
}

int mcm_service_response_init(
    struct mcm_service_session_t *this_session,
    MCM_DTYPE_USIZE_TD buf_size)
{
    MCM_SVDMSG("=> %s", __FUNCTION__);

    if(this_session == NULL)
    {
        MCM_EMSG("this_session is NULL");
        return MCM_RCODE_SERVICE_INTERNAL_ERROR;
    }

    this_session->rep_msg_buf = (char *) calloc(1, buf_size);
    if(this_session->rep_msg_buf == NULL)
    {
        MCM_EMSG("call calloc() fail [%s]", strerror(errno));
        return MCM_RCODE_SERVICE_INTERNAL_ERROR;
    }
    this_session->rep_msg_size = buf_size;
    MCM_SVDMSG("alloc rep_msg_buf[" MCM_DTYPE_USIZE_PF "][%p]",
               this_session->rep_msg_size, this_session->rep_msg_buf);

    return MCM_RCODE_PASS;
}

int mcm_service_response_exit(
    struct mcm_service_session_t *this_session)
{
    MCM_SVDMSG("=> %s", __FUNCTION__);

    if(this_session == NULL)
    {
        MCM_EMSG("this_report is NULL");
        return MCM_RCODE_SERVICE_INTERNAL_ERROR;
    }

    MCM_SVDMSG("free msg_buf[%p]", this_session->rep_msg_buf);
    if(this_session->rep_msg_buf != NULL)
        free(this_session->rep_msg_buf);

    this_session->rep_msg_buf = NULL;
    this_session->rep_msg_size = 0;

    return MCM_RCODE_PASS;
}

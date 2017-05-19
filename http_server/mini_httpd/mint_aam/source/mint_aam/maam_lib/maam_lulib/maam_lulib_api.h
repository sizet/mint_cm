/*
Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
This file is part of the MintAAM.
Some rights reserved. See README.
*/

#ifndef _MAAM_LULIB_API_H_
#define _MAAM_LULIB_API_H_




#include <sys/types.h>
#include "../../maam_buildin/maam_common.h"




// 找到 session 串列的頭.
#define MAAM_LULIB_HEAD_SESSION(auth_sys_info) \
    auth_sys_info->session_head == -1 ?                                  \
        NULL : auth_sys_info->session_info + auth_sys_info->session_head

// 找到 session 串列的尾.
#define MAAM_LULIB_TAIL_SESSION(auth_sys_info) \
    auth_sys_info->session_tail == -1 ?                                  \
        NULL : auth_sys_info->session_info + auth_sys_info->session_tail

// 找到目前 session 的後一個 session.
#define MAAM_LULIB_NEXT_SESSION(auth_sys_info, link_session) \
    link_session->next_session == -1 ?                                  \
        NULL : auth_sys_info->session_info + link_session->next_session

// 找到目前 session 的前一個 session.
#define MAAM_LULIB_PREV_SESSION(auth_sys_info, link_session) \
    link_session->prev_session == -1 ?                                  \
        NULL : auth_sys_info->session_info + link_session->prev_session

// 轉換網路格式的位址到字串格式.
#define MAAM_LULIB_IP_NTOH(usockaddr_info, host_buf, host_size) \
    MAAM_IP_NTOH(usockaddr_info, host_buf, host_size)




struct maam_lulib_t
{
    // session 資料的共享記憶體的 key 值.
    key_t sm_key;
    // 存取 session 資料的檔案互斥鎖的路徑.
    char *sm_mutex_path;
    // 紀錄使用 shmget() 開啟共享記憶體的 id.
    int sm_id;
    // 紀錄使用 open() 開啟檔案互斥鎖的 fd.
    int sm_mutex_fd;
    // 紀錄 session 資料的記憶體位址.
    struct maam_auth_sys_t *auth_sys;
};




extern int maam_lulib_show_msg;




int maam_lulib_init(
    struct maam_lulib_t *maam_lulib_info);

int maam_lulib_exit(
    struct maam_lulib_t *maam_lulib_info);

int maam_lulib_kick_session(
    struct maam_auth_sys_t *auth_sys_info,
    struct maam_session_t *session_info);




#endif

/*
Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
This file is part of the MintAAM.
Some rights reserved. See README.
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "maam_lulib_api.h"




// 是否使用 printf 顯示訊息 (MAAM_EMSG, MAAM_ECTMODE, MAAM_LUDMSG).
// 1 : 是.
// 0 : 否.
// 如果輸出被重定向到其他非 console 裝置 (如 CGI),
// 則需要在使用此函式庫的程式內將值設為 0.
int maam_lulib_show_msg = 1;




// 初始化.
int maam_lulib_init(
    struct maam_lulib_t *maam_lulib_info)
{
    int fret = MAAM_RCODE_INTERNAL_ERROR, file_fd, sm_id;
    void *sm_addr;


    sm_id = shmget(maam_lulib_info->sm_key, sizeof(struct maam_auth_sys_t),
                   IPC_CREAT | MAAM_PERMISSION);
    if(sm_id == -1)
    {
        if(maam_lulib_show_msg != 0)
        {
            MAAM_EMSG("call shmget(%d) fail [%s]", MAAM_SHARE_MEMORY_KEY, strerror(errno));
        }
	    goto FREE_01;
    }

    sm_addr = shmat(sm_id, NULL, 0);
    if(sm_addr == (void *) -1)
    {
        if(maam_lulib_show_msg != 0)
        {
            MAAM_EMSG("call shmat() fail [%s]", strerror(errno));
        }
        goto FREE_01;
    }

    file_fd = open(maam_lulib_info->sm_mutex_path, O_RDWR);
    if(file_fd == -1)
    {
        if(maam_lulib_show_msg != 0)
        {
            MAAM_EMSG("call open(%s) fail [%s]", maam_lulib_info->sm_mutex_path, strerror(errno));
        }
        fret = MAAM_RCODE_INTERNAL_ERROR;
        goto FREE_02;
    }

    if(flock(file_fd, LOCK_EX) == -1)
    {
        if(maam_lulib_show_msg != 0)
        {
            MAAM_EMSG("call flock() fail [%s]", strerror(errno));
        }
        fret = MAAM_RCODE_INTERNAL_ERROR;
        goto FREE_03;
    }

    maam_lulib_info->sm_id = sm_id;
    maam_lulib_info->sm_mutex_fd = file_fd;
    maam_lulib_info->auth_sys = (struct maam_auth_sys_t *) sm_addr;

    return MAAM_RCODE_PASS;
FREE_03:
    close(file_fd);
FREE_02:
    shmdt(sm_addr);
FREE_01:
    return fret;
}

// 釋放資源.
int maam_lulib_exit(
    struct maam_lulib_t *maam_lulib_info)
{
    flock(maam_lulib_info->sm_mutex_fd, LOCK_UN);
    close(maam_lulib_info->sm_mutex_fd);
    shmdt(maam_lulib_info->auth_sys);

    return MAAM_RCODE_PASS;
}

// 踢掉 session.
int maam_lulib_kick_session(
    struct maam_auth_sys_t *auth_sys_info,
    struct maam_session_t *session_info)
{
    if(maam_lulib_show_msg != 0)
    {
        MAAM_LUDMSG("kick session [%s][%s]", session_info->account_name, session_info->session_key);
    }

    if(session_info->prev_session != -1)
    {
        auth_sys_info->session_info[session_info->prev_session].next_session =
            session_info->next_session;
    }

    if(session_info->next_session != -1)
    {
        auth_sys_info->session_info[session_info->next_session].prev_session =
            session_info->prev_session;
    }

    if(auth_sys_info->session_head == session_info->session_index)
        auth_sys_info->session_head = session_info->next_session;

    if(auth_sys_info->session_tail == session_info->session_index)
        auth_sys_info->session_tail = session_info->prev_session;

    session_info->empty_session = auth_sys_info->session_usable;
    auth_sys_info->session_usable = session_info->session_index;

    auth_sys_info->session_count--;

    return MAAM_RCODE_PASS;
}

// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "../mcm_lib/mcm_lheader/mcm_type.h"
#include "../mcm_lib/mcm_lheader/mcm_return.h"
#include "../mcm_lib/mcm_lheader/mcm_debug.h"
#include "mcm_service_handle_extern.h"
#include "mcm_config_handle_extern.h"
#include "mcm_action_handle_extern.h"
#include "mcm_daemon_extern.h"




#define MCM_DEFAULT_MAX_SESSION_COUNT 0

#define MCM_DEFAULT_SOCKET_PATH "@mintcm" MULTIPLE_MODEL_SUFFIX_LOWER

#define MCM_DEFAULT_MODULE_PATH "./mcm_module" MULTIPLE_MODEL_SUFFIX_LOWER ".lib"

#define MCM_DEFAULT_MODEL_PATH "mcm_model_profile" MULTIPLE_MODEL_SUFFIX_LOWER ".txt"

#define MCM_DEFAULT_STORE_DEFAULT_PATH "mcm_store_profile_default" MULTIPLE_MODEL_SUFFIX_LOWER ".txt"
#define MCM_DEFAULT_STORE_CURRENT_PATH "mcm_store_profile_current" MULTIPLE_MODEL_SUFFIX_LOWER ".txt"

#define MCM_DEFAULT_STORE_SAVE_MODE MCM_SSAVE_AUTO

#define MCM_DEFAULT_STORE_ERROR_HANDLE MCM_EHANDLE_INTERNAL_RESET_DEFAULT

#define MCM_DEFAULT_PID_PATH "/var/run/mcm_daemon" MULTIPLE_MODEL_SUFFIX_LOWER ".pid"




// 主執行緒的 thread id.
pthread_t mcm_daemon_thread_id;
// 主執行緒阻塞的信號.
sigset_t mcm_daemon_sig_block;
// 存取 mcm_daemon_quit 的鎖.
sem_t mcm_daemon_mutex_quit;
// 是否結束程式.
// 0 : 否.
// 1 : 是.
MCM_DTYPE_BOOL_TD mcm_daemon_quit = 0;
// 紀錄 PID 的檔案路徑.
char *mcm_daemon_pid_path = NULL;




void mcm_signal_handle(int sig_code)
{
}

int mcm_daemon_init(
    void)
{
    int fret, cret;
    struct sigaction sig_action;
    FILE *file_fp;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    mcm_daemon_thread_id = pthread_self();

    // 發生要求結束執行的中斷時, 主執行緒會傳送 MCM_SERVICE_SHUTDOWN_SIGNAL 給 service 部分,
    // service 部分使用異步模式處理信號, 替 service 部分註冊信號處理函式.
    memset(&sig_action, 0, sizeof(struct sigaction));
    sig_action.sa_handler = mcm_signal_handle;
    sigfillset(&sig_action.sa_mask);
    if(sigaction(MCM_SERVICE_SHUTDOWN_SIGNAL, &sig_action, NULL) == -1)
    {
        MCM_EMSG("call sigaction() fail [%s]", strerror(errno));
        fret = MCM_RCODE_DAEMON_INTERNAL_ERROR;
        goto FREE_01;
    }

    // 忽略 SIGPIPE 信號.
    memset(&sig_action, 0, sizeof(struct sigaction));
    sig_action.sa_handler = SIG_IGN;
    if(sigaction(SIGPIPE, &sig_action, NULL) == -1)
    {
        MCM_EMSG("call sigaction() fail [%s]", strerror(errno));
        fret = MCM_RCODE_DAEMON_INTERNAL_ERROR;
        goto FREE_01;
    }

    // 主執行緒使用 sigwait() 處理信號, 阻塞這些信號.
    sigemptyset(&mcm_daemon_sig_block);
    sigaddset(&mcm_daemon_sig_block, SIGINT);
    sigaddset(&mcm_daemon_sig_block, SIGQUIT);
    sigaddset(&mcm_daemon_sig_block, SIGTERM);
    // MCM_SERVICE_SHUTDOWN_SIGNAL 信號 :
    // 程式收到 SIGINT, SIGTERM, SIGQUIT 後,
    // 會先設置 mcm_daemon_quit 為 1,
    // 之後傳送 MCM_SERVICE_SHUTDOWN_SIGNAL 給 service 部分, 通知結束執行,
    // 正常流程下不應該直接對程式傳送 MCM_SERVICE_SHUTDOWN_SIGNAL.
    sigaddset(&mcm_daemon_sig_block, MCM_SERVICE_SHUTDOWN_SIGNAL);
    cret = pthread_sigmask(SIG_SETMASK, &mcm_daemon_sig_block, NULL);
    if(cret != 0)
    {
        MCM_EMSG("call pthread_sigmask() fail [%s]", strerror(cret));
        fret = MCM_RCODE_DAEMON_INTERNAL_ERROR;
        goto FREE_01;
    }

    if(sem_init(&mcm_daemon_mutex_quit, 0, 1) == -1)
    {
        MCM_EMSG("call sem_init() fail [%s]", strerror(errno));
        fret = MCM_RCODE_DAEMON_INTERNAL_ERROR;
        goto FREE_01;
    }

    if(mcm_daemon_pid_path != NULL)
    {
        file_fp = fopen(mcm_daemon_pid_path, "w");
        if(file_fp == NULL)
        {
            MCM_EMSG("call fopen(%s) fail [%s]", mcm_daemon_pid_path, strerror(errno));
            fret = MCM_RCODE_DAEMON_INTERNAL_ERROR;
            goto FREE_02;
        }
        fprintf(file_fp, "%d", getpid());
        fclose(file_fp);
    }

    return MCM_RCODE_PASS;
FREE_02:
    sem_destroy(&mcm_daemon_mutex_quit);
FREE_01:
    return fret;
}

int mcm_daemon_exit(
    void)
{
    MCM_SVDMSG("=> %s", __FUNCTION__);

    if(mcm_daemon_pid_path != NULL)
        unlink(mcm_daemon_pid_path);

    sem_destroy(&mcm_daemon_mutex_quit);

    return MCM_RCODE_PASS;
}

void mcm_daemon_set_shutdown(
    MCM_DTYPE_BOOL_TD shutdown_con)
{
    do
    {
        if(sem_wait(&mcm_daemon_mutex_quit) == -1)
        {
            MCM_EMSG("call sem_wait() fail [%s]", strerror(errno));
            if(errno == EINTR)
                continue;
        }
    }
    while(0);

    mcm_daemon_quit = shutdown_con;

    if(sem_post(&mcm_daemon_mutex_quit) == -1)
    {
        MCM_EMSG("call sem_post() fail [%s]", strerror(errno));
    }
}

void mcm_daemon_get_shutdown(
    MCM_DTYPE_BOOL_TD *shutdown_buf)
{
    do
    {
        if(sem_wait(&mcm_daemon_mutex_quit) == -1)
        {
            MCM_EMSG("call sem_wait() fail [%s]", strerror(errno));
            if(errno == EINTR)
                continue;
        }
    }
    while(0);

    *shutdown_buf = mcm_daemon_quit;

    if(sem_post(&mcm_daemon_mutex_quit) == -1)
    {
        MCM_EMSG("call sem_post() fail [%s]", strerror(errno));
    }
}

int mcm_daemon_shutdown(
    void)
{
    int cret;


    MCM_SVDMSG("=> %s", __FUNCTION__);

    MCM_DMDMSG("notify daemon[%lu]", mcm_daemon_thread_id);
    cret = pthread_kill(mcm_daemon_thread_id, SIGQUIT);
    if(cret != 0)
    {
        MCM_EMSG("call pthread_kill() fail [%s]", strerror(cret));
        return MCM_RCODE_DAEMON_INTERNAL_ERROR;
    }

    return MCM_RCODE_PASS;
}

int main(
    int argc,
    char **argv)
{
    int fret = MCM_RCODE_DAEMON_INTERNAL_ERROR, cret, sig_code;
    char opt_ch, *server_socket_path = NULL;
    MCM_DTYPE_USIZE_TD max_session_count = 0;
    char *module_path = NULL, *model_profile_path = NULL;
    MCM_DTYPE_BOOL_TD assign_max_session = 0, error_exit = 0, main_exit = 0;


    while((opt_ch = getopt(argc , argv, "t:a:l:m:d:c:s:e:p:")) != -1)
        switch(opt_ch)
        {
            case 't':
                max_session_count = MCM_DTYPE_USIZE_SB(optarg, NULL, 10);
                assign_max_session = 1;
                break;
            case 'a':
                server_socket_path = optarg;
                break;
            case 'l':
                module_path = optarg;
                break;
            case 'm':
                model_profile_path = optarg;
                break;
            case 'd':
                mcm_config_store_default_profile_path = optarg;
                break;
            case 'c':
                mcm_config_store_current_profile_path = optarg;
                break;
            case 's':
                mcm_config_store_profile_save_mode = MCM_DTYPE_LIST_SB(optarg, NULL, 10);
                break;
            case 'e':
                mcm_config_store_profile_error_handle = MCM_DTYPE_LIST_SB(optarg, NULL, 10);
                break;
            case 'p':
                mcm_daemon_pid_path = optarg;
                break;
            default:
                goto FREE_HELP;
        }

    if(assign_max_session == 0)
        max_session_count = MCM_DEFAULT_MAX_SESSION_COUNT;
    if(server_socket_path == NULL)
        server_socket_path = MCM_DEFAULT_SOCKET_PATH;
    if(module_path == NULL)
        module_path = MCM_DEFAULT_MODULE_PATH;
    if(model_profile_path == NULL)
        model_profile_path = MCM_DEFAULT_MODEL_PATH;
    if(mcm_config_store_default_profile_path == NULL)
        mcm_config_store_default_profile_path = MCM_DEFAULT_STORE_DEFAULT_PATH;
    if(mcm_config_store_current_profile_path == NULL)
        mcm_config_store_current_profile_path = MCM_DEFAULT_STORE_CURRENT_PATH;
    if(mcm_config_store_profile_save_mode == -1)
        mcm_config_store_profile_save_mode = MCM_DEFAULT_STORE_SAVE_MODE;
    if(mcm_config_store_profile_error_handle == -1)
        mcm_config_store_profile_error_handle = MCM_DEFAULT_STORE_ERROR_HANDLE;
    if(mcm_daemon_pid_path == NULL)
        mcm_daemon_pid_path = MCM_DEFAULT_PID_PATH;

    if(server_socket_path == NULL)
        goto FREE_HELP;
    if(module_path == NULL)
        goto FREE_HELP;
    if(model_profile_path == NULL)
        goto FREE_HELP;
    if(mcm_config_store_default_profile_path == NULL)
        goto FREE_HELP;
    if(mcm_config_store_current_profile_path == NULL)
        goto FREE_HELP;
    if((mcm_config_store_profile_save_mode != MCM_SSAVE_AUTO) &&
       (mcm_config_store_profile_save_mode != MCM_SSAVE_MANUAL))
    {
        goto FREE_HELP;
    }
    if((mcm_config_store_profile_error_handle < MCM_EHANDLE_INTERNAL_RESET_DEFAULT) ||
       (mcm_config_store_profile_error_handle > MCM_EHANDLE_MANUAL_HANDEL_EXTERNAL))
    {
        goto FREE_HELP;
    }

    fret = mcm_daemon_init();
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_daemon_init() fail");
        goto FREE_01;
    }

    fret = mcm_config_load_model(model_profile_path);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_load_config_model() fail");
        goto FREE_02;
    }

    fret = mcm_config_load_store();
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_load_store() fail");
        goto FREE_03;
    }

    fret = mcm_config_load_module(module_path);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_load_module() fail");
        goto FREE_04;
    }

    fret = mcm_config_store_profile_error_process(&error_exit);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_config_store_profile_error_process() fail");
        goto FREE_05;
    }
    if(error_exit != 0)
        goto FREE_05;

    fret = mcm_service_init(server_socket_path, max_session_count);
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_service_init() fail");
        goto FREE_05;
    }

    fret = mcm_action_boot_other_run();
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_action_boot_other_run() fail");
        goto FREE_06;
    }

    fret = mcm_service_run_post();
    if(fret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_service_run_post() fail");
        goto FREE_06;
    }

    while(main_exit == 0)
    {
        cret = sigwait(&mcm_daemon_sig_block, &sig_code);
        if(cret != 0)
        {
            MCM_EMSG("call sigwait() fail [%s]", strerror(cret));
            fret = MCM_RCODE_DAEMON_INTERNAL_ERROR;
            break;
        }
        MCM_DMDMSG("signal [%d]", sig_code);
        switch(sig_code)
        {
            case SIGINT:
            case SIGQUIT:
            case SIGTERM:
                MCM_DMDMSG("main shutdown");
                main_exit = 1;
                break;
        }
    }

    mcm_daemon_set_shutdown(1);
    cret = mcm_service_shutdown();
    if(cret < MCM_RCODE_PASS)
    {
        MCM_ECTMSG("call mcm_service_shutdown() fail");
        fret = MCM_RCODE_DAEMON_INTERNAL_ERROR;
        goto FREE_06;
    }

FREE_06:
    mcm_service_exit();
FREE_05:
    mcm_config_free_module();
FREE_04:
    mcm_config_free_store();
FREE_03:
    mcm_config_free_model();
FREE_02:
    mcm_daemon_exit();
FREE_01:
    return fret;
FREE_HELP:
    printf("\nmcm_daemon [-t] [-a] [-l] [-m] [-d] [-c] [-s] [-e] [-p]\n");
    printf("  -t : [-t max_session_count]\n");
    printf("       0 = unlimit, 1 ~ N = max session count\n");
    printf("       default = " MCM_DTYPE_USIZE_PF "\n", MCM_DEFAULT_MAX_SESSION_COUNT);
    printf("  -a : [-a server_socket_path]\n");
    printf("       default = " MCM_DEFAULT_SOCKET_PATH "\n");
    printf("  -l : [-l module_path]\n");
    printf("       default = " MCM_DEFAULT_MODULE_PATH "\n");
    printf("  -m : [-m model_profile_path]\n");
    printf("       default = " MCM_DEFAULT_MODEL_PATH "\n");
    printf("  -d : [-d store_default_profile_path]\n");
    printf("       default = " MCM_DEFAULT_STORE_DEFAULT_PATH "\n");
    printf("  -c : [-d store_current_profile_path]\n");
    printf("       default = " MCM_DEFAULT_STORE_CURRENT_PATH "\n");
    printf("  -s : [-s save_mode]\n");
    printf("       0 ~ 3\n");
    printf("       default = " MCM_DTYPE_LIST_PF "\n", MCM_DEFAULT_STORE_SAVE_MODE);
    printf("  -e : [-e error_handle]\n");
    printf("       0 ~ 3\n");
    printf("       default = " MCM_DTYPE_LIST_PF "\n", MCM_DEFAULT_STORE_ERROR_HANDLE);
    printf("  -p : [-p pid_path]\n");
    printf("       default = " MCM_DEFAULT_PID_PATH "\n\n");
    return fret;
}

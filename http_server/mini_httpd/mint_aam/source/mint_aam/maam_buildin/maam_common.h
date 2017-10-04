/*
Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
This file is part of the MintAAM.
Some rights reserved. See README.
*/

#ifndef _MAAM_COMMON_H_
#define _MAAM_COMMON_H_




#include <sys/stat.h>
#include <netinet/in.h>




// buildin error
#define MAAM_EMSG(msg_fmt, msg_args...) \
    printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msg_args)

// buildin error (call trace for internal maam_... serial function)
#define MAAM_ECTMODE 1
#if MAAM_ECTMODE
    #define MAAM_ECTMSG(msg_fmt, msg_args...) \
        printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msg_args)
#else
    #define MAAM_ECTMSG(msg_fmt, msg_args...)
#endif

// buildin debug
#define MAAM_BDMODE 0
#if MAAM_BDMODE
    #define MAAM_BDMSG(msg_fmt, msg_args...) \
        printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msg_args)
#else
    #define MAAM_BDMSG(msg_fmt, msg_args...)
#endif

// lib debug - user space
#define MAAM_LUDMODE 0
#if MAAM_LUDMODE
    #define MAAM_LUDMSG(msg_fmt, msg_args...) \
        printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msg_args)
#else
    #define MAAM_LUDMSG(msg_fmt, msg_args...)
#endif

// 讀寫權限.
#define MAAM_PERMISSION (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

// 共享記憶體的 key 值.
#define MAAM_SHARE_MEMORY_KEY 518276493

// 檔案互斥鎖的路徑.
#define MAAM_MUTEX_PATH "/var/run/maam_mutex"

// 允許的 session 的最大數目.
#define MAAM_MAX_SESSION 8

#define MAAM_REP_COOKIE_MAX_COUNT 3

#define MAAM_BSIZE_ACCOUNT_NAME 32
#define MAAM_BSIZE_SESSION_KEY  16
#define MAAM_BSIZE_REP_COOKIE   64
#define MAAM_BSIZE_REP_BODY     256
#define MAAM_BSIZE_MD5_HASH     33

enum MAAM_RETURN_CODE
{
    MAAM_RCODE_PASS                                  = 0,

    MAAM_RCODE_INTERNAL_ERROR                        = -2000,
    MAAM_RCODE_SESSION_TABLE_FULL                    = -2001,

    MAAM_RCODE_MISS_VERIFY_NAME                      = -2100,
    MAAM_RCODE_MISS_VERIFY_PASSWORD                  = -2101,
    MAAM_RCODE_MISS_MULTIPLE_USER_ACTIVE_RULE        = -2102,
    MAAM_RCODE_MISS_SAME_ACCOUT_MULTIPLE_ACTIVE_RULE = -2103,
    MAAM_RCODE_MISS_SESSION_KEY                      = -2104,

    MAAM_RCODE_INVALID_ACCOUNT_NAME                  = -2200,
    MAAM_RCODE_INVALID_ACCOUNT_PASSWORD              = -2201,
    MAAM_RCODE_OTHER_USER_HAS_LOGIN                  = -2202,
    MAAM_RCODE_SAME_ACCOUNT_HAS_LOGIN                = -2203,
    MAAM_RCODE_INVALID_SESSION_KEY                   = -2204,

    MAAM_RCODE_IDLE_TIMEOUT                          = -2300,

    MAAM_RCODE_JSLIB_INTERNAL_ERROR                  = -2400
};

// copy from mini_httpd.c
#if defined(AF_INET6) && defined(IN6_IS_ADDR_V4MAPPED)
#define MAAM_SUPPORT_IPV6
#define USE_IPV6
#endif

// copy from mini_httpd.c
typedef union
{
    struct sockaddr sa;
    struct sockaddr_in sa_in;
#ifdef USE_IPV6
    struct sockaddr_in6 sa_in6;
    struct sockaddr_storage sa_stor;
#endif
} usockaddr;

// 登入的帳號的資料.
struct maam_session_t
{
    // 客戶端的位址.
    usockaddr client_addr;
    // 帳號的名稱.
    char account_name[MAAM_BSIZE_ACCOUNT_NAME];
    // 帳號的權限.
    int account_permission;
    // session 的 key.
    char session_key[MAAM_BSIZE_SESSION_KEY];
    // 閒置超時時間, 多久沒活動會踢掉.
    long idle_timeout;
    // 登入的時間 (系統開機後的經過時間).
    long create_uptime;
    // 最後活動的時間 (系統開機後的經過時間).
    long last_access_uptime;
    // 內部鏈結串列使用, 紀錄此節點的編號.
    int session_index;
    // 內部鏈結串列使用, 紀錄串列上前一個節點的編號.
    int prev_session;
    // 內部鏈結串列使用, 紀錄串列上後一個節點的編號.
    int next_session;
    // 內部鏈結串列使用, 紀錄下一個沒被使用的節點的編號.
    int empty_session;
};

// 認證資料.
struct maam_auth_sys_t
{
    // 系統的 uptime 能記錄的最大值.
    long max_uptime;
    // 目前的 session 數目.
    int session_count;
    // 內部鏈結串列使用, 紀錄串列上第一個節點的編號.
    int session_head;
    // 內部鏈結串列使用, 紀錄串列上最後一個節點的編號.
    int session_tail;
    // 內部鏈結串列使用, 紀錄串列上第一個沒被使用的節點的編號.
    int session_usable;
    // 紀錄 session 的資料.
    struct maam_session_t session_info[MAAM_MAX_SESSION];
};




// 轉換網路格式的位址到字串格式.
#ifdef MAAM_SUPPORT_IPV6
#define MAAM_IP_NTOH(usockaddr_info, host_buf, host_size) \
    do                                                  \
    {                                                   \
        if(usockaddr_info.sa.sa_family == AF_INET)      \
        {                                               \
            inet_ntop(usockaddr_info.sa.sa_family,      \
                      &usockaddr_info.sa_in.sin_addr,   \
                      host_buf, host_size);             \
        }                                               \
        else                                            \
        {                                               \
            inet_ntop(usockaddr_info.sa.sa_family,      \
                      &usockaddr_info.sa_in6.sin6_addr, \
                      host_buf, host_size);             \
        }                                               \
    }                                                   \
    while(0)
#else
#define MAAM_IP_NTOH(usockaddr_info, host_buf, host_size) \
    inet_ntop(usockaddr_info.sa.sa_family,    \
              &usockaddr_info.sa_in.sin_addr, \
              host_buf, host_size)
#endif




#endif

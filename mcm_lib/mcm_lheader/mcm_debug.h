// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_DEBUG_H_
#define _MCM_DEBUG_H_




#include "mcm_type.h"
#include "mcm_control.h"
#include "../mcm_lheader/mcm_connect.h"




// daemon error
#define MCM_EMSG(msg_fmt, msg_args...) \
    printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msg_args)

// daemon error (call trace for internal mcm_... serial function)
#define MCM_ECTMODE 1
#if MCM_ECTMODE
    #define MCM_ECTMSG(msg_fmt, msg_args...) \
        printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msg_args)
#else
    #define MCM_ECTMSG(msg_fmt, msg_args...)
#endif

// daemon debug - daemon
#define MCM_DMDMODE 0
#if MCM_DMDMODE
    #define MCM_DMDMSG(msg_fmt, msg_args...) \
        printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msg_args)
#else
    #define MCM_DMDMSG(msg_fmt, msg_args...)
#endif

// daemon debug - config
#define MCM_CFDMODE 0
#if MCM_CFDMODE
    #define MCM_CFDMSG(msg_fmt, msg_args...) \
        printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msg_args)
#else
    #define MCM_CFDMSG(msg_fmt, msg_args...)
#endif

// daemon debug - service
#define MCM_SVDMODE 0
#if MCM_SVDMODE
    #define MCM_SVDMSG(msg_fmt, msg_args...) \
        printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msg_args)
#else
    #define MCM_SVDMSG(msg_fmt, msg_args...)
#endif

// daemon debug - action
#define MCM_ATDMODE 0
#if MCM_ATDMODE
    #define MCM_ATDMSG(msg_fmt, msg_args...) \
        printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msg_args)
#else
    #define MCM_ATDMSG(msg_fmt, msg_args...)
#endif

// module error
#define MCM_MEMODE 1
#if MCM_MEMODE
    #define MCM_MEMSG(msg_fmt, msg_args...) \
        printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msg_args)
#else
    #define MCM_MEMSG(msg_fmt, msg_args...)
#endif

// module debug
#define MCM_MDMODE 0
#if MCM_MDMODE
    #define MCM_MDMSG(msg_fmt, msg_args...) \
        printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msg_args)
#else
    #define MCM_MDMSG(msg_fmt, msg_args...)
#endif

// lib debug - user space
#define MCM_LUDMODE 0
#if MCM_LUDMODE
    #define MCM_LUDMSG(msg_fmt, msg_args...) \
        printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msg_args)
#else
    #define MCM_LUDMSG(msg_fmt, msg_args...)
#endif

// lib error - kernel space
#define K_FILE_NAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define MCM_KEMSG(msg_fmt, msg_args...) \
    printk(KERN_ERR "%s(%04u): " msg_fmt "\n", K_FILE_NAME, __LINE__, ##msg_args)

// lib error - kernel space (call trace for internal mcm_... serial function)
#define MCM_LKECTMODE 1
#if MCM_LKECTMODE
    #define MCM_KECTMSG(msg_fmt, msg_args...) \
        printk(KERN_ERR "%s(%04u): " msg_fmt "\n", K_FILE_NAME, __LINE__, ##msg_args)
#else
    #define MCM_KECTMSG(msg_fmt, msg_args...)
#endif

// lib debug - kernel space
#define MCM_LKDMODE 0
#if MCM_LKDMODE
    #define MCM_LKDMSG(msg_fmt, msg_args...) \
        printk(KERN_INFO "%s(%04u): " msg_fmt "\n", K_FILE_NAME, __LINE__, ##msg_args)
#else
    #define MCM_LKDMSG(msg_fmt, msg_args...)
#endif

// command debug
#define MCM_CMDMODE 0
#if MCM_CMDMODE
    #define MCM_CMDMSG(msg_fmt, msg_args...) \
        printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msg_args)
#else
    #define MCM_CMDMSG(msg_fmt, msg_args...)
#endif

// cgi error
#define MCM_CGIEMODE 1

// cgi error (call trace for internal mcm_... serial function)
#define MCM_CGIECTMODE 1

// cgi debug - config
#define MCM_CCDMODE 0

// cgi debug - upload
#define MCM_CUDMODE 0

// cgi error - config module
#define MCM_CCMEMODE 1

// cgi debug - config module
#define MCM_CCMDMODE 0

// cgi error - upload module
#define MCM_CUMEMODE 1

// cgi debug - upload module
#define MCM_CUMDMODE 0


#define MCM_DBG_CONSOLE "/dev/tty"

#define MCM_DBG_BUFFER_SIZE 1024

#define MCM_DBG_SPERMISSION(type_con) type_con == MCM_SPERMISSION_RO ? "RO" : "RW"

#define MCM_DBG_FORMAT_CONFIG_S_VALUE(dbg_buf, data_buf, data_len, tmp_idx, tmp_len) \
    do                                                                                       \
    {                                                                                        \
        for(tmp_idx = tmp_len = 0; tmp_idx < data_len; tmp_idx++)                            \
        {                                                                                    \
            tmp_len += (MCM_CSTR_MIN_PRINTABLE_KEY <= *(((char *) (data_buf)) + tmp_idx)) && \
                       (*(((char *) (data_buf)) + tmp_idx) <= MCM_CSTR_MAX_PRINTABLE_KEY) && \
                       (*(((char *) (data_buf)) + tmp_idx) != MCM_CSTR_RESERVE_KEY1) &&      \
                       (*(((char *) (data_buf)) + tmp_idx) != MCM_CSTR_RESERVE_KEY2) ?       \
                       1 : 3;                                                                \
        }                                                                                    \
        if(tmp_len >= sizeof(dbg_buf))                                                       \
        {                                                                                    \
            snprintf(dbg_buf, sizeof(dbg_buf), "<dbg_buf too small>");                       \
        }                                                                                    \
        else                                                                                 \
        {                                                                                    \
            dbg_buf[0] = '\0';                                                               \
            for(tmp_idx = tmp_len = 0; tmp_idx < data_len; tmp_idx++)                        \
            {                                                                                \
                if((MCM_CSTR_MIN_PRINTABLE_KEY <= *(((char *) (data_buf)) + tmp_idx)) &&     \
                   (*(((char *) (data_buf)) + tmp_idx) <= MCM_CSTR_MAX_PRINTABLE_KEY) &&     \
                   (*(((char *) (data_buf)) + tmp_idx) != MCM_CSTR_RESERVE_KEY1) &&          \
                   (*(((char *) (data_buf)) + tmp_idx) != MCM_CSTR_RESERVE_KEY2))            \
                {                                                                            \
                    tmp_len += snprintf(dbg_buf + tmp_len,                                   \
                                        sizeof(dbg_buf) - tmp_len, "%c",                     \
                                        *(((char *) (data_buf)) + tmp_idx));                 \
                }                                                                            \
                else                                                                         \
                {                                                                            \
                    tmp_len += snprintf(dbg_buf + tmp_len,                                   \
                                        sizeof(dbg_buf) - tmp_len, "%c%02X",                 \
                                        MCM_CSTR_SPECIAL_KEY,                                \
                                        *(((unsigned char *) (data_buf)) + tmp_idx) & 0xFF); \
                }                                                                            \
            }                                                                                \
        }                                                                                    \
    }                                                                                        \
    while(0)

#define MCM_DBG_FORMAT_CONFIG_B_VALUE(dbg_buf, data_buf, data_len, tmp_idx, tmp_len) \
    do                                                                                   \
    {                                                                                    \
        if((data_len * 2) >= sizeof(dbg_buf))                                            \
        {                                                                                \
            snprintf(dbg_buf, sizeof(dbg_buf), "<dbg_buf too small>");                   \
        }                                                                                \
        else                                                                             \
        {                                                                                \
            dbg_buf[0] = '\0';                                                           \
            for(tmp_idx = tmp_len = 0; tmp_idx < data_len; tmp_idx++)                    \
            {                                                                            \
                tmp_len += snprintf(dbg_buf + tmp_len, sizeof(dbg_buf) - tmp_len,        \
                                    MCM_DTYPE_B_PF,                                      \
                                    *(((unsigned char *) (data_buf)) + tmp_idx) & 0xFF); \
            }                                                                            \
        }                                                                                \
    }                                                                                    \
    while(0)

#define MCM_DBG_SHOW_ALONE_PATH(tmp_model, tmp_member, tmp_store, tmp_dloc, tmp_key) \
    do                                                                        \
    {                                                                         \
        if(tmp_model->group_type == MCM_DTYPE_GD_INDEX)                       \
        {                                                                     \
            tmp_dloc = tmp_store->data_value_new != NULL ?                    \
                       tmp_store->data_value_new : tmp_store->data_value_sys; \
            tmp_dloc += tmp_model->entry_key_offset_value;                    \
            tmp_key = *((MCM_DTYPE_EK_TD *) tmp_dloc);                        \
        }                                                                     \
        MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF ".%s]",                           \
                   tmp_model->group_name, MCM_SPROFILE_PATH_KEY_KEY, tmp_key, \
                   tmp_member->member_name);                                  \
    }                                                                         \
    while(0)

#define MCM_DBG_SHOW_ENTRY_PATH(tmp_model, tmp_store, tmp_dloc, tmp_key) \
    do                                                                         \
    {                                                                          \
        if(tmp_model->group_type == MCM_DTYPE_GD_INDEX)                        \
        {                                                                      \
            tmp_dloc = tmp_store->data_value_new != NULL ?                     \
                       tmp_store->data_value_new : tmp_store->data_value_sys;  \
            tmp_dloc += tmp_model->entry_key_offset_value;                     \
            tmp_key = *((MCM_DTYPE_EK_TD *) tmp_dloc);                         \
        }                                                                      \
        MCM_CFDMSG("[%s.%c" MCM_DTYPE_EK_PF "]",                               \
                   tmp_model->group_name, MCM_SPROFILE_PATH_KEY_KEY, tmp_key); \
    }                                                                          \
    while(0)

#define MCM_DBG_GET_ENTRY_KEY(tmp_model, tmp_store, tmp_dloc, tmp_key) \
    do                                                                        \
    {                                                                         \
        tmp_key = 0;                                                          \
        if(tmp_model->group_type == MCM_DTYPE_GD_INDEX)                       \
        {                                                                     \
            tmp_dloc = tmp_store->data_value_new != NULL ?                    \
                       tmp_store->data_value_new : tmp_store->data_value_sys; \
            tmp_dloc += tmp_model->entry_key_offset_value;                    \
            tmp_key = *((MCM_DTYPE_EK_TD *) tmp_dloc);                        \
        }                                                                     \
    }                                                                         \
    while(0)


#define MCM_DBG_FORMAT_STATUS_LIST(dbg_buf, msg_list, assign_type, assign_status, \
                                   tmp_status, tmp_dloc, tmp_cnt, tmp_len)        \
    do                                                                   \
    {                                                                    \
        for(tmp_cnt = 0; msg_list[tmp_cnt].status_int != 0; tmp_cnt++)   \
            if(assign_type == msg_list[tmp_cnt].status_int)              \
                break;                                                   \
        snprintf(dbg_buf, sizeof(dbg_buf), "[%s]",                       \
                 msg_list[tmp_cnt].status_str);                          \
        tmp_len = strlen(dbg_buf);                                       \
        snprintf(dbg_buf + tmp_len, sizeof(dbg_buf) - tmp_len,           \
                 "[" MCM_DTYPE_DS_PF " -> " MCM_DTYPE_DS_PF "%s",        \
                 tmp_status, *((MCM_DTYPE_DS_TD *) tmp_dloc),            \
                 assign_status == 0 ? "]" : ", ");                       \
        tmp_len = strlen(dbg_buf);                                       \
        tmp_cnt = 4;                                                     \
        for(tmp_status = assign_status; tmp_status != 0;)                \
        {                                                                \
            for(; msg_list[tmp_cnt].status_int != 0; tmp_cnt++)          \
                if(tmp_status & msg_list[tmp_cnt].status_int)            \
                    break;                                               \
            tmp_status &= ~msg_list[tmp_cnt].status_int;                 \
            snprintf(dbg_buf + tmp_len, sizeof(dbg_buf) - tmp_len, "%s", \
                     msg_list[tmp_cnt].status_str);                      \
            tmp_len = strlen(dbg_buf);                                   \
            dbg_buf[tmp_len] = tmp_status != 0 ? ',' : ']';              \
            tmp_len++;                                                   \
        }                                                                \
        dbg_buf[tmp_len] = '\0';                                         \
    }                                                                    \
    while(0)




#endif

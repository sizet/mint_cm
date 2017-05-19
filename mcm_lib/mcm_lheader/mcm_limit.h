// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_LIMIT_H_
#define _MCM_LIMIT_H_




#include <float.h>
#include "mcm_type.h"




#define MCM_CLIMIT_USIZE_MIN ((MCM_DTYPE_USIZE_TD) (0U))
#define MCM_CLIMIT_USIZE_MAX ((MCM_DTYPE_USIZE_TD) ((unsigned int) (~0U)))
typedef unsigned long int    MCM_CLIMIT_USIZE_TD;

#define MCM_CLIMIT_EK_MIN MCM_CLIMIT_USIZE_MIN
#define MCM_CLIMIT_EK_MAX MCM_CLIMIT_USIZE_MAX
typedef unsigned long int MCM_CLIMIT_EK_TD;

#if MCM_SUPPORT_DTYPE_RK
#define MCM_CLIMIT_RK_MIN MCM_CLIMIT_EK_MIN
#define MCM_CLIMIT_RK_MAX MCM_CLIMIT_EK_MAX
typedef unsigned long int MCM_CLIMIT_RK_TD;
#endif

#if MCM_SUPPORT_DTYPE_ISC
#define MCM_CLIMIT_ISC_MIN ((MCM_DTYPE_ISC_TD) (-(((unsigned char) (~0U)) >> 1) - 1))
#define MCM_CLIMIT_ISC_MAX ((MCM_DTYPE_ISC_TD) (((unsigned char) (~0U)) >> 1))
typedef long int           MCM_CLIMIT_ISC_TD;
#endif

#if MCM_SUPPORT_DTYPE_IUC
#define MCM_CLIMIT_IUC_MIN ((MCM_DTYPE_IUC_TD) (0U))
#define MCM_CLIMIT_IUC_MAX ((MCM_DTYPE_IUC_TD) ((unsigned char) (~0U)))
typedef unsigned long int  MCM_CLIMIT_IUC_TD;
#endif

#if MCM_SUPPORT_DTYPE_ISS
#define MCM_CLIMIT_ISS_MIN ((MCM_DTYPE_ISS_TD) (-(((unsigned short) (~0U)) >> 1) - 1))
#define MCM_CLIMIT_ISS_MAX ((MCM_DTYPE_ISS_TD) (((unsigned short) (~0U)) >> 1))
typedef long int           MCM_CLIMIT_ISS_TD;
#endif

#if MCM_SUPPORT_DTYPE_IUS
#define MCM_CLIMIT_IUS_MIN ((MCM_DTYPE_IUS_TD) (0U))
#define MCM_CLIMIT_IUS_MAX ((MCM_DTYPE_IUS_TD) ((unsigned short) (~0U)))
typedef unsigned long int  MCM_CLIMIT_IUS_TD;
#endif

#if MCM_SUPPORT_DTYPE_ISI
#define MCM_CLIMIT_ISI_MIN ((MCM_DTYPE_ISI_TD) (-(((unsigned int) (~0U)) >> 1) - 1))
#define MCM_CLIMIT_ISI_MAX ((MCM_DTYPE_ISI_TD) (((unsigned int) (~0U)) >> 1))
typedef long int           MCM_CLIMIT_ISI_TD;
#endif

#if MCM_SUPPORT_DTYPE_IUI
#define MCM_CLIMIT_IUI_MIN ((MCM_DTYPE_IUI_TD) (0U))
#define MCM_CLIMIT_IUI_MAX ((MCM_DTYPE_IUI_TD) ((unsigned int) (~0U)))
typedef unsigned long int  MCM_CLIMIT_IUI_TD;
#endif

#if MCM_SUPPORT_DTYPE_ISLL
#define MCM_CLIMIT_ISLL_MIN ((MCM_DTYPE_ISLL_TD) (-(((unsigned long long) (~0ULL)) >> 1) - 1))
#define MCM_CLIMIT_ISLL_MAX ((MCM_DTYPE_ISLL_TD) (((unsigned long long) (~0ULL)) >> 1))
typedef long long           MCM_CLIMIT_ISLL_TD;
#endif

#if MCM_SUPPORT_DTYPE_IULL
#define MCM_CLIMIT_IULL_MIN ((MCM_DTYPE_IULL_TD) (0ULL))
#define MCM_CLIMIT_IULL_MAX ((MCM_DTYPE_IULL_TD) ((unsigned long long) (~0ULL)))
typedef unsigned long long  MCM_CLIMIT_IULL_TD;
#endif

#if MCM_SUPPORT_DTYPE_FF
#define MCM_CLIMIT_FF_MIN FLT_MIN
#define MCM_CLIMIT_FF_MAX FLT_MAX
typedef float             MCM_CLIMIT_FF_TD;
#endif

#if MCM_SUPPORT_DTYPE_FD
#define MCM_CLIMIT_FD_MIN DBL_MIN
#define MCM_CLIMIT_FD_MAX DBL_MAX
typedef double            MCM_CLIMIT_FD_TD;
#endif

#if MCM_SUPPORT_DTYPE_FLD
#define MCM_CLIMIT_FLD_MIN LDBL_MIN
#define MCM_CLIMIT_FLD_MAX LDBL_MAX
typedef long double        MCM_CLIMIT_FLD_TD;
#endif

#define MCM_CHECK_INT_RANGE_S(stob_api, limit_min, limit_max,           \
                              tmp_data, tmp_tail, tmp_error, tmp_value) \
    do                                                                   \
    {                                                                    \
        tmp_error = 1;                                                   \
        errno = 0;                                                       \
        tmp_value = stob_api(tmp_data, &tmp_tail, 10);                   \
        if(*tmp_tail == '\0')                                            \
            if(errno == 0)                                               \
                if((limit_min <= tmp_value) && (tmp_value <= limit_max)) \
                    tmp_error = 0;                                       \
    }                                                                    \
    while(0)

#define MCM_CHECK_INT_RANGE_U(stob_api, limit_min, limit_max,           \
                              tmp_data, tmp_tail, tmp_error, tmp_value) \
    do                                                     \
    {                                                      \
        tmp_error = 1;                                     \
        errno = 0;                                         \
        if(tmp_data[0] != '-')                             \
        {                                                  \
            tmp_value = stob_api(tmp_data, &tmp_tail, 10); \
            if(*tmp_tail == '\0')                          \
                if(errno == 0)                             \
                    if(tmp_value <= limit_max)             \
                        tmp_error = 0;                     \
        }                                                  \
    }                                                      \
    while(0)

#define MCM_CHECK_FLO_RANGE(stob_api, tmp_data, tmp_tail, tmp_error, tmp_value) \
    do                                             \
    {                                              \
        tmp_error = 1;                             \
        errno = 0;                                 \
        tmp_value = stob_api(tmp_data, &tmp_tail); \
        if(*tmp_tail == '\0')                      \
            if(errno == 0)                         \
                tmp_error = 0;                     \
    }                                              \
    while(0)

#define MCM_CLIMIT_USIZE_API(tmp_data, tmp_tail, tmp_error, tmp_value) \
    MCM_CHECK_INT_RANGE_U(MCM_DTYPE_USIZE_SB, MCM_CLIMIT_USIZE_MIN, MCM_CLIMIT_USIZE_MAX, \
                          tmp_data, tmp_tail, tmp_error, tmp_value)

#define MCM_CLIMIT_EK_API(tmp_data, tmp_tail, tmp_error, tmp_value) \
    MCM_CHECK_INT_RANGE_U(MCM_DTYPE_EK_SB, MCM_CLIMIT_EK_MIN, MCM_CLIMIT_EK_MAX, \
                          tmp_data, tmp_tail, tmp_error, tmp_value)

#if MCM_SUPPORT_DTYPE_RK
#define MCM_CLIMIT_RK_API(tmp_data, tmp_tail, tmp_error, tmp_value) \
    MCM_CHECK_INT_RANGE_U(MCM_DTYPE_RK_SB, MCM_CLIMIT_RK_MIN, MCM_CLIMIT_RK_MAX, \
                          tmp_data, tmp_tail, tmp_error, tmp_value)
#endif

#if MCM_SUPPORT_DTYPE_ISC
#define MCM_CLIMIT_ISC_API(tmp_data, tmp_tail, tmp_error, tmp_value) \
    MCM_CHECK_INT_RANGE_S(MCM_DTYPE_ISC_SB, MCM_CLIMIT_ISC_MIN, MCM_CLIMIT_ISC_MAX, \
                          tmp_data, tmp_tail, tmp_error, tmp_value)
#endif

#if MCM_SUPPORT_DTYPE_IUC
#define MCM_CLIMIT_IUC_API(tmp_data, tmp_tail, tmp_error, tmp_value) \
    MCM_CHECK_INT_RANGE_U(MCM_DTYPE_IUC_SB, MCM_CLIMIT_IUC_MIN, MCM_CLIMIT_IUC_MAX, \
                          tmp_data, tmp_tail, tmp_error, tmp_value)
#endif

#if MCM_SUPPORT_DTYPE_ISS
#define MCM_CLIMIT_ISS_API(tmp_data, tmp_tail, tmp_error, tmp_value) \
    MCM_CHECK_INT_RANGE_S(MCM_DTYPE_ISS_SB, MCM_CLIMIT_ISS_MIN, MCM_CLIMIT_ISS_MAX, \
                          tmp_data, tmp_tail, tmp_error, tmp_value)
#endif

#if MCM_SUPPORT_DTYPE_IUS
#define MCM_CLIMIT_IUS_API(tmp_data, tmp_tail, tmp_error, tmp_value) \
    MCM_CHECK_INT_RANGE_U(MCM_DTYPE_IUS_SB, MCM_CLIMIT_IUS_MIN, MCM_CLIMIT_IUS_MAX, \
                          tmp_data, tmp_tail, tmp_error, tmp_value)
#endif

#if MCM_SUPPORT_DTYPE_ISI
#define MCM_CLIMIT_ISI_API(tmp_data, tmp_tail, tmp_error, tmp_value) \
    MCM_CHECK_INT_RANGE_S(MCM_DTYPE_ISI_SB, MCM_CLIMIT_ISI_MIN, MCM_CLIMIT_ISI_MAX, \
                          tmp_data, tmp_tail, tmp_error, tmp_value)
#endif

#if MCM_SUPPORT_DTYPE_IUI
#define MCM_CLIMIT_IUI_API(tmp_data, tmp_tail, tmp_error, tmp_value) \
    MCM_CHECK_INT_RANGE_U(MCM_DTYPE_IUI_SB, MCM_CLIMIT_IUI_MIN, MCM_CLIMIT_IUI_MAX, \
                          tmp_data, tmp_tail, tmp_error, tmp_value)
#endif

#if MCM_SUPPORT_DTYPE_ISLL
#define MCM_CLIMIT_ISLL_API(tmp_data, tmp_tail, tmp_error, tmp_value) \
    MCM_CHECK_INT_RANGE_S(MCM_DTYPE_ISLL_SB, MCM_CLIMIT_ISLL_MIN, MCM_CLIMIT_ISLL_MAX, \
                          tmp_data, tmp_tail, tmp_error, tmp_value)
#endif

#if MCM_SUPPORT_DTYPE_IULL
#define MCM_CLIMIT_IULL_API(tmp_data, tmp_tail, tmp_error, tmp_value) \
    MCM_CHECK_INT_RANGE_U(MCM_DTYPE_IULL_SB, MCM_CLIMIT_IULL_MIN, MCM_CLIMIT_IULL_MAX, \
                          tmp_data, tmp_tail, tmp_error, tmp_value)
#endif

#if MCM_SUPPORT_DTYPE_FF
#define MCM_CLIMIT_FF_API(tmp_data, tmp_tail, tmp_error, tmp_value) \
    MCM_CHECK_FLO_RANGE(MCM_DTYPE_FF_SB, tmp_data, tmp_tail, tmp_error, tmp_value)
#endif

#if MCM_SUPPORT_DTYPE_FD
#define MCM_CLIMIT_FD_API(tmp_data, tmp_tail, tmp_error, tmp_value) \
    MCM_CHECK_FLO_RANGE(MCM_DTYPE_FD_SB, tmp_data, tmp_tail, tmp_error, tmp_value)
#endif

#if MCM_SUPPORT_DTYPE_FLD
#define MCM_CLIMIT_FLD_API(tmp_data, tmp_tail, tmp_error, tmp_value) \
    MCM_CHECK_FLO_RANGE(MCM_DTYPE_FLD_SB, tmp_data, tmp_tail, tmp_error, tmp_value)
#endif




#endif

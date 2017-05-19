// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_TYPE_H_
#define _MCM_TYPE_H_




// TD = type define
// PF = print format
// SB = string to binary

typedef unsigned char     MCM_DTYPE_BOOL_TD;
#define MCM_DTYPE_BOOL_PF "%u"
#define MCM_DTYPE_BOOL_SB strtoul

typedef signed int        MCM_DTYPE_LIST_TD;
#define MCM_DTYPE_LIST_PF "%d"
#define MCM_DTYPE_LIST_SB strtol

typedef unsigned int           MCM_DTYPE_USIZE_TD;
#define MCM_DTYPE_USIZE_PF     "%u"
#define MCM_DTYPE_USIZE_SB     strtoul
#define MCM_DTYPE_USIZE_SUFFIX "U"

typedef unsigned int      MCM_DTYPE_FLAG_TD;
#define MCM_DTYPE_FLAG_PF "0x%X"
#define MCM_DTYPE_FLAG_SB strtoul

typedef MCM_DTYPE_FLAG_TD MCM_DTYPE_DS_TD;
#define MCM_DTYPE_DS_PF   MCM_DTYPE_FLAG_PF

#define MCM_SUPPORT_DTYPE_RK   1
#define MCM_SUPPORT_DTYPE_ISC  1
#define MCM_SUPPORT_DTYPE_IUC  1
#define MCM_SUPPORT_DTYPE_ISS  1
#define MCM_SUPPORT_DTYPE_IUS  1
#define MCM_SUPPORT_DTYPE_ISI  1
#define MCM_SUPPORT_DTYPE_IUI  1
#define MCM_SUPPORT_DTYPE_ISLL 1
#define MCM_SUPPORT_DTYPE_IULL 1
#define MCM_SUPPORT_DTYPE_FF   1
#define MCM_SUPPORT_DTYPE_FD   1
#define MCM_SUPPORT_DTYPE_FLD  1
#define MCM_SUPPORT_DTYPE_S    1
#define MCM_SUPPORT_DTYPE_B    1

enum MCM_DATA_TYPE_INDEX
{
    MCM_DTYPE_GS_INDEX = 0,
    MCM_DTYPE_GD_INDEX,
    MCM_DTYPE_EK_INDEX,
#if MCM_SUPPORT_DTYPE_RK
    MCM_DTYPE_RK_INDEX,
#endif
#if MCM_SUPPORT_DTYPE_ISC
    MCM_DTYPE_ISC_INDEX,
#endif
#if MCM_SUPPORT_DTYPE_IUC
    MCM_DTYPE_IUC_INDEX,
#endif
#if MCM_SUPPORT_DTYPE_ISS
    MCM_DTYPE_ISS_INDEX,
#endif
#if MCM_SUPPORT_DTYPE_IUS
    MCM_DTYPE_IUS_INDEX,
#endif
#if MCM_SUPPORT_DTYPE_ISI
    MCM_DTYPE_ISI_INDEX,
#endif
#if MCM_SUPPORT_DTYPE_IUI
    MCM_DTYPE_IUI_INDEX,
#endif
#if MCM_SUPPORT_DTYPE_ISLL
    MCM_DTYPE_ISLL_INDEX,
#endif
#if MCM_SUPPORT_DTYPE_IULL
    MCM_DTYPE_IULL_INDEX,
#endif
#if MCM_SUPPORT_DTYPE_FF
    MCM_DTYPE_FF_INDEX,
#endif
#if MCM_SUPPORT_DTYPE_FD
    MCM_DTYPE_FD_INDEX,
#endif
#if MCM_SUPPORT_DTYPE_FLD
    MCM_DTYPE_FLD_INDEX,
#endif
#if MCM_SUPPORT_DTYPE_S
    MCM_DTYPE_S_INDEX,
#endif
#if MCM_SUPPORT_DTYPE_B
    MCM_DTYPE_B_INDEX,
#endif
    MCM_DTYPE_MAX_INDEX
    // 最大值為 max(MCM_DTYPE_SDTYPE_TD).
};

#define MCM_DTYPE_GS_KEY       "gs"

#define MCM_DTYPE_GD_KEY       "gd"

#define MCM_DTYPE_EK_KEY       "ek"
typedef MCM_DTYPE_USIZE_TD     MCM_DTYPE_EK_TD;
#define MCM_DTYPE_EK_PF        MCM_DTYPE_USIZE_PF
#define MCM_DTYPE_EK_SB        MCM_DTYPE_USIZE_SB

#if MCM_SUPPORT_DTYPE_RK
#define MCM_DTYPE_RK_KEY       "rk"
typedef MCM_DTYPE_EK_TD        MCM_DTYPE_RK_TD;
#define MCM_DTYPE_RK_PF        MCM_DTYPE_EK_PF
#define MCM_DTYPE_RK_SB        MCM_DTYPE_EK_SB
#endif

#if MCM_SUPPORT_DTYPE_ISC
#define MCM_DTYPE_ISC_KEY      "isc"
typedef signed char            MCM_DTYPE_ISC_TD;
#define MCM_DTYPE_ISC_PF       "%d"
#define MCM_DTYPE_ISC_SB       strtol
#endif

#if MCM_SUPPORT_DTYPE_IUC
#define MCM_DTYPE_IUC_KEY      "iuc"
typedef unsigned char          MCM_DTYPE_IUC_TD;
#define MCM_DTYPE_IUC_PF       "%u"
#define MCM_DTYPE_IUC_SB       strtoul
#endif

#if MCM_SUPPORT_DTYPE_ISS
#define MCM_DTYPE_ISS_KEY      "iss"
typedef signed short int       MCM_DTYPE_ISS_TD;
#define MCM_DTYPE_ISS_PF       "%d"
#define MCM_DTYPE_ISS_SB       strtol
#endif

#if MCM_SUPPORT_DTYPE_IUS
#define MCM_DTYPE_IUS_KEY      "ius"
typedef unsigned short int     MCM_DTYPE_IUS_TD;
#define MCM_DTYPE_IUS_PF       "%u"
#define MCM_DTYPE_IUS_SB       strtoul
#endif

#if MCM_SUPPORT_DTYPE_ISI
#define MCM_DTYPE_ISI_KEY      "isi"
typedef signed int             MCM_DTYPE_ISI_TD;
#define MCM_DTYPE_ISI_PF       "%d"
#define MCM_DTYPE_ISI_SB       strtol
#endif

#if MCM_SUPPORT_DTYPE_IUI
#define MCM_DTYPE_IUI_KEY      "iui"
typedef unsigned int           MCM_DTYPE_IUI_TD;
#define MCM_DTYPE_IUI_PF       "%u"
#define MCM_DTYPE_IUI_SB       strtoul
#endif

#if MCM_SUPPORT_DTYPE_ISLL
#define MCM_DTYPE_ISLL_KEY     "isll"
typedef signed long long int   MCM_DTYPE_ISLL_TD;
#define MCM_DTYPE_ISLL_PF      "%lld"
#define MCM_DTYPE_ISLL_SB      strtoll
#endif

#if MCM_SUPPORT_DTYPE_IULL
#define MCM_DTYPE_IULL_KEY     "iull"
typedef unsigned long long int MCM_DTYPE_IULL_TD;
#define MCM_DTYPE_IULL_PF      "%llu"
#define MCM_DTYPE_IULL_SB      strtoull
#endif

#if MCM_SUPPORT_DTYPE_FF
#define MCM_DTYPE_FF_KEY       "ff"
typedef float                  MCM_DTYPE_FF_TD;
#define MCM_DTYPE_FF_PF        "%.3f"
#define MCM_DTYPE_FF_SB        strtof
#endif

#if MCM_SUPPORT_DTYPE_FD
#define MCM_DTYPE_FD_KEY       "fd"
typedef double                 MCM_DTYPE_FD_TD;
#define MCM_DTYPE_FD_PF        "%.6lf"
#define MCM_DTYPE_FD_SB        strtod
#endif

#if MCM_SUPPORT_DTYPE_FLD
#define MCM_DTYPE_FLD_KEY      "fld"
typedef long double            MCM_DTYPE_FLD_TD;
#define MCM_DTYPE_FLD_PF       "%.9Lf"
#define MCM_DTYPE_FLD_SB       strtold
#endif

#if MCM_SUPPORT_DTYPE_S
#define MCM_DTYPE_S_KEY        "s"
typedef char                   MCM_DTYPE_S_TD;
#define MCM_DTYPE_S_PF         "%s"
#endif

#if MCM_SUPPORT_DTYPE_B
#define MCM_DTYPE_B_KEY        "b"
typedef unsigned char          MCM_DTYPE_B_TD;
#define MCM_DTYPE_B_PF         "%02X"
#endif

// 資料類型的大小不可超過 MCM_DTYPE_LIST_TD.
typedef unsigned char      MCM_DTYPE_SDTYPE_TD;
// 資料類型的大小不可超過 MCM_DTYPE_USIZE_TD.
typedef unsigned char      MCM_DTYPE_SNLEN_TD;
#define MCM_DTYPE_SNLEN_PF "%u"




#endif

// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_CONNECT_H_
#define _MCM_CONNECT_H_




#include "mcm_type.h"




enum MCM_CALL_FROM
{
    MCM_CFROM_BOOT = 0,
    MCM_CFROM_USER,
    MCM_CFROM_KERNEL,
    MCM_CFROM_WEB,
};

enum MCM_SESSION_PERMISSION
{
    MCM_SPERMISSION_RO = 0,
    MCM_SPERMISSION_RW
};




struct mcm_connect_option_t
{
    MCM_DTYPE_LIST_TD call_from;
    MCM_DTYPE_LIST_TD session_permission;
    MCM_DTYPE_USIZE_TD session_stack_size;
};




#endif

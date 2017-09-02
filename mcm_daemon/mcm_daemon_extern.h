// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_DAEMON_EXTERN_H_
#define _MCM_DAEMON_EXTERN_H_




#include "mcm_daemon_define.h"




void mcm_daemon_set_shutdown(
    MCM_DTYPE_BOOL_TD shutdown_con);

void mcm_daemon_get_shutdown(
    MCM_DTYPE_BOOL_TD *shutdown_buf);

int mcm_daemon_shutdown(
    void);




#endif

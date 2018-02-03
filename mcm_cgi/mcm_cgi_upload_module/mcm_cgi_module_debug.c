// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include "mcm_cgi_module_debug.h"




#if MCM_CUMEMODE | MCM_CUMDMODE
int dbg_console_fd;
char dbg_msg_buf[MCM_DBG_BUFFER_SIZE];
#endif

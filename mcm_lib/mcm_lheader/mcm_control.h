// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_CONTROL_H_
#define _MCM_CONTROL_H_




enum MCM_FILE_SOURCE
{
    MCM_FSOURCE_DEFAULT = 0,
    MCM_FSOURCE_CURRENT,
    MCM_FSOURCE_VERIFY
};

enum MCM_STORE_SAVE_MODE
{
    MCM_SSAVE_AUTO = 0,
    MCM_SSAVE_MANUAL
};

enum MCM_ERROR_HANDLE
{
    MCM_EHANDLE_INTERNAL_RESET_DEFAULT = 0,
    MCM_EHANDLE_EXTERNAL_RESET_DEFAULT,
    MCM_EHANDLE_MANUAL_HANDEL_INTERNAL,
    MCM_EHANDLE_MANUAL_HANDEL_EXTERNAL
};

enum MCM_PATH_LIMIT
{
    MCM_PLIMIT_INDEX = 0x1,
    MCM_PLIMIT_KEY   = 0x2,
    MCM_PLIMIT_BOTH  = (MCM_PLIMIT_INDEX | MCM_PLIMIT_KEY)
};

enum MCM_DATA_ACCESS_METHOD
{
    MCM_DACCESS_SYS  = 0x1,
    MCM_DACCESS_NEW  = 0x2,
    MCM_DACCESS_AUTO = (MCM_DACCESS_SYS | MCM_DACCESS_NEW)
};

enum MCM_DATA_STATUS_ASSIGN_METHOD
{
     MCM_DSASSIGN_SET = 0,
     MCM_DSASSIGN_ADD,
     MCM_DSASSIGN_DEL
};

enum MCM_DATA_STATUS_CHANGE_FLAG
{
    MCM_DSCHANGE_NONE = 0x00000000,
    MCM_DSCHANGE_SET  = 0x00000001,
    MCM_DSCHANGE_ADD  = 0x00000002,
    MCM_DSCHANGE_DEL  = 0x00000004,
    MCM_DSCHANGE_MASK = (MCM_DSCHANGE_SET |
                         MCM_DSCHANGE_ADD |
                         MCM_DSCHANGE_DEL),
};

enum MCM_DATA_STATUS_ERROR_FLAG
{
    MCM_DSERROR_NONE              = 0x00000000,
    MCM_DSERROR_LOSE_PARENT       = 0x00000010,
    MCM_DSERROR_LOSE_ENTRY        = 0x00000020,
    MCM_DSERROR_DUPLIC_ENTRY      = 0x00000040,
    MCM_DSERROR_UNKNOWN_PARAMETER = 0x00000080,
    MCM_DSERROR_UNKNOWN_MEMBER    = 0x00000100,
    MCM_DSERROR_LOSE_MEMBER       = 0x00000200,
    MCM_DSERROR_DUPLIC_MEMBER     = 0x00000400,
    MCM_DSERROR_INVALID_VALUE     = 0x00000800,
    MCM_DSERROR_MASK              = (MCM_DSERROR_LOSE_PARENT |
                                     MCM_DSERROR_LOSE_ENTRY |
                                     MCM_DSERROR_DUPLIC_ENTRY |
                                     MCM_DSERROR_UNKNOWN_PARAMETER |
                                     MCM_DSERROR_UNKNOWN_MEMBER |
                                     MCM_DSERROR_LOSE_MEMBER |
                                     MCM_DSERROR_DUPLIC_MEMBER |
                                     MCM_DSERROR_INVALID_VALUE)
};

enum MCM_DATA_UPDATE_METHOD
{
    MCM_DUPDATE_SYNC = 0,
    MCM_DUPDATE_DROP
};




#endif

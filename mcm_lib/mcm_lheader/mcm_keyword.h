// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#ifndef _MCM_KEYWORD_H_
#define _MCM_KEYWORD_H_




#define MCM_CSTR_MIN_PRINTABLE_KEY 0x20
#define MCM_CSTR_MAX_PRINTABLE_KEY 0x7E
#define MCM_CSTR_RESERVE_KEY1      ' '
#define MCM_CSTR_RESERVE_KEY2      '%'
#define MCM_CSTR_SPECIAL_KEY       MCM_CSTR_RESERVE_KEY2

#define MCM_MPROFILE_GROUP_KEY  'G'
#define MCM_MPROFILE_MEMBER_KEY 'M'
#define MCM_MPROFILE_END_KEY    '-'

#define MCM_SPROFILE_COMMENT_KEY         ';'
#define MCM_SPROFILE_BASE_DATA_KEY       '$'
#define MCM_SPROFILE_BASE_VERSION_KEY    "version"
#define MCM_SPROFILE_PATH_SPLIT_KEY      '.'
#define MCM_SPROFILE_PATH_MASK_KEY       '*'
#define MCM_SPROFILE_PATH_INDEX_KEY      '@'
#define MCM_SPROFILE_PATH_KEY_KEY        '#'
#define MCM_SPROFILE_PARAMETER_SPLIT_KEY ' '
#define MCM_SPROFILE_MEMBER_SPLIT_KEY    ':'




#endif

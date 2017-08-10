// Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
// This file is part of the MintCM.
// Some rights reserved. See README.

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../mcm_lib/mcm_lheader/mcm_type.h"
#include "../mcm_lib/mcm_lheader/mcm_keyword.h"
#include "../mcm_lib/mcm_lheader/mcm_control.h"
#include "../mcm_lib/mcm_lheader/mcm_limit.h"




#define DMSG(msg_fmt, msgs...) printf("%s(%04u): " msg_fmt "\n", __FILE__, __LINE__, ##msgs)

#define MCM_PROFILE_ERROR_PREFIX_MSG "\nprofile error, line " MCM_DTYPE_USIZE_PF ", code %s :\n"

#define WF1(msg_fmt, msg_args...) fprintf(file_fp, msg_fmt, ##msg_args)
#define WF2(file_fp, msg_fmt, msg_args...) fprintf(file_fp, msg_fmt, ##msg_args)

#define DEF_TO_STR(def_str) (#def_str)

#define READ_BUFFER_SIZE 65536
#define PATH_BUFFER_SIZE 1024

enum MCM_TAG_ATTRIBUTE_FLAG
{
    MCM_TAG_ATTRIBUTE_TYPE_FLAG    = 0x01,
    MCM_TAG_ATTRIBUTE_MAX_FLAG     = 0x02,
    MCM_TAG_ATTRIBUTE_SAVE_FLAG    = 0x04,
    MCM_TAG_ATTRIBUTE_VERSION_FLAG = 0x08,
    // group 類型標簽可以有的屬性. 
    MCM_TAG_ATTRIBUTE_GROUP  = (MCM_TAG_ATTRIBUTE_TYPE_FLAG |
                                MCM_TAG_ATTRIBUTE_MAX_FLAG |
                                MCM_TAG_ATTRIBUTE_SAVE_FLAG),
    // member 類型標簽可以有的屬性.
    MCM_TAG_ATTRIBUTE_MEMBER = (MCM_TAG_ATTRIBUTE_TYPE_FLAG)
};
#define MCM_TAG_ATTRIBUTE_TYPE_KEY    "type"
#define MCM_TAG_ATTRIBUTE_MAX_KEY     "max"
#define MCM_TAG_ATTRIBUTE_SAVE_KEY    "save"
#define MCM_TAG_ATTRIBUTE_VERSION_KEY "version"

#define MCM_TAG_XML_KEY      "<?xml"
#define MCM_TAG_XML_LEN      5
#define MCM_TAG_COMMENT1_KEY "<!--"
#define MCM_TAG_COMMENT1_LEN 4
#define MCM_TAG_COMMENT2_KEY "-->"
#define MCM_TAG_COMMENT2_LEN 3
#define MCM_TAG_BASE_KEY     "base"

#define MCM_TAG_TYPE_SEPERATE_KEY ':'

#define MCM_MCM_DAEMON_PATH               "mcm_daemon" MULTIPLE_MODEL_SUFFIX_LOWER
#define MCM_MCM_JSLIB_PATH                "mcm_lib/mcm_jslib"
#define MCM_MCM_LHEADER_PATH              "mcm_lib/mcm_lheader"
#define MCM_MCM_TYPE_FILE                 "mcm_type.h"
#define MCM_MCM_DATA_EXINFO_H_AUTO_FILE   "mcm_data_exinfo" MULTIPLE_MODEL_SUFFIX_LOWER "_auto.h"
#define MCM_MCM_DATA_ININFO_H_AUTO_FILE   "mcm_data_ininfo_auto.h"
#define MCM_MCM_DATA_INFO_C_AUTO_FILE     "mcm_data_info_auto.c"
#define MCM_MCM_JSLIB_DATA_INFO_AUTO_FILE "mcm_jslib_data_info" MULTIPLE_MODEL_SUFFIX_LOWER "_auto.js"

#define MCM_DN_DATA_EXINFO_DEFINE_NAME "_MCM_DATA_EXINFO" MULTIPLE_MODEL_SUFFIX_UPPER "_AUTO_H_"

#define MCM_DN_DATA_EXINFO_PROFILE_VERSION_NAME "MCM_PROFILE_VERSION" MULTIPLE_MODEL_SUFFIX_UPPER
#define MCM_DN_DATA_EXINFO_PATH_MAX_LEVEL       "MCM_PATH_MAX_LEVEL" MULTIPLE_MODEL_SUFFIX_UPPER
#define MCM_DN_DATA_EXINFO_PATH_MAX_LENGTH      "MCM_PATH_MAX_LENGTH" MULTIPLE_MODEL_SUFFIX_UPPER

#define MCM_DN_DATA_EXINFO_MAX_COUNT_NAME "MCM_MCOUNT_%s_MAX_COUNT"

#define MCM_DN_DATA_EXINFO_BUFFER_SIZE_NAME "MCM_BSIZE_%s"

#define MCM_SN_DATA_EXINFO_DATA_STATUS_NAME   "mcm_ds_%s_t"
#define MCM_DATA_EXINFO_DATA_STATUS_ATTRIBUTE "/*__attribute__((packed))*/"
#define MCM_SN_DATA_EXINFO_DATA_VALUE_NAME    "mcm_dv_%s_t"
#define MCM_DATA_EXINFO_DATA_VALUE_ATTRIBUTE  "/*__attribute__((packed))*/"

#define MCM_DN_DATA_ININFO_DEFINE_NAME "_MCM_DATA_ININFO_AUTO_H_"

#define MCM_SN_DATA_ININFO_CONFIG_DATA_SIZE_OFFSET_NAME             "mcm_config_data_size_offset_t"
#define MCM_MN_DATA_ININFO_CONFIG_DATA_SIZE_OFFSET_DATA_STATUS_NAME "data_status"
#define MCM_MN_DATA_ININFO_CONFIG_DATA_SIZE_OFFSET_DATA_VALUE_NAME  "data_value"
#define MCM_VN_DATA_ININFO_CONFIG_DATA_SIZE_OFFSET_LIST_NAME        "mcm_config_data_size_offset_list"

#define MCM_DN_DATA_ININFO_CONFIG_PROFILE_VERSION_NAME "MCM_CONFIG_PROFILE_VERSION"
#define MCM_DN_DATA_ININFO_CONFIG_PATH_MAX_LEVEL       "MCM_CONFIG_PATH_MAX_LEVEL"
#define MCM_DN_DATA_ININFO_CONFIG_PATH_MAX_LENGTH      "MCM_CONFIG_PATH_MAX_LENGTH"

#define MCM_VN_JSLIB_DATA_MAX_COUNT_NAME "mcm_mcount_%s_max_count"

enum MCM_TAG_CLASS
{
    // base 類型的標籤.
    MCM_TCLASS_BASE = 0,
    // node 類型的標籤.
    MCM_TCLASS_NODE
};

enum MCM_TAG_PAIR_MODE
{
    // 讀取一行, 標籤只含有開頭標籤 <...>.
    MCM_TPAIR_HEAD = 0,
    // 讀取一行, 標籤只含有結尾標籤 </...>.
    MCM_TPAIR_TAIL,
    // 讀取一行, 開頭標籤和結尾標籤都有 <...>...</...>.
    MCM_TPAIR_BOTH
};

enum MCM_TAG_ARRANGE_MODE
{
    // 開頭標籤和結尾標籤必須在同一行.
    MCM_TARRANGE_SINGLE = 0,
    // 開頭標籤和結尾標籤必須在不同行.
    MCM_TARRANGE_BLOCK
};

// 紀錄標籤的屬性名稱的對映.
struct mcm_attribute_map_t
{
    // 屬性名稱.
    char *attribute_key;
    // 屬性對映值.
    MCM_DTYPE_FLAG_TD attribute_flag;
};
struct mcm_attribute_map_t mcm_attribute_map_list[] =
{
    {MCM_TAG_ATTRIBUTE_TYPE_KEY,    MCM_TAG_ATTRIBUTE_TYPE_FLAG},
    {MCM_TAG_ATTRIBUTE_MAX_KEY,     MCM_TAG_ATTRIBUTE_MAX_FLAG},
    {MCM_TAG_ATTRIBUTE_SAVE_KEY,    MCM_TAG_ATTRIBUTE_SAVE_FLAG},
    {MCM_TAG_ATTRIBUTE_VERSION_KEY, MCM_TAG_ATTRIBUTE_VERSION_FLAG},
    {NULL, 0}
};

// 紀錄各種類型的標籤擁有的資料.
struct mcm_tag_profile_t
{
    // 標籤的 type 屬性的名稱.
    char *type_key;
    // 標籤必須的排列方式 (MCM_TAG_ARRANGE_MODE).
    MCM_DTYPE_LIST_TD tag_arrange;
    // 標籤允許的屬性.
    MCM_DTYPE_FLAG_TD allow_attribute;
    // 標籤的 type 屬性是否是含有其他資料 (ex: type="s:16").
    MCM_DTYPE_BOOL_TD type_separate;
    // 標籤的 type 屬性的 index.
    MCM_DTYPE_LIST_TD type_index;
    // 標籤的 type 屬性的儲存大小.
    MCM_DTYPE_USIZE_TD type_size;
    // 標籤的 type 屬性的字串名稱.
    char *type_typedef;
};
struct mcm_tag_profile_t mcm_tag_profile_base_list[] =
{
    {NULL, MCM_TARRANGE_BLOCK, MCM_TAG_ATTRIBUTE_VERSION_FLAG, 0, 0, 0, NULL}
};
struct mcm_tag_profile_t mcm_tag_profile_node_list[] =
{
    {MCM_DTYPE_GS_KEY,   MCM_TARRANGE_BLOCK,  MCM_TAG_ATTRIBUTE_GROUP,  0, MCM_DTYPE_GS_INDEX,   0,                         NULL},
    {MCM_DTYPE_GD_KEY,   MCM_TARRANGE_BLOCK,  MCM_TAG_ATTRIBUTE_GROUP,  0, MCM_DTYPE_GD_INDEX,   0,                         NULL},
    {MCM_DTYPE_EK_KEY,   MCM_TARRANGE_SINGLE, MCM_TAG_ATTRIBUTE_MEMBER, 0, MCM_DTYPE_EK_INDEX,   sizeof(MCM_DTYPE_EK_TD),   DEF_TO_STR(MCM_DTYPE_EK_TD)},
#if MCM_SUPPORT_DTYPE_RK
    {MCM_DTYPE_RK_KEY,   MCM_TARRANGE_SINGLE, MCM_TAG_ATTRIBUTE_MEMBER, 0, MCM_DTYPE_RK_INDEX,   sizeof(MCM_DTYPE_RK_TD),   DEF_TO_STR(MCM_DTYPE_RK_TD)},
#endif
#if MCM_SUPPORT_DTYPE_ISC
    {MCM_DTYPE_ISC_KEY,  MCM_TARRANGE_SINGLE, MCM_TAG_ATTRIBUTE_MEMBER, 0, MCM_DTYPE_ISC_INDEX,  sizeof(MCM_DTYPE_ISC_TD),  DEF_TO_STR(MCM_DTYPE_ISC_TD)},
#endif
#if MCM_SUPPORT_DTYPE_IUC
    {MCM_DTYPE_IUC_KEY,  MCM_TARRANGE_SINGLE, MCM_TAG_ATTRIBUTE_MEMBER, 0, MCM_DTYPE_IUC_INDEX,  sizeof(MCM_DTYPE_IUC_TD),  DEF_TO_STR(MCM_DTYPE_IUC_TD)},
#endif
#if MCM_SUPPORT_DTYPE_ISS
    {MCM_DTYPE_ISS_KEY,  MCM_TARRANGE_SINGLE, MCM_TAG_ATTRIBUTE_MEMBER, 0, MCM_DTYPE_ISS_INDEX,  sizeof(MCM_DTYPE_ISS_TD),  DEF_TO_STR(MCM_DTYPE_ISS_TD)},
#endif
#if MCM_SUPPORT_DTYPE_IUS
    {MCM_DTYPE_IUS_KEY,  MCM_TARRANGE_SINGLE, MCM_TAG_ATTRIBUTE_MEMBER, 0, MCM_DTYPE_IUS_INDEX,  sizeof(MCM_DTYPE_IUS_TD),  DEF_TO_STR(MCM_DTYPE_IUS_TD)},
#endif
#if MCM_SUPPORT_DTYPE_ISI
    {MCM_DTYPE_ISI_KEY,  MCM_TARRANGE_SINGLE, MCM_TAG_ATTRIBUTE_MEMBER, 0, MCM_DTYPE_ISI_INDEX,  sizeof(MCM_DTYPE_ISI_TD),  DEF_TO_STR(MCM_DTYPE_ISI_TD)},
#endif
#if MCM_SUPPORT_DTYPE_IUI
    {MCM_DTYPE_IUI_KEY,  MCM_TARRANGE_SINGLE, MCM_TAG_ATTRIBUTE_MEMBER, 0, MCM_DTYPE_IUI_INDEX,  sizeof(MCM_DTYPE_IUI_TD),  DEF_TO_STR(MCM_DTYPE_IUI_TD)},
#endif
#if MCM_SUPPORT_DTYPE_ISLL
    {MCM_DTYPE_ISLL_KEY, MCM_TARRANGE_SINGLE, MCM_TAG_ATTRIBUTE_MEMBER, 0, MCM_DTYPE_ISLL_INDEX, sizeof(MCM_DTYPE_ISLL_TD), DEF_TO_STR(MCM_DTYPE_ISLL_TD)},
#endif
#if MCM_SUPPORT_DTYPE_IULL
    {MCM_DTYPE_IULL_KEY, MCM_TARRANGE_SINGLE, MCM_TAG_ATTRIBUTE_MEMBER, 0, MCM_DTYPE_IULL_INDEX, sizeof(MCM_DTYPE_IULL_TD), DEF_TO_STR(MCM_DTYPE_IULL_TD)},
#endif
#if MCM_SUPPORT_DTYPE_FF
    {MCM_DTYPE_FF_KEY,   MCM_TARRANGE_SINGLE, MCM_TAG_ATTRIBUTE_MEMBER, 0, MCM_DTYPE_FF_INDEX,   sizeof(MCM_DTYPE_FF_TD),   DEF_TO_STR(MCM_DTYPE_FF_TD)},
#endif
#if MCM_SUPPORT_DTYPE_FD
    {MCM_DTYPE_FD_KEY,   MCM_TARRANGE_SINGLE, MCM_TAG_ATTRIBUTE_MEMBER, 0, MCM_DTYPE_FD_INDEX,   sizeof(MCM_DTYPE_FD_TD),   DEF_TO_STR(MCM_DTYPE_FD_TD)},
#endif
#if MCM_SUPPORT_DTYPE_FLD
    {MCM_DTYPE_FLD_KEY,  MCM_TARRANGE_SINGLE, MCM_TAG_ATTRIBUTE_MEMBER, 0, MCM_DTYPE_FLD_INDEX,  sizeof(MCM_DTYPE_FLD_TD),  DEF_TO_STR(MCM_DTYPE_FLD_TD)},
#endif
#if MCM_SUPPORT_DTYPE_S
    {MCM_DTYPE_S_KEY,    MCM_TARRANGE_SINGLE, MCM_TAG_ATTRIBUTE_MEMBER, 1, MCM_DTYPE_S_INDEX,    0,                         DEF_TO_STR(MCM_DTYPE_S_TD)},
#endif
#if MCM_SUPPORT_DTYPE_B
    {MCM_DTYPE_B_KEY,    MCM_TARRANGE_SINGLE, MCM_TAG_ATTRIBUTE_MEMBER, 1, MCM_DTYPE_B_INDEX,    0,                         DEF_TO_STR(MCM_DTYPE_B_TD)},
#endif
    {NULL, 0, 0, 0, 0, 0, NULL}
};

// 紀錄讀取到的標籤的資料.
struct mcm_tag_info_t
{
    // 標籤在檔案的第幾行.
    MCM_DTYPE_USIZE_TD file_line;
    // 標籤的種類 (MCM_TAG_CLASS).
    MCM_DTYPE_LIST_TD tag_class;
    // 標籤的名稱.
    char *name_con;
    // 標籤的名稱的長度.
    MCM_DTYPE_USIZE_TD name_len;
    // 標籤要套用哪種設定 (mcm_tag_profile_node_list).
    MCM_DTYPE_USIZE_TD profile_index;
    // 標籤擁有的屬性.
    MCM_DTYPE_FLAG_TD own_attribute;
    // 標籤的 type 屬性的 index 值 (mcm_tag_profile_node_list.type_index).
    MCM_DTYPE_LIST_TD attribute_type_index;
    // 標籤的 type 屬性儲存大小. (mcm_tag_profile_node_list.type_size ).
    MCM_DTYPE_USIZE_TD attribute_type_size;
    // 標籤的 max 屬性的值.
    MCM_DTYPE_USIZE_TD attribute_max;
    // 標籤的 save 屬性的值.
    MCM_DTYPE_BOOL_TD attribute_save;
    // 標籤的 version 屬性的值.
    char *attribute_version;
    // 標籤的資料的值.
    char *default_con;
    // 標籤的資料的值的長度.
    MCM_DTYPE_USIZE_TD default_len;
    // 標籤的成對與否 (MCM_TAG_PAIR_MODE).
    MCM_DTYPE_LIST_TD tag_pair;
};

// 紀錄 XML 中 base 類型的資料.
struct mcm_xml_base_t
{
    // 版本資料 (mcm_tag_info_t.attribute_version).
    char *base_version;
};

// 紀錄 XML 中 node 類型的資料.
struct mcm_xml_node_t
{
    // node 名稱 (mcm_tag_info_t.name_con).
    char *node_name;
    // node 類型 (mcm_tag_info_t.attribute_type_index).
    MCM_DTYPE_LIST_TD node_type;
    // node 的 type 的儲存大小 (mcm_tag_info_t.attribute_type_size).
    MCM_DTYPE_USIZE_TD node_size;
    // node 的 max 值 (mcm_tag_info_t.attribute_max).
    MCM_DTYPE_USIZE_TD node_max;
    // node 的 save 值 (mcm_tag_info_t.attribute_save).
    MCM_DTYPE_BOOL_TD node_save;
    // node 的預設值 (mcm_tag_info_t.default_con).
    char *node_default;
    // 套用的設定 (mcm_tag_info_t.profile_index).
    MCM_DTYPE_USIZE_TD profile_index;
    // node 在第幾層.
    MCM_DTYPE_USIZE_TD level_index;
    // node 是 parent node 的第幾個 child node.
    MCM_DTYPE_USIZE_TD order_index;
    // 紀錄註解路徑.
    char *config_comment_path_name;
    // 紀錄 mcm_lib/mcm_lheader/mcm_data_exinfo_auto.h 中 MCM_MCOUNT_%s_MAX_COUNT 使用的路徑.
    char *config_max_entry_dname;
    // 紀錄 mcm_lib/mcm_lheader/mcm_data_exinfo_auto.h 中 MCM_BSIZE_%s 使用的路徑.
    char *config_buffer_size_dname;
    // 紀錄 mcm_lib/mcm_lheader/mcm_data_exinfo_auto.h 中 struct mcm_ds_%s 使用的路徑.
    char *config_data_status_sname;
    // 紀錄 mcm_lib/mcm_lheader/mcm_data_exinfo_auto.h 中 struct mcm_dv_%s 使用的路徑.
    char *config_data_value_sname;
    // 紀錄 mcm_lib/mcm_jslib/mcm_jslib_data_info_auto.js 中 mcm_mcount_%s_max_count 使用的路徑.
    char *jslib_max_entry_vname;
    // 紀錄 mcm_store_profile.txt 中填充預設值使用的路徑.
    char *store_path_name;
    // 鍊結 member node.
    struct mcm_xml_node_t *member_node;
    // 鍊結 parent node.
    struct mcm_xml_node_t *parent_node;
    // 鍊結 child node.
    struct mcm_xml_node_t *child_node;
    // 鍊結 next node.
    struct mcm_xml_node_t *next_node;
};

struct mcm_xml_info_t
{
    // 紀錄 base 資料.
    struct mcm_xml_base_t *db_base;
    // 紀錄 node 資料.
    struct mcm_xml_node_t *db_node;
    // 紀錄 node 的最大層數.
    MCM_DTYPE_USIZE_TD cv_node_max_level;
    // 紀錄 node 串接後的最大長度.
    MCM_DTYPE_USIZE_TD cv_node_max_length;
};

// 標簽名稱的最大長度.
MCM_DTYPE_SNLEN_TD max_name_len;

// 用來計算路徑中 key 或 index 部分的最大長度,
// 例如 :
// device.vap.#1234.station.@5678
// ek_len 用來計算數字部分可能的最大長度 (例如 : 4294967295),
// 注意事項 :
// mcm_build 是運作在 PC, mcm_daemon 則可以在不同平台,
// mcm_build 的 MCM_DTYPE_LEN_TD 的儲存大小必須大於或等於 mcm_daemon 的 MCM_DTYPE_EK_TD,
// 例如 :
// mcm_build 運作的 PC 是 x86 32bit, MCM_DTYPE_LEN_TD 的儲存大小是 32bit,
// mcm_daemon 運作在 ARM 64bit, MCM_DTYPE_EK_TD 的儲存大小是 64bit,
// 則必須調整 MCM_DTYPE_LEN_TD 的儲存大小 :
// typedef unsigned long long int MCM_DTYPE_LEN_TD;
typedef MCM_DTYPE_EK_TD MCM_DTYPE_LEN_TD;
MCM_DTYPE_LEN_TD ek_len;




int free_xml_profile(
    struct mcm_xml_info_t *this_xml);

int load_xml_profile(
    char *file_path,
    struct mcm_xml_info_t *xml_buf);

int calculate_node_level_length(
    struct mcm_xml_info_t *this_xml);

int fill_node_config_name(
    struct mcm_xml_info_t *this_xml);

int output_config_data_info_file(
    struct mcm_xml_info_t *this_xml);

int output_jslib_data_info_file(
    struct mcm_xml_info_t *this_xml);

int output_model_profile_file(
    char *file_path,
    struct mcm_xml_info_t *this_xml);

int output_store_profile_file(
    char *file_path,
    struct mcm_xml_info_t *this_xml);




int main(int argc, char **argv)
{
    int fret = -1;
    char opt_ch, *data_ppath = NULL, *model_ppath = NULL, *store_ppath = NULL;
    struct mcm_xml_info_t xml_info;
    MCM_DTYPE_EK_TD tmp_ek;


    while((opt_ch = getopt(argc , argv, "d:m:s:"))!= -1)
        switch(opt_ch)
        {
            case 'd':
                data_ppath = optarg;
                break;
            case 'm':
                model_ppath = optarg;
                break;
            case 's':
                store_ppath = optarg;
                break;
            default:
                goto FREE_HELP;
        }
    if(data_ppath == NULL)
        goto FREE_HELP;
    if(model_ppath == NULL)
        goto FREE_HELP;
    if(store_ppath == NULL)
        goto FREE_HELP;

    // 計算最大長度, -- = '\0'.
    memset(&max_name_len, 0xFF, sizeof(max_name_len));
    max_name_len--;

    for(ek_len = 0, tmp_ek = MCM_CLIMIT_EK_MAX; tmp_ek > 0; tmp_ek /= 10)
        ek_len++;

    memset(&xml_info, 0, sizeof(struct mcm_xml_info_t));

    if(load_xml_profile(data_ppath, &xml_info) < 0)
    {
        DMSG("call load_xml_profile() fail");
        goto FREE_01;
    }

    calculate_node_level_length(&xml_info);

    if(fill_node_config_name(&xml_info) < 0)
    {
        DMSG("call fill_node_config_name() fail");
        goto FREE_02;
    }

    if(output_config_data_info_file(&xml_info) < 0)
    {
        DMSG("call output_config_data_info_file() fail");
        goto FREE_02;
    }

    if(output_jslib_data_info_file(&xml_info) < 0)
    {
        DMSG("call output_jslib_data_info_file() fail");
        goto FREE_02;
    }

    if(output_model_profile_file(model_ppath, &xml_info) < 0)
    {
        DMSG("call output_model_profile_file() fail");
        goto FREE_02;
    }

    if(output_store_profile_file(store_ppath, &xml_info) < 0)
    {
        DMSG("call output_store_profile_file() fail");
        goto FREE_02;
    }

    fret = 0;
FREE_02:
    free_xml_profile(&xml_info);
FREE_01:
    return fret;
FREE_HELP:
    printf("\nmcm_build <-d> <-m> <-s>\n");
    printf("  -d : <-d data_ppath>, the path of the data_profile.\n");
    printf("  -m : <-m model_ppath>, the path of the model_profile.\n");
    printf("  -s : <-s store_ppath>, the path of the store_profile.\n\n");
    return fret;
}

int check_tag_symbol_format(
    struct mcm_tag_info_t *this_tag,
    char *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    MCM_DTYPE_USIZE_TD didx, tag_loc[4];


    tag_loc[0] = tag_loc[1] = tag_loc[2] = tag_loc[3] = 0;

    // 尋找開頭標籤的 "<".
    if(data_con[0] != '<')
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [symbol], not found symbol [<]",
             this_tag->file_line, "INVALID_TAG_SYMBOL_01");
        return -1;
    }
    tag_loc[0] = 0;

    // 尋找結尾標籤的 ">".
    if(data_con[data_len - 1] != '>')
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [symbol], not found symbol [>]",
             this_tag->file_line, "INVALID_TAG_SYMBOL_02");
        return -1;
    }
    tag_loc[3] = data_len - 1;

    // 尋找開頭標籤的 ">".
    for(didx = 0; didx < data_len; didx++)
        if(data_con[didx] == '>')
            break;
    tag_loc[1] = didx;

    // 尋找結尾標籤的 "<".
    for(didx = data_len - 1; didx >= 0; didx--)
        if(data_con[didx] == '<')
            break;
    tag_loc[2] = didx;

    // <...>...</...>
    // |   |   |    |
    // [0] [1] [2]  [3]

    // [0] != [2] : 表示開頭標籤和結尾標籤在同一行, 檢查是否缺少一個 ">".
    if(tag_loc[0] != tag_loc[2])
        if(tag_loc[1] == tag_loc[3])
        {
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                 "invalid tag [symbol], not found symbol [>]",
                 this_tag->file_line, "INVALID_TAG_SYMBOL_03");
            return -1;
        }

    // [1] != [3] : 表示開頭標籤和結尾標籤在同一行, 檢查是否缺少一個 "<".
    if(tag_loc[1] != tag_loc[3])
        if(tag_loc[0] == tag_loc[2])
        {
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                 "invalid tag [symbol], not found symbol [<]",
                 this_tag->file_line, "INVALID_TAG_SYMBOL_04");
            return -1;
        }

    // [0] != [2] : 表示開頭標籤和結尾標籤在同一行, 檢查是否缺少 "/".
    if(tag_loc[0] != tag_loc[2])
        if(data_con[tag_loc[2] + 1] != '/')
        {
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                 "invalid tag [symbol], not found symbol [/]",
                 this_tag->file_line, "INVALID_TAG_SYMBOL_05");
            return -1;
        }

    return 0;
}

int check_class_format(
    struct mcm_tag_info_t *this_tag,
    MCM_DTYPE_BOOL_TD find_base,
    char *data_con)
{
    if(find_base != 0)
    {
        if(strcmp(data_con, MCM_TAG_BASE_KEY) != 0)
        {
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                 "invalid tag [class], first tag must be [%s] [%s]",
                 this_tag->file_line, "INVALID_CLASS_01", MCM_TAG_BASE_KEY, data_con);
            return -1;
        }
        this_tag->tag_class = MCM_TCLASS_BASE;
    }
    else
    {
        this_tag->tag_class = MCM_TCLASS_NODE;
    }

    return 0;
}

int check_name_format1(
    struct mcm_tag_info_t *this_tag,
    char *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    MCM_DTYPE_USIZE_TD didx;


    if(data_len <= 0)
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [name], name can not be empty [%s]",
             this_tag->file_line, "INVALID_NAME_01", data_con);
        return -1;
    }
    else
    {
        if(data_len > max_name_len)
        {
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                 "invalid tag [name], length is too long "
                 "(" MCM_DTYPE_USIZE_PF "/" MCM_DTYPE_SNLEN_PF ") [%s]",
                 this_tag->file_line, "INVALID_NAME_02", data_len, max_name_len, data_con);
            return -1;
        }
    }

    if(('0' <= data_con[0]) && (data_con[0] <= '9'))
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [name], first character must be A~Z/a~z/_ [%s]",
             this_tag->file_line, "INVALID_NAME_03", data_con);
        return -1;
    }

    for(didx = 0; didx < data_len; didx++)
    {
        if(('0' <= data_con[didx]) && (data_con[didx] <= '9'))
            continue;
        if(('A' <= data_con[didx]) && (data_con[didx] <= 'Z'))
            continue;
        if(('a' <= data_con[didx]) && (data_con[didx] <= 'z'))
            continue;
        if(data_con[didx] == '_')
            continue;
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [name], character must be 0~9/A~Z/a~z/_ [%s]",
             this_tag->file_line, "INVALID_NAME_04", data_con);
        return -1;
    }

    this_tag->name_con = data_con;
    this_tag->name_len = data_len;

    return 0;
}

int check_name_format2(
    struct mcm_tag_info_t *this_tag,
    struct mcm_xml_node_t *parent_node)
{
    struct mcm_xml_node_t *each_node;
    MCM_DTYPE_BOOL_TD is_duplic = 0;


    if(parent_node != NULL)
    {
        for(each_node = parent_node->member_node; each_node != NULL;
            each_node = each_node->next_node)
        {
            if(strcmp(this_tag->name_con, each_node->node_name) == 0)
                break;
        }
        is_duplic = each_node != NULL ? 1 : 0;

        if(is_duplic == 0)
        {
            for(each_node = parent_node->child_node; each_node != NULL;
                each_node = each_node->next_node)
            {
                if(strcmp(this_tag->name_con, each_node->node_name) == 0)
                    break;
            }
            is_duplic = each_node != NULL ? 1 : 0;
        }

        if(is_duplic != 0)
        {
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                 "invalid tag [name], name can not be duplication [<%s>...</%s>]",
                 this_tag->file_line, "INVALID_NAME_05", this_tag->name_con, this_tag->name_con);
            return -1;
        }
    }

    return 0;
}

int check_type_format(
    struct mcm_tag_info_t *this_tag,
    char *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
#if MCM_SUPPORT_DTYPE_S || MCM_SUPPORT_DTYPE_B
    int tmp_error;
    char *tmp_tail;
    MCM_CLIMIT_USIZE_TD tmp_size;
#endif
    char *acon;
    MCM_DTYPE_USIZE_TD alen, pidx;
    MCM_DTYPE_BOOL_TD find_separate = 0;


    if(this_tag->tag_class == MCM_TCLASS_BASE)
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [%s], [%s] can not use this attribute [<%s>...</%s>]",
             this_tag->file_line, "INVALID_TYPE_01", MCM_TAG_ATTRIBUTE_TYPE_KEY, MCM_TAG_BASE_KEY,
             this_tag->name_con, this_tag->name_con);
        return -1;
    }

    for(acon = data_con, alen = 0; *acon != '\0'; acon++, alen++)
        if(*acon == MCM_TAG_TYPE_SEPERATE_KEY)
        {
            *acon = '\0';
            acon++;
            alen = data_len - (alen + 1);
            find_separate = 1;
            break;
        }

    for(pidx = 0; mcm_tag_profile_node_list[pidx].type_key != NULL; pidx++)
        if(strcmp(data_con, mcm_tag_profile_node_list[pidx].type_key) == 0)
            break;
    if(mcm_tag_profile_node_list[pidx].type_key == NULL)
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [%s], unknown [%s] [<%s>...</%s>]",
             this_tag->file_line, "INVALID_TYPE_02", MCM_TAG_ATTRIBUTE_TYPE_KEY, data_con,
             this_tag->name_con, this_tag->name_con);
        return -1;
    }
    if(mcm_tag_profile_node_list[pidx].type_separate != find_separate)
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [%s], [%s] can not assign [%c] [<%s>...</%s>]",
             this_tag->file_line, "INVALID_TYPE_03", MCM_TAG_ATTRIBUTE_TYPE_KEY, data_con,
             MCM_TAG_TYPE_SEPERATE_KEY, this_tag->name_con, this_tag->name_con);
        return -1;
    }

    switch(mcm_tag_profile_node_list[pidx].type_index)
    {
        case MCM_DTYPE_GS_INDEX:
        case MCM_DTYPE_GD_INDEX:
        case MCM_DTYPE_EK_INDEX:
#if MCM_SUPPORT_DTYPE_RK
        case MCM_DTYPE_RK_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISC
        case MCM_DTYPE_ISC_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUC
        case MCM_DTYPE_IUC_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISS
        case MCM_DTYPE_ISS_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUS
        case MCM_DTYPE_IUS_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISI
        case MCM_DTYPE_ISI_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUI
        case MCM_DTYPE_IUI_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISLL
        case MCM_DTYPE_ISLL_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IULL
        case MCM_DTYPE_IULL_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FF
        case MCM_DTYPE_FF_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FD
        case MCM_DTYPE_FD_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FLD
        case MCM_DTYPE_FLD_INDEX:
#endif
            this_tag->profile_index = pidx;
            this_tag->attribute_type_index = mcm_tag_profile_node_list[pidx].type_index;
            this_tag->attribute_type_size = mcm_tag_profile_node_list[pidx].type_size;
            this_tag->own_attribute |= MCM_TAG_ATTRIBUTE_TYPE_FLAG;
            break;
#if MCM_SUPPORT_DTYPE_S
        case MCM_DTYPE_S_INDEX:
            MCM_CLIMIT_USIZE_API(acon, tmp_tail, tmp_error, tmp_size);
            if(tmp_error == 0)
                if(tmp_size < 2)
                    tmp_error = 1;
            if(tmp_error != 0)
            {
                DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                     "invalid tag [%s], [%s] string buffer size must be 2~" MCM_DTYPE_USIZE_PF
                     " [<%s>...</%s>]",
                     this_tag->file_line, "INVALID_TYPE_04", MCM_TAG_ATTRIBUTE_TYPE_KEY, data_con,
                     MCM_CLIMIT_USIZE_MAX, this_tag->name_con, this_tag->name_con);
                return -1;
            }
            this_tag->profile_index = pidx;
            this_tag->attribute_type_index = mcm_tag_profile_node_list[pidx].type_index;
            this_tag->attribute_type_size = tmp_size;
            this_tag->own_attribute |= MCM_TAG_ATTRIBUTE_TYPE_FLAG;
            break;
#endif
#if MCM_SUPPORT_DTYPE_B
        case MCM_DTYPE_B_INDEX:
            MCM_CLIMIT_USIZE_API(acon, tmp_tail, tmp_error, tmp_size);
            if(tmp_error == 0)
                if(tmp_size < 1)
                    tmp_error = 1;
            if(tmp_error != 0)
            {
                DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                     "invalid tag [%s], [%s] bytes buffer size must be 1~" MCM_DTYPE_USIZE_PF
                     " [<%s>...</%s>]",
                     this_tag->file_line, "INVALID_TYPE_05", MCM_TAG_ATTRIBUTE_TYPE_KEY, data_con,
                     MCM_CLIMIT_USIZE_MAX, this_tag->name_con, this_tag->name_con);
                return -1;
            }
            this_tag->profile_index = pidx;
            this_tag->attribute_type_index = mcm_tag_profile_node_list[pidx].type_index;
            this_tag->attribute_type_size = tmp_size;
            this_tag->own_attribute |= MCM_TAG_ATTRIBUTE_TYPE_FLAG;
            break;
#endif
    }

    return 0;
}


int check_max_format(
    struct mcm_tag_info_t *this_tag,
    char *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    int tmp_error;
    char *tmp_tail;
    MCM_CLIMIT_USIZE_TD tmp_size;


    if(this_tag->tag_class == MCM_TCLASS_BASE)
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [%s], only [%s/%s] allow use this attribute [<%s>...</%s>]",
             this_tag->file_line, "INVALID_MAX_01", MCM_TAG_ATTRIBUTE_MAX_KEY, MCM_DTYPE_GS_KEY,
             MCM_DTYPE_GD_KEY, this_tag->name_con, this_tag->name_con);
        return -1;
    }
    else
    {
        if((this_tag->own_attribute & MCM_TAG_ATTRIBUTE_TYPE_FLAG) == 0)
        {
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                 "invalid tag [%s], [%s] must be find [<%s>...</%s>]",
                 this_tag->file_line, "INVALID_MAX_02", MCM_TAG_ATTRIBUTE_MAX_KEY,
                 MCM_TAG_ATTRIBUTE_TYPE_KEY, this_tag->name_con, this_tag->name_con);
            return -1;
        }
        if((this_tag->attribute_type_index != MCM_DTYPE_GS_INDEX) &&
           (this_tag->attribute_type_index != MCM_DTYPE_GD_INDEX))
        {
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                 "invalid tag [%s], only [%s/%s] allow use this attribute [<%s>...</%s>]",
                 this_tag->file_line, "INVALID_MAX_03", MCM_TAG_ATTRIBUTE_MAX_KEY,
                 MCM_DTYPE_GS_KEY, MCM_DTYPE_GD_KEY, this_tag->name_con, this_tag->name_con);
            return -1;
        }
    }

    MCM_CLIMIT_USIZE_API(data_con, tmp_tail, tmp_error, tmp_size);
    if(tmp_error != 0)
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [%s], [%s] must be 1, [%s] must be 1~" MCM_DTYPE_USIZE_PF
             " [<%s>...</%s>]",
             this_tag->file_line, "INVALID_MAX_04", MCM_TAG_ATTRIBUTE_MAX_KEY, MCM_DTYPE_GS_KEY,
             MCM_DTYPE_GD_KEY, MCM_CLIMIT_USIZE_MAX, this_tag->name_con, this_tag->name_con);
        return -1;
    }

    this_tag->attribute_max = tmp_size;
    if(this_tag->attribute_type_index == MCM_DTYPE_GS_INDEX)
    {
        if(this_tag->attribute_max != 1)
        {
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                 "invalid tag [%s], [%s] must be 1 [<%s>...</%s>]",
                 this_tag->file_line, "INVALID_MAX_05", MCM_TAG_ATTRIBUTE_MAX_KEY,
                 MCM_DTYPE_GS_KEY, this_tag->name_con, this_tag->name_con);
            return -1;
        }
    }
    else
    {
        if(this_tag->attribute_max < 1)
        {
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                 "invalid tag [%s], [%s] must be 1~" MCM_DTYPE_USIZE_PF " [<%s>...</%s>]",
                 this_tag->file_line, "INVALID_MAX_06", MCM_TAG_ATTRIBUTE_MAX_KEY,
                 MCM_DTYPE_GD_KEY, MCM_CLIMIT_USIZE_MAX, this_tag->name_con, this_tag->name_con);
            return -1;
        }
    }
    this_tag->own_attribute |= MCM_TAG_ATTRIBUTE_MAX_FLAG;

    return 0;
}

int check_save_format1(
    struct mcm_tag_info_t *this_tag,
    char *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    if(this_tag->tag_class == MCM_TCLASS_BASE)
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [%s], only [%s/%s] allow use this attribute [<%s>...</%s>]",
             this_tag->file_line, "INVALID_SAVE_01", MCM_TAG_ATTRIBUTE_SAVE_KEY, MCM_DTYPE_GS_KEY,
             MCM_DTYPE_GD_KEY, this_tag->name_con, this_tag->name_con);
        return -1;
    }
    else
    {
        if((this_tag->own_attribute & MCM_TAG_ATTRIBUTE_MAX_FLAG) == 0)
        {
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                 "invalid tag [%s], [%s] must be find [<%s>...</%s>]",
                 this_tag->file_line, "INVALID_SAVE_02", MCM_TAG_ATTRIBUTE_SAVE_KEY,
                 MCM_TAG_ATTRIBUTE_MAX_KEY, this_tag->name_con, this_tag->name_con);
            return -1;
        }
        if((this_tag->attribute_type_index != MCM_DTYPE_GS_INDEX) &&
           (this_tag->attribute_type_index != MCM_DTYPE_GD_INDEX))
        {
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                 "invalid tag [%s], only [%s/%s] allow use this attribute [<%s>...</%s>]",
                 this_tag->file_line, "INVALID_SAVE_03", MCM_TAG_ATTRIBUTE_SAVE_KEY,
                 MCM_DTYPE_GS_KEY, MCM_DTYPE_GD_KEY, this_tag->name_con, this_tag->name_con);
            return -1;
        }
    }

    if((data_len != 1) || ((data_con[0] != '0') && (data_con[0] != '1')))
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [%s], must be 0 or 1 [<%s>...</%s>]",
             this_tag->file_line, "INVALID_SAVE_04", MCM_TAG_ATTRIBUTE_SAVE_KEY,
             this_tag->name_con, this_tag->name_con);
        return -1;
    }

    this_tag->attribute_save = MCM_DTYPE_BOOL_SB(data_con, NULL, 10);

    this_tag->own_attribute |= MCM_TAG_ATTRIBUTE_SAVE_FLAG;

    return 0;
}

int check_save_format2(
    struct mcm_tag_info_t *this_tag,
    struct mcm_xml_node_t *parent_node)
{
    if(parent_node != NULL)
        if(parent_node->node_save == 0)
            if(this_tag->attribute_save != 0)
            {
                DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                     "invalid tag [%s], parent's save is 0, it need force to 0 [<%s>...</%s>]",
                     this_tag->file_line, "INVALID_SAVE_05", MCM_TAG_ATTRIBUTE_SAVE_KEY,
                     this_tag->name_con, this_tag->name_con);
                return -1;
            }

    return 0;
}

int check_version_format(
    struct mcm_tag_info_t *this_tag,
    char *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    if(this_tag->tag_class == MCM_TCLASS_NODE)
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [%s], only [%s] allow use this attribute [<%s>...</%s>]",
             this_tag->file_line, "INVALID_VERSION_01", MCM_TAG_ATTRIBUTE_VERSION_KEY,
             MCM_TAG_BASE_KEY, this_tag->name_con, this_tag->name_con);
        return -1;
    }

    this_tag->attribute_version = data_con;
    this_tag->own_attribute |= MCM_TAG_ATTRIBUTE_VERSION_FLAG;

    return 0;
}

int check_attribute_list(
    struct mcm_tag_info_t *this_tag,
    char *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    MCM_DTYPE_USIZE_TD didx, value_len;
    char *value_con;


    for(didx = 0; didx < data_len; didx++)
        if(data_con[didx] == '=')
            break;
    if(didx >= data_len)
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [attribute], not found symbol [=] [<%s>...</%s>]",
             this_tag->file_line, "INVALID_ATTRIBUTE_01", this_tag->name_con, this_tag->name_con);
        return -1;
    }
    data_con[didx] = '\0';
    value_con = data_con + (didx + 1);
    value_len = data_len - (didx + 1);

    for(didx = 0; mcm_attribute_map_list[didx].attribute_key != NULL; didx++)
        if(strcmp(data_con, mcm_attribute_map_list[didx].attribute_key) == 0)
            break;
    if(mcm_attribute_map_list[didx].attribute_key == NULL)
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [attribute], unknown attribute [%s] [<%s>...</%s>]",
             this_tag->file_line, "INVALID_ATTRIBUTE_02", data_con,
             this_tag->name_con, this_tag->name_con);
        return -1;
    }

    if(value_con[0] != '\"')
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [attribute], not found head symbol [\"] [<%s>...</%s>]",
             this_tag->file_line, "INVALID_ATTRIBUTE_03", this_tag->name_con, this_tag->name_con);
        return -1;
    }
    if(value_con[value_len - 1] != '\"')
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [attribute], not found tail symbol [\"] [<%s>...</%s>]",
             this_tag->file_line, "INVALID_ATTRIBUTE_04", this_tag->name_con, this_tag->name_con);
        return -1;
    }
    if(value_len <= 2)
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [attribute], empty value [<%s>...</%s>]",
             this_tag->file_line, "INVALID_ATTRIBUTE_05", this_tag->name_con, this_tag->name_con);
        return -1;
    }
    value_con[value_len - 1] = '\0';
    value_con++;
    value_len -= 2;

    if(mcm_attribute_map_list[didx].attribute_flag == MCM_TAG_ATTRIBUTE_TYPE_FLAG)
    {
        if(check_type_format(this_tag, value_con, value_len) < 0)
        {
            DMSG("call check_type_format() fail");
            return -1;
        }
    }
    else
    if(mcm_attribute_map_list[didx].attribute_flag == MCM_TAG_ATTRIBUTE_MAX_FLAG)
    {
        if(check_max_format(this_tag, value_con, value_len) < 0)
        {
            DMSG("call check_max_format() fail");
            return -1;
        }
    }
    else
    if(mcm_attribute_map_list[didx].attribute_flag == MCM_TAG_ATTRIBUTE_SAVE_FLAG)
    {
        if(check_save_format1(this_tag, value_con, value_len) < 0)
        {
            DMSG("call check_save_format1() fail");
            return -1;
        }
    }
    else
    if(mcm_attribute_map_list[didx].attribute_flag == MCM_TAG_ATTRIBUTE_VERSION_FLAG)
    {
        if(check_version_format(this_tag, value_con, value_len) < 0)
        {
            DMSG("call check_version_format() fail");
            return -1;
        }
    }

    return 0;
}

int check_attribute_lose(
    struct mcm_tag_info_t *this_tag)
{
    MCM_DTYPE_USIZE_TD aidx;
    MCM_DTYPE_FLAG_TD tmp_flag;


    tmp_flag = this_tag->tag_class == MCM_TCLASS_BASE ?
               mcm_tag_profile_base_list[this_tag->profile_index].allow_attribute :
               mcm_tag_profile_node_list[this_tag->profile_index].allow_attribute;

    if((tmp_flag ^= this_tag->own_attribute) != 0)
    {
        for(aidx = 0; mcm_attribute_map_list[aidx].attribute_key != NULL; aidx++)
            if((mcm_attribute_map_list[aidx].attribute_flag & tmp_flag) != 0)
            {
                DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                     "invalid tag [attribute], lose attribute [%s] [<%s>...</%s>]",
                     this_tag->file_line, "INVALID_ATTRIBUTE_05",
                     mcm_attribute_map_list[aidx].attribute_key,
                     this_tag->name_con, this_tag->name_con);
                if(mcm_attribute_map_list[aidx].attribute_flag == MCM_TAG_ATTRIBUTE_TYPE_FLAG)
                    break;
            }
        return -1;
    }

    return 0;
}

int check_arrange_format(
    struct mcm_tag_info_t *this_tag,
    MCM_DTYPE_USIZE_TD data_len)

{
    MCM_DTYPE_LIST_TD tmp_arrange;


    this_tag->tag_pair = data_len == 0 ? MCM_TPAIR_HEAD : MCM_TPAIR_BOTH;

    tmp_arrange = this_tag->tag_class == MCM_TCLASS_BASE ?
                  mcm_tag_profile_base_list[this_tag->profile_index].tag_arrange :
                  mcm_tag_profile_node_list[this_tag->profile_index].tag_arrange;

    if(tmp_arrange == MCM_TARRANGE_SINGLE)
    {
        if(this_tag->tag_pair != MCM_TPAIR_BOTH)
        {
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                 "invalid tag [arrange], must be single style [<%s>...</%s>]",
                 this_tag->file_line, "INVALID_ARRANGE_01", this_tag->name_con, this_tag->name_con);
            return -1;
        }
    }
    else
    {
        if(this_tag->tag_pair != MCM_TPAIR_HEAD)
        {
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                 "invalid tag [arrange], must be block style [<%s>...</%s>]",
                 this_tag->file_line, "INVALID_ARRANGE_02", this_tag->name_con, this_tag->name_con);
            return -1;
        }
    }

    return 0;
}

#if MCM_SUPPORT_DTYPE_S
int convert_hex_to_char(
    char *data_con,
    char *char_buf)
{
    MCM_DTYPE_USIZE_TD didx, tmp_dec, tmp_sum = 0;


    for(didx = 0; didx < 2; didx++)
    {
        MCM_CONVERT_HEX_TO_DEC(data_con[didx], tmp_dec);
        tmp_dec *= didx == 0 ? 16 : 1;
        tmp_sum += tmp_dec;
    }

    *char_buf = tmp_sum;

    return 0;
}
#endif

int check_default_integer(
    struct mcm_tag_info_t *this_tag,
    char *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
#if MCM_SUPPORT_DTYPE_RK || \
    MCM_SUPPORT_DTYPE_ISC || MCM_SUPPORT_DTYPE_IUC || \
    MCM_SUPPORT_DTYPE_ISS || MCM_SUPPORT_DTYPE_IUS || \
    MCM_SUPPORT_DTYPE_ISI || MCM_SUPPORT_DTYPE_IUI || \
    MCM_SUPPORT_DTYPE_ISLL || MCM_SUPPORT_DTYPE_IULL
    int tmp_error;
    char *tmp_tail;
#endif
#if MCM_SUPPORT_DTYPE_RK
    MCM_CLIMIT_RK_TD tmp_rk;
#endif
#if MCM_SUPPORT_DTYPE_ISC
    MCM_CLIMIT_ISC_TD tmp_isc;
#endif
#if MCM_SUPPORT_DTYPE_IUC
    MCM_CLIMIT_IUC_TD tmp_iuc;
#endif
#if MCM_SUPPORT_DTYPE_ISS
    MCM_CLIMIT_ISS_TD tmp_iss;
#endif
#if MCM_SUPPORT_DTYPE_IUS
    MCM_CLIMIT_IUS_TD tmp_ius;
#endif
#if MCM_SUPPORT_DTYPE_ISI
    MCM_CLIMIT_ISI_TD tmp_isi;
#endif
#if MCM_SUPPORT_DTYPE_IUI
    MCM_CLIMIT_IUI_TD tmp_iui;
#endif
#if MCM_SUPPORT_DTYPE_ISLL
    MCM_CLIMIT_ISLL_TD tmp_isll;
#endif
#if MCM_SUPPORT_DTYPE_IULL
    MCM_CLIMIT_IULL_TD tmp_iull;
#endif


#define CHECK_INT_VALUE(limit_api, limit_min, limit_max, type_fmt, tmp_value) \
    do                                                              \
    {                                                               \
        limit_api(data_con, tmp_tail, tmp_error, tmp_value);        \
        if(tmp_error != 0)                                          \
        {                                                           \
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG                       \
                 "invalid tag [default-integer], must be integer "  \
                 type_fmt "~" type_fmt " [<%s>...</%s>]",           \
                 this_tag->file_line, "INVALID_DEFAULT-INTEGER_02", \
                 limit_min, limit_max,                              \
                 this_tag->name_con, this_tag->name_con);           \
            return -1;                                              \
        }                                                           \
    }                                                               \
    while(0)


    if(data_len == 0)
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [default-integer], integer type can not be empty [<%s>...</%s>]",
             this_tag->file_line, "INVALID_DEFAULT-INTEGER_01",
             this_tag->name_con, this_tag->name_con);
        return -1;
    }

    switch(this_tag->attribute_type_index)
    {
        case MCM_DTYPE_EK_INDEX:
            if((data_len > 1) || (data_con[0] != '0'))
            {
                DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                     "invalid tag [default-integer], [%s] must be 0 [<%s>...</%s>]",
                     this_tag->file_line, "INVALID_DEFAULT-INTEGER_02", MCM_DTYPE_EK_KEY,
                     this_tag->name_con, this_tag->name_con);
                return -1;
            }
            break;
#if MCM_SUPPORT_DTYPE_RK
        case MCM_DTYPE_RK_INDEX:
            CHECK_INT_VALUE(MCM_CLIMIT_RK_API, MCM_CLIMIT_RK_MIN, MCM_CLIMIT_RK_MAX,
                            MCM_DTYPE_RK_PF, tmp_rk);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISC
        case MCM_DTYPE_ISC_INDEX:
            CHECK_INT_VALUE(MCM_CLIMIT_ISC_API, MCM_CLIMIT_ISC_MIN, MCM_CLIMIT_ISC_MAX,
                            MCM_DTYPE_ISC_PF, tmp_isc);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IUC
        case MCM_DTYPE_IUC_INDEX:
            CHECK_INT_VALUE(MCM_CLIMIT_IUC_API, MCM_CLIMIT_IUC_MIN, MCM_CLIMIT_IUC_MAX,
                            MCM_DTYPE_IUC_PF, tmp_iuc);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISS
        case MCM_DTYPE_ISS_INDEX:
            CHECK_INT_VALUE(MCM_CLIMIT_ISS_API, MCM_CLIMIT_ISS_MIN, MCM_CLIMIT_ISS_MAX,
                            MCM_DTYPE_ISS_PF, tmp_iss);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IUS
        case MCM_DTYPE_IUS_INDEX:
            CHECK_INT_VALUE(MCM_CLIMIT_IUS_API, MCM_CLIMIT_IUS_MIN, MCM_CLIMIT_IUS_MAX,
                            MCM_DTYPE_IUS_PF, tmp_ius);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISI
        case MCM_DTYPE_ISI_INDEX:
            CHECK_INT_VALUE(MCM_CLIMIT_ISI_API, MCM_CLIMIT_ISI_MIN, MCM_CLIMIT_ISI_MAX,
                            MCM_DTYPE_ISI_PF, tmp_isi);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IUI
        case MCM_DTYPE_IUI_INDEX:
            CHECK_INT_VALUE(MCM_CLIMIT_IUI_API, MCM_CLIMIT_IUI_MIN, MCM_CLIMIT_IUI_MAX,
                            MCM_DTYPE_IUI_PF, tmp_iui);
            break;
#endif
#if MCM_SUPPORT_DTYPE_ISLL
        case MCM_DTYPE_ISLL_INDEX:
            CHECK_INT_VALUE(MCM_CLIMIT_ISLL_API, MCM_CLIMIT_ISLL_MIN, MCM_CLIMIT_ISLL_MAX,
                            MCM_DTYPE_ISLL_PF, tmp_isll);
            break;
#endif
#if MCM_SUPPORT_DTYPE_IULL
        case MCM_DTYPE_IULL_INDEX:
            CHECK_INT_VALUE(MCM_CLIMIT_IULL_API, MCM_CLIMIT_IULL_MIN, MCM_CLIMIT_IULL_MAX,
                            MCM_DTYPE_IULL_PF, tmp_iull);
            break;
#endif
    }

    return 0;
}

#if MCM_SUPPORT_DTYPE_FF || MCM_SUPPORT_DTYPE_FD || MCM_SUPPORT_DTYPE_FLD
int check_default_float(
    struct mcm_tag_info_t *this_tag,
    char *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    int tmp_error;
    char *tmp_tail;
#if MCM_SUPPORT_DTYPE_FF
    MCM_CLIMIT_FF_TD tmp_ff;
#endif
#if MCM_SUPPORT_DTYPE_FD
    MCM_CLIMIT_FD_TD tmp_fd;
#endif
#if MCM_SUPPORT_DTYPE_FLD
    MCM_CLIMIT_FLD_TD tmp_fld;
#endif


#define CHECK_FLO_VALUE(limit_api, limit_min, limit_max, type_fmt, tmp_value) \
    do                                                            \
    {                                                             \
        limit_api(data_con, tmp_tail, tmp_error, tmp_value);      \
        tmp_value = tmp_value;                                    \
        if(tmp_error != 0)                                        \
        {                                                         \
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG                     \
                 "invalid tag [default-float], must be float "    \
                 type_fmt "~" type_fmt " [<%s>...</%s>]",         \
                 this_tag->file_line, "INVALID_DEFAULT-FLOAT_02", \
                 limit_min, limit_max,                            \
                 this_tag->name_con, this_tag->name_con);         \
            return -1;                                            \
        }                                                         \
    }                                                             \
    while(0)


    if(data_len == 0)
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [default-float], float type can not be empty [<%s>...</%s>]",
             this_tag->file_line, "INVALID_DEFAULT-FLOAT_01",
             this_tag->name_con, this_tag->name_con);
        return -1;
    }

    switch(this_tag->attribute_type_index)
    {
#if MCM_SUPPORT_DTYPE_FF
        case MCM_DTYPE_FF_INDEX:
            CHECK_FLO_VALUE(MCM_CLIMIT_FF_API, MCM_CLIMIT_FF_MIN, MCM_CLIMIT_FF_MAX,
                            MCM_DTYPE_FF_PF, tmp_ff);
            break;
#endif
#if MCM_SUPPORT_DTYPE_FD
        case MCM_DTYPE_FD_INDEX:
            CHECK_FLO_VALUE(MCM_CLIMIT_FD_API, MCM_CLIMIT_FD_MIN, MCM_CLIMIT_FD_MAX,
                            MCM_DTYPE_FD_PF, tmp_fd);
            break;
#endif
#if MCM_SUPPORT_DTYPE_FLD
        case MCM_DTYPE_FLD_INDEX:
            CHECK_FLO_VALUE(MCM_CLIMIT_FLD_API, MCM_CLIMIT_FLD_MIN, MCM_CLIMIT_FLD_MAX,
                            MCM_DTYPE_FLD_PF, tmp_fld);
            break;
#endif
    }

    return 0;
}
#endif

#if MCM_SUPPORT_DTYPE_S
int check_default_string(
    struct mcm_tag_info_t *this_tag,
    char *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    unsigned char value_mask[7] = {0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE};
    unsigned char check_mask[7] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
    MCM_DTYPE_USIZE_TD code_len[7] = {1, 0, 2, 3, 4, 5, 6};
    MCM_DTYPE_USIZE_TD didx1, didx2, dlen, midx, clen;
    char tmp_hex, *tmp_loc;


    for(didx1 = dlen = 0; didx1 < data_len; dlen++)
        if(data_con[didx1] != MCM_CSTR_SPECIAL_KEY)
        {
            if((data_con[didx1] < MCM_CSTR_MIN_PRINTABLE_KEY) ||
               (MCM_CSTR_MAX_PRINTABLE_KEY < data_con[didx1]) ||
               (data_con[didx1] == MCM_CSTR_RESERVE_KEY1))
            {
                DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                     "invalid tag [default-string], "
                     "special character (exclude 0x%X~0x%X, '%c', '%c') "
                     "must use %%XX (XX = character's hex value (01~FF)) [<%s>...</%s>]",
                     this_tag->file_line, "INVALID_DEFAULT-STRING_01", MCM_CSTR_MIN_PRINTABLE_KEY,
                     MCM_CSTR_MAX_PRINTABLE_KEY, MCM_CSTR_RESERVE_KEY1, MCM_CSTR_RESERVE_KEY2,
                     this_tag->name_con, this_tag->name_con);
                return -1;
            }
            didx1++;
        }
        else
        {
            for(didx2 = didx1 + 1; didx2 < (didx1 + 3); didx2++)
            {
                MCM_CHECK_HEX_RANGE(data_con[didx2]);
                DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                     "invalid tag [default-string], %%XX must be hex (01~FF)) "
                     "[<%s>...</%s>]",
                     this_tag->file_line, "INVALID_DEFAULT-STRING_02",
                     this_tag->name_con, this_tag->name_con);
                return -1;
            }
            convert_hex_to_char(data_con + didx1 + 1, &tmp_hex);
            if(tmp_hex == 0)
            {
                DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                     "invalid tag [default-string], %%XX must be large 0 (01~FF)) "
                     "[<%s>...</%s>]",
                     this_tag->file_line, "INVALID_DEFAULT-STRING_03",
                     this_tag->name_con, this_tag->name_con);
                return -1;
            }
            didx1 += 3;
        }

    for(didx1 = 0; didx1 < data_len;)
        if(data_con[didx1] != MCM_CSTR_SPECIAL_KEY)
        {
            didx1++;
        }
        else
        {
            convert_hex_to_char(data_con + didx1 + 1, &tmp_hex);
            if((tmp_hex & value_mask[0]) == check_mask[0])
            {
                didx1 += code_len[0] * 3;
            }
            else
            {
                for(midx = 2; midx < 7; midx++)
                    if((tmp_hex & value_mask[midx]) == check_mask[midx])
                        break;
                if(midx == 7)
                {
                    DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                         "invalid tag [default-string], invalid UTF-8 encode [<%s>...</%s>]",
                         this_tag->file_line, "INVALID_DEFAULT-STRING_04",
                         this_tag->name_con, this_tag->name_con);
                    return -1;
                }

                tmp_loc = data_con + didx1 + 3;
                clen = code_len[midx] - 1;
                for(didx2 = 0; didx2 < clen; didx2++, tmp_loc += 3)
                {
                    convert_hex_to_char(tmp_loc + 1, &tmp_hex);
                    if((tmp_hex & value_mask[1]) != check_mask[1])
                    {
                        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                             "invalid tag [default-string], invalid UTF-8 encode [<%s>...</%s>]",
                             this_tag->file_line, "INVALID_DEFAULT-STRING_05",
                             this_tag->name_con, this_tag->name_con);
                        return -1;
                    }
                }

                didx1 += code_len[midx] * 3;
            }
        }

    if(dlen >= this_tag->attribute_type_size)
    {
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [default-string], string length over buffer size "
             "(" MCM_DTYPE_USIZE_PF "/" MCM_DTYPE_USIZE_PF ") [<%s>...</%s>]",
             this_tag->file_line, "INVALID_DEFAULT-STRING_06", dlen, this_tag->attribute_type_size,
             this_tag->name_con, this_tag->name_con);
        return -1;
    }

    return 0;
}
#endif

#if MCM_SUPPORT_DTYPE_B
int check_default_bytes(
    struct mcm_tag_info_t *this_tag,
    char *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    MCM_DTYPE_USIZE_TD didx;


    for(didx = 0; didx < data_len; didx++)
    {
        MCM_CHECK_HEX_RANGE(data_con[didx]);
        DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
             "invalid tag [default-bytes], %%XX must be hex (00~FF)) [<%s>...</%s>]",
             this_tag->file_line, "INVALID_DEFAULT-BYTES_01",
             this_tag->name_con, this_tag->name_con);
        return -1;
    }

    if(didx > 0)
    {
        if((didx % 2) != 0)
        {
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                 "invalid tag [default-bytes], %%XX must be hex (00~FF)) [<%s>...</%s>]",
                 this_tag->file_line, "INVALID_DEFAULT-BYTES_02",
                 this_tag->name_con, this_tag->name_con);
            return -1;
        }

        didx /= 2;
        if(didx > this_tag->attribute_type_size)
        {
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                 "invalid tag [default-bytes], bytes length over buffer size "
                 "(" MCM_DTYPE_USIZE_PF "/" MCM_DTYPE_USIZE_PF ") [<%s>...</%s>]",
                 this_tag->file_line, "INVALID_DEFAULT-BYTES_03", didx,
                 this_tag->attribute_type_size, this_tag->name_con, this_tag->name_con);
            return -1;
        }
    }

    return 0;
}
#endif

int check_default_format(
    struct mcm_tag_info_t *this_tag,
    char *data_con,
    MCM_DTYPE_USIZE_TD data_len)
{
    switch(this_tag->attribute_type_index)
    {
        case MCM_DTYPE_GS_INDEX:
        case MCM_DTYPE_GD_INDEX:
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                 "invalid tag [default], [%s/%s] can not assign dafault [<%s>...</%s>]",
                 this_tag->file_line, "INVALID_DEFAULT_01", MCM_DTYPE_GS_KEY, MCM_DTYPE_GD_KEY,
                 this_tag->name_con, this_tag->name_con);
            return -1;
            break;
        case MCM_DTYPE_EK_INDEX:
#if MCM_SUPPORT_DTYPE_RK
        case MCM_DTYPE_RK_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISC
        case MCM_DTYPE_ISC_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUC
        case MCM_DTYPE_IUC_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISS
        case MCM_DTYPE_ISS_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUS
        case MCM_DTYPE_IUS_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISI
        case MCM_DTYPE_ISI_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUI
        case MCM_DTYPE_IUI_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISLL
        case MCM_DTYPE_ISLL_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IULL
        case MCM_DTYPE_IULL_INDEX:
#endif
            if(check_default_integer(this_tag, data_con, data_len) < 0)
            {
                DMSG("call check_default_integer() fail");
                return -1;
            }
            break;
#if MCM_SUPPORT_DTYPE_FF
        case MCM_DTYPE_FF_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FD
        case MCM_DTYPE_FD_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FLD
        case MCM_DTYPE_FLD_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FF || MCM_SUPPORT_DTYPE_FD || MCM_SUPPORT_DTYPE_FLD
            if(check_default_float(this_tag, data_con, data_len) < 0)
            {
                DMSG("call check_default_float() fail");
                return -1;
            }
            break;
#endif
#if MCM_SUPPORT_DTYPE_S
        case MCM_DTYPE_S_INDEX:
            if(check_default_string(this_tag, data_con, data_len) < 0)
            {
                DMSG("call check_default_string() fail");
                return -1;
            }
            break;
#endif
#if MCM_SUPPORT_DTYPE_B
        case MCM_DTYPE_B_INDEX:
            if(check_default_bytes(this_tag, data_con, data_len) < 0)
            {
                DMSG("call check_default_bytes() fail");
                return -1;
            }
            break;
#endif
    }

    this_tag->default_con = data_con;
    this_tag->default_len = data_len;

    return 0;
}

int parse_tag(
    MCM_DTYPE_BOOL_TD find_base,
    char *read_con,
    MCM_DTYPE_USIZE_TD *file_line_buf,
    MCM_DTYPE_LIST_TD *comment_mode_buf,
    struct mcm_tag_info_t *tag_info_buf)
{
    MCM_DTYPE_USIZE_TD read_len, ridx;
    MCM_DTYPE_BOOL_TD find_name = 0, tag_head_end = 0;
    char *tmp_name;


    memset(tag_info_buf, 0, sizeof(struct mcm_tag_info_t));

    (*file_line_buf)++;
    tag_info_buf->file_line = *file_line_buf;

    read_len = strlen(read_con);
    if(read_len == 0)
        return 0;
    if(read_con[read_len - 1] != '\n')
    {
        DMSG("read_con buffer size too small or missing new line at the end of file");
        return -1;
    }
    else
    {
        read_len--;
        read_con[read_len] = '\0';
    }
    if(read_con[read_len - 1] == '\r')
    {
        read_len--;
        read_con[read_len] = '\0';
    }

    // 移除前端的 " " 和 "\t".
    for(ridx = 0; ridx < read_len; ridx++)
        if((read_con[ridx] != ' ') && (read_con[ridx] != '\t'))
            break;
    if(ridx == read_len)
        return 0;
    if(ridx > 0)
    {
        read_con += ridx;
        read_len -= ridx;
    }

    // 移除後端的 " " 和 "\t".
    for(ridx = read_len; ridx > 0; ridx--)
        if((read_con[ridx - 1] != ' ') && (read_con[ridx - 1] != '\t'))
            break;
    if(ridx < read_len)
    {
        read_con[ridx] = '\0';
        read_len -= (read_len - ridx);
    }

    // 如果是 xml 開頭宣告, 跳過.
    if(read_len > MCM_TAG_XML_LEN)
        if(strncmp(read_con, MCM_TAG_XML_KEY, MCM_TAG_XML_LEN) == 0)
            return 0;

    // == 0 表示不在註解模式下,
    // != 0 表示正在註解模式下.
    if(*comment_mode_buf == 0)
    {
        if(read_len < MCM_TAG_COMMENT1_LEN)
        {
            DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                 "invalid tag, content too short", tag_info_buf->file_line, "INVALID_TAG_01");
            return -1;
        }

        if(strncmp(read_con, MCM_TAG_COMMENT1_KEY, MCM_TAG_COMMENT1_LEN) == 0)
        {
            (*comment_mode_buf)++;
            if(strcmp(read_con + (read_len - MCM_TAG_COMMENT2_LEN), MCM_TAG_COMMENT2_KEY) == 0)
                (*comment_mode_buf)--;
            return 0;
        }
    }
    else
    {
        if(strncmp(read_con, MCM_TAG_COMMENT1_KEY, MCM_TAG_COMMENT1_LEN) == 0)
            (*comment_mode_buf)++;
        if(strcmp(read_con + (read_len - MCM_TAG_COMMENT2_LEN), MCM_TAG_COMMENT2_KEY) == 0)
            (*comment_mode_buf)--;
        return 0;
    }

    // 檢查 <...>...</...> 配對是否正確.
    if(check_tag_symbol_format(tag_info_buf, read_con, read_len) < 0)
    {
        DMSG("call check_tag_symbol_format() fail");
        return -1;
    }
    // 跳過 "<".
    read_con++;
    read_len--;

    // != "/", 是頭端標籤,
    // == "/", 是尾端標籤.
    if(read_con[0] != '/')
    {
        // 讀取標籤的屬性.
        while(tag_head_end == 0)
        {
            // 找到分段字元.
            for(ridx = 0; ridx < read_len; ridx++)
                if((read_con[ridx] == ' ') || (read_con[ridx] == '>'))
                    break;
            tag_head_end = read_con[ridx] == '>' ? 1 : 0;
            read_con[ridx] = '\0';

            // 如果有標籤屬性.
            if(ridx > 0)
            {
                // 首先處理標籤名稱,
                // 之後處理標籤屬性.
                if(find_name == 0)
                {
                    if(check_class_format(tag_info_buf, find_base, read_con) < 0)
                    {
                        DMSG("call check_class_format() fail");
                        return -1;
                    }

                    if(check_name_format1(tag_info_buf, read_con, ridx) < 0)
                    {
                        DMSG("call check_name_format1() fail");
                        return -1;
                    }
                    find_name = 1;
                }
                else
                {
                    if(check_attribute_list(tag_info_buf, read_con, ridx) < 0)
                    {
                        DMSG("call check_attribute_list() fail");
                        return -1;
                    }
                }
            }
            // 移動到目前標籤的尾端.
            read_con += (ridx + 1);
            read_len -= (ridx + 1);

            // 找到下一個標籤屬性.
            if(tag_head_end == 0)
            {
                for(ridx = 0; ridx < read_len; ridx++)
                    if(read_con[ridx] != ' ')
                        break;
                read_con += ridx;
                read_len -= ridx;
            }
        }
        // 己查是否有標籤屬性遺漏.
        if(check_attribute_lose(tag_info_buf) < 0)
        {
            DMSG("call check_attribute_lose() fail");
            return -1;
        }

        // 檢查排列方式.
        if(check_arrange_format(tag_info_buf, read_len) < 0)
        {
            DMSG("call check_arrange_format() fail");
            return -1;
        }

        // 處理尾端標籤和值.
        if(read_len > 0)
        {
            for(ridx = read_len - 1; ridx >= 0; ridx--)
                if(read_con[ridx] == '<')
                    break;
            read_con[ridx] = read_con[read_len - 1] = '\0';

            tmp_name = read_con + ridx + 2;
            if(strcmp(tag_info_buf->name_con, tmp_name) != 0)
            {
                DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                     "invalid tag, name not match [<%s>...</%s>]",
                     tag_info_buf->file_line, "INVALID_TAG_02", tag_info_buf->name_con, tmp_name);
                return -1;
            }

            if(check_default_format(tag_info_buf, read_con, ridx) < 0)
            {
                DMSG("call check_default_format() fail");
                return -1;
            }
        }
    }
    else
    {
        // 跳過 "/".
        read_con++;
        read_len--;

        read_con[read_len - 1] = '\0';
        read_len--;

        if(check_name_format1(tag_info_buf, read_con, read_len) < 0)
        {
            DMSG("call check_name_format1() fail");
            return -1;
        }

        tag_info_buf->tag_pair = MCM_TPAIR_TAIL;
    }

    return 1;
}

int assign_data(
    char *data_con,
    MCM_DTYPE_USIZE_TD data_len,
    char **store_buf)
{
    if(data_con != NULL)
    {
        data_len++;

        *store_buf = (char *) malloc(data_len);
        if(*store_buf == NULL)
        {
            DMSG("call malloc() fail [%s]", strerror(errno));
            return -1;
        }
        memcpy(*store_buf, data_con, data_len);
    }

    return 0;
}

int free_base_profile(
    struct mcm_xml_base_t *this_base)
{
    if(this_base->base_version != NULL)
        free(this_base->base_version);

    free(this_base);

    return 0;
}

int free_node_profile(
    struct mcm_xml_node_t *this_node)
{
    struct mcm_xml_node_t *self_node;


    while(this_node != NULL)
    {
        if(this_node->child_node != NULL)
            free_node_profile(this_node->child_node);

        while(this_node->member_node != NULL)
        {
            self_node = this_node->member_node;
            this_node->member_node = this_node->member_node->next_node;

            if(self_node->node_name != NULL)
                free(self_node->node_name);

            if(self_node->node_default != NULL)
                free(self_node->node_default);

            if(self_node->config_comment_path_name != NULL)
                free(self_node->config_comment_path_name);

            if(self_node->config_max_entry_dname != NULL)
                free(self_node->config_max_entry_dname);

            if(self_node->config_buffer_size_dname != NULL)
                free(self_node->config_buffer_size_dname);

            if(self_node->config_data_status_sname != NULL)
                free(self_node->config_data_status_sname);

            if(self_node->config_data_value_sname != NULL)
                free(self_node->config_data_value_sname);

            if(self_node->jslib_max_entry_vname != NULL)
                free(self_node->jslib_max_entry_vname);

            if(self_node->store_path_name != NULL)
                free(self_node->store_path_name);

            free(self_node);
        }

        self_node = this_node;
        this_node = this_node->next_node;

        if(self_node->node_name != NULL)
            free(self_node->node_name);

        if(self_node->node_default != NULL)
            free(self_node->node_default);

        if(self_node->config_comment_path_name != NULL)
            free(self_node->config_comment_path_name);

        if(self_node->config_max_entry_dname != NULL)
            free(self_node->config_max_entry_dname);

        if(self_node->config_buffer_size_dname != NULL)
            free(self_node->config_buffer_size_dname);

        if(self_node->config_data_status_sname != NULL)
            free(self_node->config_data_status_sname);

        if(self_node->config_data_value_sname != NULL)
            free(self_node->config_data_value_sname);

        if(self_node->jslib_max_entry_vname != NULL)
            free(self_node->jslib_max_entry_vname);

        if(self_node->store_path_name != NULL)
            free(self_node->store_path_name);

        free(self_node);
    }

    return 0;
}

int free_xml_profile(
    struct mcm_xml_info_t *this_xml)
{
    if(this_xml->db_base != NULL)
        free_base_profile(this_xml->db_base);

    if(this_xml->db_node != NULL)
        free_node_profile(this_xml->db_node);

    return 0;
}

int load_base_profile(
    struct mcm_tag_info_t *this_tag,
    struct mcm_xml_base_t **base_buf)
{
    *base_buf = (struct mcm_xml_base_t *) calloc(1, sizeof(struct mcm_xml_base_t));
    if(*base_buf == NULL)
    {
        DMSG("call calloc() fail [%s]", strerror(errno));
        return -1;
    }

    if(assign_data(this_tag->attribute_version, strlen(this_tag->attribute_version),
                   &((*base_buf)->base_version)) < 0)
    {
        DMSG("call assign_data() fail");
        return -1;
    }

    return 0;
}

int load_node_profile(
    struct mcm_tag_info_t *this_tag,
    FILE *file_fp,
    char *read_buf,
    MCM_DTYPE_USIZE_TD read_size,
    MCM_DTYPE_USIZE_TD *file_line_buf,
    MCM_DTYPE_LIST_TD *comment_mod_buf,
    struct mcm_xml_node_t *parent_node,
    struct mcm_xml_node_t **node_buf)
{
    int fret;
    struct mcm_xml_node_t *group_node = NULL, *member_node, *each_node;
    MCM_DTYPE_BOOL_TD is_child;
    MCM_DTYPE_USIZE_TD group_line, ncnt;


    group_line = this_tag->file_line;

    while(1)
    {
        if(group_node == NULL)
        {
            // 最上層必須是 gs 類型.
            if(node_buf != NULL)
                if(this_tag->attribute_type_index != MCM_DTYPE_GS_INDEX)
                {
                    DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                         "invalid tag [node], root node must be [%s] [<%s>...</%s>]",
                         this_tag->file_line, "INVALID_NODE_01", MCM_DTYPE_GS_KEY,
                         this_tag->name_con, this_tag->name_con);
                    goto FREE_01;
                }
            // 最開始必須是 gs / gd 類型.
            if((this_tag->attribute_type_index != MCM_DTYPE_GS_INDEX) &&
               (this_tag->attribute_type_index != MCM_DTYPE_GD_INDEX))
            {
                DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                     "invalid tag [node], first node must be [%s/%s] [<%s>...</%s>]",
                     this_tag->file_line, "INVALID_NODE_02", MCM_DTYPE_GS_KEY, MCM_DTYPE_GD_KEY,
                     this_tag->name_con, this_tag->name_con);
                goto FREE_01;
            }
        }
        else
        {
            if(fgets(read_buf, read_size, file_fp) == NULL)
                break;
            fret = parse_tag(0, read_buf, file_line_buf, comment_mod_buf, this_tag);
            if(fret < 0)
            {
                DMSG("call parse_tag() fail");
                goto FREE_01;
            }
            if(fret == 0)
                continue;
        }

        is_child = 0;

        if(this_tag->tag_pair != MCM_TPAIR_TAIL)
        {
            // 不是在最開始處讀取到 gs / gd 節點, 表示是子節點.
            if((this_tag->attribute_type_index == MCM_DTYPE_GS_INDEX) ||
               (this_tag->attribute_type_index == MCM_DTYPE_GD_INDEX))
            {
                is_child = group_node != NULL ? 1 : is_child;
            }

            if(is_child == 0)
            {
                each_node = (this_tag->attribute_type_index == MCM_DTYPE_GS_INDEX) ||
                            (this_tag->attribute_type_index == MCM_DTYPE_GD_INDEX) ?
                            parent_node : group_node;
                if(check_name_format2(this_tag, each_node) < 0)
                {
                    DMSG("call check_name_format2() fail");
                    goto FREE_01;
                }

                if((this_tag->attribute_type_index == MCM_DTYPE_GS_INDEX) ||
                   (this_tag->attribute_type_index == MCM_DTYPE_GD_INDEX))
                {
                    if(check_save_format2(this_tag, parent_node) < 0)
                    {
                        DMSG("call check_save_format2() fail");
                        goto FREE_01;
                    }
                }

                if((this_tag->attribute_type_index == MCM_DTYPE_GS_INDEX) ||
                   (this_tag->attribute_type_index == MCM_DTYPE_GD_INDEX))
                {
                    group_node = (struct mcm_xml_node_t *) calloc(1, sizeof(struct mcm_xml_node_t));
                    if(group_node == NULL)
                    {
                        DMSG("call calloc() fail [%s]", strerror(errno));
                        goto FREE_01;
                    }

                    if((group_node->parent_node = parent_node) != NULL)
                    {
                        group_node->level_index = parent_node->level_index + 1;

                        // 加入到 parent_node 的 child_node 串列.
                        if(parent_node->child_node == NULL)
                        {
                            parent_node->child_node = group_node;
                        }
                        else
                        {
                            for(each_node = parent_node->child_node; each_node->next_node != NULL;
                                each_node = each_node->next_node);
                            each_node->next_node = group_node;
                            group_node->order_index = each_node->order_index + 1;
                        }
                    }

                    each_node = group_node;
                }
                else
                {
                    member_node = (struct mcm_xml_node_t *)
                        calloc(1, sizeof(struct mcm_xml_node_t));
                    if(member_node == NULL)
                    {
                        DMSG("call calloc() fail [%s]", strerror(errno));
                        goto FREE_01;
                    }

                    member_node->level_index = group_node->level_index;

                    // 加入到 group_node 的 member_node 串列.
                    if(group_node->member_node == NULL)
                    {
                        group_node->member_node = member_node;
                    }
                    else
                    {
                       for(each_node = group_node->member_node; each_node->next_node != NULL;
                           each_node = each_node->next_node);
                       each_node->next_node = member_node;
                       member_node->order_index = each_node->order_index + 1;
                    }

                    each_node = member_node;
                }

                if(assign_data(this_tag->name_con, this_tag->name_len, &each_node->node_name) < 0)
                {
                    DMSG("call assign_data() fail");
                    goto FREE_01;
                }
                each_node->node_type = this_tag->attribute_type_index;
                each_node->node_size = this_tag->attribute_type_size;
                each_node->node_max = this_tag->attribute_max;
                each_node->node_save = this_tag->attribute_save;
                if(assign_data(this_tag->default_con, this_tag->default_len,
                               &each_node->node_default) < 0)
                {
                    DMSG("call assign_data() fail");
                    goto FREE_01;
                }
                each_node->profile_index = this_tag->profile_index;
            }
            else
            {
                if(load_node_profile(this_tag, file_fp, read_buf, read_size, file_line_buf,
                                     comment_mod_buf, group_node, NULL) < 0)
                {
                    DMSG("call load_node_profile() fail");
                    goto FREE_01;
                }
            }
        }
        else
        {
            if(strcmp(group_node->node_name, this_tag->name_con) != 0)
            {
                DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                     "invalid tag [node], name not match [<%s>...</%s>]",
                     this_tag->file_line, "INVALID_NODE_03",
                     group_node->node_name, this_tag->name_con);
                goto FREE_01;
            }

            for(each_node = group_node->member_node, ncnt = 0; each_node != NULL;
                each_node = each_node->next_node)
            {
                if(each_node->node_type == MCM_DTYPE_EK_INDEX)
                    ncnt++;
            }
            if(ncnt != 1)
            {
                DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                     "invalid tag [node], [%s/%s] must be assign only one [%s] member",
                     group_line, "INVALID_NODE_04", MCM_DTYPE_GS_KEY, MCM_DTYPE_GD_KEY,
                     MCM_DTYPE_EK_KEY);
                goto FREE_01;
            }

            if(group_node->child_node == NULL)
            {
                for(each_node = group_node->member_node, ncnt = 0; each_node != NULL;
                    each_node = each_node->next_node)
                {
                    if(each_node->node_type != MCM_DTYPE_EK_INDEX)
                        ncnt++;
                }
                if(ncnt == 0)
                {
                    DMSG(MCM_PROFILE_ERROR_PREFIX_MSG
                         "invalid tag [node], edge [%s/%s] must have any member",
                         group_line, "INVALID_NODE_05", MCM_DTYPE_GS_KEY, MCM_DTYPE_GD_KEY);
                    goto FREE_01;
                }
            }

            break;
        }
    }

    if(node_buf != NULL)
        *node_buf = group_node;

    return 0;
FREE_01:
    if(group_node != NULL)
    {
        if(group_node->parent_node != NULL)
        {
            if(group_node->parent_node->child_node == group_node)
            {
                group_node->parent_node->child_node = NULL;
            }
            else
            {
                for(each_node = group_node->parent_node->child_node;
                    each_node->next_node != group_node; each_node = each_node->next_node);
                each_node->next_node = NULL;
            }
        }
        free_node_profile(group_node);
    }
    return -1;
}

int load_xml_profile(
    char *file_path,
    struct mcm_xml_info_t *xml_buf)
{
    int fret = -1, parse_ret;
    struct mcm_tag_info_t tag_info;
    MCM_DTYPE_USIZE_TD file_line = 0;
    MCM_DTYPE_LIST_TD comment_mode = 0;
    char read_buf[READ_BUFFER_SIZE];
    FILE *file_fp;


    file_fp = fopen(file_path, "r");
    if(file_fp == NULL)
    {
        DMSG("call fopen(%s) fail [%s]", file_path, strerror(errno));
        goto FREE_01;
    }

    while(fgets(read_buf, sizeof(read_buf), file_fp) != NULL)
    {
        parse_ret = parse_tag(1, read_buf, &file_line, &comment_mode, &tag_info);
        if(parse_ret < 0)
        {
            DMSG("call parse_tag() fail");
            goto FREE_02;
        }
        if(parse_ret == 0)
            continue;

        if(load_base_profile(&tag_info, &xml_buf->db_base) < 0)
        {
            DMSG("call load_base_profile() fail");
            goto FREE_02;
        }
        break;
    }

    while(fgets(read_buf, sizeof(read_buf), file_fp) != NULL)
    {
        parse_ret = parse_tag(0, read_buf, &file_line, &comment_mode, &tag_info);
        if(parse_ret < 0)
        {
            DMSG("call parse_tag() fail");
            goto FREE_02;
        }
        if(parse_ret == 0)
            continue;

        if(load_node_profile(&tag_info, file_fp, read_buf, sizeof(read_buf), &file_line,
                             &comment_mode, NULL, &xml_buf->db_node) < 0)
        {
            DMSG("call load_node_profile() fail");
            goto FREE_02;
        }
        break;
    }

    fret = 0;
FREE_02:
    if(fret < 0)
        free_xml_profile(xml_buf);
    fclose(file_fp);
FREE_01:
    return fret;
}

int get_node_max_level(
    struct mcm_xml_node_t *this_node,
    MCM_DTYPE_USIZE_TD *level_buf)
{
    // 計算資料模型的最大層數.
    for(; this_node != NULL; this_node = this_node->next_node)
    {
        if(this_node->level_index > *level_buf)
            *level_buf = this_node->level_index;

        if(this_node->child_node != NULL)
            get_node_max_level(this_node->child_node, level_buf);
    }

    return 0;
}

int get_node_max_length(
    struct mcm_xml_node_t *this_node,
    MCM_DTYPE_USIZE_TD max_length,
    MCM_DTYPE_USIZE_TD *length_buf)
{
    struct mcm_xml_node_t *each_node;
    MCM_DTYPE_USIZE_TD self_nlen, sub_nlen, max_mnlen;


    // 計算資料模型的路徑的最大長度.
    for(; this_node != NULL; this_node = this_node->next_node)
    {
        self_nlen = max_length + strlen(this_node->node_name) + 2;

        if(this_node->node_type == MCM_DTYPE_GD_INDEX)
            self_nlen += ek_len + 1;

        max_mnlen = 0;
        for(each_node = this_node->member_node; each_node != NULL; each_node = each_node->next_node)
        {
            sub_nlen = strlen(each_node->node_name);
            if(max_mnlen < sub_nlen)
                max_mnlen = sub_nlen;
        }
        if(max_mnlen > 0)
            max_mnlen++;

        if(this_node->child_node != NULL)
            get_node_max_length(this_node->child_node, self_nlen, length_buf);

        if(*length_buf < (self_nlen + max_mnlen))
            *length_buf = self_nlen + max_mnlen;
    }

    return 0;
}

int calculate_node_level_length(
    struct mcm_xml_info_t *this_xml)
{
    MCM_DTYPE_USIZE_TD max_level = 0, max_length = 0;


    get_node_max_level(this_xml->db_node, &max_level);
    this_xml->cv_node_max_level = max_level + 1;

    get_node_max_length(this_xml->db_node, 0, &max_length);
    this_xml->cv_node_max_length = max_length;

    return 0;
}

int fill_node_path(
    struct mcm_xml_node_t *this_node,
    struct mcm_xml_node_t *this_member_node,
    MCM_DTYPE_LIST_TD fill_type,
    char *path_buf,
    MCM_DTYPE_USIZE_TD path_size,
    MCM_DTYPE_USIZE_TD *path_len_buf)
{
    char sub_buf[READ_BUFFER_SIZE];
    MCM_DTYPE_USIZE_TD plen = 0, slen;


    path_buf[0] = '\0';

    if(this_member_node != NULL)
    {
        if(fill_type == 0)
            snprintf(sub_buf, sizeof(sub_buf), "%s", this_member_node->node_name);
        else
        if(fill_type == 1)
            snprintf(sub_buf, sizeof(sub_buf), "%s", this_member_node->node_name);

        slen = strlen(sub_buf);
        if((slen + 1) >= sizeof(sub_buf))
        {
            DMSG("sub_buf too small");
            return -1;
        }

        memcpy(path_buf, sub_buf, slen + 1);
        plen += slen;
    }

    for(; this_node != NULL; this_node = this_node->parent_node)
    {
        if(fill_type == 0)
        {
            if(this_node->node_type == MCM_DTYPE_GS_INDEX)
            {
                snprintf(sub_buf, sizeof(sub_buf), "%s%c",
                         this_node->node_name, plen == 0 ? '\0' : MCM_SPROFILE_PATH_SPLIT_KEY);
            }
            else
            {
                snprintf(sub_buf, sizeof(sub_buf), "%s%c%c%c",
                         this_node->node_name,
                         MCM_SPROFILE_PATH_SPLIT_KEY, MCM_SPROFILE_PATH_MASK_KEY,
                         plen == 0 ? '\0' : MCM_SPROFILE_PATH_SPLIT_KEY);
            }
        }
        else
        if(fill_type == 1)
        {
            snprintf(sub_buf, sizeof(sub_buf), "%s%c",
                     this_node->node_name, plen == 0 ? '\0' : '_');
        }
        else
        if(fill_type == 2)
        {
            snprintf(sub_buf, sizeof(sub_buf), "%s%c",
                     this_node->node_name, plen == 0 ? '\0' : MCM_SPROFILE_PATH_SPLIT_KEY);
        }

        slen = strlen(sub_buf);
        if((slen + 1) >= sizeof(sub_buf))
        {
            DMSG("sub_buf too small");
            return -1;
        }

        if((plen + slen + 1) >= path_size)
        {
            DMSG("path_buf too small");
            return -1;
        }
        if(plen > 0)
            memmove(path_buf + slen, path_buf, plen);
        memcpy(path_buf, sub_buf, slen);
        plen += slen;
        path_buf[plen] = '\0';
    }

    if(path_len_buf != NULL)
        *path_len_buf = plen;

    return 0;
}

int fill_node_name(
    struct mcm_xml_node_t *this_node,
    char *name_buf,
    MCM_DTYPE_USIZE_TD name_size,
    char *path_buf,
    MCM_DTYPE_USIZE_TD path_size)
{
    struct mcm_xml_node_t *each_node;
    MCM_DTYPE_USIZE_TD nlen, plen;


#define ASSIGN_NAME(target_buf, msg_fmt, msg_args...) \
    do                                                        \
    {                                                         \
        snprintf(name_buf, name_size, msg_fmt, ##msg_args);   \
        nlen = strlen(name_buf) + 1;                          \
        if(nlen >= name_size)                                 \
        {                                                     \
            DMSG("name_buf too small");                       \
            return -1;                                        \
        }                                                     \
        target_buf = (char *) malloc(nlen);                   \
        if(target_buf == NULL)                                \
        {                                                     \
            DMSG("call malloc() fail [%s]", strerror(errno)); \
            return -1;                                        \
        }                                                     \
        memcpy(target_buf, name_buf, nlen);                   \
    }                                                         \
    while(0)

#define CONVERT_UPPER_CASE(msg_buf, msg_len, tmp_idx) \
    do                                                                 \
    {                                                                  \
        for(tmp_idx = 0; tmp_idx < msg_len; tmp_idx++)                 \
            if(('a' <= msg_buf[tmp_idx]) && (msg_buf[tmp_idx] <= 'z')) \
                msg_buf[tmp_idx] -= 32;                                \
    }                                                                  \
    while(0)


    for(; this_node != NULL; this_node = this_node->next_node)
    {
        if(fill_node_path(this_node, NULL, 0, path_buf, path_size, NULL) < 0)
        {
            DMSG("call fill_node_path() fail");
            return -1;
        }
        ASSIGN_NAME(this_node->config_comment_path_name, "%s", path_buf);

        if(fill_node_path(this_node, NULL, 1, path_buf, path_size, &plen) < 0)
        {
            DMSG("call fill_node_path() fail");
            return -1;
        }
        CONVERT_UPPER_CASE(path_buf, plen, nlen);
        ASSIGN_NAME(this_node->config_max_entry_dname,
                    MCM_DN_DATA_EXINFO_MAX_COUNT_NAME, path_buf);

        if(fill_node_path(this_node, NULL, 1, path_buf, path_size, NULL) < 0)
        {
            DMSG("call fill_node_path() fail");
            return -1;
        }
        ASSIGN_NAME(this_node->config_data_status_sname,
                    MCM_SN_DATA_EXINFO_DATA_STATUS_NAME, path_buf);
        ASSIGN_NAME(this_node->config_data_value_sname,
                    MCM_SN_DATA_EXINFO_DATA_VALUE_NAME, path_buf);

        ASSIGN_NAME(this_node->jslib_max_entry_vname,
                    MCM_VN_JSLIB_DATA_MAX_COUNT_NAME, path_buf);

        if(fill_node_path(this_node, NULL, 2, path_buf, path_size, NULL) < 0)
        {
            DMSG("call fill_node_path() fail");
            return -1;
        }
        ASSIGN_NAME(this_node->store_path_name, "%s", path_buf);

        for(each_node = this_node->member_node; each_node != NULL; each_node = each_node->next_node)
        {
            if(fill_node_path(this_node, each_node, 0, path_buf, path_size, NULL) < 0)
            {
                DMSG("call fill_node_path() fail");
                return -1;
            }
            ASSIGN_NAME(each_node->config_comment_path_name, "%s", path_buf);

            if(fill_node_path(this_node, each_node, 1, path_buf, path_size, &plen) < 0)
            {
                DMSG("call fill_node_path() fail");
                return -1;
            }
            CONVERT_UPPER_CASE(path_buf, plen, nlen);
            ASSIGN_NAME(each_node->config_buffer_size_dname,
                        MCM_DN_DATA_EXINFO_BUFFER_SIZE_NAME, path_buf);
        }

        if(this_node->child_node != NULL)
            if(fill_node_name(this_node->child_node, name_buf, name_size, path_buf, path_size) < 0)
            {
                DMSG("call assign_name() fail");
                return -1;
            }
    }

    return 0;
}

int fill_node_config_name(
    struct mcm_xml_info_t *this_xml)
{
    int fret = -1;
    char path1_buf[READ_BUFFER_SIZE], path2_buf[READ_BUFFER_SIZE];


    if(fill_node_name(this_xml->db_node, path1_buf, sizeof(path1_buf),
                      path2_buf, sizeof(path2_buf)) < 0)
    {
        DMSG("call fill_node_name() fail");
        goto FREE_01;
    }

    fret = 0;
FREE_01:
    return fret;
}

int output_config_sub_data_info_h_01(
    struct mcm_xml_node_t *this_node,
    FILE *file_fp)
{
    // 產生紀錄 group 的 max-count 的定義.
    for(; this_node != NULL; this_node = this_node->next_node)
    {
        WF1("// %s\n", this_node->config_comment_path_name);
        WF1("#define %s " MCM_DTYPE_USIZE_PF MCM_DTYPE_USIZE_SUFFIX "\n\n",
            this_node->config_max_entry_dname, this_node->node_max);

        if(this_node->child_node != NULL)
            output_config_sub_data_info_h_01(this_node->child_node, file_fp);
    }

    return 0;
}

int output_config_sub_data_info_h_02(
    struct mcm_xml_node_t *this_node,
    FILE *file_fp)
{
    struct mcm_xml_node_t *each_node;

    // 產生紀錄 s:$(size) b:$(size) 的大小的定義.
    for(; this_node != NULL; this_node = this_node->next_node)
    {
        for(each_node = this_node->member_node; each_node != NULL;
            each_node = each_node->next_node)
        {
            switch(each_node->node_type)
            {
                case MCM_DTYPE_EK_INDEX:
#if MCM_SUPPORT_DTYPE_RK
                case MCM_DTYPE_RK_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISC
                case MCM_DTYPE_ISC_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUC
                case MCM_DTYPE_IUC_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISS
                case MCM_DTYPE_ISS_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUS
                case MCM_DTYPE_IUS_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISI
                case MCM_DTYPE_ISI_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUI
                case MCM_DTYPE_IUI_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISLL
                case MCM_DTYPE_ISLL_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IULL
                case MCM_DTYPE_IULL_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FF
                case MCM_DTYPE_FF_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FD
                case MCM_DTYPE_FD_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FLD
                case MCM_DTYPE_FLD_INDEX:
#endif
                    break;
#if MCM_SUPPORT_DTYPE_S
                case MCM_DTYPE_S_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_B
                case MCM_DTYPE_B_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_S || MCM_SUPPORT_DTYPE_B
                    WF1("// %s\n", each_node->config_comment_path_name);
                    WF1("#define %s " MCM_DTYPE_USIZE_PF MCM_DTYPE_USIZE_SUFFIX "\n\n",
                        each_node->config_buffer_size_dname, each_node->node_size);
                    break;
#endif
            }
        }

        if(this_node->child_node != NULL)
            output_config_sub_data_info_h_02(this_node->child_node, file_fp);
    }

    return 0;
}

int output_config_sub_data_info_h_03(
    struct mcm_xml_node_t *this_node,
    FILE *file_fp)
{
    struct mcm_xml_node_t *each_node;


    for(; this_node != NULL; this_node = this_node->next_node)
    {
        WF1("// %s\n", this_node->config_comment_path_name);

        if(this_node->member_node != NULL)
        {
            // 產生紀錄 group 內所有 member 的狀態的結構.
            WF1("struct %s\n", this_node->config_data_status_sname);
            WF1("{\n");
            for(each_node = this_node->member_node; each_node != NULL;
                each_node = each_node->next_node)
            {
                WF1("    %s %s;\n", DEF_TO_STR(MCM_DTYPE_DS_TD), each_node->node_name);
            }
            WF1("} %s;\n", MCM_DATA_EXINFO_DATA_STATUS_ATTRIBUTE);

            // 產生紀錄 group 內所有 member 的數值的結構.
            WF1("struct %s\n", this_node->config_data_value_sname);
            WF1("{\n");
            for(each_node = this_node->member_node; each_node != NULL;
                each_node = each_node->next_node)
            {
                switch(each_node->node_type)
                {
                    case MCM_DTYPE_EK_INDEX:
#if MCM_SUPPORT_DTYPE_RK
                    case MCM_DTYPE_RK_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISC
                    case MCM_DTYPE_ISC_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUC
                    case MCM_DTYPE_IUC_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISS
                    case MCM_DTYPE_ISS_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUS
                    case MCM_DTYPE_IUS_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISI
                    case MCM_DTYPE_ISI_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IUI
                    case MCM_DTYPE_IUI_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_ISLL
                    case MCM_DTYPE_ISLL_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_IULL
                    case MCM_DTYPE_IULL_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FF
                    case MCM_DTYPE_FF_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FD
                    case MCM_DTYPE_FD_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_FLD
                    case MCM_DTYPE_FLD_INDEX:
#endif
                        WF1("    %s %s;\n",
                            mcm_tag_profile_node_list[each_node->profile_index].type_typedef,
                            each_node->node_name);
                        break;
#if MCM_SUPPORT_DTYPE_S
                    case MCM_DTYPE_S_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_B
                    case MCM_DTYPE_B_INDEX:
#endif
#if MCM_SUPPORT_DTYPE_S || MCM_SUPPORT_DTYPE_B
                        WF1("    %s %s[%s];\n",
                            mcm_tag_profile_node_list[each_node->profile_index].type_typedef,
                            each_node->node_name, each_node->config_buffer_size_dname);
                        break;
#endif
                }
            }
            WF1("} %s;\n", MCM_DATA_EXINFO_DATA_VALUE_ATTRIBUTE);
        }

        WF1("\n");

        if(this_node->child_node != NULL)
            output_config_sub_data_info_h_03(this_node->child_node, file_fp);
    }

    return 0;
}

int output_config_sub_data_info_c_01(
    struct mcm_xml_node_t *this_node,
    FILE *file_fp)
{
    struct mcm_xml_node_t *each_node;


    // 產生紀錄 member 的狀態和數值的變數在結構中的偏移值.
    // (output_config_sub_data_info_h_03() 產生的結構).
    for(; this_node != NULL; this_node = this_node->next_node)
    {
        if(this_node->member_node == NULL)
        {
            WF1("    {0, 0},\n");
        }
        else
        {
            WF1("    {sizeof(struct %s), sizeof(struct %s)},\n",
                this_node->config_data_status_sname, this_node->config_data_value_sname);
        }

        for(each_node = this_node->member_node; each_node != NULL; each_node = each_node->next_node)
        {
            WF1("    {offsetof(struct %s, %s), offsetof(struct %s, %s)},\n",
                this_node->config_data_status_sname, each_node->node_name,
                this_node->config_data_value_sname, each_node->node_name);
        }

        if(this_node->child_node != NULL)
            output_config_sub_data_info_c_01(this_node->child_node, file_fp);
    }

    return 0;
}

int output_config_data_info_file(
    struct mcm_xml_info_t *this_xml)
{
    int fret = -1;
    char path_exinfo_buf[PATH_BUFFER_SIZE], path_ininfo_buf[PATH_BUFFER_SIZE],
        path_info_buf[PATH_BUFFER_SIZE];
    FILE *file_exinfo_fp, *file_ininfo_fp, *file_info_fp;


    snprintf(path_exinfo_buf, sizeof(path_exinfo_buf), "../%s/%s",
             MCM_MCM_LHEADER_PATH, MCM_MCM_DATA_EXINFO_H_AUTO_FILE);

    snprintf(path_ininfo_buf, sizeof(path_ininfo_buf), "../%s/%s",
             MCM_MCM_DAEMON_PATH, MCM_MCM_DATA_ININFO_H_AUTO_FILE);

    snprintf(path_info_buf, sizeof(path_info_buf), "../%s/%s",
             MCM_MCM_DAEMON_PATH, MCM_MCM_DATA_INFO_C_AUTO_FILE);


    file_exinfo_fp = fopen(path_exinfo_buf, "w");
    if(file_exinfo_fp == NULL)
    {
        DMSG("call fopen(%s) fail [%s]", path_exinfo_buf, strerror(errno));
        goto FREE_01;
    }

    file_ininfo_fp = fopen(path_ininfo_buf, "w");
    if(file_ininfo_fp == NULL)
    {
        DMSG("call fopen(%s) fail [%s]", path_ininfo_buf, strerror(errno));
        goto FREE_02;
    }

    file_info_fp = fopen(path_info_buf, "w");
    if(file_info_fp == NULL)
    {
        DMSG("call fopen(%s) fail [%s]", path_info_buf, strerror(errno));
        goto FREE_03;
    }

    WF2(file_exinfo_fp, "#ifndef %s\n", MCM_DN_DATA_EXINFO_DEFINE_NAME);
    WF2(file_exinfo_fp, "#define %s\n\n\n\n\n", MCM_DN_DATA_EXINFO_DEFINE_NAME);

    WF2(file_exinfo_fp, "#include \"%s\"\n\n\n\n\n", MCM_MCM_TYPE_FILE);

    WF2(file_exinfo_fp, "#define %s \"%s\"\n\n",
        MCM_DN_DATA_EXINFO_PROFILE_VERSION_NAME, this_xml->db_base->base_version);

    WF2(file_exinfo_fp, "#define %s " MCM_DTYPE_USIZE_PF MCM_DTYPE_USIZE_SUFFIX "\n\n",
        MCM_DN_DATA_EXINFO_PATH_MAX_LEVEL, this_xml->cv_node_max_level);

    WF2(file_exinfo_fp, "#define %s " MCM_DTYPE_USIZE_PF MCM_DTYPE_USIZE_SUFFIX "\n\n\n\n\n",
        MCM_DN_DATA_EXINFO_PATH_MAX_LENGTH, this_xml->cv_node_max_length);

    output_config_sub_data_info_h_01(this_xml->db_node, file_exinfo_fp);
    WF2(file_exinfo_fp, "\n\n\n");

    output_config_sub_data_info_h_02(this_xml->db_node, file_exinfo_fp);
    WF2(file_exinfo_fp, "\n\n\n");

    output_config_sub_data_info_h_03(this_xml->db_node, file_exinfo_fp);
    WF2(file_exinfo_fp, "\n\n\n");

    WF2(file_exinfo_fp, "#endif\n");

    WF2(file_ininfo_fp, "#ifndef %s\n", MCM_DN_DATA_ININFO_DEFINE_NAME);
    WF2(file_ininfo_fp, "#define %s\n\n\n\n\n", MCM_DN_DATA_ININFO_DEFINE_NAME);

    WF2(file_ininfo_fp, "#include \"../%s/%s\"\n",
        MCM_MCM_LHEADER_PATH, MCM_MCM_TYPE_FILE);
    WF2(file_ininfo_fp, "#include \"../%s/%s\"\n\n\n\n\n",
        MCM_MCM_LHEADER_PATH, MCM_MCM_DATA_EXINFO_H_AUTO_FILE);

    WF2(file_ininfo_fp, "#define %s %s\n\n",
        MCM_DN_DATA_ININFO_CONFIG_PROFILE_VERSION_NAME, MCM_DN_DATA_EXINFO_PROFILE_VERSION_NAME);

    WF2(file_ininfo_fp, "#define %s %s\n\n",
        MCM_DN_DATA_ININFO_CONFIG_PATH_MAX_LEVEL, MCM_DN_DATA_EXINFO_PATH_MAX_LEVEL);

    WF2(file_ininfo_fp, "#define %s %s\n\n\n\n\n",
        MCM_DN_DATA_ININFO_CONFIG_PATH_MAX_LENGTH, MCM_DN_DATA_EXINFO_PATH_MAX_LENGTH);

    WF2(file_ininfo_fp, "struct %s\n", MCM_SN_DATA_ININFO_CONFIG_DATA_SIZE_OFFSET_NAME);
    WF2(file_ininfo_fp, "{\n");
    WF2(file_ininfo_fp, "    %s %s;\n",
        DEF_TO_STR(MCM_DTYPE_USIZE_TD),
        MCM_MN_DATA_ININFO_CONFIG_DATA_SIZE_OFFSET_DATA_STATUS_NAME);
    WF2(file_ininfo_fp, "    %s %s;\n",
        DEF_TO_STR(MCM_DTYPE_USIZE_TD),
        MCM_MN_DATA_ININFO_CONFIG_DATA_SIZE_OFFSET_DATA_VALUE_NAME);
    WF2(file_ininfo_fp, "};\n\n\n\n\n");

    WF2(file_ininfo_fp, "extern const struct %s %s[];\n\n\n\n\n",
        MCM_SN_DATA_ININFO_CONFIG_DATA_SIZE_OFFSET_NAME,
        MCM_VN_DATA_ININFO_CONFIG_DATA_SIZE_OFFSET_LIST_NAME);

    WF2(file_ininfo_fp, "#endif\n");

    WF2(file_info_fp, "#include <stddef.h>\n");
    WF2(file_info_fp, "#include \"%s\"\n\n\n\n\n", MCM_MCM_DATA_ININFO_H_AUTO_FILE);

    WF2(file_info_fp, "const struct %s %s[] =\n",
        MCM_SN_DATA_ININFO_CONFIG_DATA_SIZE_OFFSET_NAME,
        MCM_VN_DATA_ININFO_CONFIG_DATA_SIZE_OFFSET_LIST_NAME);
    WF2(file_info_fp, "{\n");
    output_config_sub_data_info_c_01(this_xml->db_node, file_info_fp);
    WF2(file_info_fp, "    {0, 0}\n");
    WF2(file_info_fp, "};\n");

    fret = 0;
    fclose(file_info_fp);
    fret < 0 ? unlink(path_info_buf) : chmod(path_info_buf, S_IRUSR | S_IRGRP | S_IROTH);
FREE_03:
    fclose(file_ininfo_fp);
    fret < 0 ? unlink(path_ininfo_buf) : chmod(path_ininfo_buf, S_IRUSR | S_IRGRP | S_IROTH);
FREE_02:
    fclose(file_exinfo_fp);
    fret < 0 ? unlink(path_exinfo_buf) : chmod(path_exinfo_buf, S_IRUSR | S_IRGRP | S_IROTH);
FREE_01:
    return fret;
}

int output_jslib_sub_data_info_file(
    struct mcm_xml_node_t *this_node,
    FILE *file_fp)
{
    for(; this_node != NULL; this_node = this_node->next_node)
    {
        WF1("// %s\n", this_node->config_comment_path_name);
        WF1("var %s = " MCM_DTYPE_USIZE_PF ";\n\n",
            this_node->jslib_max_entry_vname, this_node->node_max);

        if(this_node->child_node != NULL)
            output_jslib_sub_data_info_file(this_node->child_node, file_fp);
    }

    return 0;
}

int output_jslib_data_info_file(
    struct mcm_xml_info_t *this_xml)
{
    int fret = -1;
    char path_buf[PATH_BUFFER_SIZE];
    FILE *file_fp;


    snprintf(path_buf, sizeof(path_buf), "../%s/%s",
             MCM_MCM_JSLIB_PATH, MCM_MCM_JSLIB_DATA_INFO_AUTO_FILE);

    file_fp = fopen(path_buf, "w");
    if(file_fp == NULL)
    {
        DMSG("call fopen(%s) fail [%s]", path_buf, strerror(errno));
        goto FREE_01;
    }

    output_jslib_sub_data_info_file(this_xml->db_node, file_fp);

    fret = 0;
    fclose(file_fp);
    chmod(path_buf, S_IRUSR | S_IRGRP | S_IROTH);
FREE_01:
    return fret;
}

int output_sub_model_profile(
    struct mcm_xml_node_t *this_node,
    FILE *file_fp,
    MCM_DTYPE_USIZE_TD *info_index)
{
    struct mcm_xml_node_t *each_node;
    MCM_DTYPE_USIZE_TD xidx;


    for(; this_node != NULL; this_node = this_node->next_node)
    {
        WF1("%c\n", MCM_MPROFILE_GROUP_KEY);

        // group 的名稱 (model->group_name).
        WF1("%s\n", this_node->node_name);

        // group 的類型 (model->group_type).
        WF1(MCM_DTYPE_LIST_PF "\n", this_node->node_type);

        // group 的 max-count (model->group_max).
        WF1(MCM_DTYPE_USIZE_PF "\n", this_node->node_max);

        // group 是否儲存 (model->group_save).
        WF1(MCM_DTYPE_BOOL_PF "\n", this_node->node_save);

        // group 的 mbmer 數目 (不含 ek 類型) (model->member_real_count).
        for(each_node = this_node->member_node, xidx = 0; each_node != NULL;
            each_node = each_node->next_node)
        {
            if(each_node->node_type != MCM_DTYPE_EK_INDEX)
                xidx++;
        }
        WF1(MCM_DTYPE_USIZE_PF "\n", xidx);

        // 紀錄 group 的 member 的狀態的結構的大小 (紀錄索引) (model->data_status_size).
        // 紀錄 group 的 member 的數值的結構的大小 (紀錄索引) (model->data_value_size).
        // output_config_sub_data_info_h_03() 產生的結構.
        WF1(MCM_DTYPE_USIZE_PF "\n", *info_index);

        // gorup 是 parent-group 的第幾個 child-group (model->store_index_in_parent).
        WF1(MCM_DTYPE_USIZE_PF "\n", this_node->order_index);

        // group 內有幾個 child-group (model->store_child_count).
        for(each_node = this_node->child_node, xidx = 0; each_node != NULL;
            each_node = each_node->next_node, xidx++);
        WF1(MCM_DTYPE_USIZE_PF "\n", xidx);

        (*info_index)++;

        for(each_node = this_node->member_node; each_node != NULL; each_node = each_node->next_node)
        {
            WF1("%c\n", MCM_MPROFILE_MEMBER_KEY);

            // member 的名稱 (member->node_name).
            WF1("%s\n", each_node->node_name);

            // member 的資料類型 (member->node_type).
            WF1(MCM_DTYPE_LIST_PF "\n", each_node->node_type);

            // member 的儲存大小 (s:$(size), b:$(size)) (member->node_size).
            WF1(MCM_DTYPE_USIZE_PF "\n", each_node->node_size);

            // member 的資料的預設值 (member->node_default).
            WF1("%s\n", each_node->node_default);

            // 紀錄 member 的狀態的變數在結構內的偏移值 (member->offset_in_status).
            // 紀錄 member 的數值的變數在結構內的偏移值 (member->offset_in_value).
            // 參考 output_config_sub_data_info_c_01() 產生的檔案.
            WF1(MCM_DTYPE_USIZE_PF "\n", *info_index);

            (*info_index)++;

            WF1("%c\n", MCM_MPROFILE_END_KEY);
        }

        if(this_node->child_node != NULL)
            output_sub_model_profile(this_node->child_node, file_fp, info_index);

        WF1("%c\n", MCM_MPROFILE_END_KEY);
    }

    return 0;
}

int output_model_profile_file(
    char *file_path,
    struct mcm_xml_info_t *this_xml)
{
    int fret = -1;
    MCM_DTYPE_USIZE_TD info_index = 0;
    FILE *file_fp;


    file_fp = fopen(file_path, "w");
    if(file_fp == NULL)
    {
        DMSG("call fopen(%s) fail [%s]", file_path, strerror(errno));
        goto FREE_01;
    }

    output_sub_model_profile(this_xml->db_node, file_fp, &info_index);

    fret = 0;
    fclose(file_fp);
    chmod(file_path, S_IRUSR | S_IRGRP | S_IROTH);
FREE_01:
    return fret;
}

int output_sub_store_profile(
    struct mcm_xml_node_t *this_node,
    FILE *file_fp)
{
    struct mcm_xml_node_t *each_node;
    MCM_DTYPE_USIZE_TD mcnt;


    for(; this_node != NULL; this_node = this_node->next_node)
    {
        WF1("%c %s\n", MCM_SPROFILE_COMMENT_KEY, this_node->config_comment_path_name);

        for(each_node = this_node->member_node; each_node != NULL; each_node = each_node->next_node)
            if(each_node->node_type != MCM_DTYPE_EK_INDEX)
                break;

        for(each_node = this_node; each_node != NULL; each_node = each_node->parent_node)
            if(each_node->node_type != MCM_DTYPE_GS_INDEX)
                break;
        if(each_node == NULL)
        {
            for(each_node = this_node->member_node, mcnt = 0; each_node != NULL;
                each_node = each_node->next_node)
            {
                if(each_node->node_type != MCM_DTYPE_EK_INDEX)
                    mcnt++;
            }

            if(mcnt > 0)
                if(this_node->node_save != 0)
                {
                    WF1("%s%c", this_node->store_path_name, MCM_SPROFILE_PARAMETER_SPLIT_KEY);

                    for(each_node = this_node->member_node; each_node != NULL;
                        each_node = each_node->next_node)
                    {
                        if(each_node->node_type == MCM_DTYPE_EK_INDEX)
                            continue;

                        WF1("%s%c%s%c",
                            each_node->node_name, MCM_SPROFILE_MEMBER_SPLIT_KEY,
                            each_node->node_default,
                            each_node->next_node != NULL ? MCM_SPROFILE_PARAMETER_SPLIT_KEY : '\n');
                    }
                }
        }

        WF1("\n");

        if(this_node->child_node != NULL)
            output_sub_store_profile(this_node->child_node, file_fp);
    }

    return 0;
}

int output_store_profile_file(
    char *file_path,
    struct mcm_xml_info_t *this_xml)
{
    int fret = -1;
    FILE *file_fp;


    file_fp = fopen(file_path, "w");
    if(file_fp == NULL)
    {
        DMSG("call fopen(%s) fail [%s]", file_path, strerror(errno));
        goto FREE_01;
    }

    WF1("%c%s%c%s\n\n",
        MCM_SPROFILE_BASE_DATA_KEY, MCM_SPROFILE_BASE_VERSION_KEY,
        MCM_SPROFILE_PARAMETER_SPLIT_KEY, this_xml->db_base->base_version);

    output_sub_store_profile(this_xml->db_node, file_fp);

    fret = 0;
    fclose(file_fp);
    chmod(file_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
FREE_01:
    return fret;
}

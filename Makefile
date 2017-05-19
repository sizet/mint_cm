# Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
# This file is part of the MintCM.
# Some rights reserved. See README.

CROSS        =
export CC    = $(CROSS)gcc
export LD    = $(CROSS)ld
export AR    = $(CROSS)ar
export STRIP = $(CROSS)strip

MV0_MULTIPLE_MODEL_SUFFIX_LOWER =
MV0_MULTIPLE_MODEL_SUFFIX_UPPER =
CC0_MULTIPLE_MODEL_SUFFIX = -DMULTIPLE_MODEL_SUFFIX_LOWER=\"\" -DMULTIPLE_MODEL_SUFFIX_UPPER=\"\"
ifeq ($(MODEL_SUFFIX_LOWER),)
else
    ifeq ($(MODEL_SUFFIX_UPPER),)
    else
        MV0_MULTIPLE_MODEL_SUFFIX_LOWER = _$(MODEL_SUFFIX_LOWER)
        MV0_MULTIPLE_MODEL_SUFFIX_UPPER = _$(MODEL_SUFFIX_UPPER)
        CC0_MULTIPLE_MODEL_SUFFIX = -DMULTIPLE_MODEL_SUFFIX_LOWER=\"$(MV0_MULTIPLE_MODEL_SUFFIX_LOWER)\" \
                                    -DMULTIPLE_MODEL_SUFFIX_UPPER=\"$(MV0_MULTIPLE_MODEL_SUFFIX_UPPER)\"
	endif
endif
export MV_MULTIPLE_MODEL_SUFFIX_LOWER = $(MV0_MULTIPLE_MODEL_SUFFIX_LOWER)
export MV_MULTIPLE_MODEL_SUFFIX_UPPER = $(MV0_MULTIPLE_MODEL_SUFFIX_UPPER)
export CC_MULTIPLE_MODEL_SUFFIX = $(CC0_MULTIPLE_MODEL_SUFFIX)

MCM_BUILD_PATH   = ./mcm_build$(MV_MULTIPLE_MODEL_SUFFIX_LOWER)
MCM_DAEMON_PATH  = ./mcm_daemon$(MV_MULTIPLE_MODEL_SUFFIX_LOWER)
MCM_CGI_PATH     = ./mcm_cgi
MCM_LULIB_PATH   = ./mcm_lib/mcm_lulib
MCM_LKLIB_PATH   = ./mcm_lib/mcm_lklib
MCM_JSLIB_PATH   = ./mcm_lib/mcm_jslib
HTTP_SERVER_PATH = ./http_server/mini_httpd/last

EXAMPLE_PATH = ./usage/zh-TW/example/$(KEY)

export MCM_RUN_ROOT_PATH = $(PWD)/run
export MCM_RUN_WEB_PATH  = $(MCM_RUN_ROOT_PATH)/web
export MCM_RUN_CGI_PATH  = $(MCM_RUN_WEB_PATH)/cgi


all : binary_profile binary_library binary_httpd binary_daemon \
      install_library install_httpd install_profile install_daemon


binary_profile :
	$(MAKE) -C $(MCM_BUILD_PATH) binary
	$(MAKE) -C $(MCM_BUILD_PATH) profile

binary_library :
	$(MAKE) -C $(MCM_LULIB_PATH) binary
	cd $(MCM_LKLIB_PATH); $(MAKE) binary
	$(MAKE) -C $(MCM_CGI_PATH) binary

binary_daemon :
	$(MAKE) -C $(MCM_DAEMON_PATH) binary

binary_httpd :
	$(MAKE) -C $(HTTP_SERVER_PATH) binary

install_library :
	$(MAKE) -C $(MCM_LULIB_PATH) install
	cd $(MCM_LKLIB_PATH); $(MAKE) install
	$(MAKE) -C $(MCM_JSLIB_PATH) install_common
	$(MAKE) -C $(MCM_CGI_PATH) install

install_profile :
	$(MAKE) -C $(MCM_BUILD_PATH) install

install_daemon :
	$(MAKE) -C $(MCM_JSLIB_PATH) install_custom
	$(MAKE) -C $(MCM_DAEMON_PATH) install

install_httpd :
	$(MAKE) -C $(HTTP_SERVER_PATH) install

clean :
	$(MAKE) -C $(MCM_LULIB_PATH) clean
	cd $(MCM_LKLIB_PATH); $(MAKE) clean
	$(MAKE) -C $(MCM_JSLIB_PATH) clean_common
	$(MAKE) -C $(MCM_CGI_PATH) clean
	$(MAKE) -C $(MCM_JSLIB_PATH) clean_custom
	$(MAKE) -C $(MCM_BUILD_PATH) clean
	$(MAKE) -C $(MCM_DAEMON_PATH) clean
	$(MAKE) -C $(HTTP_SERVER_PATH) clean

example_add : example_in_add all
example_in_add :
	$(MAKE) -C $(EXAMPLE_PATH) add

example_del : clean example_in_del
example_in_del :
	$(MAKE) -C $(EXAMPLE_PATH) del

# Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
# This file is part of the MintCM.
# Some rights reserved. See README.

MCM_PATH       = $(MCM_TOP_PATH)
MCM_LULIB_PATH = $(MCM_PATH)/mcm_lib/mcm_lulib

CC_FLAGS = $(CFLAGS) -fPIC -Wall -Wno-unused-result -O2
LD_FLAGS = $(LDFLAGS)

INCLUDE_PATH = -I$(MCM_PATH)
LIBRARY_PATH = -L$(MCM_LULIB_PATH)

CGI_CONFIG_LIBRARY_FILE = -ldl

ifeq (${USE_STATIC_MCM_LIB}, YES)
CGI_CONFIG_LIBRARY_FILE += $(MCM_LULIB_PATH)/mcm_lulib_api.a
else
CGI_CONFIG_LIBRARY_FILE += -lmcm_lulib_api
endif

CGI_UPLOAD_LIBRARY_FILE = -ldl

SOURCE_FILE = $(wildcard *.c)

CGI_CONFIG_OBJECT_FILE = mcm_cgi_config.o mcm_cgi_common.o
CGI_CONFIG_TARGET_FILE = mcm_cgi_config.cgi

CGI_UPLOAD_OBJECT_FILE = mcm_cgi_upload.o mcm_cgi_common.o
CGI_UPLOAD_TARGET_FILE = mcm_cgi_upload.cgi

CGI_CONFIG_MODULE_PATH = ./mcm_cgi_config_module

CGI_UPLOAD_MODULE_PATH = ./mcm_cgi_upload_module


binary : $(CGI_CONFIG_TARGET_FILE) $(CGI_UPLOAD_TARGET_FILE) config_module_flag upload_module_flag


config_module_flag :
	@make -C $(CGI_CONFIG_MODULE_PATH) binary

upload_module_flag :
	@make -C $(CGI_UPLOAD_MODULE_PATH) binary

$(CGI_CONFIG_TARGET_FILE) : $(CGI_CONFIG_OBJECT_FILE)
	$(CC) $(LD_FLAGS) $(LIBRARY_PATH) $^ $(CGI_CONFIG_LIBRARY_FILE) -o $@
	$(STRIP) $@

$(CGI_UPLOAD_TARGET_FILE) : $(CGI_UPLOAD_OBJECT_FILE)
	$(CC) $(LD_FLAGS) $(LIBRARY_PATH) $^ $(CGI_UPLOAD_LIBRARY_FILE) -o $@
	$(STRIP) $@

%.o : %.c
	$(CC) $(CC_FLAGS) $(INCLUDE_PATH) -c $< -o $@

%.d : %.c
	@set -e; rm -f $@; \
	$(CC) $(CC_FLAGS) $(INCLUDE_PATH) -MM $< > $@.$$$$; 2>/dev/null; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(SOURCE_FILE:.c=.d)

install :
	@make -C $(CGI_CONFIG_MODULE_PATH) install
	@make -C $(CGI_UPLOAD_MODULE_PATH) install
	cp -f $(CGI_CONFIG_TARGET_FILE) $(CGI_UPLOAD_TARGET_FILE) $(MCM_RUN_CGI_PATH)

clean :
	@make -C $(CGI_CONFIG_MODULE_PATH) clean
	@make -C $(CGI_UPLOAD_MODULE_PATH) clean
	rm -f *.d *.d* *.o *.cgi
	cd $(MCM_RUN_CGI_PATH); rm -f $(CGI_CONFIG_TARGET_FILE) $(CGI_UPLOAD_TARGET_FILE)

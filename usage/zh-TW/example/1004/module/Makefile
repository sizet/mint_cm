# Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
# This file is part of the MintCM.
# Some rights reserved. See README.

CC_FLAGS = -Wall -O2 -fPIC
LD_FLAGS = -fPIC -shared

MAAM_PATH       = $(PWD)/http_server/mini_httpd/last
MAAM_LIB_PATH   = $(MAAM_PATH)/mint_aam/maam_lib
MAAM_LULIB_PATH = $(MAAM_PATH)

INCLUDE_PATH =
LIBRARY_PATH =

LIBRARY_FILE =

INCLUDE_PATH += -I$(MAAM_LIB_PATH)
LIBRARY_PATH += -L$(MAAM_LULIB_PATH)

USE_STATIC_MAAM_LIB = YES
ifeq (${USE_STATIC_MAAM_LIB}, YES)
LIBRARY_FILE += $(MAAM_LULIB_PATH)/maam_lulib_api.a
else
LIBRARY_FILE += -lmaam_lulib_api
endif

SOURCE_FILE = $(wildcard *.c)
OBJECT_FILE = $(SOURCE_FILE:.c=.o)
TARGET_FILE = mcm_module$(MV_MULTIPLE_MODEL_SUFFIX_LOWER).lib


binary : $(TARGET_FILE)


$(TARGET_FILE) : $(OBJECT_FILE)
	$(CC) $(LD_FLAGS) $(LIBRARY_PATH) $^ $(LIBRARY_FILE) -o $@
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
	cp -f $(TARGET_FILE) $(MCM_RUN_ROOT_PATH)

clean :
	rm -f *.d *.d* *.o $(TARGET_FILE)
	cd $(MCM_RUN_ROOT_PATH); rm -f $(TARGET_FILE)

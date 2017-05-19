# Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
# This file is part of the MintCM.
# Some rights reserved. See README.

MCM_PATH       = $(PWD)/../../../../..
MCM_LIB_PATH   = $(MCM_PATH)/mcm_lib
MCM_LULIB_PATH = $(MCM_LIB_PATH)/mcm_lulib

CROSS =
CC    = $(CROSS)gcc
LD    = $(CROSS)ld
STRIP = $(CROSS)strip

CC_FLAGS = -Wall -Wno-unused-result -O2
LD_FLAGS =

INCLUDE_PATH = -I$(MCM_LIB_PATH)
LIBRARY_PATH = -L$(MCM_LULIB_PATH)

USE_STATIC_MCM_LIB = YES
ifeq (${USE_STATIC_MCM_LIB}, YES)
LIBRARY_FILE = $(MCM_LULIB_PATH)/mcm_lulib_api.a
else
LIBRARY_FILE = -lmcm_lulib_api
endif

SOURCE_FILE = $(wildcard *.c)
OBJECT_FILE = $(SOURCE_FILE:.c=.o)
TARGET_FILE = user_app_0302


all : $(TARGET_FILE)


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

clean :
	rm -f *.d *.d* *.o $(TARGET_FILE)

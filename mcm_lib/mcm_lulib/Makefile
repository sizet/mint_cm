# Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
# This file is part of the MintCM.
# Some rights reserved. See README.

CC_FLAGS = $(CFLAGS) -fPIC -Wall -Wno-strict-aliasing -Wunused-result -O2
LD_FLAGS = $(LDFLAGS) -fPIC -shared
AR_FLAGS = -rs

INCLUDE_PATH =
LIBRARY_PATH =

LIBRARY_FILE =

SOURCE_FILE = $(wildcard *.c)
OBJECT_FILE = $(SOURCE_FILE:.c=.o)
TARGET_FILE_S = mcm_lulib_api.a
TARGET_FILE_D = libmcm_lulib_api.so


binary : $(TARGET_FILE_S) $(TARGET_FILE_D)


$(TARGET_FILE_S) $(TARGET_FILE_D) : $(OBJECT_FILE)
	$(AR) $(AR_FLAGS) $(TARGET_FILE_S) $^
	$(CC) $(LD_FLAGS) $(LIBRARY_PATH) $^ $(LIBRARY_FILE) -o $(TARGET_FILE_D)
	$(STRIP) $(TARGET_FILE_D)

%.o : %.c
	$(CC) $(CC_FLAGS) $(INCLUDE_PATH) -c $< -o $@

%.d : %.c
	@set -e; rm -f $@; \
	$(CC) $(CC_FLAGS) $(INCLUDE_PATH) -MM $< > $@.$$$$; 2>/dev/null; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(SOURCE_FILE:.c=.d)

install :
	cp -f $(TARGET_FILE_D) $(MCM_RUN_ROOT_PATH)

clean :
	rm -f *.d *.d* *.o $(TARGET_FILE_S) $(TARGET_FILE_D)
	cd $(MCM_RUN_ROOT_PATH); rm -f $(TARGET_FILE_D)

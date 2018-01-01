# Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
# This file is part of the MintCM.
# Some rights reserved. See README.

CC_FLAGS = -Wall -Wno-unused-result -O2 -fPIC
LD_FLAGS = -fPIC -shared

INCLUDE_PATH =
LIBRARY_PATH =

LIBRARY_FILE =

SOURCE_FILE = $(wildcard *.c) ../mcm_cgi_common.c
OBJECT_FILE = $(SOURCE_FILE:.c=.o)
TARGET_FILE = mcm_cgi_upload_module.lib


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
	cp -f $(TARGET_FILE) $(MCM_RUN_CGI_PATH)

clean :
	rm -f *.d *.d* *.o $(TARGET_FILE)
	cd $(MCM_RUN_CGI_PATH); rm -f $(TARGET_FILE)

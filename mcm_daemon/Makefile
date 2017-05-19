# Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
# This file is part of the MintCM.
# Some rights reserved. See README.

CC_FLAGS = $(CC_MULTIPLE_MODEL_SUFFIX) -Wall -Wno-strict-aliasing -O2
LD_FLAGS = -Wl,--export-dynamic

INCLUDE_PATH =
LIBRARY_PATH =

LIBRARY_FILE = -ldl -lpthread

SOURCE_FILE = $(wildcard *.c)
OBJECT_FILE = $(SOURCE_FILE:.c=.o)
TARGET_FILE = mcm_daemon$(MV_MULTIPLE_MODEL_SUFFIX_LOWER)

MCM_MODULE_PATH = ./mcm_module	


binary : module_flag $(TARGET_FILE)


module_flag :
	@make -C $(MCM_MODULE_PATH) binary

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
	@make -C $(MCM_MODULE_PATH) install
	cp -f $(TARGET_FILE) $(MCM_RUN_ROOT_PATH) 

clean :
	@make -C $(MCM_MODULE_PATH) clean
	rm -f *.d *.d* *.o $(TARGET_FILE)
	cd $(MCM_RUN_ROOT_PATH); rm -f $(TARGET_FILE)

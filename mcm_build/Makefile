# Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
# This file is part of the MintCM.
# Some rights reserved. See README.

CC    = gcc
LD    = ld
STRIP = strip

CC_FLAGS = $(CC_MULTIPLE_MODEL_SUFFIX) -Wall -Wunused-result -O2
LD_FLAGS =

INCLUDE_PATH =
LIBRARY_PATH =

LIBRARY_FILE =

SOURCE_FILE = $(wildcard *.c)
OBJECT_FILE = $(SOURCE_FILE:.c=.o)
TARGET_FILE = mcm_build$(MV_MULTIPLE_MODEL_SUFFIX_LOWER)

DATA_PROFILE          = mcm_data_profile$(MV_MULTIPLE_MODEL_SUFFIX_LOWER).xml
MODEL_PROFILE         = mcm_model_profile$(MV_MULTIPLE_MODEL_SUFFIX_LOWER).txt
STORE_PROFILE         = mcm_store_profile$(MV_MULTIPLE_MODEL_SUFFIX_LOWER).txt
STORE_DEFAULT_PROFILE = mcm_store_profile_default$(MV_MULTIPLE_MODEL_SUFFIX_LOWER).txt
STORE_CURRENT_PROFILE = mcm_store_profile_current$(MV_MULTIPLE_MODEL_SUFFIX_LOWER).txt

AUTO_FILE_EACH =
AUTO_SOURCE_PATH_LIST =
AUTO_SOURCE_PATH_LIST += ../mcm_lib/mcm_lheader/mcm_data_exinfo$(MV_MULTIPLE_MODEL_SUFFIX_LOWER)_auto.h
AUTO_SOURCE_PATH_LIST += ../mcm_daemon$(MV_MULTIPLE_MODEL_SUFFIX_LOWER)/mcm_data_ininfo_auto.h
AUTO_SOURCE_PATH_LIST += ../mcm_daemon$(MV_MULTIPLE_MODEL_SUFFIX_LOWER)/mcm_data_info_auto.c
AUTO_SOURCE_PATH_LIST += ../mcm_lib/mcm_jslib/mcm_jslib_data_info$(MV_MULTIPLE_MODEL_SUFFIX_LOWER)_auto.js


binary : $(TARGET_FILE)

profile : $(MODEL_PROFILE) $(STORE_PROFILE)


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

$(MODEL_PROFILE) $(STORE_PROFILE) : $(DATA_PROFILE) $(TARGET_FILE)
	./$(TARGET_FILE) -d $(DATA_PROFILE) -m $(MODEL_PROFILE) -s $(STORE_PROFILE)
	@if [ ! -f "$(STORE_DEFAULT_PROFILE)" ]; then cp -f $(STORE_PROFILE) $(STORE_DEFAULT_PROFILE); fi

install :
	cp -f $(MODEL_PROFILE) $(MCM_RUN_ROOT_PATH)
	if [ -f "$(STORE_DEFAULT_PROFILE)" ]; then cp -f $(STORE_DEFAULT_PROFILE) $(MCM_RUN_ROOT_PATH); fi

clean :
	rm -f *.d *.d* *.o $(TARGET_FILE) $(MODEL_PROFILE) $(STORE_PROFILE)
	cd $(MCM_RUN_ROOT_PATH); rm -f $(MODEL_PROFILE) $(STORE_DEFAULT_PROFILE) $(STORE_CURRENT_PROFILE)
	for AUTO_FILE_EACH in $(AUTO_SOURCE_PATH_LIST); do echo "" > $$AUTO_FILE_EACH; done

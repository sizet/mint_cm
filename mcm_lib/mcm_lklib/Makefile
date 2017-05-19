# Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
# This file is part of the MintCM.
# Some rights reserved. See README.

CC_FLAGS = -Wall -Wno-strict-aliasing -Wno-unused-result

export EXTRA_CFLAGS = $(CC_FLAGS)

SOURCE_FILE = mcm_lklib_api.c
OBJECT_FILE = $(SOURCE_FILE:.c=.o)
TARGET_FILE = $(SOURCE_FILE:.c=.ko)

KERNEL_PATH = /lib/modules/$(shell uname -r)/build

obj-m := $(OBJECT_FILE)


binary : all


all:
	$(MAKE) CROSS_COMPILE=$(CROSS) -C $(KERNEL_PATH) M=$(PWD) modules

install :
	cp -f $(TARGET_FILE) $(MCM_RUN_ROOT_PATH)

clean:
	$(MAKE) -C $(KERNEL_PATH) M=$(PWD) clean
	rm -rf Module.symvers *.mod.c *.ko *.o *~
	cd $(MCM_RUN_ROOT_PATH); rm -f $(TARGET_FILE)

# Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
# This file is part of the MintCM.
# Some rights reserved. See README.

MCM_PATH       = $(PWD)/../../../../..
MCM_LKLIB_PATH = $(MCM_PATH)/mcm_lib/mcm_lklib

CC_FLAGS = -Wall -Wno-unused-result

INCLUDE_PATH = -I$(MCM_PATH)

export EXTRA_CFLAGS = $(CC_FLAGS) $(INCLUDE_PATH)

SOURCE_FILE = kernel_app_0303.c
OBJECT_FILE = $(SOURCE_FILE:.c=.o)
TARGET_FILE = $(SOURCE_FILE:.c=.ko)

KERNEL_PATH = /lib/modules/$(shell uname -r)/build

export KBUILD_EXTRA_SYMBOLS = $(MCM_LKLIB_PATH)/Module.symvers

obj-m := $(OBJECT_FILE)


exe : all


all:
	$(MAKE) -C $(KERNEL_PATH) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_PATH) M=$(PWD) clean
	rm -rf Module.symvers *.mod.c *.ko *.o *~

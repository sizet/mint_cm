# Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
# This file is part of the MintCM.
# Some rights reserved. See README.

include ../common/make.path


add :
	cp -f $(EXAMPLE_PROFILE_PATH)/mcm_data_profile_0302.xml $(MCM_BUILD_PATH)/$(DATA_PROFILE)
	cp -f $(EXAMPLE_PROFILE_PATH)/mcm_store_profile_default_0302.txt $(MCM_BUILD_PATH)/$(STORE_DEFAULT_PROFILE)
	cp -f $(EXAMPLE_MODULE_SOURCE_FILE1) $(MCM_MODULE_PATH)

del :
	cd $(MCM_BUILD_PATH); rm -f $(DATA_PROFILE) $(STORE_DEFAULT_PROFILE)
	cd $(MCM_MODULE_PATH); rm -f $(EXAMPLE_MODULE_SOURCE_FILE2)

# Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
# This file is part of the MintCM.
# Some rights reserved. See README.

include ../common/make.path


add :
	cp -f $(EXAMPLE_PROFILE_PATH)/mcm_data_profile_0802.xml $(MCM_BUILD_PATH)/$(DATA_PROFILE)
	cp -f $(EXAMPLE_PROFILE_PATH)/mcm_store_profile_default_0802.txt $(MCM_BUILD_PATH)/$(STORE_DEFAULT_PROFILE)
	cp -f $(EXAMPLE_PROFILE_PATH)/mcm_store_upload_0802.txt $(MCM_RUN_ROOT_PATH)/$(STORE_UPLOAD_PROFILE)

del :
	cd $(MCM_BUILD_PATH); rm -f $(DATA_PROFILE) $(STORE_DEFAULT_PROFILE)
	cd $(MCM_RUN_ROOT_PATH); rm -f $(STORE_UPLOAD_PROFILE)

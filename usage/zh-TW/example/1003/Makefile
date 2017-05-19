# Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
# This file is part of the MintCM.
# Some rights reserved. See README.

include ../common/make.path


add :
	cp -f $(EXAMPLE_PROFILE_PATH)/mcm_data_profile_1003.xml $(MCM_BUILD_PATH)/$(DATA_PROFILE)
	cp -f $(EXAMPLE_PROFILE_PATH)/mcm_store_profile_default_1003.txt $(MCM_BUILD_PATH)/$(STORE_DEFAULT_PROFILE)
	cp -f $(EXAMPLE_WEB_APP_SOURCE_FILE1) $(MCM_RUN_WEB_PATH)
	cp -f $(EXAMPLE_MINI_HTTPD_MAKE_FILE1) $(MINI_HTTPD_PATH)
	cp -f $(EXAMPLE_MAAM_BUILDIN_SOURCE_FILE1) $(MAAM_BUILDIN_PATH)

del :
	cd $(MCM_BUILD_PATH); rm -f $(DATA_PROFILE) $(STORE_DEFAULT_PROFILE)
	cd $(MCM_RUN_WEB_PATH); rm -f $(EXAMPLE_WEB_APP_SOURCE_FILE2)
	cp -f $(EXAMPLE_MINI_HTTPD_MAKE_FILE2) $(MINI_HTTPD_PATH)
	cp -f $(EXAMPLE_MAAM_BUILDIN_SOURCE_FILE2) $(MAAM_BUILDIN_PATH)

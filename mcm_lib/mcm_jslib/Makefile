# Copyright © 2017, Che-Wei Hsu <cwxhsu@gmail.com>
# This file is part of the MintCM.
# Some rights reserved. See README.

SOURCE_COMMON_FILE = jquery_main.js   \
                     jquery_form.js   \
                     json3.js         \
                     mcm_jslib_api.js

SOURCE_CUSTOM_FILE =  mcm_jslib_data_info$(MV_MULTIPLE_MODEL_SUFFIX_LOWER)_auto.js




install_common :
	cp -f $(SOURCE_COMMON_FILE) $(MCM_RUN_WEB_PATH)

install_custom :
	cp -f $(SOURCE_CUSTOM_FILE) $(MCM_RUN_WEB_PATH)

clean_common :
	cd $(MCM_RUN_WEB_PATH); rm -f $(SOURCE_COMMON_FILE)

clean_custom :
	cd $(MCM_RUN_WEB_PATH); rm -f $(SOURCE_CUSTOM_FILE)

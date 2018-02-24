# Makefile for mini_httpd

# CONFIGURE: If you are using a SystemV-based operating system, such as
# Solaris, you will need to uncomment this definition.
#MH_SYSV_LIBS = -lnsl -lsocket

# CONFIGURE: Some systems don't need -lcrypt, and indeed they get an
# error if you try to link with it.  If you get an error about libcrypt
# not found, try commenting out this definition.
MH_CRYPT_LIB = -lcrypt

# CONFIGURE: If you want to compile in support for https, uncomment these
# definitions.  You will need to have already built OpenSSL, available at
# http://www.openssl.org/  Make sure the MH_SSL_TREE definition points to the
# tree with your OpenSSL installation - depending on how you installed it,
# it may be in /usr/local instead of /usr/local/ssl.
#MH_SSL_TREE = /usr/local/ssl
#MH_SSL_DEFS = -DUSE_SSL
#MH_SSL_INC  = -I$(MH_SSL_TREE)/include
#MH_SSL_LIBS = -L$(MH_SSL_TREE)/lib -lssl -lcrypto

MCM_MODIFY = YES
ifeq (${MCM_MODIFY}, YES)
    MCM_DEFINE = -DMCM_MODIFY
endif

MINT_ACCOUNT_AUTH_MINIHTTPD = YES
ifeq (${MINT_ACCOUNT_AUTH_MINIHTTPD}, YES)
    MAAM_PATH         = mint_aam
    MAAM_BUILDIN_PATH = $(MAAM_PATH)/maam_buildin
    MAAM_LULIB_PATH   = $(MAAM_PATH)/maam_lib/maam_lulib
    MAAM_JSLIB_PATH   = $(MAAM_PATH)/maam_lib/maam_jslib

    MCM_PATH       = $(MCM_TOP_PATH)
    MCM_LULIB_PATH = $(MCM_PATH)/mcm_lib/mcm_lulib

    MAAM_DEFINE = -DMINT_ACCOUNT_AUTH_MINIHTTPD

    MAAM_BUILDIN_INCLUDE_PATH =
    MAAM_BUILDIN_LIBRARY_FILE =

    MAAM_BUILDIN_INCLUDE_PATH += -I$(MCM_PATH)

    ifeq (${USE_STATIC_MCM_LIB}, YES)
    MAAM_BUILDIN_LIBRARY_FILE += $(MCM_LULIB_PATH)/mcm_lulib_api.a
    else
    MAAM_BUILDIN_LIBRARY_FILE += -L$(MCM_LULIB_PATH) -lmcm_lulib_api
    endif

    MAAM_LULIB_CC_FLAGS = $(MAAM_DEFINE) $(CFLAGS) -fPIC -Wall -O2
    MAAM_LULIB_LD_FLAGS = $(LDFLAGS) -fPIC -shared -Wl,--no-undefined
    MAAM_LULIB_AR_FLAGS = -rs

    MAAM_SHARE_DEPEND_FILE = $(MAAM_BUILDIN_PATH)/maam_common.h

    MAAM_BUILDIN_DEPEND_FILE = $(MAAM_SHARE_DEPEND_FILE)         \
                               $(MAAM_BUILDIN_PATH)/maam_local.h

    MAAM_BUILDIN_SOURCE_FILE = $(MAAM_BUILDIN_PATH)/maam_handle.c \
                               $(MAAM_BUILDIN_PATH)/maam_md5.c    \
                               $(MAAM_BUILDIN_PATH)/maam_custom.c

    MAAM_BUILDIN_OBJECT_FILE = $(notdir $(patsubst %.c,%.o,$(MAAM_BUILDIN_SOURCE_FILE)))

    MAAM_LULIB_DEPEND_FILE = $(MAAM_SHARE_DEPEND_FILE)           \
                             $(MAAM_LULIB_PATH)/maam_lulib_api.h

    MAAM_LULIB_SOURCE_FILE = $(MAAM_LULIB_PATH)/maam_lulib_api.c

    MAAM_LULIB_OBJECT_FILE = $(notdir $(patsubst %.c,%.o,$(MAAM_LULIB_SOURCE_FILE)))

    MAAM_LULIB_TARGET_FILE_S = maam_lulib_api.a
    MAAM_LULIB_TARGET_FILE_D = libmaam_lulib_api.so

    MAAM_JSLIB_SOURCE_FILE = $(MAAM_JSLIB_PATH)/md5.js            \
                             $(MAAM_JSLIB_PATH)/jquery_cookie.js  \
                             $(MAAM_JSLIB_PATH)/maam_jslib_api.js
endif

MH_CC_DEFS  = $(MH_SSL_DEFS) $(MH_SSL_INC) $(MCM_DEFINE) $(MAAM_DEFINE) $(MAAM_BUILDIN_INCLUDE_PATH)
MH_CC_FLAGS = $(MH_CC_DEFS) $(CFLAGS) -Wall -Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wno-long-long -O2
MH_LD_FLAGS = $(LDFLAGS) -s
MH_LD_LIBS  = $(MH_CRYPT_LIB) $(MH_SSL_LIBS) $(MH_SYSV_LIBS) $(MAAM_BUILDIN_LIBRARY_FILE)


binary : mini_httpd htpasswd $(MAAM_LULIB_TARGET_FILE_S) $(MAAM_LULIB_TARGET_FILE_D)


mini_httpd : mini_httpd.o match.o tdate_parse.o $(MAAM_BUILDIN_OBJECT_FILE) mini_httpd.conf
	$(CC) $(MH_LD_FLAGS) mini_httpd.o match.o tdate_parse.o $(MAAM_BUILDIN_OBJECT_FILE) $(MH_LD_LIBS) -o mini_httpd

mini_httpd.o : mini_httpd.c version.h port.h match.h tdate_parse.h mime_encodings.h mime_types.h
	$(CC) $(MH_CC_FLAGS) -c mini_httpd.c

match.o : match.c match.h
	$(CC) $(MH_CC_FLAGS) -c match.c

tdate_parse.o : tdate_parse.c tdate_parse.h
	$(CC) $(MH_CC_FLAGS) -c tdate_parse.c

mime_encodings.h : mime_encodings.txt
	rm -f mime_encodings.h
	sed < mime_encodings.txt > mime_encodings.h \
		-e 's/#.*//' -e 's/[ 	]*$$//' -e '/^$$/d' \
		-e 's/[ 	][ 	]*/", 0, "/' -e 's/^/{ "/' -e 's/$$/", 0 },/'

mime_types.h : mime_types.txt
	rm -f mime_types.h
	sed < mime_types.txt > mime_types.h \
		-e 's/#.*//' -e 's/[ 	]*$$//' -e '/^$$/d' \
		-e 's/[ 	][ 	]*/", 0, "/' -e 's/^/{ "/' -e 's/$$/", 0 },/'

htpasswd : htpasswd.o
	$(CC) $(MH_LD_FLAGS) htpasswd.o $(MH_CRYPT_LIB) -o htpasswd

htpasswd.o : htpasswd.c
	$(CC) $(MH_CC_FLAGS) -c htpasswd.c

maam_handle.o : $(MAAM_BUILDIN_PATH)/maam_handle.c $(MAAM_BUILDIN_PATH)/maam_handle.h $(MAAM_BUILDIN_DEPEND_FILE)
	$(CC) $(MH_CC_FLAGS) -c $(MAAM_BUILDIN_PATH)/maam_handle.c

maam_md5.o : $(MAAM_BUILDIN_PATH)/maam_md5.c $(MAAM_BUILDIN_PATH)/maam_md5.h $(MAAM_BUILDIN_DEPEND_FILE)
	$(CC) $(MH_CC_FLAGS) -c $(MAAM_BUILDIN_PATH)/maam_md5.c

maam_custom.o : $(MAAM_BUILDIN_PATH)/maam_custom.c $(MAAM_BUILDIN_PATH)/maam_custom.h $(MAAM_BUILDIN_DEPEND_FILE)
	$(CC) $(MH_CC_FLAGS) -c $(MAAM_BUILDIN_PATH)/maam_custom.c

$(MAAM_LULIB_TARGET_FILE_S) $(MAAM_LULIB_TARGET_FILE_D) : $(MAAM_LULIB_OBJECT_FILE)
	$(AR) $(MAAM_LULIB_AR_FLAGS) $(MAAM_LULIB_TARGET_FILE_S) $^
	$(CC) $(MAAM_LULIB_LD_FLAGS) $^ -o $(MAAM_LULIB_TARGET_FILE_D)
	$(STRIP) $(MAAM_LULIB_TARGET_FILE_D)
	mv $(MAAM_LULIB_TARGET_FILE_S) $(MAAM_LULIB_TARGET_FILE_D) $(MAAM_LULIB_PATH)

maam_lulib_api.o : $(MAAM_LULIB_PATH)/maam_lulib_api.c $(MAAM_LULIB_PATH)/maam_lulib_api.h $(MAAM_LULIB_DEPEND_FILE)
	$(CC) $(MAAM_LULIB_CC_FLAGS) -c $(MAAM_LULIB_PATH)/maam_lulib_api.c

cert : mini_httpd.pem
mini_httpd.pem : mini_httpd.cnf
	openssl req -new -x509 -days 3650 -nodes -config mini_httpd.cnf -out mini_httpd.pem -keyout mini_httpd.pem
	openssl x509 -subject -dates -fingerprint -noout -in mini_httpd.pem
	chmod 600 mini_httpd.pem

install :
	cp -f mini_httpd mini_httpd.conf $(MCM_RUN_ROOT_PATH)
ifeq (${MINT_ACCOUNT_AUTH_MINIHTTPD}, YES)
	cp -f $(MAAM_LULIB_PATH)/$(MAAM_LULIB_TARGET_FILE_D) $(MCM_RUN_ROOT_PATH)
	cp -f $(MAAM_JSLIB_SOURCE_FILE) $(MCM_RUN_WEB_PATH)
endif

clean:
	rm -f mini_httpd mime_encodings.h mime_types.h htpasswd mini_httpd.rnd *.o core core.* *.core
ifeq (${MINT_ACCOUNT_AUTH_MINIHTTPD}, YES)
	rm -f $(MAAM_LULIB_PATH)/*.a $(MAAM_LULIB_PATH)/*.so
	cd $(MCM_RUN_ROOT_PATH); rm -f mini_httpd mini_httpd.conf $(MAAM_LULIB_TARGET_FILE_D)
	cd $(MCM_RUN_WEB_PATH); rm -f $(notdir $(MAAM_JSLIB_SOURCE_FILE))
endif

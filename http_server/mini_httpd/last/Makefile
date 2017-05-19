# Makefile for mini_httpd

# CONFIGURE: If you are using a SystemV-based operating system, such as
# Solaris, you will need to uncomment this definition.
#SYSV_LIBS = -lnsl -lsocket

# CONFIGURE: Some systems don't need -lcrypt, and indeed they get an
# error if you try to link with it.  If you get an error about libcrypt
# not found, try commenting out this definition.
CRYPT_LIB = -lcrypt

# CONFIGURE: If you want to compile in support for https, uncomment these
# definitions.  You will need to have already built OpenSSL, available at
# http://www.openssl.org/  Make sure the SSL_TREE definition points to the
# tree with your OpenSSL installation - depending on how you installed it,
# it may be in /usr/local instead of /usr/local/ssl.
#SSL_TREE = /usr/local/ssl
#SSL_DEFS = -DUSE_SSL
#SSL_INC  = -I$(SSL_TREE)/include
#SSL_LIBS = -L$(SSL_TREE)/lib -lssl -lcrypto

MCM_MODIFY = YES
ifeq (${MCM_MODIFY}, YES)
    MCM_DEFINE = -DMCM_MODIFY
endif

BINDIR = /usr/local/sbin
MANDIR = /usr/local/man

#CC     = cc
CDEFS   = $(SSL_DEFS) $(SSL_INC) $(MCM_DEFINE)
CFLAGS  = -O $(CDEFS) -Wall -Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wno-long-long
LDFLAGS = -s
LDLIBS  = $(CRYPT_LIB) $(SSL_LIBS) $(SYSV_LIBS)


binary : mini_httpd htpasswd


mini_httpd : mini_httpd.o match.o tdate_parse.o mini_httpd.conf
	$(CC) $(LDFLAGS) mini_httpd.o match.o tdate_parse.o $(LDLIBS) -o mini_httpd

mini_httpd.o : mini_httpd.c version.h port.h match.h tdate_parse.h mime_encodings.h mime_types.h
	$(CC) $(CFLAGS) -c mini_httpd.c

match.o : match.c match.h
	$(CC) $(CFLAGS) -c match.c

tdate_parse.o : tdate_parse.c tdate_parse.h
	$(CC) $(CFLAGS) -c tdate_parse.c

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
	$(CC) $(LDFLAGS) htpasswd.o $(CRYPT_LIB) -o htpasswd

htpasswd.o : htpasswd.c
	$(CC) $(CFLAGS) -c htpasswd.c

cert : mini_httpd.pem
mini_httpd.pem : mini_httpd.cnf
	openssl req -new -x509 -days 3650 -nodes -config mini_httpd.cnf -out mini_httpd.pem -keyout mini_httpd.pem
	openssl x509 -subject -dates -fingerprint -noout -in mini_httpd.pem
	chmod 600 mini_httpd.pem

install :
	cp -f mini_httpd mini_httpd.conf $(MCM_RUN_ROOT_PATH)

clean:
	rm -f mini_httpd mime_encodings.h mime_types.h htpasswd mini_httpd.rnd *.o core core.* *.core
	cd $(MCM_RUN_ROOT_PATH); rm -f mini_httpd mini_httpd.conf

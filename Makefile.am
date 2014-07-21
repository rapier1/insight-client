AM_CFLAGS= -g -Wall -I/usr/include/mysql
AM_LDFLAGS= -L/usr/lib64/mysql -lestats -ljson-c -lwebsock -lmaxminddb -lmysqlclient
bin_PROGRAMS = insight
insight_SOURCES= main.c parse.c report.c string-funcs.c geoip.c debug.c \
                 parse.h scripts.h main.h string-funcs.h report.h geoip.h \
		 debug.h uthash.h version.h
EXTRA_DIST = dependencies geoipdb

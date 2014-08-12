if LINK_LIBJSON
  USE_LIBJSON=-llibjson
endif
if LINK_LIBJSONC
  USE_LIBJSON=-ljson-c
endif
AM_CFLAGS= -g -O3 -Wall -I/usr/include/mysql -std=gnu99 -fgnu89-inline
AM_LDFLAGS= -L/usr/lib64/mysql -lestats -lwebsock -lmaxminddb -lmysqlclient $(USE_LIBJSON)
bin_PROGRAMS = insight
insight_SOURCES= main.c parse.c report.c string-funcs.c geoip.c debug.c \
                 parse.h scripts.h main.h string-funcs.h report.h geoip.h \
		 debug.h uthash.h version.h
EXTRA_DIST = dependencies geoipdb

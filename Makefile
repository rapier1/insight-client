CC=gcc
CFLAGS= -g -c -Wall -I/usr/include/mysql
LDFLAGS= -L/usr/lib64/mysql -lestats -ljson-c -lwebsock -lmaxminddb -lmysqlclient
SOURCES= main.c parse.c report.c string-funcs.c geoip.c debug.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE = insight


all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.o:
	$(CC) $(CFLAGS) $< -o $@

clean: 
	rm -f *.o $(EXECUTABLE)

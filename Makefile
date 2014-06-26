CC=gcc
CFLAGS= -g -c -Wall 
LDFLAGS= -lestats -ljson-c -lwebsock
SOURCES= main.c parse.c report.c string-funcs.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE = insight


all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.o:
	$(CC) $(CFLAGS) $< -o $@

clean: 
	rm -f *.o $(EXECUTABLE)

CC=gcc -g 
OFED_PATH=/usr
DEFAULT_CFLAGS=-I${OFED_PATH}/include 
DEFAULT_LDFLAGS=-L${OFED_PATH}/lib64 -L${OFED_PATH}/lib 

LDFLAGS+=$(DEFAULT_LDFLAGS) -libverbs 
OBJECTS =hello.o sock.o get_clock.o 
TARGETS=hello 

all:${TARGETS}

hello:hello.o sock.o get_clock.o
	$(CC) $^ -o $@ $(LDFLAGS)

hello.o:hello.c sock.h get_clock.h
	$(CC) -c $(CFLAGS) $<

sock.o: sock.c sock.h
	$(CC) -c $(CFLAGS) $<

get_clock.o: get_clock.c get_clock.h
	$(CC) -c $(CFLAGS) $<

clean:
	rm -f $(OBJECTS) $(TARGETS)

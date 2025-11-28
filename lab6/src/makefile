CC = gcc
LDFLAGS = -lpthread

all: client server

libcommon.a: common.o
	ar rcs libcommon.a common.o

common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c -o common.o

client: client.o libcommon.a
	$(CC) client.o -L. -lcommon $(LDFLAGS) -o client

client.o: client.c common.h
	$(CC) $(CFLAGS) -c client.c -o client.o

server: server.o libcommon.a
	$(CC) server.o -L. -lcommon $(LDFLAGS) -o server

server.o: server.c common.h
	$(CC) $(CFLAGS) -c server.c -o server.o

clean:
	rm -f *.o *.a client server

rebuild: clean all

test: all
	@echo "Starting servers..."
	@./server --port 20001 --tnum 4 &
	@./server --port 20002 --tnum 4 &
	@sleep 2
	@echo "Starting client..."
	@echo "127.0.0.1:20001" > servers.txt
	@echo "127.0.0.1:20002" >> servers.txt
	@./client --k 10 --mod 1000000007 --servers servers.txt
	@killall server 2>/dev/null || true

.PHONY: all clean rebuild test
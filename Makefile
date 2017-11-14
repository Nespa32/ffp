CC=gcc
CFLAGS=-std=c11 -g -Wall -Og -flto
LFLAGS=-pthread -lpthread -L`jemalloc-config --libdir` -Wl,-rpath,`jemalloc-config --libdir` -ljemalloc `jemalloc-config --libs` -static

all: bench

bench: bench.o ffp.o
	$(CC) $(CFLAGS) bench.o ffp.o $(LFLAGS) -I. -o bench

bench.o: bench2.c
	$(CC) -c bench.c -I. $(CFLAGS) $(LFLAGS)

ffp.o: ffp.c
	$(CC) -c ffp.c -I. $(CFLAGS) $(LFLAGS)

clean:
	rm *.o bench
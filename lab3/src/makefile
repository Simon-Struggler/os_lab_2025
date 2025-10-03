CC=gcc
CFLAGS=-I.

# Default target - build all programs
all: sequential_min_max parallel_min_max

# Sequential program
sequential_min_max: utils.o find_min_max.o sequential_min_max.o
	$(CC) -o sequential_min_max utils.o find_min_max.o sequential_min_max.o $(CFLAGS)

# Parallel program
parallel_min_max: utils.o find_min_max.o parallel_min_max.o
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.o $(CFLAGS)

# Object files
utils.o: utils.c utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o: find_min_max.c find_min_max.h utils.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

sequential_min_max.o: sequential_min_max.c find_min_max.h utils.h
	$(CC) -o sequential_min_max.o -c sequential_min_max.c $(CFLAGS)

parallel_min_max.o: parallel_min_max.c find_min_max.h utils.h
	$(CC) -o parallel_min_max.o -c parallel_min_max.c $(CFLAGS)

# Clean target
clean:
	rm -f *.o sequential_min_max parallel_min_max

# Phony targets (not real files)
.PHONY: all clean
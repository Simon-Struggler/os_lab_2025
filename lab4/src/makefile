CC=gcc
CFLAGS=-I.
LDFLAGS=-pthread

# Default target - build all programs
all: process_memory parallel_min_max parallel_sum

process_memory: process_memory.c
	gcc -o process_memory process_memory.c

parallel_min_max: parallel_min_max.c find_min_max.c utils.c
	$(CC) -o parallel_min_max parallel_min_max.c find_min_max.c utils.c

parallel_sum: utils.c sum.c parallel_sum.c
	$(CC) -o parallel_sum utils.c sum.c parallel_sum.c $(CFLAGS) $(LDFLAGS)

# Clean target
clean:
	rm -f *.o  parallel_min_max zombie_demo parallel_sum

# Phony targets (not real files)
.PHONY: all clean
CC=gcc
CFLAGS=-I.

# Default target - build all programs
all: process_memory parallel_min_max 

process_memory: process_memory.c
	gcc -o process_memory process_memory.c

parallel_min_max: parallel_min_max.c find_min_max.c utils.c
	$(CC) -o parallel_min_max parallel_min_max.c find_min_max.c utils.c
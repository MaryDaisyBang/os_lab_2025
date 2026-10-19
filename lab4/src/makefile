CC=gcc
CFLAGS=-I.

LAB3_SRC = ../../lab3/src

.PHONY: all clean

all : parallel_sum

process_memory : process_memory.c
	$(CC) -o process_memory process_memory.c $(CFLAGS)


parallel_sum : parallel_sum.c utils_lab3.o sum_lib.o
	$(CC) -o parallel_sum parallel_sum.c utils_lab3.o sum_lib.o $(CFLAGS) -lpthread

# Собираем utils.o из lab3
utils_lab3.o : $(LAB3_SRC)/utils.c $(LAB3_SRC)/utils.h
	$(CC) -o utils_lab3.o -c $(LAB3_SRC)/utils.c $(CFLAGS)

# Собираем нашу библиотеку суммы
sum_lib.o : sum_lib.c sum_lib.h
	$(CC) -o sum_lib.o -c sum_lib.c $(CFLAGS)

clean :
	rm -f parallel_sum utils_lab3.o sum_lib.o process_memory
JUNK =  *.o

CC = gcc
CFLAGS = -g -Wall -Wno-unused -Wno-sign-compare

all:  sim


cache.o:  cache.c cache.h
	$(CC) $(CFLAGS) -c cache.c

main.o:  main.c main.h
	$(CC) $(CFLAGS) -c main.c

sim:  main.o cache.o
	$(CC) -o sim main.o cache.o -lm


.PHONY: clean

clean:
	rm -f $(JUNK) ./sim

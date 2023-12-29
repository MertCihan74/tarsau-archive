# tarsau-archive

CC = gcc
CFLAGS = -Wall

all: tarsau

tarsau: tarsau.o
	$(CC) $(CFLAGS) -o tarsau tarsau.o

tarsau.o: tarsau.c
	$(CC) $(CFLAGS) -c tarsau.c

clean:
	rm -f tarsau tarsau.o

.PHONY: all clean

run-example: all
	./tarsau -b t1 t2 t3 t4.txt t5.dat -o s1.sau
	./tarsau -a s1.sau d1

.PHONY: run-example

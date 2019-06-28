# gcc -g -std=c99 input.c my_ht.c my_rb.c -o common_finder -lpthread -lck

CC=gcc
CFLAGS=-g -std=c99 -Werror -lpthread -lck
DEPS=my_ht.h my_rb.h
OBJ=input.o my_ht.o my_rb.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

common_finder: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm ./*.o ./common_finder

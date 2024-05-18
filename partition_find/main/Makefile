CC=gcc
CFLAGS=-I.
DEPS = nvm.h storage.h log.h
OBJ = test.c nvm_file.c storage.c

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
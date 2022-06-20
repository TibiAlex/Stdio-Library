CC = gcc
CFLAGS = -Wall -fPIC -g
LIBFLAGS = -shared

.PHONY: build clean

build: libso_stdio.so

libso_stdio.so: tema2SO.c
	$(CC) $(CFLAGS) -c tema2SO.c
	$(CC) $(LIBFLAGS) tema2SO.o -o libso_stdio.so

clean: tema2SO.o libso_stdio.so
	rm tema2SO.o libso_stdio.so
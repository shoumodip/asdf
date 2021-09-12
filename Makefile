LIBS = -lncurses
CFLAGS = -Wall -Wextra -std=c11 -pedantic

asdf: asdf.c
	$(CC) -o asdf asdf.c $(CFLAGS) $(LIBS)

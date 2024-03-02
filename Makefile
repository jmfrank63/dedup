CFLAGS=-Wall -Wextra -Werror -std=c11 -pedantic -ggdb
LIBS=

dedup: dedup.c
	$(CC) $(CFLAGS) dedup.c -o dedup $(LIBS)

clean:
	rm -f dedup

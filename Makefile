CFLAGS=-Wall -Wextra -Werror -std=c11 -pedantic -ggdb
LIBS=

dedup: src/dedup.c
	$(CC) $(CFLAGS) src/dedup.c -o dedup $(LIBS)

clean:
	rm -f dedup

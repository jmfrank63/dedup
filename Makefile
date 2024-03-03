CFLAGS=-Wall -Wextra -Werror -std=c11 -pedantic -ggdb -mavx -mavx2 -msse4.1 -mavx512f -mavx512vl
LIBS=
INCLUDES=-Isubmodules/BLAKE3/c

dedup: src/dedup.c
	$(CC) $(CFLAGS) src/dedup.c submodules/BLAKE3/c/blake3.c submodules/BLAKE3/c/blake3_dispatch.c submodules/BLAKE3/c/blake3_portable.c submodules/BLAKE3/c/blake3_sse2.c submodules/BLAKE3/c/blake3_sse41.c submodules/BLAKE3/c/blake3_avx2.c submodules/BLAKE3/c/blake3_avx512.c -o dedup $(INCLUDES) $(LIBS) 

clean:
	rm -f dedup

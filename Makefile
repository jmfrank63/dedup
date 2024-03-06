CFLAGS=-Wall -Wextra -Werror -std=c11 -pedantic \
	   -ggdb -mavx -mavx2 -msse4.1 -mavx512f -mavx512vl
LIBS=-Lsubmodules/criterion/build/src
INCLUDES=-Isubmodules/BLAKE3/c -Isubmodules/criterion/include

dedup: src/dedup.c lib/ring_buffer.c
	@$(CC) $(CFLAGS) \
	src/dedup.c \
	lib/ring_buffer.c \
	submodules/BLAKE3/c/blake3.c \
	submodules/BLAKE3/c/blake3_dispatch.c \
	submodules/BLAKE3/c/blake3_portable.c \
	submodules/BLAKE3/c/blake3_sse2.c \
	submodules/BLAKE3/c/blake3_sse41.c \
	submodules/BLAKE3/c/blake3_avx2.c \
	submodules/BLAKE3/c/blake3_avx512.c \
	-o dedup $(INCLUDES) $(LIBS) 

clean:
	@rm -f dedup

clean_all:
	@rm -f dedup test_ring_buffer

all: dedup test

run_tests: test
	@LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):submodules/criterion/build/src ./test_ring_buffer

.PHONY: criterion
criterion:
	@if [ ! -d "submodules/criterion/build" ]; then \
        cd submodules/criterion && meson build && cd build && ninja; \
    fi

test: tests/test_ring_buffer.c criterion
	@$(CC) $(CFLAGS) \
    lib/ring_buffer.c \
    tests/test_ring_buffer.c \
    -o test_ring_buffer \
    $(INCLUDES) $(LIBS) -lcriterion

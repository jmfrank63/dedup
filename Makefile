CFLAGS=-Wall -Wextra -Werror -std=c11 -pedantic \
	   -ggdb -mavx -mavx2 -msse4.1 \
	   -DBLAKE3_NO_AVX512
CFLAGS_RELEASE=-Wall -Wextra -Werror -std=c11 -pedantic \
       -O3 -mavx -mavx2 -msse4.1 \
	   -DBLAKE3_NO_AVX512
LIBS=-Lsubmodules/criterion/build/src
INCLUDES=-Isubmodules/BLAKE3/c -Isubmodules/criterion/include -Isubmodules/uthash/src

SRC_FILES=src/dedup.c
LIB_SRC_FILES=src/lib/ring_buffer.c src/lib/hashing.c
MODULE_SRC_FILES=submodules/BLAKE3/c/blake3.c submodules/BLAKE3/c/blake3_dispatch.c \
	submodules/BLAKE3/c/blake3_portable.c submodules/BLAKE3/c/blake3_sse2.c \
	submodules/BLAKE3/c/blake3_sse41.c submodules/BLAKE3/c/blake3_avx2.c
TEST_SRC_FILES=tests/test_ring_buffer.c

dedup: $(SRC_FILES) $(LIB_SRC_FILES) $(MODULE_SRC_FILES)
	@$(CC) $(CFLAGS) $(SRC_FILES) $(LIB_SRC_FILES) $(MODULE_SRC_FILES) -o dedup $(INCLUDES) $(LIBS)

dedup_release: $(SRC_FILES) $(LIB_SRC_FILES) $(MODULE_SRC_FILES)
	@$(CC) $(CFLAGS_RELEASE) $(SRC_FILES) $(LIB_SRC_FILES) $(MODULE_SRC_FILES) -o dedup $(INCLUDES) $(LIBS)

clean:
	@rm -f dedup

clean_all:
	@rm -f dedup test_*

all: dedup tests

run: dedup
	@echo "Running dedup..." && \
	./dedup $(ARG)

run_tests: tests
	@echo "Running ring_buffer tests..." && \
    LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):submodules/criterion/build/src ./test_ring_buffer

.PHONY: criterion
criterion:
	@if [ ! -d "submodules/criterion/build" ]; then \
        cd submodules/criterion && meson build && cd build && ninja; \
    fi

.PHONY: memcheck
memcheck: dedup
	valgrind --leak-check=full ./dedup $(ARG)

tests: $(TEST_SRC_FILES) $(LIB_SRC_FILES) criterion
	@$(CC) $(CFLAGS) \
	$(LIB_SRC_FILES) \
    $(INCLUDES) $(LIBS) -lcriterion \
	tests/test_ring_buffer.c \
    -o test_ring_buffer

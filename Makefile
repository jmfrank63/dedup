CFLAGS=-Wall -Wextra -Werror -std=c11 -pedantic \
	   -ggdb -mavx -mavx2 -msse4.1 -mavx512f -mavx512vl
LIBS=-Lsubmodules/criterion/build/src
INCLUDES=-Isubmodules/BLAKE3/c -Isubmodules/criterion/include

SRC_FILES=src/dedup.c
LIB_SRC_FILES=src/lib/ring_buffer.c
MODULE_SRC_FILES=submodules/BLAKE3/c/blake3.c submodules/BLAKE3/c/blake3_dispatch.c \
	submodules/BLAKE3/c/blake3_portable.c submodules/BLAKE3/c/blake3_sse2.c \
	submodules/BLAKE3/c/blake3_sse41.c submodules/BLAKE3/c/blake3_avx2.c \
	submodules/BLAKE3/c/blake3_avx512.c
TEST_SRC_FILES=tests/test_ring_buffer.c

dedup: $(SRC_FILES) $(LIB_SRC_FILES) $(MODULE_SRC_FILES)
	@$(CC) $(CFLAGS) $(SRC_FILES) $(LIB_SRC_FILES) $(MODULE_SRC_FILES) -o dedup $(INCLUDES) $(LIBS)

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

tests: $(TEST_SRC_FILES) $(LIB_SRC_FILES) criterion
	@$(CC) $(CFLAGS) \
	$(LIB_SRC_FILES) \
    $(INCLUDES) $(LIBS) -lcriterion \
	tests/test_ring_buffer.c \
    -o test_ring_buffer

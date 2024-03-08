CFLAGS=-Wall -Wextra -Werror -std=c11 -pedantic \
	   -ggdb -mavx -mavx2 -msse4.1 -mavx512f -mavx512vl
LIBS=-Lsubmodules/criterion/build/src
INCLUDES=-Isubmodules/BLAKE3/c -Isubmodules/criterion/include

SRC_FILES=src/dedup.c
LIB_SRC_FILES=lib/ring_buffer.c lib/stack.c lib/stack_list.c
MODULE_SRC_FILES=submodules/BLAKE3/c/blake3.c submodules/BLAKE3/c/blake3_dispatch.c \
	submodules/BLAKE3/c/blake3_portable.c submodules/BLAKE3/c/blake3_sse2.c \
	submodules/BLAKE3/c/blake3_sse41.c submodules/BLAKE3/c/blake3_avx2.c \
	submodules/BLAKE3/c/blake3_avx512.c
TEST_SRC_FILES=tests/test_ring_buffer.c tests/test_stack.c tests/test_stack_list.c

dedup: $(SRC_FILES) $(LIB_SRC_FILES) $(MODULE_SRC_FILES)
	@$(CC) $(CFLAGS) $(SRC_FILES) $(LIB_SRC_FILES) $(MODULE_SRC_FILES) -o dedup $(INCLUDES) $(LIBS)

clean:
	@rm -f dedup

clean_all:
	@rm -f dedup test_ring_buffer test_stack test_stack_list

all: dedup tests

run_tests: tests
	@echo "Running ring_buffer tests..." && \
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):submodules/criterion/build/src ./test_ring_buffer && \
	echo "Running stack tests..." && \
    LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):submodules/criterion/build/src ./test_stack && \
	echo "Running stack_list tests..." && \
    LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):submodules/criterion/build/src ./test_stack_list

.PHONY: criterion
criterion:
	@if [ ! -d "submodules/criterion/build" ]; then \
        cd submodules/criterion && meson build && cd build && ninja; \
    fi

tests: $(TEST_SRC_FILES) $(LIB_SRC_FILES) criterion
	@$(CC) $(CFLAGS) \
    lib/ring_buffer.c \
    tests/test_ring_buffer.c \
    -o test_ring_buffer \
    $(INCLUDES) $(LIBS) -lcriterion
	@$(CC) $(CFLAGS) \
    lib/stack.c \
	lib/stack_list.c \
    tests/test_stack.c \
    -o test_stack \
    $(INCLUDES) $(LIBS) -lcriterion
	@$(CC) $(CFLAGS) \
	lib/stack.c \
    lib/stack_list.c \
    tests/test_stack_list.c \
    -o test_stack_list \
    $(INCLUDES) $(LIBS) -lcriterion

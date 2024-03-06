#include <criterion/criterion.h>
#include <string.h>
#include "../lib/ring_buffer.h"

Test(ring_buffer, enqueue_dequeue) {
    RingBuffer* ring_buffer = ring_buffer_init();

    char* strings[] = {"Hello", "World", "Test", "Ring", "Buffer"};
    for (int i = 0; i < 5; i++) {
        ring_buffer_enqueue(ring_buffer, strings[i]);
    }

    for (int i = 0; i < 5; i++) {
        char* str = ring_buffer_dequeue(ring_buffer);
        cr_assert_str_eq(str, strings[i], "Expected %s, but got %s", strings[i], str);
    }

    ring_buffer_free(ring_buffer);
}

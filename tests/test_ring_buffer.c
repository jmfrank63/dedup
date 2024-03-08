#include "../lib/ring_buffer.h"
#include "criterion/assert.h"
#include "criterion/internal/assert.h"
#include <criterion/criterion.h>
#include <pthread.h>
#include <stdio.h>

void *write_to_buffer(void *arg) {
    RingBuffer *buffer = (RingBuffer *)arg;
    for (int i = 0; i < 3; i++) {
        char *elem = malloc(20);
        sprintf(elem, "test%d", i + 1);
        write_ring_buffer(buffer, elem);
    }
    return NULL;
}

void *read_from_buffer(void *arg) {
    RingBuffer *buffer = (RingBuffer *)arg;
    pthread_mutex_lock(&buffer->mutex);
    while (!buffer->full) {
        pthread_cond_wait(&buffer->cond,
                          &buffer->mutex); // wait for buffer to be full
    }
    pthread_mutex_unlock(&buffer->mutex);
    char *elem = read_ring_buffer(buffer);
    free(elem); // free the element after reading it
    return NULL;
}

Test(ring_buffer, write_blocks_when_full) {
    RingBuffer *buffer = create_ring_buffer(2);

    pthread_t write_thread, read_thread;
    pthread_create(&write_thread, NULL, write_to_buffer, buffer);
    pthread_create(&read_thread, NULL, read_from_buffer, buffer);

    pthread_join(write_thread, NULL);
    pthread_join(read_thread, NULL);

    cr_assert_eq(buffer->start, 1,
                 "Buffer start was not updated correctly, expected 1 got %d",
                 buffer->start);

    cr_assert_eq(buffer->end, 1,
                 "Buffer end was not updated correctly, expected 1 got %d",
                 buffer->end);

    cr_assert_str_eq(
        buffer->elems[buffer->start], "test2",
        "Element 1 was not written correctly, expected test3 got %s",
        buffer->elems[buffer->start]);

    cr_assert_str_eq(
        buffer->elems[(buffer->start + 1) % buffer->size], "test3",
        "Element 0 was not written correctly, expected test2 got %s",
        buffer->elems[(buffer->start + 1) % buffer->size]);

    destroy_ring_buffer(buffer);
}

Test(ring_buffer, create_and_destroy) {
    RingBuffer *buffer = create_ring_buffer(2);
    cr_assert_not_null(buffer, "Buffer was not created");
    cr_assert_null(buffer->elems[0],
                   "Buffer elements should be initialized to NULL");
    cr_assert_null(buffer->elems[1],
                   "Buffer elements should be initialized to NULL");
    destroy_ring_buffer(buffer);
}

Test(ring_buffer, write_single_element) {
    RingBuffer *buffer = create_ring_buffer(2);

    write_ring_buffer(buffer, "test1");

    cr_assert_str_eq(buffer->elems[0], "test1",
                     "First element was not written correctly");

    destroy_ring_buffer(buffer);
}

Test(ring_buffer, write_two_elements) {
    RingBuffer *buffer = create_ring_buffer(2);

    write_ring_buffer(buffer, "test1");
    write_ring_buffer(buffer, "test2");

    cr_assert_str_eq(buffer->elems[1], "test2",
                     "Second element was not written correctly");

    cr_assert(buffer->full,
              "Buffer not marked as full after writing 2 elements");

    destroy_ring_buffer(buffer);
}

Test(ring_buffer, read_single_element) {
    RingBuffer *buffer = create_ring_buffer(2);

    write_ring_buffer(buffer, "test1");
    write_ring_buffer(buffer, "test2");

    cr_assert_str_eq(read_ring_buffer(buffer), "test1",
                     "First element was not read correctly");

    cr_assert_not(buffer->full,
                  "Buffer should not be marked full after reading one element");

    destroy_ring_buffer(buffer);
}

Test(ring_buffer, write_after_read) {
    RingBuffer *buffer = create_ring_buffer(2);

    write_ring_buffer(buffer, "test1");
    write_ring_buffer(buffer, "test2");
    read_ring_buffer(buffer);
    write_ring_buffer(buffer, "test3");

    cr_assert(buffer->full,
              "Buffer not marked as full after writing second elements again");

    destroy_ring_buffer(buffer);
}

Test(ring_buffer, read_second_element) {
    RingBuffer *buffer = create_ring_buffer(2);

    write_ring_buffer(buffer, "test1");
    write_ring_buffer(buffer, "test2");
    read_ring_buffer(buffer);
    write_ring_buffer(buffer, "test3");
    cr_assert_str_eq(read_ring_buffer(buffer), "test2",
                     "Second element was not read correctly");
    cr_assert_str_eq(read_ring_buffer(buffer), "test3",
                     "Third element was not read correctly");
    destroy_ring_buffer(buffer);
}

Test(ring_buffer, buffer_not_full_initially) {
    RingBuffer *buffer = create_ring_buffer(2);

    cr_assert_not(buffer->full,
                  "Buffer should not be marked as full initially");

    destroy_ring_buffer(buffer);
}

Test(ring_buffer, buffer_full_after_two_writes) {
    RingBuffer *buffer = create_ring_buffer(2);

    write_ring_buffer(buffer, "test1");
    write_ring_buffer(buffer, "test2");

    cr_assert(buffer->full,
              "Buffer should be marked as full after writing 2 elements");

    destroy_ring_buffer(buffer);
}

Test(ring_buffer, buffer_not_full_after_read) {
    RingBuffer *buffer = create_ring_buffer(2);

    write_ring_buffer(buffer, "test1");
    write_ring_buffer(buffer, "test2");
    read_ring_buffer(buffer);

    cr_assert_not(
        buffer->full,
        "Buffer should not be marked as full after reading one element");

    destroy_ring_buffer(buffer);
}

Test(ring_buffer, buffer_full_after_write_read_write) {
    RingBuffer *buffer = create_ring_buffer(2);

    write_ring_buffer(buffer, "test1");
    write_ring_buffer(buffer, "test2");
    read_ring_buffer(buffer);
    write_ring_buffer(buffer, "test3");

    cr_assert(buffer->full, "Buffer should be marked as full after writing, "
                            "reading, and writing again");

    destroy_ring_buffer(buffer);
}

Test(ring_buffer, test_get_free_space) {
    RingBuffer *buffer = create_ring_buffer(5);
    cr_assert_eq(get_free_space(buffer), 5, "After initialization, free space should be equal to buffer size");
    write_ring_buffer(buffer, "test");
    cr_assert_eq(get_free_space(buffer), 4, "After writing one element, free space should decrease by 1");
    clear_buffer(buffer);
    cr_assert_eq(get_free_space(buffer), 5, "After clearing, free space should be equal to buffer size");
    destroy_ring_buffer(buffer);
}

Test(ring_buffer, test_is_buffer_full) {
    RingBuffer *buffer = create_ring_buffer(2);
    cr_assert_eq(is_buffer_full(buffer), false, "After initialization, buffer should not be full");
    write_ring_buffer(buffer, "test");
    cr_assert_eq(is_buffer_full(buffer), false, "After writing one element to a buffer of size 2, buffer should not be full");
    write_ring_buffer(buffer, "test");
    cr_assert_eq(is_buffer_full(buffer), true, "After writing two elements to a buffer of size 2, buffer should be full");
    clear_buffer(buffer);
    cr_assert_eq(is_buffer_full(buffer), false, "After clearing, buffer should not be full");
    destroy_ring_buffer(buffer);
}

Test(ring_buffer, test_clear_buffer) {
    RingBuffer *buffer = create_ring_buffer(2);
    write_ring_buffer(buffer, "test");
    write_ring_buffer(buffer, "test");
    cr_assert_eq(is_buffer_full(buffer), true, "After writing two elements to a buffer of size 2, buffer should be full");
    clear_buffer(buffer);
    cr_assert_eq(is_buffer_full(buffer), false, "After clearing, buffer should not be full");
    cr_assert_eq(get_free_space(buffer), 2, "After clearing, free space should be equal to buffer size");
    destroy_ring_buffer(buffer);
}

Test(ring_buffer, test_is_buffer_empty) {
    RingBuffer *buffer = create_ring_buffer(2);
    cr_assert(is_buffer_empty(buffer), "After initialization, buffer should be empty");
    write_ring_buffer(buffer, "test");
    cr_assert_not(is_buffer_empty(buffer), "After writing an element, buffer should not be empty");
    read_ring_buffer(buffer);
    cr_assert(is_buffer_empty(buffer), "After reading the only element, buffer should be empty");
    destroy_ring_buffer(buffer);
}

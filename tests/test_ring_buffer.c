#define _POSIX_C_SOURCE 200809L

#include "../src/lib/ring_buffer.h"
#include "criterion/assert.h"
#include "criterion/internal/assert.h"
#include <criterion/criterion.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

struct timespec timeout;

void setup_timeout(void) {
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 5;  // Wait up to 5 seconds
}

void *write_to_buffer(void *arg) {
    RingBuffer *buffer = (RingBuffer *)arg;
    for (int i = 0; i < 3; i++) {
        char *filename = malloc(20);
        sprintf(filename, "test%d", i + 1);
        write_ring_buffer(buffer, "path", filename);
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
    char *elem = read_ring_buffer(buffer, &timeout);
    char *elem_copy = strdup(elem);
    free_ring_buffer(buffer);
    // free(elem); // free the element after reading it
    return elem_copy;
}

TestSuite(ring_buffer, .init = setup_timeout);

Test(ring_buffer, write_blocks_when_full) {
    RingBuffer *buffer = create_ring_buffer(2);

    pthread_t write_thread, read_thread;
    pthread_create(&write_thread, NULL, write_to_buffer, buffer);
    pthread_create(&read_thread, NULL, read_from_buffer, buffer);

    pthread_join(write_thread, NULL);
    char *elem = NULL;
    pthread_join(read_thread, (void **)&elem);

    cr_assert_str_eq(elem, "path/test1", "First element was not read correctly, expected 'path/test1', got %s", elem);
    free(elem);
    cr_assert_eq(buffer->start, 1,
                 "Buffer start was not updated correctly, expected 1 got %d",
                 buffer->start);

    cr_assert_eq(buffer->end, 1,
                 "Buffer end was not updated correctly, expected 1 got %d",
                 buffer->end);

    cr_assert_str_eq(
        buffer->elems[buffer->start], "path/test2",
        "Element 1 was not written correctly, expected 'path/test2' got %s",
        buffer->elems[buffer->start]);

    cr_assert_str_eq(
        buffer->elems[(buffer->start + 1) % buffer->size], "path/test3",
        "Element 0 was not written correctly, expected 'path/test3' got %s",
        buffer->elems[(buffer->start + 1) % buffer->size]);

    destroy_ring_buffer(buffer);
}

Test(ring_buffer, create_and_destroy) {
    RingBuffer *buffer = create_ring_buffer(2);
    cr_assert_not_null(buffer, "Buffer was not created");
    cr_assert_not_null(buffer->elems[0],
                   "Memory for buffer elements was not allocated");
    cr_assert_not_null(buffer->elems[1],
                   "Memory for buffer elements was not allocated");
    destroy_ring_buffer(buffer);
}

Test(ring_buffer, write_single_element) {
    RingBuffer *buffer = create_ring_buffer(2);

    write_ring_buffer(buffer, "path", "test1");

    cr_assert_str_eq(buffer->elems[0], "path/test1",
                     "First element was not written correctly, expected 'path/test1' got %s", buffer->elems[0]);

    destroy_ring_buffer(buffer);
}

Test(ring_buffer, write_two_elements) {
    RingBuffer *buffer = create_ring_buffer(2);

    write_ring_buffer(buffer, "path", "test1");
    write_ring_buffer(buffer, "path", "test2");

    cr_assert_str_eq(buffer->elems[1], "path/test2",
                     "Second element was not written correctly, expected 'path/test2' got %s", buffer->elems[1]);

    cr_assert(buffer->full,
              "Buffer not marked as full after writing 2 elements");

    destroy_ring_buffer(buffer);
}

Test(ring_buffer, read_single_element) {
    RingBuffer *buffer = create_ring_buffer(2);

    write_ring_buffer(buffer, "path", "test1");
    write_ring_buffer(buffer, "path", "test2");

    char *path = read_ring_buffer(buffer, &timeout);
    cr_assert_str_eq(path, "path/test1",
                     "First element was not read correctly, , expected 'path/test1' got %s", path);
    free_ring_buffer(buffer);
    cr_assert_not(buffer->full,
                  "Buffer should not be marked full after reading one element");

    destroy_ring_buffer(buffer);
}

Test(ring_buffer, write_after_read) {
    RingBuffer *buffer = create_ring_buffer(2);

    write_ring_buffer(buffer, "path", "test1");
    write_ring_buffer(buffer, "path", "test2");
    read_ring_buffer(buffer, &timeout);
    free_ring_buffer(buffer);
    write_ring_buffer(buffer, "path", "test3");

    cr_assert(buffer->full,
              "Buffer not marked as full after writing second elements again");

    destroy_ring_buffer(buffer);
}

Test(ring_buffer, read_second_element) {
    RingBuffer *buffer = create_ring_buffer(2);

    write_ring_buffer(buffer, "path", "test1");
    write_ring_buffer(buffer, "path", "test2");
    read_ring_buffer(buffer, &timeout);
    free_ring_buffer(buffer);
    write_ring_buffer(buffer, "path", "test3");
    char *path = read_ring_buffer(buffer, &timeout);
    cr_assert_str_eq(path, "path/test2",
                     "Second element was not read correctly, expected 'path/test2' got %s", path);
    free_ring_buffer(buffer);
    path = read_ring_buffer(buffer, &timeout);
    cr_assert_str_eq(path, "path/test3",
                     "Third element was not read correctly, expected 'path/test3' got %s", path);
    free_ring_buffer(buffer);
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

    write_ring_buffer(buffer, "path", "test1");
    write_ring_buffer(buffer, "path", "test2");

    cr_assert(buffer->full,
              "Buffer should be marked as full after writing 2 elements");

    destroy_ring_buffer(buffer);
}

Test(ring_buffer, buffer_not_full_after_read) {
    RingBuffer *buffer = create_ring_buffer(2);

    write_ring_buffer(buffer, "path", "test1");
    write_ring_buffer(buffer, "path", "test2");
    read_ring_buffer(buffer, &timeout);
    free_ring_buffer(buffer);

    cr_assert_not(
        buffer->full,
        "Buffer should not be marked as full after reading one element");

    destroy_ring_buffer(buffer);
}

Test(ring_buffer, buffer_full_after_write_read_write) {
    RingBuffer *buffer = create_ring_buffer(2);

    write_ring_buffer(buffer, "path", "test1");
    write_ring_buffer(buffer, "path", "test2");
    read_ring_buffer(buffer, &timeout);
    free_ring_buffer(buffer);
    write_ring_buffer(buffer, "path", "test3");

    cr_assert(buffer->full, "Buffer should be marked as full after writing, "
                            "reading, and writing again");

    destroy_ring_buffer(buffer);
}

Test(ring_buffer, test_get_free_space) {
    RingBuffer *buffer = create_ring_buffer(5);
    cr_assert_eq(get_ring_buffer_free_space(buffer), 5, "After initialization, free space should be equal to buffer size");
    write_ring_buffer(buffer, "path", "test");
    cr_assert_eq(get_ring_buffer_free_space(buffer), 4, "After writing one element, free space should decrease by 1");
    clear_ring_buffer(buffer);
    cr_assert_eq(get_ring_buffer_free_space(buffer), 5, "After clearing, free space should be equal to buffer size");
    destroy_ring_buffer(buffer);
}

Test(ring_buffer, test_is_buffer_full) {
    RingBuffer *buffer = create_ring_buffer(2);
    cr_assert_eq(is_ring_buffer_full(buffer), false, "After initialization, buffer should not be full");
    write_ring_buffer(buffer, "path", "test");
    cr_assert_eq(is_ring_buffer_full(buffer), false, "After writing one element to a buffer of size 2, buffer should not be full");
    write_ring_buffer(buffer, "path", "test");
    cr_assert_eq(is_ring_buffer_full(buffer), true, "After writing two elements to a buffer of size 2, buffer should be full");
    clear_ring_buffer(buffer);
    cr_assert_eq(is_ring_buffer_full(buffer), false, "After clearing, buffer should not be full");
    destroy_ring_buffer(buffer);
}

Test(ring_buffer, test_clear_buffer) {
    RingBuffer *buffer = create_ring_buffer(2);
    write_ring_buffer(buffer, "path", "test");
    write_ring_buffer(buffer, "path", "test");
    cr_assert_eq(is_ring_buffer_full(buffer), true, "After writing two elements to a buffer of size 2, buffer should be full");
    clear_ring_buffer(buffer);
    cr_assert_eq(is_ring_buffer_full(buffer), false, "After clearing, buffer should not be full");
    cr_assert_eq(get_ring_buffer_free_space(buffer), 2, "After clearing, free space should be equal to buffer size");
    destroy_ring_buffer(buffer);
}

Test(ring_buffer, test_is_buffer_empty) {
    RingBuffer *buffer = create_ring_buffer(2);
    cr_assert(is_ring_buffer_empty(buffer), "After initialization, buffer should be empty");
    write_ring_buffer(buffer, "path", "test");
    cr_assert_not(is_ring_buffer_empty(buffer), "After writing an element, buffer should not be empty");
    read_ring_buffer(buffer, &timeout);
    free_ring_buffer(buffer);
    cr_assert(is_ring_buffer_empty(buffer), "After reading the only element, buffer should be empty");
    destroy_ring_buffer(buffer);
}

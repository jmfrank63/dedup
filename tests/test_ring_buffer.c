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
        writeRingBuffer(buffer, elem);
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
    char *elem = readRingBuffer(buffer);
    free(elem); // free the element after reading it
    return NULL;
}

Test(ring_buffer, write_blocks_when_full) {
    RingBuffer *buffer = createRingBuffer(2);

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

    destroyRingBuffer(buffer);
}

Test(ring_buffer, create_and_destroy) {
    RingBuffer *buffer = createRingBuffer(2);
    cr_assert_not_null(buffer, "Buffer was not created");
    cr_assert_null(buffer->elems[0],
                   "Buffer elements should be initialized to NULL");
    cr_assert_null(buffer->elems[1],
                   "Buffer elements should be initialized to NULL");
    destroyRingBuffer(buffer);
}

Test(ring_buffer, write_single_element) {
    RingBuffer *buffer = createRingBuffer(2);

    writeRingBuffer(buffer, "test1");

    cr_assert_str_eq(buffer->elems[0], "test1",
                     "First element was not written correctly");

    destroyRingBuffer(buffer);
}

Test(ring_buffer, write_two_elements) {
    RingBuffer *buffer = createRingBuffer(2);

    writeRingBuffer(buffer, "test1");
    writeRingBuffer(buffer, "test2");

    cr_assert_str_eq(buffer->elems[1], "test2",
                     "Second element was not written correctly");

    cr_assert(buffer->full,
              "Buffer not marked as full after writing 2 elements");

    destroyRingBuffer(buffer);
}

Test(ring_buffer, read_single_element) {
    RingBuffer *buffer = createRingBuffer(2);

    writeRingBuffer(buffer, "test1");
    writeRingBuffer(buffer, "test2");

    cr_assert_str_eq(readRingBuffer(buffer), "test1",
                     "First element was not read correctly");

    cr_assert_not(buffer->full,
                  "Buffer should not be marked full after reading one element");

    destroyRingBuffer(buffer);
}

Test(ring_buffer, write_after_read) {
    RingBuffer *buffer = createRingBuffer(2);

    writeRingBuffer(buffer, "test1");
    writeRingBuffer(buffer, "test2");
    readRingBuffer(buffer);
    writeRingBuffer(buffer, "test3");

    cr_assert(buffer->full,
              "Buffer not marked as full after writing second elements again");

    destroyRingBuffer(buffer);
}

Test(ring_buffer, read_second_element) {
    RingBuffer *buffer = createRingBuffer(2);

    writeRingBuffer(buffer, "test1");
    writeRingBuffer(buffer, "test2");
    readRingBuffer(buffer);
    writeRingBuffer(buffer, "test3");
    cr_assert_str_eq(readRingBuffer(buffer), "test2",
                     "Second element was not read correctly");
    cr_assert_str_eq(readRingBuffer(buffer), "test3",
                     "Third element was not read correctly");
    destroyRingBuffer(buffer);
}

Test(ring_buffer, buffer_not_full_initially) {
    RingBuffer *buffer = createRingBuffer(2);

    cr_assert_not(buffer->full,
                  "Buffer should not be marked as full initially");

    destroyRingBuffer(buffer);
}

Test(ring_buffer, buffer_full_after_two_writes) {
    RingBuffer *buffer = createRingBuffer(2);

    writeRingBuffer(buffer, "test1");
    writeRingBuffer(buffer, "test2");

    cr_assert(buffer->full,
              "Buffer should be marked as full after writing 2 elements");

    destroyRingBuffer(buffer);
}

Test(ring_buffer, buffer_not_full_after_read) {
    RingBuffer *buffer = createRingBuffer(2);

    writeRingBuffer(buffer, "test1");
    writeRingBuffer(buffer, "test2");
    readRingBuffer(buffer);

    cr_assert_not(
        buffer->full,
        "Buffer should not be marked as full after reading one element");

    destroyRingBuffer(buffer);
}

Test(ring_buffer, buffer_full_after_write_read_write) {
    RingBuffer *buffer = createRingBuffer(2);

    writeRingBuffer(buffer, "test1");
    writeRingBuffer(buffer, "test2");
    readRingBuffer(buffer);
    writeRingBuffer(buffer, "test3");

    cr_assert(buffer->full, "Buffer should be marked as full after writing, "
                            "reading, and writing again");

    destroyRingBuffer(buffer);
}

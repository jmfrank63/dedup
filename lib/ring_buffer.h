#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdlib.h>
#include <stdatomic.h>

#define BUFFER_SIZE 1024

typedef struct {
    char* buffer[BUFFER_SIZE];
    _Atomic size_t write_index;
    _Atomic size_t read_index;
} RingBuffer;

RingBuffer* ring_buffer_init();
void ring_buffer_free(RingBuffer* ring_buffer);
void ring_buffer_enqueue(RingBuffer* ring_buffer, char* path);
char* ring_buffer_dequeue(RingBuffer* ring_buffer);

#endif // RING_BUFFER_H
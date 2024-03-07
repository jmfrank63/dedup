#include "ring_buffer.h"

RingBuffer* ring_buffer_init() {
    RingBuffer* ring_buffer = malloc(sizeof(RingBuffer));
    ring_buffer->write_index = 0;
    ring_buffer->read_index = 0;
    return ring_buffer;
}

void ring_buffer_free(RingBuffer* ring_buffer) {
    free(ring_buffer);
}

void ring_buffer_enqueue(RingBuffer* ring_buffer, char* path) {
    size_t next_write_index = (ring_buffer->write_index + 1) % BUFFER_SIZE;
    while (next_write_index == atomic_load(&ring_buffer->read_index)) {
        // Buffer is full, wait
    }
    ring_buffer->buffer[ring_buffer->write_index] = path;
    atomic_store(&ring_buffer->write_index, next_write_index);
}

char* ring_buffer_dequeue(RingBuffer* ring_buffer) {
    while (ring_buffer->read_index == atomic_load(&ring_buffer->write_index)) {
        // Buffer is empty, wait
    }
    char* path = ring_buffer->buffer[ring_buffer->read_index];
    atomic_store(&ring_buffer->read_index, (ring_buffer->read_index + 1) % BUFFER_SIZE);
    return path;
}

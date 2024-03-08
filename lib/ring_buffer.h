// ring_buffer.h
#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <pthread.h>
#include <stdbool.h>

typedef struct {
    int size;  // maximum number of elements   
    int start;  // index of oldest element
    int end;  // index at which to write new element
    char **elems;  // vector of elements
    bool full;  // true if buffer is full
    pthread_mutex_t mutex;  // protects access to the buffer
    pthread_cond_t cond;  // signals when space is available
} RingBuffer;

RingBuffer* create_ring_buffer(int size);
void destroy_ring_buffer(RingBuffer *buffer);
void write_ring_buffer(RingBuffer *buffer, char *elem);
char* read_ring_buffer(RingBuffer *buffer);
int get_free_space(RingBuffer *buffer);
bool is_buffer_full(RingBuffer *buffer);
void clear_buffer(RingBuffer *buffer);
bool is_buffer_empty(RingBuffer *buffer);

#endif // RING_BUFFER_H

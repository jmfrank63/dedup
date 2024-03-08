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

RingBuffer* createRingBuffer(int size);
void destroyRingBuffer(RingBuffer *buffer);
void writeRingBuffer(RingBuffer *buffer, char *elem);
char* readRingBuffer(RingBuffer *buffer);

#endif // RING_BUFFER_H

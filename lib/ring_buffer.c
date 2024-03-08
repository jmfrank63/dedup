#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "ring_buffer.h"


RingBuffer* createRingBuffer(int size) {
    RingBuffer *buffer = (RingBuffer*)malloc(sizeof(RingBuffer));
    buffer->size = size;
    buffer->start = 0;
    buffer->end = 0;
    buffer->elems = (char**)calloc(size, sizeof(char*));
    pthread_mutex_init(&buffer->mutex, NULL);
    pthread_cond_init(&buffer->cond, NULL);
    return buffer;
}

void destroyRingBuffer(RingBuffer *buffer) {
    pthread_mutex_destroy(&buffer->mutex);
    pthread_cond_destroy(&buffer->cond);
    free(buffer->elems);
    free(buffer);
}

void writeRingBuffer(RingBuffer *buffer, char *elem) {
    pthread_mutex_lock(&buffer->mutex);
    if (buffer->full) {
        pthread_cond_wait(&buffer->cond, &buffer->mutex);  // wait for space
    }
    buffer->elems[buffer->end] = elem;
    buffer->end = (buffer->end + 1) % buffer->size;
    if (buffer->end == buffer->start) {
        buffer->full = true;
    }
    pthread_cond_signal(&buffer->cond);  // signal new data available
    pthread_mutex_unlock(&buffer->mutex);
}

char* readRingBuffer(RingBuffer *buffer) {
    pthread_mutex_lock(&buffer->mutex);
    if (!buffer->full && buffer->start == buffer->end) {
        pthread_cond_wait(&buffer->cond, &buffer->mutex);  // wait for data
    }
    char* elem = buffer->elems[buffer->start];
    buffer->start = (buffer->start + 1) % buffer->size;
    buffer->full = false;
    pthread_cond_signal(&buffer->cond);  // signal new space available
    pthread_mutex_unlock(&buffer->mutex);
    return elem;
}


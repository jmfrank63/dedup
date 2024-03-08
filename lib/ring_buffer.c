#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "ring_buffer.h"


RingBuffer* create_ring_buffer(int size) {
    RingBuffer *buffer = (RingBuffer*)malloc(sizeof(RingBuffer));
    buffer->size = size;
    buffer->start = 0;
    buffer->end = 0;
    buffer->elems = (char**)calloc(size, sizeof(char*));
    pthread_mutex_init(&buffer->mutex, NULL);
    pthread_cond_init(&buffer->cond, NULL);
    return buffer;
}

void destroy_ring_buffer(RingBuffer *buffer) {
    pthread_mutex_destroy(&buffer->mutex);
    pthread_cond_destroy(&buffer->cond);
    free(buffer->elems);
    free(buffer);
}

void write_ring_buffer(RingBuffer *buffer, char *elem) {
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

char* read_ring_buffer(RingBuffer *buffer) {
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

int get_free_space(RingBuffer *buffer) {
    pthread_mutex_lock(&buffer->mutex);
    int free_space = buffer->full ? 0 : (buffer->end >= buffer->start ? buffer->size - buffer->end + buffer->start : buffer->start - buffer->end);
    pthread_mutex_unlock(&buffer->mutex);
    return free_space;
}

bool is_buffer_full(RingBuffer *buffer) {
    pthread_mutex_lock(&buffer->mutex);
    bool full = buffer->full;
    pthread_mutex_unlock(&buffer->mutex);
    return full;
}

void clear_buffer(RingBuffer *buffer) {
    pthread_mutex_lock(&buffer->mutex);
    buffer->start = 0;
    buffer->end = 0;
    buffer->full = false;
    pthread_cond_signal(&buffer->cond);  // signal new space available
    pthread_mutex_unlock(&buffer->mutex);
}

bool is_buffer_empty(RingBuffer *buffer) {
    pthread_mutex_lock(&buffer->mutex);
    bool empty = (!buffer->full && (buffer->start == buffer->end));
    pthread_mutex_unlock(&buffer->mutex);
    return empty;
}


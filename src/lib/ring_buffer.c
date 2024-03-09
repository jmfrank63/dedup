#include "ring_buffer.h"
#include "../shared/consts.h"
#include <errno.h>
#include <linux/limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

RingBuffer *create_ring_buffer(int size) {
    RingBuffer *buffer = (RingBuffer *)malloc(sizeof(RingBuffer));
    buffer->size = size;
    buffer->start = 0;
    buffer->end = 0;

    // Try to allocate one big chunk of memory for the 2D array
    buffer->elems =
        malloc(size * sizeof(char *) + size * PATH_MAX * sizeof(char));
    if (buffer->elems != NULL) {
        char *rowStart = (char *)(buffer->elems + size);
        for (int i = 0; i < size; i++) {
            buffer->elems[i] = rowStart + i * PATH_MAX;
        }
    } else {
        // If that fails, fall back to allocating each row separately
        buffer->elems = malloc(size * sizeof(char *));
        if (buffer->elems != NULL) {
            for (int i = 0; i < size; i++) {
                buffer->elems[i] = malloc(PATH_MAX * sizeof(char));
                if (buffer->elems[i] == NULL) {
                    perror("Failed to allocate memory for buffer->elems");
                    exit(1);
                }
            }
        } else {
            perror("Failed to allocate memory for buffer->elems indices");
        }
    }

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

void write_ring_buffer(RingBuffer *buffer, char *path, char *filename) {
    pthread_mutex_lock(&buffer->mutex);
    if (buffer->full) {
        pthread_cond_wait(&buffer->cond, &buffer->mutex); // wait for space
    }
    // printf("Write Entry Start: %d, End: %d\n", buffer->start, buffer->end);

    if (strcmp(path, "/") == 0) {
        snprintf(buffer->elems[buffer->end], PATH_MAX, "%s%s", path, filename);
    } else {
        snprintf(buffer->elems[buffer->end], PATH_MAX, "%s" PATH_SEPARATOR "%s",
                 path, filename);
    }

    buffer->end = (buffer->end + 1) % buffer->size;
    if (buffer->end == buffer->start) {
        buffer->full = true;
    }
    // printf("Write Exit Start: %d, End: %d\n", buffer->start, buffer->end);
    pthread_cond_signal(&buffer->cond); // signal new data available
    pthread_mutex_unlock(&buffer->mutex);
}

char *read_ring_buffer(RingBuffer *buffer, const struct timespec *timeout) {
    pthread_mutex_lock(&buffer->mutex);
    if (!buffer->full && (buffer->start == buffer->end)) {
        if (pthread_cond_timedwait(&buffer->cond, &buffer->mutex, timeout) ==
            ETIMEDOUT) {
            pthread_mutex_unlock(&buffer->mutex);
            return NULL;
        }
    }
    char *elem = buffer->elems[buffer->start];
    pthread_mutex_unlock(&buffer->mutex);
    return elem;
}

void free_ring_buffer(RingBuffer *buffer) {
    pthread_mutex_lock(&buffer->mutex);
    // printf("Read Entry Start: %d, End: %d\n", buffer->start, buffer->end);
    if (!buffer->full && (buffer->start == buffer->end)) {
        pthread_mutex_unlock(&buffer->mutex);
        return;
    }
    buffer->start = (buffer->start + 1) % buffer->size;
    buffer->full = false;
    // printf("Read Exit Start: %d, End: %d\n", buffer->start, buffer->end);
    pthread_cond_signal(&buffer->cond); // signal new space available
    pthread_mutex_unlock(&buffer->mutex);
}

int get_ring_buffer_free_space(RingBuffer *buffer) {
    pthread_mutex_lock(&buffer->mutex);
    int free_space = buffer->full
                         ? 0
                         : (buffer->end >= buffer->start
                                ? buffer->size - buffer->end + buffer->start
                                : buffer->start - buffer->end);
    pthread_mutex_unlock(&buffer->mutex);
    return free_space;
}

bool is_ring_buffer_full(RingBuffer *buffer) {
    pthread_mutex_lock(&buffer->mutex);
    bool full = buffer->full;
    pthread_mutex_unlock(&buffer->mutex);
    return full;
}

void clear_ring_buffer(RingBuffer *buffer) {
    pthread_mutex_lock(&buffer->mutex);
    buffer->start = 0;
    buffer->end = 0;
    buffer->full = false;
    pthread_cond_signal(&buffer->cond); // signal new space available
    pthread_mutex_unlock(&buffer->mutex);
}

bool is_ring_buffer_empty(RingBuffer *buffer) {
    pthread_mutex_lock(&buffer->mutex);
    bool empty = (!buffer->full && (buffer->start == buffer->end));
    pthread_mutex_unlock(&buffer->mutex);
    return empty;
}

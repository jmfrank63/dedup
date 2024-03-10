#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#include "blake3.h"
#include "lib/hash_table.h"
#include "lib/hashing.h"
#include <linux/limits.h>
#include <stdint.h>
#include <time.h>
#endif

#include "lib/ring_buffer.h"
#include "shared/consts.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>

#define COLOR_DIRECTORY "\x1b[33m"
#define COLOR_FILE "\x1b[94m"
#define COLOR_RESET "\x1b[0m"

#define NUM_WORKERS 24
#define BUFFER_SIZE 4096

typedef struct {
    char *path;
    unsigned *file_count;
    unsigned *dir_count;
    RingBuffer *buffer;
} ThreadArgs;

volatile int writing = 1;

void *list_directory(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    DIR *dir = opendir(args->path);
    if (dir == NULL) {
        perror("Failed to open directory");
        return NULL;
    }

    struct dirent *entry;
    struct stat path_stat;
    char path[PATH_MAX];
    while ((entry = readdir(dir)) != NULL) {
        // Skip the entries "." and ".." as we don't want to loop on them.
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (strcmp(args->path, "/") == 0) {
            snprintf(path, sizeof(path), "%s%s", args->path, entry->d_name);
        } else {
            snprintf(path, sizeof(path), "%s" PATH_SEPARATOR "%s", args->path,
                     entry->d_name);
        }

        if (stat(path, &path_stat) != 0) {
            fprintf(stderr, "File: %s ", path);
            perror("Error");
            continue;
        }

        if (S_ISDIR(path_stat.st_mode)) {
            (*args->dir_count)++;
            char *current_path = args->path;
            args->path = path;
            list_directory(args);
            args->path = current_path;
        } else {
            if (S_ISREG(path_stat.st_mode)) {
                write_ring_buffer(args->buffer, args->path, entry->d_name);
                (*args->file_count)++;
            }
        }
    }
    closedir(dir);
    return NULL;
}

char *format_size(size_t size) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    double display_size = (double)size;
    for (; display_size > 1024 && i < 4; ++i) {
        display_size /= 1024;
    }

    char *result = malloc(20); // Allocate enough space for the result
    snprintf(result, 20, "%.2f %s", display_size, units[i]);
    return result;
}

void *print_file_path(void *arg) {
    HashAlgorithm blake3_algorithm = {.init = blake3_init,
                                      .update = blake3_update,
                                      .finalize = blake3_finalize};
    RingBuffer *buffer = (RingBuffer *)arg;
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_nsec += 50000000; // Wait up to 50 mseconds

    char *path;
    while (1) {
        if (is_ring_buffer_empty(buffer)) {
            if (writing == 0) 
                break;
            continue;
        }
        path = read_and_free_ring_buffer(buffer, &timeout);

        // Calculate and print the hash of the file
        uint8_t hash[BLAKE3_OUT_LEN];
        size_t size = 0;
        if (compute_hash(path, &blake3_algorithm, hash, &size) == 0) {
            char hash_str[BLAKE3_OUT_LEN * 2 +
                          1]; // Each byte will be 2 characters
                              // in hex, plus null terminator
            for (size_t i = 0; i < BLAKE3_OUT_LEN; i++) {
                sprintf(&hash_str[i * 2], "%02x", hash[i]);
            }
            // printf(COLOR_FILE "%s %s (size %ld)\n" COLOR_RESET, hash_str, path,
            //        size);

            // Add the file path and hash to the hashmap
            add_new_hash(hash_str, path);
        }
        // free_ring_buffer(buffer);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    unsigned file_count = 0;
    unsigned dir_count = 0;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    // Create and initialize the ring buffer
    RingBuffer *buffer = create_ring_buffer(BUFFER_SIZE);

    // Create the directory listing thread
    pthread_t list_dir_thread;
    ThreadArgs list_dir_args = {.path = argv[1],
                                .file_count = &file_count,
                                .dir_count = &dir_count,
                                .buffer = buffer};
    if (pthread_create(&list_dir_thread, NULL, list_directory,
                       &list_dir_args) != 0) {
        perror("Failed to create directory listing thread");
        return 1;
    }

    // Create the worker threads
    pthread_t workers[NUM_WORKERS];
    for (int i = 0; i < NUM_WORKERS; i++) {
        if (pthread_create(&workers[i], NULL, print_file_path, buffer) != 0) {
            perror("Failed to create worker thread");
            return 1;
        }
    }

    // Wait for the directory listing thread to finish
    pthread_join(list_dir_thread, NULL);
    writing = 0;
    // Wait for the worker threads to finish
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_join(workers[i], NULL);
    }
    print_duplicates();
    printf("Found %d files and %d directories\n", file_count, dir_count);

    // Don't forget to free the buffer when you're done with it
    destroy_ring_buffer(buffer);
    return 0;
}

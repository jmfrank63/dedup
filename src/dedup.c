#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#include <linux/limits.h>
#endif

#include "../lib/ring_buffer.h"
#include "blake3.h"
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef _WIN32
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

#define COLOR_DIRECTORY "\x1b[33m"
#define COLOR_FILE "\x1b[94m"
#define COLOR_RESET "\x1b[0m"

#define MAX_THREADS 2
#define BUFFER_SIZE 4096

typedef struct {
    char *path;
    int *file_count;
    int *dir_count;
    RingBuffer *buffer;
} ThreadArgs;

typedef struct {
    void (*init)(void *);
    void (*update)(void *, const void *, size_t);
    void (*finalize)(void *, uint8_t *, size_t);
} HashAlgorithm;

void blake3_init(void *state) { blake3_hasher_init((blake3_hasher *)state); }

void blake3_update(void *state, const void *input, size_t input_len) {
    blake3_hasher_update((blake3_hasher *)state, input, input_len);
}

void blake3_finalize(void *state, uint8_t *output, size_t output_len) {
    blake3_hasher_finalize((blake3_hasher *)state, output, output_len);
}

HashAlgorithm blake3_algorithm = {
    .init = blake3_init, .update = blake3_update, .finalize = blake3_finalize};

void compute_hash(const char *path, HashAlgorithm *algorithm, uint8_t *hash,
                  size_t *total) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        // Handle error
        return;
    }

    blake3_hasher hasher;
    algorithm->init(&hasher);

    uint8_t buffer[16384];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        algorithm->update(&hasher, buffer, n);
        *total += n;
    }
    fclose(file);

    algorithm->finalize(&hasher, hash, BLAKE3_OUT_LEN);
}

void *list_directory(void *arg) {
    // const char *dir_path, int *file_count, int *dir_count, RingBuffer
    // *buffer) {
    ThreadArgs *args = (ThreadArgs *)arg;
    DIR *dir = opendir(args->path);
    if (dir == NULL) {
        perror("Failed to open directory");
        return NULL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip the entries "." and ".." as we don't want to loop on them.
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[PATH_MAX];
        if (strcmp(args->path, "/") == 0) {
            snprintf(path, sizeof(path), "%s%s", args->path, entry->d_name);
        } else {
            snprintf(path, sizeof(path), "%s" PATH_SEPARATOR "%s", args->path,
                     entry->d_name);
        }
#if defined(_DIRENT_HAVE_D_TYPE)
        if (entry->d_type == DT_DIR) {
#else
        stat(path, &path_stat);
        if (S_ISDIR(path_stat.st_mode)) {
#endif
            // printf(COLOR_DIRECTORY "%s (directory)\n" COLOR_RESET, path);
            ThreadArgs new_args = {.path = path,
                                   .file_count = args->file_count,
                                   .dir_count = args->dir_count + 1,
                                   .buffer = args->buffer};
            list_directory(&new_args);
        } else {
            // Calculate and print the hash of the file
            // uint8_t hash[BLAKE3_OUT_LEN];
            // size_t size = 0;
            // compute_hash(path, &blake3_algorithm, hash, &size);

            // char hash_str[BLAKE3_OUT_LEN * 2 +
            //               1]; // Each byte will be 2 characters in hex, plus
            //                   // null terminator
            // for (size_t i = 0; i < BLAKE3_OUT_LEN; i++) {
            //     sprintf(&hash_str[i * 2], "%02x", hash[i]);
            // }
            // printf(COLOR_FILE "%s %s (size %ld)\n" COLOR_RESET, hash_str,
            // path,
            //        size);
            // printf(COLOR_FILE "Writing path: %s\n" COLOR_RESET, path);
            write_ring_buffer(args->buffer, path);
            *args->file_count += 1;
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
    RingBuffer *buffer = (RingBuffer *)arg;
    printf("Waiting for files to be added to the buffer of size %d\n", buffer->size);
    fflush(stdout);
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 5; // Wait up to 5 seconds

    char *path;
    while (1) {
        printf("Free: %d\n", get_ring_buffer_free_space(buffer));
        path = read_ring_buffer(buffer, &timeout);
        printf(COLOR_FILE "Read path of FILE: %s\n" COLOR_RESET, path);
        free_ring_buffer(buffer);
        if (path == NULL) {
            printf("Ring buffer is empty, exiting\n");
            break;
        }
    }
    return NULL;
}

// TODO: For each file, calculate the hash
// TODO: Build a hash table
// TODO: Find duplicates and empty files
// TODO: Print the results
int main(int argc, char *argv[]) {
    int file_count = 0;
    int dir_count = 0;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    // RingBuffer *buffer = create_ring_buffer(BUFFER_SIZE);
    // list_directory(argv[1], &file_count, &dir_count, buffer);
    // printf("Total file count: %d, total dir count %d\n", file_count,
    // dir_count); return 0;

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

    // Create the print thread
    pthread_t print_thread;
    if (pthread_create(&print_thread, NULL, print_file_path, buffer) != 0) {
        perror("Failed to create print thread");
        return 1;
    }

    // Wait for the threads to finish
    pthread_join(list_dir_thread, NULL);
    pthread_join(print_thread, NULL);

    // Don't forget to free the buffer when you're done with it
    destroy_ring_buffer(buffer);

    return 0;
}

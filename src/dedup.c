#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#include <linux/limits.h>
#endif

#include "blake3.h"
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

volatile int writing = 1;

void blake3_init(void *state) { blake3_hasher_init((blake3_hasher *)state); }

void blake3_update(void *state, const void *input, size_t input_len) {
    blake3_hasher_update((blake3_hasher *)state, input, input_len);
}

void blake3_finalize(void *state, uint8_t *output, size_t output_len) {
    blake3_hasher_finalize((blake3_hasher *)state, output, output_len);
}

HashAlgorithm blake3_algorithm = {
    .init = blake3_init, .update = blake3_update, .finalize = blake3_finalize};

// void compute_hash(const char *path, HashAlgorithm *algorithm, uint8_t *hash,
//                   size_t *total) {
//     FILE *file = fopen(path, "rb");
//     if (file == NULL) {
//         // Ignore open errors
//         return;
//     }

//     blake3_hasher hasher;
//     algorithm->init(&hasher);

//     uint8_t buffer[16384];
//     size_t n;
//     while ((n = fread(buffer, 1, sizeof(buffer), file)) > 0) {
//         algorithm->update(&hasher, buffer, n);
//         *total += n;
//     }
//     fclose(file);

//     algorithm->finalize(&hasher, hash, BLAKE3_OUT_LEN);
// }

int compute_hash(const char *path, HashAlgorithm *algorithm, uint8_t *hash,
                 size_t *total) {

    struct stat st;
    if (stat(path, &st) != 0) {
        // Ignore stat errors
        return -1;
    }

    // Check if the file is a regular file
    if (!S_ISREG(st.st_mode)) {
        // Ignore non-regular files
        return -1;
    }

    int fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        // Ignore open errors
        return -1;
    }

    blake3_hasher hasher;
    algorithm->init(&hasher);

    uint8_t buffer[16384];
    ssize_t n;

    fd_set set;
    struct timeval timeout;

    // Initialize the file descriptor set.
    FD_ZERO(&set);
    FD_SET(fd, &set);

    // Initialize the timeout data structure.
    timeout.tv_sec = 1; // 1 seconds timeout
    timeout.tv_usec = 0;

    while (select(fd + 1, &set, NULL, NULL, &timeout) > 0) {
        n = read(fd, buffer, sizeof(buffer));
        if (n > 0) {
            algorithm->update(&hasher, buffer, n);
            *total += n;
        } else if (n == 0 || (n < 0 && errno != EAGAIN)) {
            // End of file or read error other than EAGAIN
            break;
        }

        // Reinitialize the file descriptor set for the next select call.
        FD_ZERO(&set);
        FD_SET(fd, &set);

        // Reinitialize the timeout for the next select call.
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
    }
    close(fd);

    algorithm->finalize(&hasher, hash, BLAKE3_OUT_LEN);
    return 0;
}

void *list_directory(void *arg) {
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

#if defined(_DIRENT_HAVE_D_TYPE)
        if (entry->d_type == DT_DIR) {
#else
        stat(path, &path_stat);
        if (S_ISDIR(path_stat.st_mode)) {
#endif
            char path[PATH_MAX];
            if (strcmp(args->path, "/") == 0) {
                snprintf(path, sizeof(path), "%s%s", args->path, entry->d_name);
            } else {
                snprintf(path, sizeof(path), "%s" PATH_SEPARATOR "%s",
                         args->path, entry->d_name);
            }
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
            write_ring_buffer(args->buffer, args->path, entry->d_name);
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
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 60; // Wait up to 60 seconds

    char *path;
    while (writing || !is_ring_buffer_empty(buffer)) {
        if (is_ring_buffer_empty(buffer)) {
            continue;
        }
        path = read_ring_buffer(buffer, &timeout);

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
            printf(COLOR_FILE "%s %s (size %ld)\n" COLOR_RESET, hash_str, path,
                   size);
        }
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
    writing = 0;
    pthread_join(print_thread, NULL);

    // Don't forget to free the buffer when you're done with it
    destroy_ring_buffer(buffer);

    return 0;
}

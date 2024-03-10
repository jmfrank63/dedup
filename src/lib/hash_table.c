#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <pthread.h>
#include <bits/pthreadtypes.h>
#include "uthash.h"
#include "blake3.h"
#include "hash_table.h"
#include <string.h>

FileHash *hashes = NULL; // The hashmap
pthread_mutex_t hash_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to add a file path to an existing hash
void add_to_existing_hash(FileHash *file_hash, const char *file_path) {
    // Print a message indicating that the file is being processed
    // If the array is full, double its capacity
    if (file_hash->num_paths == file_hash->paths_capacity) {
        file_hash->paths_capacity *= 2;
        file_hash->file_paths = realloc(file_hash->file_paths, file_hash->paths_capacity * sizeof(char *));
    }

    // Add the file path to the array
    file_hash->file_paths[file_hash->num_paths] = strdup(file_path);
    file_hash->num_paths++;
}

// Function to add a new hash to the hashmap
void add_new_hash(const char *hash, const char *file_path) {
    pthread_mutex_lock(&hash_mutex);
    FileHash *file_hash;

    // Look for the hash in the hashmap
    HASH_FIND_STR(hashes, hash, file_hash);

    if (file_hash == NULL) {
        // If the hash is not in the hashmap, add a new entry
        file_hash = malloc(sizeof(FileHash));
        if (file_hash == NULL) {
            perror("Failed to allocate memory for new file hash");
            pthread_mutex_unlock(&hash_mutex);
            return;
        }

        strcpy(file_hash->hash, hash);
        file_hash->num_paths = 0;
        file_hash->paths_capacity = 1;
        file_hash->file_paths = malloc(file_hash->paths_capacity * sizeof(char *));
        if (file_hash->file_paths == NULL) {
            perror("Failed to allocate memory for file paths");
            free(file_hash);
            pthread_mutex_unlock(&hash_mutex);
            return;
        }
        HASH_ADD_STR(hashes, hash, file_hash);
    }

    // Add the file path to the hash
    add_to_existing_hash(file_hash, file_path);
    pthread_mutex_unlock(&hash_mutex);
}

void print_duplicates() {
    FileHash *current_hash, *tmp;
    HASH_ITER(hh, hashes, current_hash, tmp) {
        if (current_hash->num_paths > 1) {
            printf("Duplicate files found for hash %s:\n", current_hash->hash);
            for (int i = 0; i < current_hash->num_paths; i++) {
                printf("  %s\n", current_hash->file_paths[i]);
            }
        }
    }
}

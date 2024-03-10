#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "uthash.h"
#include "blake3.h"

typedef struct {
    char hash[BLAKE3_OUT_LEN * 2 + 1]; // Key
    char **file_paths; // Value
    int num_paths; // Number of paths in the array
    int paths_capacity; // Capacity of the array
    UT_hash_handle hh; // Makes this structure hashable
} FileHash;

// Function to add a file path to an existing hash
void add_to_existing_hash(FileHash *file_hash, const char *file_path);
// Function to add a new hash to the hashmap
void add_new_hash(const char *hash, const char *file_path);
// Function to get the duplicates
void print_duplicates();

#endif // HASH_TABLE_H
#include "uthash.h"
#include "blake3.h"

typedef struct {
    char hash[BLAKE3_OUT_LEN * 2 + 1]; // Key
    char **file_paths; // Value
    int num_paths; // Number of paths in the array
    int paths_capacity; // Capacity of the array
    UT_hash_handle hh; // Makes this structure hashable
} FileHash;

FileHash *hashes = NULL; // The hashmap

// Function to add a file path to a hash
void add_file_path(FileHash *file_hash, const char *file_path) {
    // If the array is full, double its capacity
    if (file_hash->num_paths == file_hash->paths_capacity) {
        file_hash->paths_capacity *= 2;
        file_hash->file_paths = realloc(file_hash->file_paths, file_hash->paths_capacity * sizeof(char *));
    }

    // Add the file path to the array
    file_hash->file_paths[file_hash->num_paths] = strdup(file_path);
    file_hash->num_paths++;
}

// Function to add a file to the hashmap
void add_file(const char *hash, const char *file_path) {
    FileHash *file_hash;

    // Look for the hash in the hashmap
    HASH_FIND_STR(hashes, hash, file_hash);

    if (file_hash == NULL) {
        // If the hash is not in the hashmap, add a new entry
        file_hash = malloc(sizeof(FileHash));
        strcpy(file_hash->hash, hash);
        file_hash->num_paths = 0;
        file_hash->paths_capacity = 1;
        file_hash->file_paths = malloc(file_hash->paths_capacity * sizeof(char *));
        HASH_ADD_STR(hashes, hash, file_hash);
    }

    // Add the file path to the hash
    add_file_path(file_hash, file_path);
}

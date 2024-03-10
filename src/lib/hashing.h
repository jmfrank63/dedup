#ifndef HASHING_H
#define HASHING_H

#include "blake3.h"

typedef struct {
    void (*init)(void *);
    void (*update)(void *, const void *, size_t);
    void (*finalize)(void *, uint8_t *, size_t);
} HashAlgorithm;

// Declare the functions
void blake3_init(void *state);
void blake3_update(void *state, const void *input, size_t input_len);
void blake3_finalize(void *state, uint8_t *output, size_t output_len);

// Function to hash a file
int compute_hash(const char *path, HashAlgorithm *algorithm, uint8_t *hash,
                 size_t *total);

#endif // HASHING_H
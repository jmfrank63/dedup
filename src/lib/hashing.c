#include "hashing.h"
#include <blake3.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>

void blake3_init(void *state) { blake3_hasher_init((blake3_hasher *)state); }

void blake3_update(void *state, const void *input, size_t input_len) {
    blake3_hasher_update((blake3_hasher *)state, input, input_len);
}

void blake3_finalize(void *state, uint8_t *output, size_t output_len) {
    blake3_hasher_finalize((blake3_hasher *)state, output, output_len);
}

int compute_hash(const char *path, HashAlgorithm *algorithm, uint8_t *hash,
                 size_t *total) {

    int fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        // Return if we cannot open the file
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
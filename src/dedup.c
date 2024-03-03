#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

#define COLOR_DIRECTORY "\x1b[33m"
#define COLOR_FILE "\x1b[94m"
#define COLOR_RESET "\x1b[0m"

#define MAX_THREADS 4

typedef struct {
    char path[1024];
    int file_count;
} ThreadArgs;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int total_file_count = 0;

void *list_directory(void *args) {

    ThreadArgs *thread_args = (ThreadArgs *)args;
    char *dir_path = thread_args->path;

    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("Failed to open directory");
        return NULL;
    }

    struct dirent *entry;
    struct stat path_stat;
    while ((entry = readdir(dir)) != NULL) {
        // Skip the entries "." and ".." as we don't want to loop on them.
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[1024];
        if (strcmp(dir_path, "/") == 0) {
            snprintf(path, sizeof(path), "%s%s", dir_path, entry->d_name);
        } else {
            snprintf(path, sizeof(path), "%s" PATH_SEPARATOR "%s", dir_path,
                     entry->d_name);
        }

#if defined(_DIRENT_HAVE_D_TYPE)
        if (entry->d_type == DT_DIR) {

#else
        stat(path, &path_stat);
        if (S_ISDIR(path_stat.st_mode)) {
#endif
            printf(COLOR_DIRECTORY "%s (directory)\n" COLOR_RESET, path);

            pthread_t thread;
            ThreadArgs *newArgs = malloc(sizeof(ThreadArgs));
            strcpy(newArgs->path, path);
            newArgs->file_count = 0;

            if (pthread_create(&thread, NULL, list_directory, newArgs) != 0) {
                perror("Failed to create thread");
            }

            pthread_join(thread, NULL);
            thread_args->file_count += newArgs->file_count;
            free(newArgs);
        } else {
            
            stat(path, &path_stat);
            
            printf(COLOR_FILE "%s (size: %ld)\n" COLOR_RESET, path, path_stat.st_size);
            thread_args->file_count++;
        }
    }

    closedir(dir);

    pthread_mutex_lock(&mutex);
    total_file_count += thread_args->file_count;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

char* format_size(size_t size) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    double display_size = (double)size;
    for (; display_size > 1024 && i < 4; ++i) {
        display_size /= 1024;
    }

    char *result = malloc(20);  // Allocate enough space for the result
    snprintf(result, 20, "%.2f %s", display_size, units[i]);
    return result;
}

// TODO: For each file, calculate the hash
// TODO: Build a hash table
// TODO: Find duplicates and empty files
// TODO: Print the results
int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    ThreadArgs args;
    strcpy(args.path, argv[1]);
    args.file_count = 0;

    list_directory(&args);

    printf("Total file count: %d\n", total_file_count);

    return 0;
}

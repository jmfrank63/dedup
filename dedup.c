#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

void list_directory(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("Failed to open directory");
        return;
    }

    errno = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip the entries "." and ".." as we don't want to loop on them.
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

        struct stat info;
        if (stat(path, &info) != 0) {
            perror("Failed to stat");
            continue;
        }

        if (S_ISDIR(info.st_mode)) {
            printf("%s (directory)\n", path);
            list_directory(path);
        } else {
            printf("%s\n", path);
        }
    }

    closedir(dir);
}

// TODO: Recursively traverse the file system
// TODO: For each file, calculate the hash
// TODO: Build a hash table
// TODO: Find duplicates and empty files
// TODO: Print the results
int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    list_directory(argv[1]);
    return 0;
}

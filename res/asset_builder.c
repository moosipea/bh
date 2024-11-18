#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <dirent.h>
#else
#include <io.h>
#define NAME_MAX _MAX_PATH
#endif // _WIN32

static void preamble(FILE *file) {
    fputs("#ifndef BUILT_ASSETS_H\n", file);
    fputs("#define BUILT_ASSETS_H\n", file);
    fputc('\n', file);
}

static void remove_extension(char *string, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (string[i] == '.') {
            string[i] = '\0';
            break;
        } 
    }
}

static void write_file_data(FILE *header, FILE *file) {
    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    rewind(file);

    unsigned char *buffer = malloc(length);
    fread(buffer, 1, length, file);

    for (size_t i = 0; i < length; i++) {
        fprintf(header, "0x%02x, ", buffer[i]);
    }
    fputs("0x00", header);

    free(buffer);
}

static void write_file(FILE *header, const char *dir_name, const char *file_name) {
    char name[NAME_MAX];
    memcpy(name, file_name, NAME_MAX);
    remove_extension(name, NAME_MAX);

    fprintf(header, "const char ASSET_%s[] = { ", name);

    char full_path[2 * NAME_MAX + 1];
    strcpy(full_path, dir_name);
    strcat(full_path, "/");
    strcat(full_path, file_name);

    FILE *file = fopen(full_path, "r");
    if (file == NULL) {
        fprintf(stderr, "Couldn't open file `%s`\n", full_path);
        return;
    }
    
    write_file_data(header, file);

    fclose(file); 
    fputs(" };\n\n", header);
}

#ifndef _WIN32
static void entries(FILE *file, const char *dir_name) {
    DIR *assets_dir = opendir(dir_name);
    if (assets_dir == NULL) {
        fprintf(stderr, "Couldn't open directory `%s`\n", dir_name);
        exit(EXIT_FAILURE);
    }

    struct dirent *asset;
    while ((asset = readdir(assets_dir))) {
        if (asset->d_type != DT_REG) {
            continue;
        }
        write_file(file, dir_name, asset->d_name);
    }
    
    closedir(assets_dir);
}
#else
static void entries(FILE *file, const char *dir_name) {
    const char append[] = "/*";
    char search[NAME_MAX + sizeof(append)];
    strcpy(search, dir_name);
    strcat(search, append);
    
    struct _finddata_t asset;
    intptr_t handle = _findfirst(search, &asset);

    if (handle == -1) {
        fprintf(stderr, "Couldn't open directory `%s`\n", dir_name);
        exit(EXIT_FAILURE);
    }

    if (!_findnext(handle, &asset)) {
        while (!_findnext(handle, &asset)) {
            write_file(file, dir_name, asset.name);
        }
    }

    _findclose(handle);
}
#endif // _WIN32

static void addendum(FILE *file) {
    fputs("#endif // !BUILT_ASSETS_H\n", file);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Provide assets directory and header name as arguments!\n");
        exit(EXIT_FAILURE);
    }

    /* Open file for output */
    FILE *header = fopen(argv[2], "w");
    if (header == NULL) {
        fprintf(stderr, "Couldn't open file `%s`\n", argv[2]);
        exit(EXIT_FAILURE);
    }

    /* Write to file */
    preamble(header);
    entries(header, argv[1]);
    addendum(header);

    /* Clean up */
    fclose(header);

    return EXIT_SUCCESS;
}

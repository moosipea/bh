#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <dirent.h>
#else
#include <io.h>
#define NAME_MAX _MAX_PATH
#endif // _WIN32

static void header_preamble(FILE *file) {
    fputs("#ifndef BUILT_ASSETS_H\n", file);
    fputs("#define BUILT_ASSETS_H\n", file);
    fputc('\n', file);
}

static void source_preamble(FILE *file, const char *header_name) {
    fprintf(file, "#include \"%s\"\n", header_name);
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

static size_t file_size(FILE* file) {
    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    rewind(file);
    return length;
}

static void write_file_data(FILE *dest_file, FILE *file) {
    size_t length = file_size(file);

    unsigned char *buffer = malloc(length);
    fread(buffer, 1, length, file);

    for (size_t i = 0; i < length; i++) {
        fprintf(dest_file, "0x%02x, ", buffer[i]);
    }
    fputs("0x00", dest_file);

    free(buffer);
}

static void write_files(FILE *header, FILE* source, const char *dir_name, const char *file_name) {
    char name[NAME_MAX];
    memcpy(name, file_name, NAME_MAX);
    remove_extension(name, NAME_MAX);

    char full_path[2 * NAME_MAX + 1];
    strcpy(full_path, dir_name);
    strcat(full_path, "/");
    strcat(full_path, file_name);

    FILE *asset_file = fopen(full_path, "rb");
    if (asset_file == NULL) {
        fprintf(stderr, "Couldn't open file `%s`\n", full_path);
        return;
    }
    
    size_t size = file_size(asset_file);
    fprintf(header, "extern const unsigned char ASSET_%s[%zu];\n", name, size + 1);
    fprintf(source, "const unsigned char ASSET_%s[] = { ", name);
    
    write_file_data(source, asset_file);

    printf("Generated asset for `%s`\n", file_name);

    fclose(asset_file); 
    fputs(" };\n\n", source);
}

#ifndef _WIN32
static void entries(FILE *header, FILE* source, const char *dir_name) {
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
        write_files(header, source, dir_name, asset->d_name);
    }
    
    closedir(assets_dir);
}
#else

static void entries(FILE *header, FILE* source, const char *dir_name) {
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
            write_files(header, source, dir_name, asset.name);
        }
    }

    _findclose(handle);
}
#endif // _WIN32

static void header_addendum(FILE *file) {
    fputs("#endif\n", file);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Provide assets directory and source name and header name as arguments!\n");
        goto error;    
    }

    const char *assets_dir = argv[1];
    const char *source_name = argv[2];
    const char *header_name = argv[3];

    FILE *source = fopen(source_name, "w");
    if (source == NULL) {
        fprintf(stderr, "Couldn't open source file `%s`\n", argv[3]);
        goto error;
    }

    FILE *header = fopen(header_name, "w");
    if (header == NULL) {
        fprintf(stderr, "Couldn't open header file `%s`\n", argv[2]);
        goto error;
    }

    /* Write to files */
    header_preamble(header);
    source_preamble(source, header_name);
    entries(header, source, assets_dir);
    header_addendum(header);

    /* Clean up */
    fclose(header);

    /* Success! */
    return EXIT_SUCCESS;

error:
    return EXIT_FAILURE;
}

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

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
        fprintf(header, "0x%02hhx", buffer[i]);
        if (i + 1 != length) {
            fputs(", ", header);
        }
    }

    free(buffer);
}

static void write_file(FILE *header, const char *dir_name, const char *d_name) {
    char name[NAME_MAX];
    memcpy(name, d_name, NAME_MAX);
    remove_extension(name, NAME_MAX);

    fprintf(header, "const unsigned char %s[] = { ", name);

    char full_path[2 * NAME_MAX + 1];
    strcpy(full_path, dir_name);
    strcat(full_path, "/");
    strcat(full_path, d_name);

    FILE *file = fopen(full_path, "r");
    if (file == NULL) {
        fprintf(stderr, "Couldn't open file `%s`\n", full_path);
        return;
    }
    
    write_file_data(header, file);

    fclose(file); 
    fputs(" };\n\n", header);
}

static void entries(FILE *file, const char *dir_name, DIR *dir) {
    struct dirent *asset;
    while ((asset = readdir(dir))) {
        if (asset->d_type != DT_REG) {
            continue;
        }
        write_file(file, dir_name, asset->d_name);
    }
}

static void addendum(FILE *file) {
    fputs("#endif // !BUILT_ASSETS_H\n", file);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Provide assets directory and header name as arguments!\n");
        return EXIT_FAILURE;
    }

    /* Open file for output */
    FILE *header = fopen(argv[2], "w");
    if (header == NULL) {
        fprintf(stderr, "Couldn't open file `%s`\n", argv[2]);
        return EXIT_FAILURE;
    }

    /* Open directory for reading */
    DIR *assets_dir = opendir(argv[1]);
    if (assets_dir == NULL) {
        fprintf(stderr, "Couldn't open directory `%s`\n", argv[1]);
        return EXIT_FAILURE;
    }

    /* Write to file */
    preamble(header);
    entries(header, argv[1], assets_dir);
    addendum(header);

    /* Clean up */
    closedir(assets_dir);
    fclose(header);

    return EXIT_SUCCESS;
}

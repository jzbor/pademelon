#include "common.h"
#include "tools.h"
#include "x11-utils.h"
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define BUFSIZE     100

static char* wallpaper_path(void);

int tl_load_wallpaper(int argc, char *argv[]) {
    char *path;
    path = wallpaper_path();
    /* @TODO check if path exists */
    if (path) {
        if (x11_wallpaper_all(path))
            return EXIT_SUCCESS;
    }
    fprintf(stderr, "load-wallpaper: unable to set wallpaper to all screens\n");
    return EXIT_FAILURE;
}

int tl_set_wallpaper(int argc, char *argv[]) {
    char buffer[BUFSIZE];
    int status;
    size_t n;
    char *path, *dirpath;
    FILE *source, *target;

    if (argc < 0) {
        fprintf(stderr, "set-wallpaper: not enough arguments\n");
        return EXIT_FAILURE;
    }

    path = wallpaper_path();
    if (!path) {
        fprintf(stderr, "set-wallpaper: no wallpaper destination available\n");
        return EXIT_FAILURE;
    }

    /* open input file */
    source = fopen(argv[0], "r");
    if (!source) {
        fprintf(stderr, "set-wallpaper: unable to open file\n");
        return EXIT_FAILURE;
    }

    /* check if output dir exists */
    dirpath = strdup(path);
    if (!dirpath) {
        fprintf(stderr, "set-wallpaper: unable to allocate enough memory\n");
        fclose(source);
        return EXIT_FAILURE;
    }

    status = mkdir(dirname(dirpath), S_IRWXU|S_IRGRP|S_IROTH);
    if (status == -1 && errno != EEXIST) {
        perror("set-wallpaper: unable to create data dir");
        fclose(source);
        return EXIT_FAILURE;
    }
    free(dirpath);

    /* open output file */
    target = fopen(path, "w");
    if (!target) {
        fprintf(stderr, "set-wallpaper: unable to open target file\n");
        fclose(source);
        return EXIT_FAILURE;
    }

    while ((n = fread(buffer, sizeof(char), sizeof(buffer), source)) > 0) {
        if (fwrite(buffer, sizeof(char), n, target) != n) {
            fprintf(stderr, "set-wallpaper: error while writing to target file\n");
            return EXIT_FAILURE;
        }
    }

    if (ferror(source)) {
        fprintf(stderr, "set-wallpaper: error while reading from source file\n");
        return EXIT_FAILURE;
    }

    fclose(source);
    fclose(target);

    return tl_load_wallpaper(0, NULL);
}

char* wallpaper_path(void) {
    return user_data_path(WALLPAPER_FILE_NAME);
}


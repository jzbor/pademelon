#include "common.h"
#include "tools.h"
#include "x11-utils.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    char c = 0;
    int bytes_written;
    char *path;
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

    source = fopen(argv[0], "r");
    if (!source) {
        fprintf(stderr, "set-wallpaper: unable to open file\n");
        return EXIT_FAILURE;
    }

    target = fopen(path, "w");
    if (!target) {
        fprintf(stderr, "set-wallpaper: unable to open target file\n");
        fclose(source);
        return EXIT_FAILURE;
    }

    bytes_written = 0;
    while ((c = fgetc(source)) != EOF) {
        if(fputc(c, target) == EOF) {
            fprintf(stderr, "set-wallpaper: error while writing to target file\n");
            return EXIT_FAILURE;
        }
        bytes_written++;
    }
    fprintf(stderr, "%d bytes copied\n", bytes_written);
    fprintf(stderr, "%d is the last copied byte (feof: %d, ferror: %d, errno: %d/%s)\n", c, feof(source), ferror(source), errno, strerror(errno));

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


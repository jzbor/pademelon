#include "common.h"
#include "desktop-application.h"
#include "tools.h"
#include "x11-utils.h"
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define BUFSIZE             512
#define UNXRANDR_CMD        "unxrandr"
#define DISPLAY_CONF_FILE   "displayconfiguration"

static char* wallpaper_path(void);
static int print_category(struct dcategory *c);

int tl_load_display_conf(int argc, char *argv[]) {
    int status;
    char *path;
    if (argc > 0) {
        path = argv[0];
    } else {
        path = user_data_path(DISPLAY_CONF_FILE);
        if (!path)
            return EXIT_FAILURE;
    }

    status = execute(path);
    if (status == -1) {
        return EXIT_FAILURE;
    } else {
        if (chmod(path, S_IRWXU|S_IRGRP|S_IROTH) == -1)
            return EXIT_FAILURE;
        return status;
    }
}


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

int tl_print_applications(int argc, char *argv[]) {
    struct dcategory *c;

    load_applications();

    if (printf("These are the available categories ([d]efault, [a]vailable):\n\n") < 0)
        return EXIT_FAILURE;

    for (c = get_categories(); c; c = c->next) {
        if (print_category(c) < 0)
            return EXIT_FAILURE;
        if (printf("\n") < 0)
            return EXIT_FAILURE;
    }
    if (fflush(stdout) == EOF)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}


int tl_save_display_conf(int argc, char *argv[]) {
    int status;
    char *path = user_data_path(DISPLAY_CONF_FILE);
    if (!path)
        return EXIT_FAILURE;

    char temp[strlen(UNXRANDR_CMD) + strlen(" > ") + strlen(path) + 1];
    strcpy(temp, UNXRANDR_CMD);
    strcat(temp, " > ");
    strcat(temp, path);
    status = execute(temp);
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    else
        return EXIT_SUCCESS;
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

int print_category(struct dcategory *c) {
    int status, tested;
    struct dapplication *d;
    status = printf("%s:\n", c->name);
    if (status < 0)
        return -1;
    for (d = c->applications; d; d = d->cnext) {
        status = printf("%s", d->id_name);
        if (status < 0) return -1;
        tested = test_application(d);
        if (d->cdefault || tested) {
            status = printf("(%s%s)", d->cdefault ? "d" : "", tested ? "a" : "");
            if (status < 0) return -1;
        }
        status = printf(" -> ");
        if (status < 0) return -1;
    }
    status = printf("%p\n", NULL);
    if (status < 0)
        return -1;
    return 0;
}

char* wallpaper_path(void) {
    return user_data_path(WALLPAPER_FILE_NAME);
}


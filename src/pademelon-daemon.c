#include "common.h"
#include "desktop-daemon.h"
#include <dirent.h>
#include <errno.h>
#include <ini.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define DAEMON_FILE_ENDING      ".ddaemon"

static void load_daemons(const char *dir);
static void load_daemon_file(const char *path);

void load_daemons(const char *dir) {
    int status;
    DIR *directory;
    struct dirent *diriter;
    struct stat filestats = {0};

    /* open directory for iteration */
    directory = opendir(dir);
    if (directory == NULL) {
        report_value(R_WARNING, "Unable to load daemons from the following directory", dir, R_STRING);
        return;
    }

    errno = 0;
    /* iterate over all files in the specified directory and check file ending */
    while ((diriter = readdir(directory)) != NULL) {
        /* ignore current and parent directory */
        if (strcmp(diriter->d_name, ".") == 0 || strcmp(diriter->d_name, "..") == 0)
            continue;

        /* ignore files with an inappropriate ending */
        if (!STR_ENDS_WITH(diriter->d_name, DAEMON_FILE_ENDING)) {
            report_value(R_DEBUG, "Not a desktop daemon file", diriter->d_name, R_STRING);
            continue;
        }

        /* put together path for subfiles */
        char subpath[strlen(dir) + strlen("/") + strlen(diriter->d_name) + 1];
        status = snprintf(subpath, sizeof(subpath), "%s/%s", dir, diriter->d_name);
        if (status < 0)
            report(R_FATAL, "Unable to print path to variable on the stack");

        /* check if path is actually a regular file */
        status = stat(subpath, &filestats);
        if (status) {
            report(R_ERROR, "Unable to get file stats for potential daemon config file");
            continue;
        } else if (!S_ISREG(filestats.st_mode))
            continue;

        report_value(R_DEBUG, "Parsing daemon file", subpath, R_STRING);
        status = ini_parse(subpath, &ini_ddaemon_callback, NULL);
        if (status < 0)
            report_value(R_ERROR, "An error occurred while reading desktop daemon file", subpath, R_STRING);
    }

    if (errno != 0) {
        report_value(R_ERROR, "An error was encountered while iterating through the following directory",
                dir, R_STRING);
    }

    status = closedir(directory);
    if (status)
        report(R_FATAL, "Unable to close directory");
}

void load_daemon_file(const char *path) {
    report(R_DEBUG, path);
}

int main(int argc, char *argv[]) {
    int i;
    char *sysconf, *userconf;
    sysconf = system_config_path("daemons");
    userconf = user_config_path("daemons");
    report(R_DEBUG, "Config paths:");
    report(R_DEBUG, sysconf);
    report(R_DEBUG, userconf);

    load_daemons(sysconf);
    load_daemons(userconf);

    for (i = 1; argv[i]; i++) {
        if (strcmp(argv[i], "--daemons") == 0 || strcmp(argv[i], "-d") == 0) {
            if (printf("\n;;; DAEMONS ;;;\n\n") < 0)
                report(R_ERROR, "Unable to write to stderr");
            if (print_ddaemons() < 0)
                report(R_ERROR, "Unable to write to stderr");
        } else if (strcmp(argv[i], "--categories") == 0 || strcmp(argv[i], "-c") == 0) {
            if (printf("\n;;; CATEGORIES ;;;\n\n") < 0)
                report(R_ERROR, "Unable to write to stderr");
            if (print_categories() < 0)
                report(R_ERROR, "Unable to write to stderr");
        } else {
            if (printf("Usage: %s [--daemons] [--categories]\n", argv[0]) < 0)
                report(R_ERROR, "Unable to write to stderr");
        }

    }


    free_ddaemons();
    free_categories();
    free(sysconf);
    free(userconf);
}

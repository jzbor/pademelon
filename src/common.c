#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

#define MAX(A, B)               ((A) > (B) ? (A) : (B))
#define MIN(A, B)               ((A) < (B) ? (A) : (B))
#define BETWEEN(X, A, B)        ((A) <= (X) && (X) <= (B))

const char *name = "pademelon";
const char *sysconf = "/etc/%s/%s";
const char *userconf = "%s/%s/%s";
char *def_userconf = "%s/.config";

void report(int mode, const char *msg) {
    report_value(mode, msg, NULL, R_NONE);
}

void report_value(int mode, const char *msg, const void *value, int type) {
    char *prefix;
    if (mode < DEFAULT_REPORT_LEVEL)
        return;

    if (mode == R_DEBUG) {
        prefix = "DEBUG";
    } else if (mode == R_INFO) {
        prefix = "INFO";
    } else if (mode == R_WARNING) {
        prefix = "WARNING";
    } else if (mode == R_ERROR) {
        prefix = "ERROR";
    } else if (mode == R_FATAL) {
        prefix = "FATAL";
    } else {
        report(R_ERROR, "Invalid report mode");
    }

    if (fprintf(stderr, "%s: %s", prefix, msg) < 0)
        perror("Unable to report important messages");
    if ((value || type == R_POINTER) && type != R_NONE) {
        switch (type) {
            case R_STRING:
                if (fprintf(stderr, ": %s", (char *) value) < 0)
                    perror("Unable to report important messages");
                break;
            case R_INTEGER:
                if (fprintf(stderr, ": %d", *((int *) value)) < 0)
                    perror("Unable to report important messages");
                break;
            case R_POINTER:
            default:
                if (fprintf(stderr, ": %p", value) < 0)
                    perror("Unable to report important messages");
        }
    }

    if (mode > R_INFO && errno != 0)
        if (fprintf(stderr, "\n%s:\t\tError: %s", prefix, strerror(errno)) < 0)
            perror("Unable to report important messages");

    if (fprintf(stderr, "\n") < 0)
        perror("Unable to report important messages");

    if (mode == R_FATAL)
        exit(EXIT_FAILURE);

    errno = 0;
}


char *user_config_path(char *file) {
    char *path, *file_cpy, *xdg_config;
    file_cpy = file ? file : "";

    /* get config dir */
    if (!getenv("HOME"))
        report(R_FATAL, "Unable to read $HOME variable");
    xdg_config = getenv("XDG_CONFIG_HOME");
    char home_config[strlen(def_userconf) + strlen(getenv("HOME")) + 1];
    if(sprintf(home_config, def_userconf, getenv("HOME")) < 0)
        report(R_FATAL, "Unable to configure fallback user config dir");

    /* allocate space for the string; must be freed by user */
    path = malloc(strlen(userconf) + strlen(xdg_config ? xdg_config : home_config) + strlen(name) + strlen(file_cpy) + 1);
    if (!path)
        report(R_FATAL, "Unable to allocate memory for the user config dir");
    /* configure config dir according to userconf variable, program name and file_cpy */
    if (sprintf(path, userconf, xdg_config ? xdg_config : home_config, name, file_cpy) < 0)
        report(R_FATAL, "Unable to configure the user config dir");
    return path;
}

char *system_config_path(char *file) {
    char *path, *file_cpy;
    file_cpy = file ? file : "";

    /* allocate space for the string; must be freed by user */
    path = malloc(strlen(sysconf) + strlen(name) + strlen(file_cpy) + 1);
    if (!path)
        report(R_FATAL, "Unable to allocate memory for the system config dir");
    /* configure config dir according to sysconf variable, program name and file_cpy */
    if (sprintf(path, sysconf, name, file_cpy) < 0)
        report(R_FATAL, "Unable to configure the system config dir");
    return path;
}

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

void report(int mode, char *msg) {
    if (mode >= DEFAULT_REPORT_LEVEL) {
        if (mode == R_DEBUG) {
            if (fprintf(stderr, "DEBUG: %s\n", msg) < 0)
                report(R_FATAL, "Unable to report important messages");
        } else if (mode == R_INFO) {
            if (fprintf(stderr, "INFO: %s\n", msg) < 0)
                report(R_FATAL, "Unable to report important messages");
        } else if (mode == R_WARNING) {
            if (fprintf(stderr, "WARNING: %s\n", msg) < 0)
                report(R_FATAL, "Unable to report important messages");
        } else if (mode == R_ERROR) {
            if (fprintf(stderr, "ERROR: %s\n", msg) < 0)
                report(R_FATAL, "Unable to report important messages");
        } else if (mode == R_FATAL) {
            if (fprintf(stderr, "FATAL: %s\n", msg) < 0)
                report(R_FATAL, "Unable to report important messages");
        } else {
            report(R_ERROR, "Invalid report mode");
        }
        if (mode > R_INFO && errno != 0)
            perror("\tError");
    }

    if (mode == R_FATAL)
        exit(EXIT_FAILURE);
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

#include "common.h"
#include "signals.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *name = "pademelon";
const char *sysconf = "/etc/%s/%s";
const char *sysdata = "/usr/share/%s/%s";
const char *syslocaldata = "/usr/local/share/%s/%s";
const char *userconf = "%s/%s/%s";
const char *userdata = "%s/%s/%s";
char *def_userconf = "%s/.config";
char *def_userdata = "%s/.local/share";

void bye(const char *msg) {
    fprintf(stderr,"%s\n", msg);
    exit(EXIT_FAILURE);
}

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int execute(const char *command) {
    int status;

    if (!command)
        return -1;

    /* @TODO improve code so the caller can be sure of the sigchld handler */
    if (!block_signal(SIGCHLD))
        return -1;
    if (!install_default_sigchld_handler()) {
        unblock_signal(SIGCHLD);
        return -1;
    }

    status = system(command);

    if (!restore_sigchld_handler()) {
        unblock_signal(SIGCHLD);
        return -1;
    }
    if (!unblock_signal(SIGCHLD))
        return -1;

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    else
        return 1;
}

int str_to_int(const char *str, int *integer) {
    char *endptr;
    long ltemp;
    int val;
    errno = 0;
    ltemp = strtol(str, &endptr, 0);
    val = ltemp;
    if (val != ltemp || (ltemp == 0 && errno != 0) || str == endptr || (*endptr) != '\0') {
        return 0;
    }
    *integer = val;
    return 1;
}

char *system_config_path(char *file) {
    char *path, *file_cpy;
    file_cpy = file ? file : "";

    /* allocate space for the string; must be freed by user */
    path = malloc(strlen(sysconf) + strlen(name) + strlen(file_cpy) + 1);
    if (!path)
        die("Unable to allocate memory for the system config dir");
    /* configure config dir according to sysconf variable, program name and file_cpy */
    if (sprintf(path, sysconf, name, file_cpy) < 0)
        die("Unable to configure the system config dir");
    return path;
}

char *system_data_path(char *file) {
    char *path, *file_cpy;
    file_cpy = file ? file : "";

    /* allocate space for the string; must be freed by user */
    path = malloc(strlen(sysdata) + strlen(name) + strlen(file_cpy) + 1);
    if (!path)
        die("Unable to allocate memory for the system config dir");
    /* configure config dir according to sysdata variable, program name and file_cpy */
    if (sprintf(path, sysdata, name, file_cpy) < 0)
        die("Unable to configure the system config dir");
    return path;
}

char *system_local_data_path(char *file) {
    char *path, *file_cpy;
    file_cpy = file ? file : "";

    /* allocate space for the string; must be freed by user */
    path = malloc(strlen(syslocaldata) + strlen(name) + strlen(file_cpy) + 1);
    if (!path)
        die("Unable to allocate memory for the system config dir");
    /* configure config dir according to syslocaldata variable, program name and file_cpy */
    if (sprintf(path, syslocaldata, name, file_cpy) < 0)
        die("Unable to configure the system config dir");
    return path;
}

char *user_config_path(char *file) {
    char *path, *file_cpy, *xdg_config;
    file_cpy = file ? file : "";

    /* get config dir */
    if (!getenv("HOME"))
        die("Unable to read $HOME variable");
    xdg_config = getenv("XDG_CONFIG_HOME");
    char home_config[strlen(def_userconf) + strlen(getenv("HOME")) + 1];
    if(sprintf(home_config, def_userconf, getenv("HOME")) < 0)
        die("Unable to configure fallback user config dir");

    /* allocate space for the string; must be freed by user */
    path = malloc(strlen(userconf) + strlen(xdg_config ? xdg_config : home_config) + strlen(name) + strlen(file_cpy) + 1);
    if (!path)
        die("Unable to allocate memory for the user config dir");
    /* configure config dir according to userconf variable, program name and file_cpy */
    if (sprintf(path, userconf, xdg_config ? xdg_config : home_config, name, file_cpy) < 0)
        die("Unable to configure the user config dir");
    return path;
}

char *user_data_path(char *file) {
    char *path, *file_cpy, *xdg_config;
    file_cpy = file ? file : "";

    /* get config dir */
    if (!getenv("HOME"))
        die("Unable to read $HOME variable");
    xdg_config = getenv("XDG_DATA_HOME");
    char home_config[strlen(def_userdata) + strlen(getenv("HOME")) + 1];
    if(sprintf(home_config, def_userdata, getenv("HOME")) < 0)
        die("Unable to configure fallback user data dir");

    /* allocate space for the string; must be freed by user */
    path = malloc(strlen(userdata) + strlen(xdg_config ? xdg_config : home_config) + strlen(name) + strlen(file_cpy) + 1);
    if (!path)
        die("Unable to allocate memory for the user data dir");
    /* configure data dir according to userdata variable, program name and file_cpy */
    if (sprintf(path, userdata, xdg_config ? xdg_config : home_config, name, file_cpy) < 0)
        die("Unable to configure the user data dir");
    return path;
}

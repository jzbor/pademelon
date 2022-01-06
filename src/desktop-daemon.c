#include "common.h"
#include "desktop-daemon.h"
#include "signals.h"
#include <dirent.h>
#include <errno.h>
#include <ini.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define IS_TRUE(S)                  (strcmp((S), "True") == 0 || strcmp((S), "true") == 0 || strcmp((S), "1") == 0)
#define PRINT_PROPERTY_STR(K, V)    if (printf("%s = %s\n", (K), (V)) < 0) return -1;
#define PRINT_PROPERTY_BOOL(K, V)   if (printf("%s = %s\n", (K), (V) ? "True" : "False") < 0) return -1;

struct dcategory *categories = NULL;
struct ddaemon *daemons = NULL;

void add_to_category(const char *name, struct ddaemon *d) {
    struct dcategory *c;
    struct ddaemon *diter;

    /* daemon already has category - remove daemon from it*/
    if (d->category != NULL) {
        /* check if head matches */
        if (d->category->daemons == d) {
            d->category->daemons = d->cnext;
            d->category = NULL;
            d->cnext = NULL;
        } else {
            for (diter = d->category->daemons; diter && diter->next; diter = diter->cnext) {
                if (diter->cnext == d) {
                    diter->cnext = d->cnext;
                    d->category = NULL;
                    d->cnext = NULL;
                    break;
                }
            }
        }
    }

    /* add daemon if category exists */
    for (c = categories; c; c = c->next) {
        if (strcmp(name, c->name) == 0) {
            d->cnext = c->daemons;
            d->category = c;
            c->daemons = d;
            break;
        }
    }

    /* create category - it does not exist yet */
    if (d->category == NULL) {
        c = malloc(sizeof(struct dcategory));
        if (!c)
            report(R_FATAL, "Unable to allocate enough memory for new category");
        memcpy(c, &dcategory_default, sizeof(dcategory_default));

        /* set name and add to lists */
        c->name = strdup(name);
        if (!c->name) {
            report(R_FATAL, "Unable to allocate space for new category name");
        }
        c->daemons = d;
        c->next = categories;
        categories = c;
        d->category = c;
    }
}

struct dcategory *get_categories(void) {
    return categories;
}

struct ddaemon *find_ddaemon(const char *id_name, const char *category, int init_if_not_found) {
    struct ddaemon *d;
    struct dcategory *c;
    if (!category) {
        for (d = daemons; d; d = d->next) {
            if (strcmp(id_name, d->id_name) == 0) {
                return d;
            }
        }
    } else {
        c = find_category(category);
        if (!c)
            return NULL;
        for (d = c->daemons; d; d = d->cnext) {
            if (strcmp(id_name, d->id_name) == 0) {
                return d;
            }
        }
    }

    /* daemon with this id_name doesn't exist yet */
    if (!d && init_if_not_found) {
        d = malloc(sizeof(struct ddaemon));
        if (!d)
            report(R_FATAL, "Unable to allocate enough memory for new daemon");
        memcpy(d, &ddaemon_default, sizeof(ddaemon_default));

        /* set name and add to lists */
        d->id_name = strdup(id_name);
        if (!d->id_name) {
            report(R_FATAL, "Unable to allocate space for new desktop id_name");
        }
        d->next = daemons;
        daemons = d;
        return d;
    }

    return NULL;
}

struct dcategory *find_category(const char *name) {
    struct dcategory *c;
    for (c = categories; c; c = c->next) {
        if (strcmp(c->name, name) == 0)
            return c;
    }
    return NULL;
}

void free_categories(void) {
    struct dcategory *c, *n;

    for (c = categories; c; c = n) {
        n = c->next;
        free(c->name);
        free(c);
    }
}

void free_ddaemon(struct ddaemon *d) {
    free(d->display_name);
    free(d->id_name);
    free(d->desc);

    free(d->launch_cmd);
    free(d->test_cmd);
    free(d->settings);

    free(d);
}

void free_ddaemons(void) {
    struct ddaemon *d, *n;

    for (d = daemons; d; d = n) {
        n = d->next;
        free_ddaemon(d);
    }
}

int ini_ddaemon_callback(void* user, const char* section, const char* name, const char* value) {
    struct ddaemon *d = find_ddaemon(section, NULL, 1);
    char **write_to_str = NULL;
    int *write_to_int = NULL;

    /* string attributes */
    if (strcmp(name, "name") == 0)
        write_to_str = &d->display_name;
    else if (strcmp(name, "description") == 0)
        write_to_str = &d->desc;
    else if (strcmp(name, "command") == 0)
        write_to_str = &d->launch_cmd;
    else if (strcmp(name, "test") == 0)
        write_to_str = &d->test_cmd;
    else if (strcmp(name, "settings") == 0)
        write_to_str = &d->settings;

    if (write_to_str) {
        /* hacky workaround to avoid reallocing stuff in read-only segments */
        for (int i = 0; i < sizeof(ddaemon_default) / sizeof(char *); i++)
            if (*write_to_str == ((char **)&ddaemon_default)[i]) {
                *write_to_str = NULL;
                break;
            }
        *write_to_str = realloc(*write_to_str, sizeof(char) * (strlen(value) + 1));
        if (!*write_to_str)
            report(R_FATAL, "Unable to allocate memory for daemon attribute");
        strcpy(*write_to_str, value);
        return 1;
    }

    /* boolean attributes */
    if (strcmp(name, "default") == 0)
        write_to_int = &d->cdefault;

    if (write_to_int) {
        *write_to_int = IS_TRUE(value);
        return 1;
    }

    /* category */
    if (strcmp(name, "category") == 0) {
        add_to_category(value, d);
        return 1;
    }

    report_value(R_WARNING, "Unknown key", name, R_STRING);
    return 1;
}

void launch_ddaemon(struct ddaemon *daemon) {
    pid_t pid;

    if (!daemon)
        return;

    block_signal(SIGCHLD);
    pid = fork();

    if (pid == 0) { /* child */
        unblock_signal(SIGCHLD);
        char *args[] = { "/bin/sh", "-c", daemon->launch_cmd, NULL };
        execvp(args[0], args);
        report_value(R_ERROR, "Unable to launch daemon", daemon->launch_cmd, R_STRING);
        exit(EXIT_FAILURE);
    } else if (pid > 0) { /* parent */
        plist_add(pid, daemon);
    } else {
        report(R_FATAL, "Unable to fork into a new process");
    }
    unblock_signal(SIGCHLD);
}

void load_daemons(void) {
    char *path;
    static int daemons_loaded = 0;

    /* only load daemons on the first call */
    if (daemons_loaded)
        return;
    else
        daemons_loaded = 1;

    /* /usr/local/share/... */
    path = system_data_path("daemons");
    if (path) {
        load_daemons_from_dir(path);
        free(path);
    }

    /* /usr/local/share/local... */
    path = system_local_data_path("daemons");
    if (path) {
        load_daemons_from_dir(path);
        free(path);
    }

    /* ~/.local/share */
    path = user_data_path("daemons");
    if (path) {
        load_daemons_from_dir(path);
        free(path);
    }
}

void load_daemons_from_dir(const char *dir) {
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

int print_ddaemon(struct ddaemon *d) {
    int status;
    status = printf("[%s]\t\t; %p\n", d->id_name, (void *)d);
    if (status < 0)
        return -1;
    PRINT_PROPERTY_STR("name", d->display_name);
    PRINT_PROPERTY_STR("description", d->desc);
    if (d->category) {
        PRINT_PROPERTY_STR("category", d->category->name);
    } else {
        PRINT_PROPERTY_STR("; category", dcategory_default.name);
    }
    PRINT_PROPERTY_STR("command", d->launch_cmd);
    PRINT_PROPERTY_STR("test", d->test_cmd);
    PRINT_PROPERTY_BOOL("default", d->cdefault);

    if (fflush(stdout) == EOF)
        return -1;
    return 0;
}

int print_ddaemons(void) {
    struct ddaemon *d;
    for (d = daemons; d; d = d->next) {
        if (print_ddaemon(d) < 0)
            return -1;
        if (printf("\n") < 0)
            return -1;
    }
    if (fflush(stdout) == EOF)
        return -1;
    return 0;
}

struct ddaemon *select_ddaemon(const char *user_preference, const char *category, int auto_fallback) {
    struct dcategory *c;
    struct ddaemon *d;
    /* @TODO use last *working* daemon instead of just last */

    if (user_preference && (d = find_ddaemon(user_preference, category, 0))) {
        return d;
    } else if (!auto_fallback) {
        return NULL;
    } else if (user_preference) {
        report_value(R_WARNING, "Preferred daemon not found - using fallback", user_preference, R_STRING);
    }

    /* preference was not found */
    c = find_category(category);
    if (!c) {
        report_value(R_WARNING, "Unable to launch daemon - category empty", category, R_STRING);
        return NULL;
    }
    for (d = c->daemons; d->cnext; d = d->cnext) {
        if (d->cdefault)
            return d;
    }

    return d;
}

int test_ddaemon(struct ddaemon *daemon) {
    int status;

    if (!daemon || !daemon->test_cmd)
        return 1;

    status = execute(daemon->test_cmd);
    if (status == -1)
        report(R_FATAL, "Execute failed");
    else if (status == 0)
        return 1;
    return 0;
}

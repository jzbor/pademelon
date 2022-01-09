#include "common.h"
#include "desktop-application.h"
#include "signals.h"
#include <dirent.h>
#include <errno.h>
#include <ini.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define PRINT_PROPERTY_STR(K, V)    if (printf("%s = %s\n", (K), (V)) < 0) return -1;
#define PRINT_PROPERTY_BOOL(K, V)   if (printf("%s = %s\n", (K), (V) ? "True" : "False") < 0) return -1;
#define TEST_TIMEOUT                5   /* in seconds */
#define PREFERENCE_NONE             "none"

static inline int IS_TRUE(const char *s)       { return strcmp(s, "True") == 0 || strcmp(s, "true") == 0 || strcmp(s, "1") == 0; }

struct dcategory *categories = NULL;
struct dapplication *applications = NULL;

void add_to_category(const char *name, struct dapplication *a) {
    struct dcategory *c;
    struct dapplication *aiter;

    /* application already has category - remove application from it*/
    if (a->category != NULL) {
        /* check if head matches */
        if (a->category->applications == a) {
            a->category->applications = a->cnext;
            a->category = NULL;
            a->cnext = NULL;
        } else {
            for (aiter = a->category->applications; aiter && aiter->next; aiter = aiter->cnext) {
                if (aiter->cnext == a) {
                    aiter->cnext = a->cnext;
                    a->category = NULL;
                    a->cnext = NULL;
                    break;
                }
            }
        }
    }

    /* add application if category exists */
    for (c = categories; c; c = c->next) {
        if (strcmp(name, c->name) == 0) {
            a->cnext = c->applications;
            a->category = c;
            c->applications = a;
            break;
        }
    }

    /* create category - it does not exist yet */
    if (a->category == NULL) {
        c = malloc(sizeof(struct dcategory));
        if (!c)
            report(R_FATAL, "Unable to allocate enough memory for new category");
        memcpy(c, &category_default, sizeof(category_default));

        /* set name and add to lists */
        c->name = strdup(name);
        if (!c->name) {
            report(R_FATAL, "Unable to allocate space for new category name");
        }
        c->applications = a;
        c->next = categories;
        categories = c;
        a->category = c;
    }
}

int export_application(struct dapplication *application, const char *name) {
    return setenv(name, application->launch_cmd, 0);
}

struct dcategory *get_categories(void) {
    return categories;
}

struct dapplication *find_application(const char *id_name, const char *category, int init_if_not_found) {
    struct dapplication *a;
    struct dcategory *c;
    if (!category) {
        for (a = applications; a; a = a->next) {
            if (strcmp(id_name, a->id_name) == 0) {
                return a;
            }
        }
    } else {
        c = find_category(category);
        if (!c)
            return NULL;
        for (a = c->applications; a; a = a->cnext) {
            if (strcmp(id_name, a->id_name) == 0) {
                return a;
            }
        }
    }

    /* application with this id_name doesn't exist yet */
    if (!a && init_if_not_found) {
        a = malloc(sizeof(struct dapplication));
        if (!a)
            report(R_FATAL, "Unable to allocate enough memory for new application");
        memcpy(a, &application_default, sizeof(application_default));

        /* set name and add to lists */
        a->id_name = strdup(id_name);
        if (!a->id_name) {
            report(R_FATAL, "Unable to allocate space for new desktop id_name");
        }
        a->next = applications;
        applications = a;
        return a;
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

void free_application(struct dapplication *a) {
    free(a->display_name);
    free(a->id_name);
    free(a->desc);

    free(a->launch_cmd);
    free(a->test_cmd);
    free(a->settings);

    free(a);
}

void free_applications(void) {
    struct dapplication *a, *n;

    for (a = applications; a; a = n) {
        n = a->next;
        free_application(a);
    }
}

int ini_application_callback(void* user, const char* section, const char* name, const char* value) {
    struct dapplication *a = find_application(section, NULL, 1);
    char **write_to_str = NULL;
    int *write_to_int = NULL;

    /* string attributes */
    if (strcmp(name, "name") == 0)
        write_to_str = &a->display_name;
    else if (strcmp(name, "description") == 0)
        write_to_str = &a->desc;
    else if (strcmp(name, "command") == 0)
        write_to_str = &a->launch_cmd;
    else if (strcmp(name, "test") == 0)
        write_to_str = &a->test_cmd;
    else if (strcmp(name, "settings") == 0)
        write_to_str = &a->settings;

    if (write_to_str) {
        *write_to_str = realloc(*write_to_str, sizeof(char) * (strlen(value) + 1));
        if (!*write_to_str)
            report(R_FATAL, "Unable to allocate memory for application attribute");
        strcpy(*write_to_str, value);
        return 1;
    }

    /* boolean attributes */
    if (strcmp(name, "default") == 0)
        write_to_int = &a->cdefault;

    if (write_to_int) {
        *write_to_int = IS_TRUE(value);
        return 1;
    }

    /* category */
    if (strcmp(name, "category") == 0) {
        add_to_category(value, a);
        return 1;
    }

    report_value(R_WARNING, "Unknown key", name, R_STRING);
    return 1;
}

void launch_application(struct dapplication *application) {
    pid_t pid;

    if (!application)
        return;

    block_signal(SIGCHLD);
    pid = fork();

    if (pid == 0) { /* child */
        unblock_signal(SIGCHLD);
        char *args[] = { "/bin/sh", "-c", application->launch_cmd, NULL };
        execvp(args[0], args);
        report_value(R_ERROR, "Unable to launch application", application->launch_cmd, R_STRING);
        exit(EXIT_FAILURE);
    } else if (pid > 0) { /* parent */
        plist_add(pid, application);
    } else {
        report(R_FATAL, "Unable to fork into a new process");
    }
    unblock_signal(SIGCHLD);
}

void load_applications(void) {
    char *path;
    static int applications_loaded = 0;

    /* only load applications on the first call */
    if (applications_loaded)
        return;
    else
        applications_loaded = 1;

    /* /usr/local/share/... */
    path = system_data_path("applications");
    if (path) {
        load_applications_from_dir(path);
        free(path);
    }

    /* /usr/local/share/local... */
    path = system_local_data_path("applications");
    if (path) {
        load_applications_from_dir(path);
        free(path);
    }

    /* ~/.local/share */
    path = user_data_path("applications");
    if (path) {
        load_applications_from_dir(path);
        free(path);
    }
}

void load_applications_from_dir(const char *dir) {
    int status;
    DIR *directory;
    struct dirent *diriter;
    struct stat filestats = {0};

    /* open directory for iteration */
    directory = opendir(dir);
    if (directory == NULL) {
        report_value(R_WARNING, "Unable to load applications from the following directory", dir, R_STRING);
        return;
    }

    errno = 0;
    /* iterate over all files in the specified directory and check file ending */
    while ((diriter = readdir(directory)) != NULL) {
        /* ignore current and parent directory */
        if (strcmp(diriter->d_name, ".") == 0 || strcmp(diriter->d_name, "..") == 0)
            continue;

        /* ignore files with an inappropriate ending */
        if (!STR_ENDS_WITH(diriter->d_name, APPLICATION_FILE_ENDING)) {
            report_value(R_DEBUG, "Not a desktop application file", diriter->d_name, R_STRING);
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
            report(R_ERROR, "Unable to get file stats for potential application config file");
            continue;
        } else if (!S_ISREG(filestats.st_mode))
            continue;

        status = ini_parse(subpath, &ini_application_callback, NULL);
        if (status < 0)
            report_value(R_ERROR, "An error occurred while reading desktop application file", subpath, R_STRING);
    }

    if (errno != 0) {
        report_value(R_ERROR, "An error was encountered while iterating through the following directory",
                dir, R_STRING);
    }

    status = closedir(directory);
    if (status)
        report(R_FATAL, "Unable to close directory");
}

int print_application(struct dapplication *a) {
    int status;
    status = printf("[%s]\t\t; %p\n", a->id_name, (void *)a);
    if (status < 0)
        return -1;
    PRINT_PROPERTY_STR("name", a->display_name);
    PRINT_PROPERTY_STR("description", a->desc);
    if (a->category) {
        PRINT_PROPERTY_STR("category", a->category->name);
    } else {
        PRINT_PROPERTY_STR("; category", category_default.name);
    }
    PRINT_PROPERTY_STR("command", a->launch_cmd);
    PRINT_PROPERTY_STR("test", a->test_cmd);
    PRINT_PROPERTY_BOOL("default", a->cdefault);

    if (fflush(stdout) == EOF)
        return -1;
    return 0;
}

int print_applications(void) {
    struct dapplication *a;
    for (a = applications; a; a = a->next) {
        if (print_application(a) < 0)
            return -1;
        if (printf("\n") < 0)
            return -1;
    }
    if (fflush(stdout) == EOF)
        return -1;
    return 0;
}

struct dapplication *select_application(struct category_option *co) {
    struct dcategory *c;
    struct dapplication *a;

    if (!co)
        return NULL;

    if (co->user_preference && strcmp(co->user_preference, PREFERENCE_NONE) == 0) {
        return NULL;
    } else if (co->user_preference && (a = find_application(co->user_preference, co->name, 0))) {
        return a;
    } else if (!co->fallback) {
        return NULL;
    } else if (co->user_preference) {
        report_value(R_WARNING, "Preferred application not found - using fallback", co->user_preference, R_STRING);
    }

    /* preference was not found */
    c = find_category(co->name);
    if (!c) {
        report_value(R_WARNING, "Unable to launch application - category empty", co->name, R_STRING);
        return NULL;
    }

    /* go through defaults */
    for (a = c->applications; a; a = a->cnext) {
        if (a->cdefault) {
            if (test_application(a))
                return a;
            else
                fprintf(stderr, "defaults: %s - not suitable\n", a->id_name);
        }
    }

    /* go through all applications */
    for (a = c->applications; a; a = a->cnext) {
        if (test_application(a))
            return a;
        else
            fprintf(stderr, "other: %s - not suitable\n", a->id_name);
    }

    return NULL;
}

void shutdown_daemon(struct category_option *co) {
    struct plist *pl;
    pl = plist_search(NULL, co->name);
    if (!pl)
        return;

    if (kill(pl->pid, SIGTERM) == -1)
        return;
    plist_wait(pl, 1000);
    if (!pl->status_changed) {
        if (kill(pl->pid, SIGKILL) == -1)
            return;
    }
}

void shutdown_optionals(struct category_option *co) {
    struct plist *pl;
    struct dcategory *c;
    char *s, *token;

    if (!co)
        return;

    c = find_category(co->name);
    if (c && co->user_preference) {
        s = strdup(co->user_preference);
        if (!s)
            report(R_FATAL, "Unable to allocate memory for optional daemons");
        for(token = strtok(s, " "); token; token = strtok(NULL, " ")) {
            pl = plist_search(token, NULL);
            if (!pl)
                return;

            if (kill(pl->pid, SIGTERM) == -1)
                return;
            plist_wait(pl, 1000);
            if (!pl->status_changed) {
                if (kill(pl->pid, SIGKILL) == -1)
                    return;
            }
        }
        free(s);
    }
}

void startup_daemon(struct category_option *co) {
    struct dapplication *a;

    if (!co)
        return;

    a = select_application(co);
    if (a && test_application(a))
        launch_application(a);
}

void startup_optionals(struct category_option *co) {
    struct dcategory *c;
    struct dapplication *d;
    char *s, *token;

    if (!co)
        return;

    c = find_category(co->name);
    if (c && co->user_preference) {
        s = strdup(co->user_preference);
        if (!s)
            report(R_FATAL, "Unable to allocate memory for optional daemons");
        for(token = strtok(s, " "); token; token = strtok(NULL, " ")) {
            d = find_application(token, co->name, 0);
            if (d && test_application(d))
                launch_application(d);
        }
        free(s);
    }
}

int test_application(struct dapplication *application) {
    int status, wstatus;
    pid_t pid;
    /* int sleep_remaining = TEST_TIMEOUT; */

    if (!application || !application->test_cmd)
        return 1;

    /* @TODO handle errors */
    install_default_sigchld_handler();
    unblock_signal(SIGCHLD);
    pid = fork();

    if (pid == 0) { /* child */
        char *args[] = { "/bin/sh", "-c", application->test_cmd, NULL };
        execvp(args[0], args);
        return 0; /* exec has failed */
    } else if (pid > 0) { /* parent */
        /* while ((status = waitpid(pid, &wstatus, WUNTRACED|WNOHANG)) == 0) { */
        /*     sleep_remaining = sleep(sleep_remaining); */
        /*     fprintf(stderr, "sleep interupted: %d remaining\n", sleep_remaining); */
        /*     if (sleep_remaining == 0) */
        /*         break; */
        /* } */
        status = waitpid(pid, &wstatus, WUNTRACED);

        if (status == -1) { /* an error occured */
            perror("waitpid");
            restore_sigchld_handler();
            return 0;
        } else if (status == 0) {   /* child has not returned yet */
            /* kill child */
            kill(pid, SIGKILL);
            restore_sigchld_handler();
            return 0;
        } else {    /* child has terminated and no error occured */
            if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == 0) {
                restore_sigchld_handler();
                return 1;
            } else {
                restore_sigchld_handler();
                return 0;
            }
        }
    } else {
        /* unable to fork into new process */
        restore_sigchld_handler();
        return 0;
    }
}

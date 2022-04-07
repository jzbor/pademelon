#include "common.h"
#include "desktop-application.h"
#include "desktop-files.h"
#include "signals.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
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

static struct dcategory categories[] = {
    /* CONFIG_SECTION_DAEMONS */
    { .name = "window-manager",     .section = CONFIG_SECTION_DAEMONS, .fallback = 1 },
    { .name = "compositor",         .xdg_name = "X11Compositor",    .section = CONFIG_SECTION_DAEMONS, .fallback = 1 },
    { .name = "dock",               .xdg_name = "Dock",             .section = CONFIG_SECTION_DAEMONS, .fallback = 0 },
    { .name = "hotkeys",            .xdg_name = "HotkeyDaemon",     .section = CONFIG_SECTION_DAEMONS, .fallback = 0 },
    { .name = "notifications",      .xdg_name = "NotificationDaemon", .section = CONFIG_SECTION_DAEMONS, .fallback = 1 },
    { .name = "polkit",             .xdg_name = "Polkit",           .section = CONFIG_SECTION_DAEMONS, .fallback = 1 },
    { .name = "power",              .xdg_name = "PowerManager",     .section = CONFIG_SECTION_DAEMONS, .fallback = 1 },
    { .name = "status",             .xdg_name = "Status",           .section = CONFIG_SECTION_DAEMONS, .fallback = 0 },
    /* the "special ones" */
    { .name = "applets",            .xdg_name = "Applet",           .section = CONFIG_SECTION_DAEMONS, .optional = 1 },
    { .name = "optional",           .xdg_name = "Autostart",        .section = CONFIG_SECTION_DAEMONS, .optional = 1 },

    /* CONFIG_SECTION_APPLICATIONS */
    { .name = "browser",            .xdg_name = "WebBrowser",   .section = CONFIG_SECTION_APPLICATIONS, .fallback = 1 },
    { .name = "dmenu",              .xdg_name = "AppLauncher",  .section = CONFIG_SECTION_APPLICATIONS, .fallback = 1 },
    { .name = "filemanager",        .xdg_name = "FileManager",  .section = CONFIG_SECTION_APPLICATIONS, .fallback = 1 },
    { .name = "terminal",           .xdg_name = "TerminalEmulator", .section = CONFIG_SECTION_APPLICATIONS, .fallback = 1 },
    { .name = NULL },
};

int export_application(struct dapplication *application, const char *name) {
    if ((!getenv(name))
            || (application->category && application->category->exported)) {
        if (application->category)
            application->category->exported = 1;
        return setenv(name, application->launch_cmd, 1);
    }
    return 1;
}

struct dcategory *get_categories(void) {
    return categories;
}

struct dcategory *find_category(const char *name) {
    int i;
    for (i = 0; categories[i].name; i++) {
        if ((categories[i].name && strcmp(categories[i].name, name) == 0)
            || (categories[i].xdg_name && strcmp(categories[i].xdg_name, name) == 0))
            return &categories[i];
    }
    return NULL;
}

void free_application(struct dapplication *a) {
    if (!a)
        return;

    free(a->display_name);
    free(a->id_name);
    free(a->desc);

    free(a->launch_cmd);
    free(a->test_cmd);
    free(a->settings);

    free(a);
}

void free_categories(void) {
    int i;
    for (i = 0; categories[i].name; i++) {
        free(categories[i].user_preference);
        free_application(categories[i].active_application);
    }
}

void launch_application(struct dapplication *application) {
    pid_t pid;
    int stderr_fd, devnull;

    if (!application)
        return;

    block_signal(SIGCHLD);
    pid = fork();

    if (pid == 0) { /* child */
        unblock_signal(SIGCHLD);

        /* disable output if possible */
        if ((stderr_fd = dup(STDERR_FILENO)) == -1) {
            stderr_fd = STDERR_FILENO;
            DBGPRINT("Unable to copy stderr file descriptor: %s\n", strerror(errno));
        } else if ((devnull = open("/dev/null", O_WRONLY)) == -1) {
            DBGPRINT("Unable to open /dev/null: %s\n", strerror(errno));
        } else {
            if (dup2(devnull, STDOUT_FILENO) == -1)
                DBGPRINT("Unable to redirect stdout of child process: %s\n", strerror(errno));
            if (dup2(devnull, STDERR_FILENO) == -1)
                DBGPRINT("Unable to redirect stderr of child process: %s\n", strerror(errno));
        }

        char *args[] = { "/bin/sh", "-c", application->launch_cmd, NULL };
        execvp(args[0], args);

        if (dup2(stderr_fd, STDERR_FILENO) == -1)
            DBGPRINT("Unable to reset stderr of child process: %s\n", strerror(errno));
        if (fprintf(stderr, "WARNING: Unable to launch application '%s'\n", application->launch_cmd) < 0)
            DBGPRINT("%s\n", "Unable to print to stderr");
        exit(EXIT_FAILURE);
    } else if (pid > 0) { /* parent */
        plist_add(pid, application);
        if (fprintf(stderr, "Launched application: %s (`%s`)\n", application->id_name, application->launch_cmd) < 0)
            DBGPRINT("%s\n", "Unable to print to stderr");
    } else {
        /* @TODO do we really want do die here? */
        die("Unable to fork into a new process");
    }
    unblock_signal(SIGCHLD);
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
        PRINT_PROPERTY_STR("; category", "unknown");
    }
    PRINT_PROPERTY_STR("command", a->launch_cmd);
    PRINT_PROPERTY_STR("test", a->test_cmd);
    PRINT_PROPERTY_BOOL("default", a->cdefault);

    if (fflush(stdout) == EOF)
        return -1;
    return 0;
}

struct dapplication *select_application(struct dcategory *c) {
    const char** dirs;
    struct dapplication *app = NULL;

    if (!c)
        return NULL;

    dirs = desktop_entry_dirs();
    if (!dirs) {
        DBGPRINT("unable to get desktop entry dirs");
        return NULL;
    }

    if (c->user_preference)
        app = application_by_name(dirs, c->user_preference, c->xdg_name);
    if (!app && c->fallback && c->xdg_name)
        app = application_by_category(dirs, c->xdg_name);

    return app;
}

void shutdown_all_daemons(void) {
    struct plist *pl;
    while ((pl = plist_peek())) {
        if (kill(pl->pid, SIGTERM) == -1) {
            plist_pop();
            continue;
        }
        plist_wait(pl, 1000);
        if (!pl->status_changed) {
            kill(pl->pid, SIGKILL);
        }
        plist_pop();
    }
}

void shutdown_daemon(struct dcategory *c) {
    struct plist *pl;
    pl = plist_search(NULL, c->name);
    if (!pl)
        return;

    if (kill(pl->pid, SIGTERM) == -1)
        return;
    plist_wait(pl, 1000);
    if (!pl->status_changed) {
        kill(pl->pid, SIGKILL);
    }
    plist_remove(pl->pid);
}

void shutdown_optionals(struct dcategory *c) {
    struct plist *pl;
    char *s, *token;

    if (!c)
        return;

    if (c->user_preference) {
        s = strdup(c->user_preference);
        if (!s)
            die("Unable to allocate memory for optional daemons");
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

void startup_daemon(struct dcategory *c) {
    struct dapplication *app;

    if (!c)
        return;

    app = select_application(c);
    if (!app)
        return;

    if (test_application(app))
        launch_application(app);
    c->active_application = app;
}

void startup_optionals(struct dcategory *c) {
    struct dapplication *d;
    char *s, *token;
    const char **dirs;

    if (!c)
        return;

    dirs = desktop_entry_dirs();
    if (!dirs) {
        DBGPRINT("unable to get desktop entry dirs");
        return;
    }

    if (c->user_preference) {
        s = strdup(c->user_preference);
        if (!s)
            die("Unable to allocate memory for optional daemons");
        for(token = strtok(s, " "); token; token = strtok(NULL, " ")) {
            d = application_by_name(dirs, token, c->xdg_name);
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

    return 1; /* @TODO implement later */

    if (!application || !application->test_cmd)
        return 1;

    /* @TODO handle errors */
    install_default_sigchld_handler();
    unblock_signal(SIGCHLD);
    pid = fork();

    if (pid == 0) { /* child */
        char *args[] = { "/bin/sh", "-c", application->test_cmd, NULL };
        execvp(args[0], args);
        exit(EXIT_SUCCESS); /* exec has failed */
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

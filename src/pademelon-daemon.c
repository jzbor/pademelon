#include "common.h"
#include "desktop-daemon.h"
#include "pademelon-daemon-config.h"
#include <dirent.h>
#include <errno.h>
#include <ini.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef DAEMON_FILE_ENDING
#define DAEMON_FILE_ENDING      ".ddaemon"
#endif


static void load_daemons(const char *dir);
static void loop(void);
static void setup_signals(void);
static void sigchld_handler(int signal);
static void sigint_handler(int signal);
static void startup_daemons(void);

static struct config *config;
static int end = 0;


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

void loop(void) {
    int temp;
    struct plist *pl;
    while (!end) {
        while ((pl = plist_next_event(NULL)) != NULL) {
            if (WIFEXITED(pl->status)) {
                temp = WEXITSTATUS(pl->status);
                report_value(R_DEBUG, "Process exited", &pl->pid, R_INTEGER);
                report_value(R_DEBUG, "Exit status", &temp, R_INTEGER);
                plist_remove(pl->pid);
            } else if (WIFSIGNALED(pl->status)) {
                temp = WTERMSIG(pl->status);
                report_value(R_DEBUG, "Process terminated", &pl->pid, R_INTEGER);
                report_value(R_DEBUG, "Terminated by signal", &temp, R_INTEGER);
                plist_remove(pl->pid);
            } else if (WIFSTOPPED(pl->status)) {
                temp = WSTOPSIG(pl->status);
                report_value(R_DEBUG, "Process stopped", &pl->pid, R_INTEGER);
                report_value(R_DEBUG, "Stopped by", &temp, R_INTEGER);
            } else if (WIFCONTINUED(pl->status)) {
                report_value(R_DEBUG, "Process continued", &pl->pid, R_INTEGER);
            }
        }

        sleep(5);
    }
}

static void setup_signals(void) {
	int status;
	struct sigaction sigaction_sigchld_handler = { .sa_handler = &sigchld_handler, .sa_flags = SA_NODEFER|SA_NOCLDSTOP|SA_RESTART};
	struct sigaction sigaction_sigint_handler = { .sa_handler = &sigint_handler, .sa_flags = SA_NODEFER|SA_RESTART};

	/* handle SIGCHLD*/
	status = sigfillset(&sigaction_sigchld_handler.sa_mask); // @TODO do I have to block anything here?
	if (status == -1)
		report(R_FATAL, "Unable to clear out a sigset");

	status = sigaction(SIGCHLD, &sigaction_sigchld_handler, NULL);
	if (status == -1)
		report(R_FATAL, "Unable to install signal handler");

    /* handle SIGINT */
	status = sigfillset(&sigaction_sigint_handler.sa_mask); // @TODO do I have to block anything here?
	if (status == -1)
		report(R_FATAL, "Unable to clear out a sigset");

	status = sigaction(SIGINT, &sigaction_sigint_handler, NULL);
	if (status == -1)
		report(R_FATAL, "Unable to install signal handler");

    init_sigset_sigchld();
}

void sigchld_handler(int signal) {
	pid_t pid;
	int status;
	int errno_save = errno;
    struct plist *pl;

	if (signal != SIGCHLD) {
		/* should not happen */
		return;
	}

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        pl = plist_get(pid);
        if (pl) {
            pl->status = status;
            pl->status_changed = 1;
        }
	}

	errno = errno_save;
}

void sigint_handler(int signal) {
	int errno_save = errno;

	if (signal != SIGINT) {
		/* should not happen */
		return;
	}

    end = 1;
	errno = errno_save;
}

void startup_daemon(const char *preference, const char *category, int auto_fallback) {
    struct ddaemon *d = select_ddaemon(preference, category, auto_fallback);
    if (d && test_ddaemon(d))
        launch_ddaemon(d);
}

void startup_daemons(void) {
    startup_daemon(config->compositor_daemon, "compositor", 1);
    startup_daemon(config->hotkey_daemon, "hotkeys", 1);
    startup_daemon(config->notification_daemon, "notifications", 1);
    startup_daemon(config->polkit_daemon, "polkit", 1);
    startup_daemon(config->power_daemon, "power", 1);
}

int main(int argc, char *argv[]) {
    int i, status;
    int print_only = 0;
    char *path;
    char *config_overwrite = NULL;
    char *daemon_dir_overwrite = NULL;

    setup_signals();

    /* load config */
    config = init_config();
    path = system_config_path("pademelon-daemon.conf");
    if (path) {
        status = ini_parse(path, &ini_config_callback, config);
        if (status < 0)
            report_value(R_WARNING, "Unable to read config file", path, R_STRING);
    }
    free(path);
    path = user_config_path("pademelon-daemon.conf");
    if (path) {
        status = ini_parse(path, &ini_config_callback, config);
        if (status < 0)
            report_value(R_WARNING, "Unable to read config file", path, R_STRING);
    }
    free(path);

    /* load daemons */
    path = system_data_path("daemons");
    load_daemons(path);
    free(path);
    path = system_local_data_path("daemons");
    load_daemons(path);
    free(path);
    path = user_data_path("daemons");
    load_daemons(path);
    free(path);

    for (i = 1; argv[i]; i++) {
        if (strcmp(argv[i], "--config") == 0 || strcmp(argv[i], "-c") == 0) {
            if (!argv[i + 1])
                report(R_FATAL, "Not enough arguments for --config");
            config_overwrite = argv[++i];
            report_value(R_DEBUG, "Config overwrite", config_overwrite, R_STRING);
            status = ini_parse(config_overwrite, &ini_config_callback, config);
            if (status < 0)
                report_value(R_WARNING, "Unable to read config file", path, R_STRING);
        } else if (strcmp(argv[i], "--daemon-dir") == 0 || strcmp(argv[i], "-d") == 0) {
            if (!argv[i + 1])
                report(R_FATAL, "Not enough arguments for --daemon-dir");
            daemon_dir_overwrite = argv[++i];
            free_ddaemons();
            free_categories();
            report_value(R_DEBUG, "Daemon dir overwrite", daemon_dir_overwrite, R_STRING);
            load_daemons(daemon_dir_overwrite);
        } else if (strcmp(argv[i], "--categories") == 0 || strcmp(argv[i], "-k") == 0) {
            if (printf("\n;;; CATEGORIES ;;;\n\n") < 0)
                report(R_ERROR, "Unable to write to stdout");
            if (print_categories() < 0)
                report(R_ERROR, "Unable to write to stdout");
            print_only = 1;
        } else if (strcmp(argv[i], "--print-config") == 0 || strcmp(argv[i], "-p") == 0) {
            if (printf("\n;;; CONFIG ;;;\n\n") < 0)
                report(R_ERROR, "Unable to write to stdout");
            if (print_config(config) < 0)
                report(R_ERROR, "Unable to write to stdout");
            print_only = 1;
        } else if (strcmp(argv[i], "--daemons") == 0 || strcmp(argv[i], "-t") == 0) {
            if (printf("\n;;; DAEMONS ;;;\n\n") < 0)
                report(R_ERROR, "Unable to write to stdout");
            if (print_ddaemons() < 0)
                report(R_ERROR, "Unable to write to stdout");
            print_only = 1;
        } else {
            if (printf("Usage: %s [--daemons] [--categories] [--print-config] [--config <config>] [--daemon-dir <dir>]\n", argv[0]) < 0)
                report(R_ERROR, "Unable to write to stderr");
        }
    }

    if (!print_only) {
        startup_daemons();
        loop();
    }

    plist_free();
    free_config(config);
    free_ddaemons();
    free_categories();
}

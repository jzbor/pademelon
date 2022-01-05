#include "common.h"
#include "desktop-daemon.h"
#include "pademelon-daemon-config.h"
#include "signals.h"
#include "tools.h"
#include "x11-utils.h"
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
#define CYCLE_LENGTH                1   /* seconds */
#define SECS_TO_WALLPAPER_REFRESH   5   /* seconds, bigger than CYCLE_LENGTH */

static void load_daemons(const char *dir);
static void load_keyboard(void);
static void load_wallpaper(void);
static void loop(void);
static void setup_signals(void);
static void sigint_handler(int signal);
static void startup_daemons(void);

static struct config *config;
static int end = 0;
static char *wm_overwrite = NULL;


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

void load_keyboard(void) {
    if (config->keyboard_settings) {
        char temp[sizeof("setxkbmap ") + strlen(config->keyboard_settings) + 1];
        strcpy(temp, "setxkbmap ");
        strcat(temp, config->keyboard_settings);
        execute(temp);
    }
}

void load_wallpaper(void) {
    if (config->set_wallpaper) {
        tl_load_wallpaper(0, NULL);
    }
}

void loop(void) {
    int temp, wp_cycle_counter;
    struct plist *pl;

    wp_cycle_counter = -1;

    while (!end) {
        while ((pl = plist_next_event(NULL)) != NULL) {
            if (WIFEXITED(pl->status)) {
                temp = WEXITSTATUS(pl->status);
                report_value(R_DEBUG, "Process exited", &pl->pid, R_INTEGER);
                report_value(R_DEBUG, "Process belonged to daemon", ((struct ddaemon*) pl->content)->id_name, R_STRING);
                report_value(R_DEBUG, "Exit status", &temp, R_INTEGER);
                plist_remove(pl->pid);
            } else if (WIFSIGNALED(pl->status)) {
                temp = WTERMSIG(pl->status);
                report_value(R_DEBUG, "Process terminated", &pl->pid, R_INTEGER);
                report_value(R_DEBUG, "Process belonged to daemon", ((struct ddaemon*) pl->content)->id_name, R_STRING);
                report_value(R_DEBUG, "Terminated by signal", &temp, R_INTEGER);
                plist_remove(pl->pid);
            } else if (WIFSTOPPED(pl->status)) {
                temp = WSTOPSIG(pl->status);
                report_value(R_DEBUG, "Process stopped", &pl->pid, R_INTEGER);
                report_value(R_DEBUG, "Process belongs to daemon", ((struct ddaemon*) pl->content)->id_name, R_STRING);
                report_value(R_DEBUG, "Stopped by", &temp, R_INTEGER);
            } else if (WIFCONTINUED(pl->status)) {
                report_value(R_DEBUG, "Process belongs to daemon", ((struct ddaemon*) pl->content)->id_name, R_STRING);
                report_value(R_DEBUG, "Process continued", &pl->pid, R_INTEGER);
            }

            if (WIFEXITED(pl->status)|| WIFSIGNALED(pl->status)) {
                if (((struct ddaemon*) pl->content) && strcmp(((struct ddaemon*) pl->content)->category->name, "window-manager") == 0) {
                    end = 1;
                }
            }
        }

        if (x11_screen_has_changed()) {
            report(R_DEBUG, "Screen configuration has changed");
            wp_cycle_counter = SECS_TO_WALLPAPER_REFRESH / CYCLE_LENGTH;
            tl_save_display_conf(0, NULL);
            load_wallpaper();
        }

        /* supposed to workaround bugs but it does not seem to work */
        if (wp_cycle_counter == 0) {
            load_wallpaper();
        }
        if (wp_cycle_counter >= 0)
            wp_cycle_counter--;

        sleep(CYCLE_LENGTH);
    }
}

static void setup_signals(void) {
	int status;
	struct sigaction sigaction_sigint_handler = { .sa_handler = &sigint_handler, .sa_flags = SA_NODEFER|SA_RESTART};

    /* handle SIGCHLD */
    if (!install_plist_sigchld_handler())
		report(R_FATAL, "Unable to install plist sigchld handler");

    /* handle SIGINT */
	status = sigfillset(&sigaction_sigint_handler.sa_mask); // @TODO do I have to block anything here?
	if (status == -1)
		report(R_FATAL, "Unable to clear out a sigset");

	status = sigaction(SIGINT, &sigaction_sigint_handler, NULL);
	if (status == -1)
		report(R_FATAL, "Unable to install signal handler");
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
    struct dcategory *c;
    struct ddaemon *d;
    char *s, *token;
    if (!config->no_window_manager)
        startup_daemon(wm_overwrite ? wm_overwrite :config->window_manager, "window-manager", 1);

    sleep(5);

    startup_daemon(config->compositor_daemon, "compositor", 1);
    startup_daemon(config->hotkey_daemon, "hotkeys", 0);
    startup_daemon(config->notification_daemon, "notifications", 1);
    startup_daemon(config->polkit_daemon, "polkit", 1);
    startup_daemon(config->power_daemon, "power", 1);
    startup_daemon(config->status_daemon, "status", 0);

    c = find_category("applets");
    if (c && config->applets) {
        s = strdup(config->applets);
        if (!s)
            report(R_FATAL, "Unable to allocate memory for applets");
        for(token = strtok(s, " "); token; token = strtok(NULL, " ")) {
            d = find_ddaemon(token, "applets", 0);
            if (d && test_ddaemon(d))
                launch_ddaemon(d);
        }
        free(s);
    }
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
    path = system_config_path("pademelon.conf");
    if (path) {
        status = ini_parse(path, &ini_config_callback, config);
        if (status < 0)
            report_value(R_WARNING, "Unable to read config file", path, R_STRING);
    }
    free(path);
    path = user_config_path("pademelon.conf");
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
        } else if (strcmp(argv[i], "--no-window-manager") == 0 || strcmp(argv[i], "-n") == 0) {
            config->no_window_manager = 1;
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
        } else if (strcmp(argv[i], "--window-manager") == 0 || strcmp(argv[i], "-w") == 0) {
            if (!argv[i + 1])
                report(R_FATAL, "Not enough arguments for --window-manager");
            free(config->window_manager);
            config->window_manager = strdup(argv[++i]);
            if (!config->window_manager)
                report(R_FATAL, "Unable to allocate memory for settings");
        } else {
            if (printf("Usage: %s [--daemons] [--categories] [--print-config] [--config <config>] [--daemon-dir <dir>]\n", argv[0]) < 0)
                report(R_FATAL, "Unable to write to stderr");
        }
    }

    /* MoonWM workaround */
    if (setenv("MOONWM_NO_AUTOSTART", "1", 1) == -1)
        report(R_ERROR, "Unable to set env var");

    if (!print_only) {
        x11_init();
        load_wallpaper();
        load_keyboard();
        tl_load_display_conf(0, NULL);
        startup_daemons();
        loop();
        x11_deinit();
    }

    plist_free();
    free_config(config);
    free_ddaemons();
    free_categories();
}

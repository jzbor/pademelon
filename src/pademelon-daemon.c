#include "common.h"
#include "desktop-application.h"
#include "pademelon-config.h"
#include "signals.h"
#include "tools.h"
#ifdef X11
#include "x11-utils.h"
#endif /* X11 */
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define CYCLE_LENGTH                1   /* seconds */
#define SECS_TO_WALLPAPER_REFRESH   5   /* seconds, bigger than CYCLE_LENGTH */

static void load_keyboard(void);
static void loop(void);
void set_application(struct category_option *co, const char *export_name);
static void reload_config(void);
static void setup_signals(void);
static void sigint_handler(int signal);
static void sigusr1_handler(int signal);
static void sigusr2_handler(int signal);
static void startup_daemons(int initial);

static struct config *config;
static int end = 0;
static int ignore_wm_shutdown = 0;
static int launch_setup = 0;
static int restart_daemons = 0;
static int restart_wm = 0;


void load_keyboard(void) {
    if (config->keyboard_settings) {
        char temp[sizeof("setxkbmap ") + strlen(config->keyboard_settings) + 1];
        strcpy(temp, "setxkbmap ");
        strcat(temp, config->keyboard_settings);
        execute(temp);
    }
}

void loop(void) {
    int temp, wp_cycle_counter;
    struct plist *pl;

    wp_cycle_counter = -1;

    while (!end) {
        while ((pl = plist_next_event(NULL)) != NULL) {
            if (WIFEXITED(pl->status)|| WIFSIGNALED(pl->status)) {
                if (((struct dapplication*) pl->content)
                        && strcmp(((struct dapplication*) pl->content)->category->name, "window-manager") == 0) {
                    if (ignore_wm_shutdown)
                        ignore_wm_shutdown = 0;
                    else
                        end = 1;
                }
            }

            if (WIFEXITED(pl->status)) {
                temp = WEXITSTATUS(pl->status);
                report_value(R_DEBUG, "Process exited", &pl->pid, R_INTEGER);
                report_value(R_DEBUG, "Process belonged to daemon", ((struct dapplication*) pl->content)->id_name, R_STRING);
                report_value(R_DEBUG, "Exit status", &temp, R_INTEGER);
                plist_remove(pl->pid);
            } else if (WIFSIGNALED(pl->status)) {
                temp = WTERMSIG(pl->status);
                report_value(R_DEBUG, "Process terminated", &pl->pid, R_INTEGER);
                report_value(R_DEBUG, "Process belonged to daemon", ((struct dapplication*) pl->content)->id_name, R_STRING);
                report_value(R_DEBUG, "Terminated by signal", &temp, R_INTEGER);
                plist_remove(pl->pid);
            } else if (WIFSTOPPED(pl->status)) {
                temp = WSTOPSIG(pl->status);
                report_value(R_DEBUG, "Process stopped", &pl->pid, R_INTEGER);
                report_value(R_DEBUG, "Process belongs to daemon", ((struct dapplication*) pl->content)->id_name, R_STRING);
                report_value(R_DEBUG, "Stopped by", &temp, R_INTEGER);
            } else if (WIFCONTINUED(pl->status)) {
                report_value(R_DEBUG, "Process belongs to daemon", ((struct dapplication*) pl->content)->id_name, R_STRING);
                report_value(R_DEBUG, "Process continued", &pl->pid, R_INTEGER);
            }
        }

#ifdef X11
        if (x11_screen_has_changed() || restart_wm) {
            report(R_DEBUG, "Screen configuration has changed");
            wp_cycle_counter = SECS_TO_WALLPAPER_REFRESH / CYCLE_LENGTH;
            tl_save_display_conf(0, NULL);
            tl_load_wallpaper(0, NULL);
        }
        /* supposed to workaround bugs but it does not seem to work */
        if (wp_cycle_counter == 0) {
            tl_load_wallpaper(0, NULL);
        }
        if (wp_cycle_counter >= 0)
            wp_cycle_counter--;
#endif /* X11 */

        if (restart_daemons) {
            restart_daemons = 0;
            restart_wm = 0;
            reload_config();
            startup_daemons(0);
        }
        if (restart_wm) {
            ignore_wm_shutdown = 1;
            restart_wm = 0;
            reload_config();
            shutdown_daemon(config->window_manager);
            startup_daemon(config->window_manager);
        }

        sleep(CYCLE_LENGTH);
    }
}

void set_application(struct category_option *co, const char *export_name) {
    struct dapplication *a = select_application(co);
    if (a && test_application(a))
        export_application(a, export_name);
}

void reload_config(void) {
    struct config *new_config;
    new_config = load_config();
    if (new_config) {
        free(config);
        config = new_config;
    }
}

static void setup_signals(void) {
	int status;
	struct sigaction sigaction_sigint_handler = { .sa_handler = &sigint_handler, .sa_flags = SA_NODEFER|SA_RESTART};
	struct sigaction sigaction_sigusr1_handler = { .sa_handler = &sigusr1_handler, .sa_flags = SA_NODEFER|SA_RESTART};
	struct sigaction sigaction_sigusr2_handler = { .sa_handler = &sigusr2_handler, .sa_flags = SA_NODEFER|SA_RESTART};

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

    /* handle SIGUSR1 */
	status = sigfillset(&sigaction_sigusr1_handler.sa_mask); // @TODO do I have to block anything here?
	if (status == -1)
		report(R_FATAL, "Unable to clear out a sigset");
	status = sigaction(SIGUSR1, &sigaction_sigusr1_handler, NULL);
	if (status == -1)
		report(R_FATAL, "Unable to install signal handler");

    /* handle SIGUSR2 */
	status = sigfillset(&sigaction_sigusr2_handler.sa_mask); // @TODO do I have to block anything here?
	if (status == -1)
		report(R_FATAL, "Unable to clear out a sigset");
	status = sigaction(SIGUSR2, &sigaction_sigusr2_handler, NULL);
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

void sigusr1_handler(int signal) {
	int errno_save = errno;

	if (signal != SIGUSR1) {
		/* should not happen */
		return;
	}

    restart_daemons = 1;
	errno = errno_save;
}

void sigusr2_handler(int signal) {
	int errno_save = errno;

	if (signal != SIGUSR2) {
		/* should not happen */
		return;
	}

    restart_wm = 1;
	errno = errno_save;
}

void startup_daemons(int initial) {
    /* set default applications */
    set_application(config->browser, "BROWSER");
    set_application(config->terminal, "TERMINAL");

    if (initial) {
        /* start window manager */
        if (launch_setup) {
            /* a = find_application("pademelon-setup", NULL, 0); */
            /* if (a && test_application(a)) */
            /*     launch_application(a); */
            /* else */
            /*     return; */
            char *args[] = { "/bin/sh", "-c", "pademelon-settings", NULL };
            execvp(args[0], args);
            exit(EXIT_FAILURE);
        } else if (!config->no_window_manager) {
            startup_daemon(config->window_manager);
            sleep(3);
        }
    }

    if (!initial) {
        /* shutdown daemons */
        shutdown_daemon(config->compositor_daemon);
        shutdown_daemon(config->hotkey_daemon);
        shutdown_daemon(config->notification_daemon);
        shutdown_daemon(config->polkit_daemon);
        shutdown_daemon(config->power_daemon);
        shutdown_daemon(config->status_daemon);

        /* shutdown optional daemons */
        shutdown_optionals(config->applets);
        shutdown_optionals(config->optional);
    }

    /* start daemons */
    startup_daemon(config->compositor_daemon);
    startup_daemon(config->hotkey_daemon);
    startup_daemon(config->notification_daemon);
    startup_daemon(config->polkit_daemon);
    startup_daemon(config->power_daemon);
    startup_daemon(config->status_daemon);

    /* start optional daemons */
    startup_optionals(config->applets);
    startup_optionals(config->optional);
}

int main(int argc, char *argv[]) {
    int i;

    setup_signals();

    /* load config */
    config = load_config();

    /* load applications */
    load_applications();

    for (i = 1; argv[i]; i++) {
        if (strcmp(argv[i], "--no-window-manager") == 0 || strcmp(argv[i], "-n") == 0) {
            config->no_window_manager = 1;
        } else if (strcmp(argv[i], "--setup") == 0 || strcmp(argv[i], "-n") == 0) {
            launch_setup = 1;
        } else if (strcmp(argv[i], "--window-manager") == 0 || strcmp(argv[i], "-w") == 0) {
            if (!argv[i + 1])
                report(R_FATAL, "Not enough arguments for --window-manager");
            free(config->window_manager);
            config->window_manager->user_preference = strdup(argv[++i]);
            if (!config->window_manager)
                report(R_FATAL, "Unable to allocate memory for settings");
        } else {
            if (printf("Usage: %s [--no-window-manager] [--window-manager <window-manager>] [--setup]\n", argv[0]) < 0)
                report(R_FATAL, "Unable to write to stderr");
        }
    }

    /* MoonWM workaround */
    if (setenv("MOONWM_NO_AUTOSTART", "1", 1) == -1)
        report(R_ERROR, "Unable to set env var");
    if (setenv("MOONWM_NO_STATUS", "1", 1) == -1)
        report(R_ERROR, "Unable to set env var");

#ifdef X11
    x11_init();
#endif /* X11 */

    tl_load_wallpaper(0, NULL);
    load_keyboard();
    tl_load_display_conf(0, NULL);
    startup_daemons(1);
    loop();

#ifdef X11
    x11_deinit();
#endif /* X11 */
    plist_free();
    free_config(config);
    free_applications();
    free_categories();
}

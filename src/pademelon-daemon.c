#include "common.h"
#include "desktop-application.h"
#include "pademelon-config.h"
#include "signals.h"
#include "tools.h"
#include "x11-utils.h"
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
void set_application(const char *preference, const char *category, const char *export_name, int auto_fallback);
static void setup_signals(void);
static void sigint_handler(int signal);
static void startup_applications(void);

static struct config *config;
static int end = 0;


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

            if (WIFEXITED(pl->status)|| WIFSIGNALED(pl->status)) {
                if (((struct dapplication*) pl->content) && strcmp(((struct dapplication*) pl->content)->category->name, "window-manager") == 0) {
                    end = 1;
                }
            }
        }

        if (x11_screen_has_changed()) {
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

        sleep(CYCLE_LENGTH);
    }
}

void set_application(const char *preference, const char *category, const char *export_name, int auto_fallback) {
    struct dapplication *a = select_application(preference, category, auto_fallback);
    if (a && test_application(a))
        export_application(a, export_name);
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

void startup_application(const char *preference, const char *category, int auto_fallback) {
    struct dapplication *a = select_application(preference, category, auto_fallback);
    if (a && test_application(a))
        launch_application(a);
}

void startup_applications(void) {
    struct dcategory *c;
    struct dapplication *d;
    char *s, *token;

    /* set default applications */
    set_application(config->browser, "browser", "BROWSER", 1);
    set_application(config->terminal, "terminal", "TERMINAL", 1);

    /* start window manager */
    if (!config->no_window_manager)
        startup_application(config->window_manager, "window-manager", 1);

    sleep(3);

    /* start daemons */
    startup_application(config->compositor_daemon, "compositor", 1);
    startup_application(config->hotkey_daemon, "hotkeys", 0);
    startup_application(config->notification_daemon, "notifications", 1);
    startup_application(config->polkit_daemon, "polkit", 1);
    startup_application(config->power_daemon, "power", 1);
    startup_application(config->status_daemon, "status", 0);

    c = find_category("applets");
    if (c && config->applets) {
        s = strdup(config->applets);
        if (!s)
            report(R_FATAL, "Unable to allocate memory for applets");
        for(token = strtok(s, " "); token; token = strtok(NULL, " ")) {
            d = find_application(token, "applets", 0);
            if (d && test_application(d))
                launch_application(d);
        }
        free(s);
    }
}

int main(int argc, char *argv[]) {
    int i;
    int print_only = 0;

    setup_signals();

    /* load config */
    config = load_config();

    /* load applications */
    load_applications();

    for (i = 1; argv[i]; i++) {
        if (strcmp(argv[i], "--no-window-manager") == 0 || strcmp(argv[i], "-n") == 0) {
            config->no_window_manager = 1;
        } else if (strcmp(argv[i], "--window-manager") == 0 || strcmp(argv[i], "-w") == 0) {
            if (!argv[i + 1])
                report(R_FATAL, "Not enough arguments for --window-manager");
            free(config->window_manager);
            config->window_manager = strdup(argv[++i]);
            if (!config->window_manager)
                report(R_FATAL, "Unable to allocate memory for settings");
        } else {
            if (printf("Usage: %s [--no-window-manager] [--window-manager <window-manager>]\n", argv[0]) < 0)
                report(R_FATAL, "Unable to write to stderr");
        }
    }

    /* MoonWM workaround */
    if (setenv("MOONWM_NO_AUTOSTART", "1", 1) == -1)
        report(R_ERROR, "Unable to set env var");

    if (!print_only) {
        x11_init();
        tl_load_wallpaper(0, NULL);
        load_keyboard();
        tl_load_display_conf(0, NULL);
        startup_applications();
        loop();
        x11_deinit();
    }

    plist_free();
    free_config(config);
    free_applications();
    free_categories();
}

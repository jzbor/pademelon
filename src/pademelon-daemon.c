#include "common.h"
#include "desktop-application.h"
#include "pademelon-config.h"
#include "signals.h"
#include "tools.h"
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef X11
#include "x11-utils.h"
#include <poll.h>
#endif /* X11 */

#ifdef LIBNOTIFY
#include <libnotify/notify.h>
#endif /* LIBNOTIFY */


#define CYCLE_TIMEOUT               2   /* seconds */
#define CYCLE_TIMEOUT_X11           10   /* seconds */
#define SECS_TO_WALLPAPER_REFRESH   5   /* seconds, bigger than CYCLE_LENGTH */
#define TIMEOUT_AFTER_WM_START      0   /* seconds */

static void export_applications(void);
static void launch_wm(void);
static void load_keyboard(void);
static void loop(void);
static void notify_termination(struct dapplication *app, pid_t pid, char *msg);
#ifdef LIBNOTIFY
static void notification_callback(NotifyNotification *notification, char *action, gpointer user_data);
static void notification_closed (NotifyNotification *notify, void *user_data);
#endif
void set_application(struct dcategory *c, const char *export_name);
static void reload_config(void);
static void setup_signals(void);
static void shutdown_daemons(void);
static void sigint_handler(int signal);
static void sigusr1_handler(int signal);
static void sigusr2_handler(int signal);
static void startup_daemons(void);

static struct config *config;
static int end = 0;
static int ignore_wm_shutdown = 0;
static int launch_setup = 0;
static int reload = 0;


void export_applications(void) {
    /* set default applications */
    set_application(config->browser, "BROWSER");
    set_application(config->dmenu, "DMENUCMD");
    set_application(config->filemanager, "FILEMANAGER");
    set_application(config->terminal, "TERMINAL");
}

static void launch_wm(void) {
    /* start window manager */
    if (launch_setup) {
        char *args[] = { "/bin/sh", "-c", "pademelon-settings", NULL };
        execvp(args[0], args);
        exit(EXIT_FAILURE);
    } else if (!config->no_window_manager) {
        if (!startup_daemon(config->window_manager)) {
            fprintf(stderr, "No window manager found\n");
            exit(1);
        }
    }
}

void load_keyboard(void) {
    if (config->keyboard_settings) {
        char temp[sizeof("setxkbmap ") + strlen(config->keyboard_settings) + 1];
        strcpy(temp, "setxkbmap ");
        strcat(temp, config->keyboard_settings);
        execute(temp);
    }
}

void loop(void) {
    struct plist *pl;

#ifdef X11
    struct pollfd fds = {0};
    int poll_status = 0;
    fds.fd = x11_connection_number();
    fds.events = POLLIN;
#endif /* X11 */

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
                DBGPRINT("Process '%s' (pid: %d) exited with return code %d\n", ((struct dapplication*) pl->content)->id_name, pl->pid, WEXITSTATUS(pl->status));
                notify_termination(((struct dapplication*) pl->content), pl->pid, "exited");
                plist_remove(pl->pid);
            } else if (WIFSIGNALED(pl->status)) {
                DBGPRINT("Process '%s' (pid: %d) was terminated by signal %d\n", ((struct dapplication*) pl->content)->id_name, pl->pid, WTERMSIG(pl->status));
                notify_termination(((struct dapplication*) pl->content), pl->pid, "was terminated by a signal");
                plist_remove(pl->pid);
            } else if (WIFSTOPPED(pl->status)) {
                DBGPRINT("Process '%s' (pid: %d) was stopped by signal %d\n", ((struct dapplication*) pl->content)->id_name, pl->pid, WSTOPSIG(pl->status));
            } else if (WIFCONTINUED(pl->status)) {
                DBGPRINT("Process '%s' (pid: %d) was continued\n", ((struct dapplication*) pl->content)->id_name, pl->pid);
            }
        }

#ifdef X11
        if (x11_screen_has_changed()) {
            DBGPRINT("%s\n", "Screen configuration has changed");
            tl_save_display_conf();
            tl_load_wallpaper();
        }

        if (x11_keyboard_has_changed()) {
            DBGPRINT("%s\n", "Keyboard configuration has changed");
            load_keyboard();
        }
#endif /* X11 */

        if (reload) {
            reload = 0;
            /* ignore_wm_shutdown = 1; */

            reload_config();
            shutdown_daemons();
            shutdown_daemon(config->window_manager);

            /* remove wm from plist, so it pademelon does not exit */
            pl = plist_search(NULL, config->window_manager->name);
            if (pl)
                plist_remove(pl->pid);

            export_applications();
            startup_daemon(config->window_manager);
            sleep(TIMEOUT_AFTER_WM_START);
            startup_daemons();

            load_keyboard();
            tl_load_wallpaper();
        }


#ifdef X11
        poll_status = poll(&fds, 1, CYCLE_TIMEOUT_X11 * 1000);
        if (poll_status < 0) { /* error or signal */
            if (errno != EINTR)
                end = 1;
        }
#else /* X11 */
        sleep(CYCLE_TIMEOUT);
#endif /* X11 */

    }
}

void notify_termination(struct dapplication *app, pid_t pid, char *msg) {
#ifdef LIBNOTIFY
    int status;
    char title[] = "A process has terminated";
    char content_format[] = "Process '%s' (pid: %d) %s";
    char content[strlen(content_format) + strlen(app->id_name) + strlen("really long pid") + strlen(msg) + 1 ];
    GMainLoop *loop = NULL;

    DBGPRINT("Creating notification\n");
    status = snprintf(content, sizeof(content) / sizeof(char), content_format, app->id_name, pid, msg);
    if (status < 0) {
        DBGPRINT("snprintf failed: %s\n", strerror(errno));
        return;
    }

	NotifyNotification *notification = notify_notification_new(title, content, "dialog-information");
    notify_notification_add_action(notification, "restart", "Restart Application", notification_callback, NULL, NULL);
	notify_notification_show(notification, NULL);

    /* wait for feedback */
    loop = g_main_loop_new (NULL, FALSE);
    DBGPRINT("user_data in: %p\n", (void *) loop);
    g_signal_connect(G_OBJECT(notification), "closed", G_CALLBACK(notification_closed), (void *) loop);
    /* g_timeout_add(NOTIFY_EXPIRES_DEFAULT, notification_timeout, NULL); */
    g_main_loop_run(loop);
    g_main_loop_unref(loop);

	g_object_unref(G_OBJECT(notification));

    DBGPRINT("Showed notification\n");
#else /* LIBNOTIFY */
    DBGPRINT("Not compiled with libnotify support\n");
    return;
#endif /* LIBNOTIFY */
}

#ifdef LIBNOTIFY
void notification_callback(NotifyNotification *notification, char *action, gpointer user_data) {
    DBGPRINT("Callback: %s\n", action);
    printf("test\n");
    fflush(stdout);
}

int notification_timeout(void *data) {
    DBGPRINT("Notification timed out\n");
    GMainLoop *loop = (GMainLoop *) loop;
    g_main_loop_quit(loop);
    return 0;
}

void notification_closed (NotifyNotification *notify, void *user_data) {
    DBGPRINT("user_data out: %p\n", user_data);
    GMainLoop *loop = (GMainLoop *) user_data;
    g_main_loop_quit(loop);
}
#endif /* LIBNOTIFY */

void set_application(struct dcategory *c, const char *export_name) {
    struct dapplication *app;
    if (!export_name || !c)
        return;

    app = select_application(c);
    if (!app)
        return;

    else if (test_application(app))
        export_application(app, export_name);
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
		die("Unable to install plist sigchld handler");

    /* handle SIGINT */
	status = sigfillset(&sigaction_sigint_handler.sa_mask); // @TODO do I have to block anything here?
	if (status == -1)
		die("Unable to clear out a sigset");
	status = sigaction(SIGINT, &sigaction_sigint_handler, NULL);
	if (status == -1)
		die("Unable to install signal handler");
	status = sigaction(SIGTERM, &sigaction_sigint_handler, NULL);
	if (status == -1)
		die("Unable to install signal handler");

    /* handle SIGUSR1 */
	status = sigfillset(&sigaction_sigusr1_handler.sa_mask); // @TODO do I have to block anything here?
	if (status == -1)
		die("Unable to clear out a sigset");
	status = sigaction(SIGUSR1, &sigaction_sigusr1_handler, NULL);
	if (status == -1)
		die("Unable to install signal handler");

    /* handle SIGUSR2 */
	status = sigfillset(&sigaction_sigusr2_handler.sa_mask); // @TODO do I have to block anything here?
	if (status == -1)
		die("Unable to clear out a sigset");
	status = sigaction(SIGUSR2, &sigaction_sigusr2_handler, NULL);
	if (status == -1)
		die("Unable to install signal handler");
}

void shutdown_daemons() {
    /* shutdown daemons */
    shutdown_daemon(config->compositor_daemon);
    shutdown_daemon(config->dock_daemon);
    shutdown_daemon(config->hotkey_daemon);
    shutdown_daemon(config->notification_daemon);
    shutdown_daemon(config->polkit_daemon);
    shutdown_daemon(config->power_daemon);
    shutdown_daemon(config->status_daemon);

    /* shutdown optional daemons */
    shutdown_optionals(config->applets);
    shutdown_optionals(config->optional);
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

    reload = 1;
	errno = errno_save;
}

void sigusr2_handler(int signal) {
	int errno_save = errno;

	if (signal != SIGUSR2) {
		/* should not happen */
		return;
	}

    /* currently ignored */

	errno = errno_save;
}

void startup_daemons() {
    /* start daemons */
    startup_daemon(config->compositor_daemon);
    startup_daemon(config->dock_daemon);
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

    for (i = 1; argv[i]; i++) {
        if (strcmp(argv[i], "--no-window-manager") == 0 || strcmp(argv[i], "-n") == 0) {
            config->no_window_manager = 1;
        } else if (strcmp(argv[i], "--setup") == 0 || strcmp(argv[i], "-n") == 0) {
            launch_setup = 1;
        } else if (strcmp(argv[i], "--window-manager") == 0 || strcmp(argv[i], "-w") == 0) {
            if (!argv[i + 1])
                die("Not enough arguments for --window-manager");
            free(config->window_manager);
            config->window_manager->user_preference = strdup(argv[++i]);
            if (!config->window_manager)
                die("Unable to allocate memory for settings");
        } else {
            if (printf("Usage: %s [--no-window-manager] [--window-manager <window-manager>] [--setup]\n", argv[0]) < 0)
                die("Unable to write to stderr");
        }
    }

    export_applications();
    launch_wm();
#ifdef X11
    x11_init();
    load_keyboard();
    tl_load_display_conf(NULL);
    x11_screen_has_changed(); /* clear event queue */
    tl_load_wallpaper();
#endif /* X11 */
#ifdef LIBNOTIFY
    notify_init("Pademelon Daemon");
#endif /* LIBNOTIFY */
    sleep(TIMEOUT_AFTER_WM_START);
    startup_daemons();
    loop();

    shutdown_all_daemons();
#ifdef LIBNOTIFY
    notify_uninit();
#endif /* LIBNOTIFY */
#ifdef X11
    x11_deinit();
#endif /* X11 */
    plist_free();
    free_config(config);
    free_categories();
}

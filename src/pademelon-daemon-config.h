#ifndef H_PADEMELON_DAEMON_CONFIG
#define H_PADEMELON_DAEMON_CONFIG

#include <stddef.h>

#define CONFIG_SECTION_DAEMONS      "daemons"
#define CONFIG_SECTION_APPEARANCE   "appearance"

struct config {
    /* CONFIG_SECTION_DAEMONS */
    int no_window_manager;
    char *window_manager;
    char *compositor_daemon;
    char *hotkey_daemon;
    char *notification_daemon;
    char *polkit_daemon;
    char *power_daemon;
    char *status_daemon;
    char *applets;

    /* CONFIG_SECTION_APPEARANCE */
    int set_wallpaper;
};

void free_config(struct config *cfg);
int ini_config_callback(void* user, const char* section, const char* name, const char* value);
struct config *init_config(void);
int print_config(struct config *cfg);

#endif /* H_PADEMELON_DAEMON_CONFIG */

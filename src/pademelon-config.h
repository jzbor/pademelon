#ifndef H_PADEMELON_CONFIG
#define H_PADEMELON_CONFIG

#include <stddef.h>

#define CONFIG_SECTION_DAEMONS      "daemons"
#define CONFIG_SECTION_APPLICATIONS "applications"
#define CONFIG_SECTION_INPUT        "input"

struct config {
    /* CONFIG_SECTION_DAEMONS */
    int no_window_manager;
    struct dcategory *window_manager;
    struct dcategory *compositor_daemon;
    struct dcategory *dock_daemon;
    struct dcategory *hotkey_daemon;
    struct dcategory *notification_daemon;
    struct dcategory *polkit_daemon;
    struct dcategory *power_daemon;
    struct dcategory *status_daemon;
    struct dcategory *applets;
    struct dcategory *optional;

    /* CONFIG_SECTION_APPLICATIONS */
    struct dcategory *browser;
    struct dcategory *dmenu;
    struct dcategory *filemanager;
    struct dcategory *terminal;

    /* CONFIG_SECTION_INPUT */
    char *keyboard_settings;
};

void free_config(struct config *cfg);
int ini_config_callback(void* user, const char* section, const char* name, const char* value);
struct config *init_config(void);
struct config *load_config(void);
int print_config(struct config *cfg);


static const struct config default_config = {
    /* CONFIG_SECTION_DAEMONS */
    .no_window_manager = 0,
};

#endif /* H_PADEMELON_CONFIG */

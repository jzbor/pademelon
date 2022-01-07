#ifndef H_PADEMELON_CONFIG
#define H_PADEMELON_CONFIG

#include <stddef.h>

#define CONFIG_SECTION_DAEMONS      "daemons"
#define CONFIG_SECTION_APPLICATIONS "applications"
#define CONFIG_SECTION_INPUT        "input"

struct config {
    /* CONFIG_SECTION_DAEMONS */
    int no_window_manager;
    struct category_option *window_manager;
    struct category_option *compositor_daemon;
    struct category_option *hotkey_daemon;
    struct category_option *notification_daemon;
    struct category_option *polkit_daemon;
    struct category_option *power_daemon;
    struct category_option *status_daemon;
    struct category_option *applets;
    struct category_option *optional;

    /* CONFIG_SECTION_APPLICATIONS */
    struct category_option *browser;
    struct category_option *terminal;

    /* CONFIG_SECTION_INPUT */
    char *keyboard_settings;
};

struct category_option {
    int fallback, optional;
    char *name, *section, *user_preference;
};


void free_config(struct config *cfg);
struct category_option *get_category_option(const char *cname);
int ini_config_callback(void* user, const char* section, const char* name, const char* value);
struct config *init_config(void);
struct config *load_config(void);
int print_config(struct config *cfg);


static const struct config default_config = {
    /* CONFIG_SECTION_DAEMONS */
    .no_window_manager = 0,
};

#endif /* H_PADEMELON_CONFIG */

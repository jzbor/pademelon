#include "common.h"
#include "pademelon-config.h"
#include <ini.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IS_TRUE(S)                  (strcmp((S), "True") == 0 || strcmp((S), "true") == 0 || strcmp((S), "1") == 0)
#define PRINT_SECTION(S)            if (printf("\n[%s]\n", (S)) < 0) return -1;
#define PRINT_PROPERTY_BOOL(K, V)   if (printf("%s = %s\n", (K), (V) ? "True" : "False") < 0) return -1;
#define PRINT_PROPERTY_STR(K, V)    if (printf("%s = %s\n", (K), (V)) < 0) return -1;
#define PRINT_PROPERTY_CAT(C)       if (printf("%s = %s\n", (C)->name, (C)->user_preference) < 0) return -1;


static struct category_option category_options[] = {
    /* CONFIG_SECTION_DAEMONS */
    { .name = "window-manager",     .section = CONFIG_SECTION_DAEMONS, .fallback = 1 },
    { .name = "compositor",         .section = CONFIG_SECTION_DAEMONS, .fallback = 1 },
    { .name = "hotkeys",            .section = CONFIG_SECTION_DAEMONS, .fallback = 0 },
    { .name = "notifications",      .section = CONFIG_SECTION_DAEMONS, .fallback = 1 },
    { .name = "polkit",             .section = CONFIG_SECTION_DAEMONS, .fallback = 1 },
    { .name = "power",              .section = CONFIG_SECTION_DAEMONS, .fallback = 1 },
    { .name = "status",             .section = CONFIG_SECTION_DAEMONS, .fallback = 0 },
    /* the "special ones" */
    { .name = "applets",            .section = CONFIG_SECTION_DAEMONS, .optional = 1 },
    { .name = "optional",           .section = CONFIG_SECTION_DAEMONS, .optional = 1 },

    /* CONFIG_SECTION_APPLICATIONS */
    { .name = "browser",            .section = CONFIG_SECTION_APPLICATIONS, .fallback = 1 },
    { .name = "terminal",           .section = CONFIG_SECTION_APPLICATIONS, .fallback = 1 },
    { .name = NULL },
};


void free_config(struct config *cfg) {
    int i;
    for (i = 0; category_options[i].name; i++) {
        free(category_options[i].user_preference);
    }

    free(cfg->keyboard_settings);
    free(cfg);
}

struct category_option *get_category_option(const char *cname) {
    int i;
    for (i = 0; category_options[i].name; i++) {
        if (strcmp(category_options[i].name, cname) == 0)
            return &category_options[i];
    }
    return NULL;
}

int ini_config_callback(void* user, const char* section, const char* name, const char* value) {
    struct config *cfg = (struct config *) user;
    struct category_option *co;
    char **write_to_str = NULL;
    int *write_to_int = NULL;

    if (strcmp(section, CONFIG_SECTION_DAEMONS) == 0) {
        co = get_category_option(name);
        if (co && strcmp(CONFIG_SECTION_DAEMONS, co->section) == 0) {
            co->user_preference = realloc(co->user_preference, sizeof(char) * (strlen(value) + 1));
            if (!co->user_preference)
                report(R_FATAL, "Unable to allocate memory for config");
            strcpy(co->user_preference, value);
            return 1;
        }


        /* boolean attributes */
        if (strcmp(name, "no-window-manager") == 0)
            write_to_int = &cfg->no_window_manager;

        if (write_to_int) {
            *write_to_int = IS_TRUE(value);
            return 1;
        }
    } else if (strcmp(section, CONFIG_SECTION_APPLICATIONS) == 0) {
        co = get_category_option(name);
        if (co && strcmp(CONFIG_SECTION_APPLICATIONS, co->section) == 0) {
            co->user_preference = realloc(co->user_preference, sizeof(char) * (strlen(value) + 1));
            if (!co->user_preference)
                report(R_FATAL, "Unable to allocate memory for config");
            strcpy(co->user_preference, value);
            return 1;
        }
    } else if (strcmp(section, CONFIG_SECTION_INPUT) == 0) {
        if (strcmp(name, "keyboard-layout") == 0) {
            write_to_str = &cfg->keyboard_settings;
        }

        if (write_to_str) {
            *write_to_str = realloc(*write_to_str, sizeof(char) * (strlen(value) + 1));
            if (!*write_to_str)
                report(R_FATAL, "Unable to allocate memory for config");
            strcpy(*write_to_str, value);
            return 1;
        }
    }

    report_value(R_WARNING, "Unknown key", name, R_STRING);
    return 1;
}

struct config *load_config(void) {
    int status;
    char *path;
    struct config *cfg;

    /* init config struct */
    cfg = malloc(sizeof(struct config));
    if (!cfg)
        return NULL;
    memcpy(cfg, &default_config, sizeof(struct config));

    /* add category options */
    /* CONFIG_SECTION_DAEMONS */
    cfg->window_manager         = get_category_option("window-manager");
    cfg->compositor_daemon      = get_category_option("compositor");
    cfg->hotkey_daemon          = get_category_option("hotkeys");
    cfg->notification_daemon    = get_category_option("notifications");
    cfg->polkit_daemon          = get_category_option("polkit");
    cfg->power_daemon           = get_category_option("power");
    cfg->status_daemon          = get_category_option("status");
    cfg->applets                = get_category_option("applets");
    cfg->optional               = get_category_option("optional");
    /* CONFIG_SECTION_APPLICATIONS */
    cfg->browser                = get_category_option("browser");
    cfg->terminal               = get_category_option("terminal");

    path = system_config_path("pademelon.conf");
    if (path) {
        status = ini_parse(path, &ini_config_callback, cfg);
        if (status < 0)
            report_value(R_WARNING, "Unable to read config file", path, R_STRING);
    }
    free(path);
    path = user_config_path("pademelon.conf");
    if (path) {
        status = ini_parse(path, &ini_config_callback, cfg);
        if (status < 0)
            report_value(R_WARNING, "Unable to read config file", path, R_STRING);
    }
    free(path);

    return cfg;
}

int print_config(struct config *cfg) {
    /* CONFIG_SECTION_DAEMONS */
    PRINT_SECTION(CONFIG_SECTION_DAEMONS)
    PRINT_PROPERTY_BOOL("no-window-manager", cfg->no_window_manager);
    PRINT_PROPERTY_CAT(cfg->window_manager);
    PRINT_PROPERTY_CAT(cfg->compositor_daemon);
    PRINT_PROPERTY_CAT(cfg->hotkey_daemon);
    PRINT_PROPERTY_CAT(cfg->notification_daemon);
    PRINT_PROPERTY_CAT(cfg->polkit_daemon);
    PRINT_PROPERTY_CAT(cfg->power_daemon);
    PRINT_PROPERTY_CAT(cfg->status_daemon);
    PRINT_PROPERTY_CAT(cfg->power_daemon);
    PRINT_PROPERTY_CAT(cfg->status_daemon);
    PRINT_PROPERTY_CAT(cfg->applets);
    PRINT_PROPERTY_CAT(cfg->optional);

    /* CONFIG_SECTION_APPLICATIONS */
    PRINT_SECTION(CONFIG_SECTION_APPLICATIONS);
    PRINT_PROPERTY_CAT(cfg->browser);
    PRINT_PROPERTY_CAT(cfg->terminal);

    PRINT_SECTION(CONFIG_SECTION_INPUT);
    PRINT_PROPERTY_STR("keyboard-layout", cfg->keyboard_settings);

    if (fflush(stdout) == EOF)
        return -1;
    return 0;
}

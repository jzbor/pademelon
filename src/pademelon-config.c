#include "common.h"
#include "pademelon-config.h"
#include "desktop-application.h"
#include <ini.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRINT_SECTION(S)            if (printf("\n[%s]\n", (S)) < 0) return -1;
#define PRINT_PROPERTY_BOOL(K, V)   if (printf("%s = %s\n", (K), (V) ? "True" : "False") < 0) return -1;
#define PRINT_PROPERTY_STR(K, V)    if (printf("%s = %s\n", (K), (V)) < 0) return -1;
#define PRINT_PROPERTY_CAT(C)       if (printf("%s = %s\n", (C)->name, (C)->user_preference) < 0) return -1;

static inline int IS_TRUE(const char *s)       { return strcmp(s, "True") == 0 || strcmp(s, "true") == 0 || strcmp(s, "1") == 0; }


void free_config(struct config *cfg) {
    free(cfg->keyboard_settings);
    free(cfg);
}

int ini_config_callback(void* user, const char* section, const char* name, const char* value) {
    struct config *cfg = (struct config *) user;
    struct dcategory *c;
    char **write_to_str = NULL;
    int *write_to_int = NULL;

    if (strcmp(section, CONFIG_SECTION_DAEMONS) == 0) {
        c = find_category(name);
        if (c && strcmp(CONFIG_SECTION_DAEMONS, c->section) == 0) {
            c->user_preference = realloc(c->user_preference, sizeof(char) * (strlen(value) + 1));
            if (!c->user_preference)
                die("Unable to allocate memory for config");
            strcpy(c->user_preference, value);
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
        c = find_category(name);
        if (c && strcmp(CONFIG_SECTION_APPLICATIONS, c->section) == 0) {
            c->user_preference = realloc(c->user_preference, sizeof(char) * (strlen(value) + 1));
            if (!c->user_preference)
                die("Unable to allocate memory for config");
            strcpy(c->user_preference, value);
            return 1;
        }
    } else if (strcmp(section, CONFIG_SECTION_INPUT) == 0) {
        if (strcmp(name, "keyboard-layout") == 0) {
            write_to_str = &cfg->keyboard_settings;
        }

        if (write_to_str) {
            *write_to_str = realloc(*write_to_str, sizeof(char) * (strlen(value) + 1));
            if (!*write_to_str)
                die("Unable to allocate memory for config");
            strcpy(*write_to_str, value);
            return 1;
        }
    }

    DBGPRINT("Unknown config key: '%s'\n", name);
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
    cfg->window_manager         = find_category("window-manager");
    cfg->compositor_daemon      = find_category("compositor");
    cfg->dock_daemon            = find_category("dock");
    cfg->hotkey_daemon          = find_category("hotkeys");
    cfg->notification_daemon    = find_category("notifications");
    cfg->polkit_daemon          = find_category("polkit");
    cfg->power_daemon           = find_category("power");
    cfg->status_daemon          = find_category("status");
    cfg->applets                = find_category("applets");
    cfg->optional               = find_category("optional");
    /* CONFIG_SECTION_APPLICATIONS */
    cfg->browser                = find_category("browser");
    cfg->dmenu                  = find_category("dmenu");
    cfg->filemanager            = find_category("filemanager");
    cfg->terminal               = find_category("terminal");

    path = system_config_path("pademelon.conf");
    if (path) {
        status = ini_parse(path, &ini_config_callback, cfg);
        if (status < 0)
            DBGPRINT("Unable to read config file: '%s'\n", path);
    }
    free(path);
    path = user_config_path("pademelon.conf");
    if (path) {
        status = ini_parse(path, &ini_config_callback, cfg);
        if (status < 0)
            DBGPRINT("Unable to read config file: '%s'\n", path);
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
    PRINT_PROPERTY_CAT(cfg->dmenu);
    PRINT_PROPERTY_CAT(cfg->filemanager);
    PRINT_PROPERTY_CAT(cfg->terminal);

    PRINT_SECTION(CONFIG_SECTION_INPUT);
    PRINT_PROPERTY_STR("keyboard-layout", cfg->keyboard_settings);

    if (fflush(stdout) == EOF)
        return -1;
    return 0;
}

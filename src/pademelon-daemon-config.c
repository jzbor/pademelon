#include "common.h"
#include "pademelon-daemon-config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define IS_TRUE(S)                  (strcmp((S), "True") == 0 || strcmp((S), "true") == 0 || strcmp((S), "1") == 0)
#define PRINT_PROPERTY_STR(K, V)    if (printf("%s = %s\n", (K), (V)) < 0) return -1;
#define PRINT_PROPERTY_BOOL(K, V)   if (printf("%s = %s\n", (K), (V) ? "True" : "False") < 0) return -1;


static const struct config default_config = {
    .compositor_daemon = NULL,
    .hotkey_daemon = NULL,
    .notification_daemon = NULL,
    .polkit_daemon = NULL,
    .power_daemon = NULL,
};


void free_config(struct config *cfg) {
    free(cfg->compositor_daemon);
    free(cfg->hotkey_daemon);
    free(cfg->notification_daemon);
    free(cfg->polkit_daemon);
    free(cfg->power_daemon);
    free(cfg);
}

struct config *init_config(void) {
    struct config *cfg = malloc(sizeof(struct config));
    if (!cfg)
        report(R_FATAL, "Unable to allocate enough memory for config");
    memcpy(cfg, &default_config, sizeof(struct config));
    return cfg;
}

int ini_config_callback(void* user, const char* section, const char* name, const char* value) {
    struct config *cfg = (struct config *) user;
    char **write_to_str;
    int *write_to_int = NULL;

    if (strcmp(section, CONFIG_SECTION_DAEMONS) == 0) {
        if (strcmp(name, "window-manager") == 0) {
            write_to_str = &cfg->window_manager;
        } else if (strcmp(name, "compositor") == 0) {
            write_to_str = &cfg->compositor_daemon;
        } else if (strcmp(name, "hotkeys") == 0) {
            write_to_str = &cfg->hotkey_daemon;
        } else if (strcmp(name, "notifications") == 0) {
            write_to_str = &cfg->notification_daemon;
        } else if (strcmp(name, "polkit") == 0) {
            write_to_str = &cfg->polkit_daemon;
        } else if (strcmp(name, "power") == 0) {
            write_to_str = &cfg->power_daemon;
        }

        if (write_to_str) {
            /* hacky workaround to avoid reallocing stuff in read-only segments */
            for (int i = 0; i < sizeof(default_config) / sizeof(char *); i++)
                if (*write_to_str == ((char **)&default_config)[i]) {
                    *write_to_str = NULL;
                    break;
                }
            *write_to_str = realloc(*write_to_str, sizeof(char) * (strlen(value) + 1));
            if (!*write_to_str)
                report(R_FATAL, "Unable to allocate memory for config");
            strcpy(*write_to_str, value);
            return 1;
        }

        /* boolean attributes */
        if (strcmp(name, "no-window-manager") == 0)
            write_to_int = &cfg->no_window_manager;

        if (write_to_int) {
            *write_to_int = IS_TRUE(value);
            return 1;
        }
    }

    report_value(R_WARNING, "Unknown key", name, R_STRING);
    return 1;
}

int print_config(struct config *cfg) {
    int status;
    status = printf("[%s]\t\t; %p\n", "config", (void *)cfg);
    if (status < 0)
        return -1;

    PRINT_PROPERTY_STR("window-manager", cfg->window_manager);

    PRINT_PROPERTY_STR("compositor", cfg->compositor_daemon);
    PRINT_PROPERTY_STR("hotkey-daemon", cfg->hotkey_daemon);
    PRINT_PROPERTY_STR("notification-daemon", cfg->notification_daemon);
    PRINT_PROPERTY_STR("polkit-daemon", cfg->polkit_daemon);
    PRINT_PROPERTY_STR("power-daemon", cfg->power_daemon);

    PRINT_PROPERTY_BOOL("no-window-manager", cfg->no_window_manager);

    if (fflush(stdout) == EOF)
        return -1;
    return 0;
}

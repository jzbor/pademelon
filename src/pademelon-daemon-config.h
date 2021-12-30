#ifndef H_PADEMELON_DAEMON_CONFIG
#define H_PADEMELON_DAEMON_CONFIG

#include <stddef.h>

#define CONFIG_SECTION_NAME     "config"

struct config {
    char *compositor_daemon;
    char *hotkey_daemon;
    char *notification_daemon;
    char *polkit_daemon;
    char *power_daemon;
};

void free_config(struct config *cfg);
int ini_config_callback(void* user, const char* section, const char* name, const char* value);
struct config *init_config(void);
int print_config(struct config *cfg);

#endif /* H_PADEMELON_DAEMON_CONFIG */

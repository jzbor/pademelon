#ifndef H_DESKTOP_APPLICATION
#define H_DESKTOP_APPLICATION

#include "pademelon-config.h"

#define APPLICATION_FILE_ENDING      ".dapp"

struct dapplication {
    int cdefault;
    char *display_name, *id_name, *desc; /* allocated by user, freed in free_application() */
    char *launch_cmd, *test_cmd; /* allocated by user, freed in free_application() */
    char *settings; /* allocated by user, freed in free_application() */
    struct dapplication *next_optional;
    struct dcategory *category;
};

struct dcategory { /* linked list with applications in category */
    int exported; /* runtime variables */
    const int fallback, optional; /* configuration variables */
    char *name, *xdg_name, *section, *user_preference;
    struct dapplication *active_application;
    /* @TODO free user_preference */
};

int export_application(struct dapplication *application, const char *name);
struct dapplication *find_application(const char *id_name, const char *category, int init_if_not_found);
struct dcategory *find_category(const char *name);
void free_application(struct dapplication *a);
void free_categories(void);
struct dcategory *get_categories(void);
int ini_application_callback(void* user, const char* section, const char* name, const char* value);
void init_sigset_sigchld(void);
void launch_application(struct dapplication *application);
int print_application(struct dapplication *a);
int print_applications(void);
struct dapplication *select_application(struct dcategory *c);
void shutdown_all_daemons(void);
void shutdown_daemon(struct dcategory *c);
void shutdown_optionals(struct dcategory *c);
void startup_daemon(struct dcategory *c);
void startup_optionals(struct dcategory *c);
int test_application(struct dapplication *application);


static const struct dapplication application_default = { /* do NOT define strings here (invalid free) */
    .cdefault = 0
};

static const struct dcategory category_default = { /* do NOT define strings here (invalid free) */
    0
};

/* The forbidden workaround: */
        /* /1* hacky workaround to avoid reallocing stuff in read-only segments *1/ */
        /* for (int i = 0; i < sizeof(application_default) / sizeof(char *); i++) */
        /*     if (*write_to_str == ((char **)&application_default)[i]) { */
        /*         *write_to_str = NULL; */
        /*         break; */
        /*     } */

#endif /* H_DESKTOP_APPLICATION */

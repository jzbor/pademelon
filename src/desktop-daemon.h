#ifndef H_DESKTOP_DAEMON
#define H_DESKTOP_DAEMON

struct ddaemon {
    int cdefault;
    char *display_name, *id_name, *desc; /* allocated by user, freed in free_ddaemon() */
    char *launch_cmd, *test_cmd; /* allocated by user, freed in free_ddaemon() */
    struct ddaemon *next, *cnext;
    struct dcategory *category;
};

struct dcategory { /* linked list with daemons in category */
    char *name;
    struct ddaemon *daemons;
    struct dcategory *next;
};

void add_to_category(const char *name, struct ddaemon *d); /* creates category if necessary */
struct ddaemon *find_ddaemon(const char *id_name);
void free_ddaemon(struct ddaemon *d);
void free_categories(void);
struct dcategory *get_categories(void);
int ini_ddaemon_callback(void* user, const char* section, const char* name, const char* value);
int print_categories(void);
int print_category(struct dcategory *c);
int print_ddaemon(struct ddaemon *d);
int print_ddaemons(void);


static const struct ddaemon ddaemon_default = {
    .display_name = "unknown", .id_name= "unknown", .desc = "unknown",
    .cdefault = 0,.launch_cmd = "", .test_cmd = "",
};

static const struct dcategory dcategory_default = {
    .name = "default",
};

#endif /* H_DESKTOP_DAEMON */

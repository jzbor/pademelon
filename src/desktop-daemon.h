#ifndef H_DESKTOP_DAEMON
#define H_DESKTOP_DAEMON

struct ddaemon {
    int isoptional, isapplet, cdefault;
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

void add_to_category(char *name, struct ddaemon *d); /* creates category if necessary */
struct ddaemon *find_ddaemon(char *id_name);
void free_ddaemon(struct ddaemon *d);
void free_categories(void);
struct dcategory *get_categories(void);


const struct ddaemon ddaemon_default = {
    .isoptional = 0, .isapplet = 0, .cdefault = 0,
    .display_name = "unknown", .id_name= "unknown", .desc = "unknown",
    .launch_cmd = "", .test_cmd = "",
};

const struct dcategory dcategory_default = {
    .name = "default",
};

#endif /* H_DESKTOP_DAEMON */

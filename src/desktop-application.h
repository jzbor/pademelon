#ifndef H_DESKTOP_APPLICATION
#define H_DESKTOP_APPLICATION

#define APPLICATION_FILE_ENDING      ".dapp"

struct dapplication {
    int cdefault;
    char *display_name, *id_name, *desc; /* allocated by user, freed in free_application() */
    char *launch_cmd, *test_cmd; /* allocated by user, freed in free_application() */
    char *settings; /* allocated by user, freed in free_application() */
    struct dapplication *next, *cnext;
    struct dcategory *category;
};

struct dcategory { /* linked list with applications in category */
    char *name;
    struct dapplication *applications;
    struct dcategory *next;
};

void add_to_category(const char *name, struct dapplication *a); /* creates category if necessary */
int export_application(struct dapplication *application, const char *name);
struct dapplication *find_application(const char *id_name, const char *category, int init_if_not_found);
struct dcategory *find_category(const char *name);
void free_application(struct dapplication *a);
void free_applications(void);
void free_categories(void);
struct dcategory *get_categories(void);
int ini_application_callback(void* user, const char* section, const char* name, const char* value);
void init_sigset_sigchld(void);
void launch_application(struct dapplication *application);
void load_applications(void);
void load_applications_from_dir(const char *dir);
int print_application(struct dapplication *a);
int print_applications(void);
struct dapplication *select_application(const char *user_preference, const char *category, int auto_fallback);
int test_application(struct dapplication *application);


static const struct dapplication application_default = {
    .display_name = "unknown", .id_name= "unknown", .desc = "unknown",
    .cdefault = 0, .launch_cmd = "", .test_cmd = "", .settings = "",
};

static const struct dcategory category_default = {
    .name = "default",
};

#endif /* H_DESKTOP_APPLICATION */

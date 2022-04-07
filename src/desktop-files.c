#include "common.h"
#include "desktop-application.h"
#include "desktop-files.h"
#include <dirent.h>
#include <ini.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#define DESKTOP_FILE_ENDING     ".desktop"

struct cmapping {
    char *xdg_name, *internal_name;
};


static int desktop_file_callback(void* user, const char* section, const char* name, const char* value);
static struct dcategory *parse_categories(const char *string);


const char **desktop_entry_dirs(void) {
    /* @TODO add user and xdg directories */
    static const char *dirs[] = {
        NULL,
        "/usr/local/share/pademelon/applications",
        "/usr/share/pademelon/applications",
        NULL,
    };

    if (!dirs[0]) {
        dirs[0] = user_data_path("applications");
    }

    return dirs;
}

int desktop_file_callback(void* user, const char* section, const char* name, const char* value) {
    struct dapplication *app = (struct dapplication *) user;
    char **write_to_str = NULL;

    if (strcmp(section, "Desktop Entry") != 0)
        return 1;

    /* string attributes */
    if (strcmp(name, "Name") == 0)
        write_to_str = &app->display_name;
    else if (strcmp(name, "Comment") == 0)
        write_to_str = &app->desc;
    else if (strcmp(name, "Exec") == 0)
        write_to_str = &app->launch_cmd;
    /* @TODO implement TryExec */
    else if (strcmp(name, "X-Pademelon-Settings") == 0)
        write_to_str = &app->settings;

    if (write_to_str) {
        *write_to_str = realloc(*write_to_str, sizeof(char) * (strlen(value) + 1));
        if (!*write_to_str)
            die("Unable to allocate memory for application attribute");
        strcpy(*write_to_str, value);
        return 1;
    }

    /* category */
    if (strcmp(name, "Categories") == 0) {
        app->category = parse_categories(value);
        return 1;
    }

    return 1;
}

struct dapplication *application_by_category(const char **dirs, const char *category) {
    int i, status;
    DIR *directory;
    struct dapplication *app;
    struct dirent *diriter;
    struct stat filestats = {0};

    if (!dirs || !category)
        return NULL;

    for (i = 0; dirs[i]; i++) {
        /* open directory for iteration */
        directory = opendir(dirs[i]);
        if (directory == NULL) {
            DBGPRINT("Unable to launch applications from directory '%s'\n", dirs[i]);
            continue;
        }

        errno = 0;
        /* iterate over all files in the specified directory and check file ending */
        while ((diriter = readdir(directory)) != NULL) {
            /* ignore current and parent directory */
            if (strcmp(diriter->d_name, ".") == 0 || strcmp(diriter->d_name, "..") == 0)
                continue;

            /* ignore files with an inappropriate ending */
            if (!STR_ENDS_WITH(diriter->d_name, DESKTOP_FILE_ENDING)) {
                /* DBGPRINT("Not a desktop application file: %s\n", diriter->d_name); */
                continue;
            }

            /* put together path for subfiles */
            char subpath[strlen(dirs[i]) + strlen("/") + strlen(diriter->d_name) + 1];
            status = snprintf(subpath, sizeof(subpath), "%s/%s", dirs[i], diriter->d_name);
            if (status < 0) {
                DBGPRINT("snprintf failed\n");
                continue;
            }

            /* check if path is actually a regular file */
            status = stat(subpath, &filestats);
            if (status) {
                if (fprintf(stderr, "ERROR: Unable to get file stats for potential application config file '%s'\n", subpath) < 0)
                    DBGPRINT("%s\n", "Unable to print to stderr");
                continue;
            } else if (!S_ISREG(filestats.st_mode))
                continue;

            app = parse_desktop_file(subpath, diriter->d_name);
            if (!app || !app->category || !app->category->xdg_name
                    || strcmp(app->category->xdg_name, category) != 0) {
                free_application(app);
                continue;
            } else {
                status = closedir(directory);
                if (status)
                    DBGPRINT("%s\n", "Unable to close directory");
                return app;
            }
        }

        if (errno != 0) {
            DBGPRINT("ERROR: An error was encountered while iterating through directory '%s'\n", dirs[i]);
        }

        status = closedir(directory);
        if (status)
            DBGPRINT("%s\n", "Unable to close directory");
    }
    return NULL;
}

struct dapplication *application_by_name(const char **dirs, const char *name, const char *expected_category) {
    int i, status;
    struct dapplication *app;
    struct stat filestats = {0};

    if (!dirs || !name)
        return NULL;

    for (i = 0; dirs[i]; i++) {
        char filename[strlen(name) + strlen("/.desktop") + 1];
        char filepath[strlen(dirs[i]) + strlen(name) + strlen("/.desktop") + 1];
        status = snprintf(filename, sizeof(filename), "%s.desktop", name);
        if (status < 0) {
            DBGPRINT("snprintf failed\n");
            continue;
        }
        snprintf(filepath, sizeof(filepath), "%s/%s.desktop", dirs[i], name);
        if (status < 0) {
            DBGPRINT("snprintf failed\n");
            continue;
        }

        /* check if path is actually a regular file */
        status = stat(filename, &filestats);
        if (status) {
            /* DBGPRINT("ERROR: Unable to get file stats for potential application config file '%s'\n", filepath); */
            continue;
        } else if (!S_ISREG(filestats.st_mode))
            continue;

        app = parse_desktop_file(filepath, filename);
        if (!app || (expected_category && (!app->category || strcmp(expected_category, app->category->xdg_name) != 0))) {
            free_application(app);
            continue;
        } else {
            return app;
        }
    }
    return NULL;
}

struct dcategory *parse_categories(const char *string) {
    struct dcategory *d;
    char *current;
    char *token_string = strdup(string);
    if (!token_string) {
        // @TODO additional handling?
        return NULL;
    }

    current = strtok(token_string, ";");
    if (!current)
        return NULL;
    do {
        if (strcmp(string, "TrayIcon") == 0)
            d = find_category("Applet");
        else if (strcmp(string, "Panel") == 0)
            d = find_category("Status");
        else
            d = find_category(current);

        if (d)
            return d;
    } while ((current = strtok(NULL, ";")));

    return NULL;
}

struct dapplication *parse_desktop_file(const char *filepath, const char *filename) {
    int status;
    DBGPRINT("parsing file '%s'\n", filepath);
    struct dapplication *app = calloc(1, sizeof(struct dapplication));
    if (!app)
        return NULL;

    app->id_name = strdup(filename);
    if (!app->id_name) {
        free_application(app);
        return NULL;
    }

    if (!STR_ENDS_WITH(filename, ".desktop")) {
        free_application(app);
        return NULL;
    }

    app->id_name[strlen(app->id_name) - strlen(".desktop")] = '\0';

    status = ini_parse(filepath, &desktop_file_callback, (void *) app);
    if (status < 0) {
        free_application(app);
        return NULL;
    } else {
        return app;
    }
}

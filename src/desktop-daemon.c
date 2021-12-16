#include "common.h"
#include "desktop-daemon.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IS_TRUE(S)                  (strcmp((S), "True") == 0 || strcmp((S), "true") == 0 || strcmp((S), "1") == 0)
#define PRINT_PROPERTY_STR(K, V)    if (printf("%s = %s\n", (K), (V)) < 0) return -1;
#define PRINT_PROPERTY_BOOL(K, V)   if (printf("%s = %s\n", (K), (V) ? "True" : "False") < 0) return -1;

struct dcategory *categories = NULL;
struct ddaemon *daemons = NULL;

void add_to_category(const char *name, struct ddaemon *d) {
    struct dcategory *c;

    report_value(R_DEBUG, "Category", d->category, R_POINTER);

    /* daemon already has category */
    /* @TODO implement overwrite */
    if (d->category != NULL)
        return;

    /* add daemon if category exists */
    for (c = categories; c; c = c->next) {
        if (strcmp(name, c->name) == 0) {
            d->cnext = c->daemons;
            d->category = c;
            c->daemons = d;
            break;
        }
    }

    /* create category - it does not exist yet */
    if (d->category == NULL) {
        report_value(R_DEBUG, "Category not found - creating a new one", name, R_STRING);
        c = malloc(sizeof(struct dcategory));
        if (!c)
            report(R_FATAL, "Unable to allocate enough memory for new category");
        memcpy(c, &dcategory_default, sizeof(dcategory_default));

        /* set name and add to lists */
        c->name = strdup(name);
        if (!c->name) {
            report(R_FATAL, "Unable to allocate space for new category name");
        }
        c->daemons = d;
        c->next = categories;
        categories = c;
        d->category = c;
    }
}

struct ddaemon *find_ddaemon(const char *id_name) {
    struct ddaemon *d;
    for (d = daemons; d; d = d->next) {
        if (strcmp(id_name, d->id_name) == 0) {
            return d;
        }
    }

    /* daemon with this id_name doesn't exist yet */
    if (!d) {
        d = malloc(sizeof(struct ddaemon));
        if (!d)
            report(R_FATAL, "Unable to allocate enough memory for new daemon");
        memcpy(d, &ddaemon_default, sizeof(ddaemon_default));

        /* set name and add to lists */
        d->id_name = strdup(id_name);
        if (!d->id_name) {
            report(R_FATAL, "Unable to allocate space for new desktop id_name");
        }
        d->next = daemons;
        daemons = d;
    }

    return d;
}

void free_categories(void) {
    struct dcategory *c, *n;

    for (c = categories; c; c = n) {
        n = c->next;
        free(c->name);
        free(c);
    }
}

void free_ddaemon(struct ddaemon *d) {
    free(d->display_name);
    free(d->id_name);
    free(d->desc);

    free(d->launch_cmd);
    free(d->test_cmd);

    free(d);
}

struct dcategory *get_categories(void) {
    return categories;
}

int ini_ddaemon_callback(void* user, const char* section, const char* name, const char* value) {
    struct ddaemon *d = find_ddaemon(section);
    char **write_to_str = NULL;
    int *write_to_int = NULL;

    /* string attributes */
    if (strcmp(name, "name") == 0)
        write_to_str = &d->display_name;
    else if (strcmp(name, "description") == 0)
        write_to_str = &d->desc;
    else if (strcmp(name, "command") == 0)
        write_to_str = &d->launch_cmd;
    else if (strcmp(name, "test") == 0)
        write_to_str = &d->test_cmd;

    if (write_to_str) {
        *write_to_str = malloc(sizeof(char) * (strlen(value) + 1));
        if (!*write_to_str)
            report(R_FATAL, "Unable to allocate memory for daemon attribute");
        strcpy(*write_to_str, value);
        return 1;
    }

    /* boolean attributes */
    else if (strcmp(name, "default") == 0)
        write_to_int = &d->cdefault;

    if (write_to_int) {
        *write_to_int = IS_TRUE(value);
        return 1;
    }

    /* category */
    if (strcmp(name, "category") == 0) {
        report_value(R_DEBUG, "Adding category for", d->id_name, R_STRING);
        add_to_category(value, d);
        return 1;
    }

    report_value(R_WARNING, "Unknown key", name, R_STRING);
    return 1;
}

int print_categories(void) {
    struct dcategory *c;
    for (c = categories; c; c = c->next) {
        if (print_category(c) < 0)
            return -1;
        if (printf("\n") < 0)
            return -1;
    }
    if (fflush(stdout) == EOF)
        return -1;
    return 0;
}


int print_category(struct dcategory *c) {
    int status;
    struct ddaemon *d;
    status = printf("%s:\n", c->name);
    if (status < 0)
        return -1;
    for (d = c->daemons; d; d = d->cnext) {
        status = printf("%s -> ", d->id_name);
        if (status < 0)
            return -1;
    }
    status = printf("%p\n", NULL);
    if (status < 0)
        return -1;
    return 0;
}

int print_ddaemon(struct ddaemon *d) {
    int status;
    status = printf("[%s]\t\t; %p\n", d->id_name, (void *)d);
    if (status < 0)
        return -1;
    PRINT_PROPERTY_STR("name", d->display_name);
    PRINT_PROPERTY_STR("description", d->desc);
    if (d->category) {
        PRINT_PROPERTY_STR("category", d->category->name);
    } else {
        PRINT_PROPERTY_STR("; category", dcategory_default.name);
    }
    PRINT_PROPERTY_STR("command", d->launch_cmd);
    PRINT_PROPERTY_STR("test", d->test_cmd);
    PRINT_PROPERTY_BOOL("default", d->cdefault);

    if (fflush(stdout) == EOF)
        return -1;
    return 0;
}

int print_ddaemons(void) {
    struct ddaemon *d;
    for (d = daemons; d; d = d->next) {
        if (print_ddaemon(d) < 0)
            return -1;
        if (printf("\n") < 0)
            return -1;
    }
    if (fflush(stdout) == EOF)
        return -1;
    return 0;
}


#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "desktop-daemon.h"

struct dcategory *categories = NULL;
struct ddaemon *daemons = NULL;

void add_to_category(char *name, struct ddaemon *d) {
    struct dcategory *c;

    /* daemon already has category */
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
    }
}

struct ddaemon *find_ddaemon(char *id_name) {
    struct ddaemon *d;
    for (d = daemons; d; d = d->next) {
        if (strcmp(id_name, d->id_name)) {
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

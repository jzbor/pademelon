#include "common.h"
#include "desktop-daemon.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define IS_TRUE(S)                  (strcmp((S), "True") == 0 || strcmp((S), "true") == 0 || strcmp((S), "1") == 0)
#define PRINT_PROPERTY_STR(K, V)    if (printf("%s = %s\n", (K), (V)) < 0) return -1;
#define PRINT_PROPERTY_BOOL(K, V)   if (printf("%s = %s\n", (K), (V) ? "True" : "False") < 0) return -1;
#define BLOCK_SIGCHLD		        if (sigprocmask(SIG_BLOCK, &sigset_sigchld, NULL) == -1) report(R_FATAL, "Unable to block a signal");
#define UNBLOCK_SIGCHLD		        if (sigprocmask(SIG_UNBLOCK, &sigset_sigchld, NULL) == -1) report(R_FATAL, "Unable to unblock a signal");

struct dcategory *categories = NULL;
struct ddaemon *daemons = NULL;
static struct plist *plist_head = NULL;
static sigset_t sigset_sigchld;

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

struct ddaemon *find_ddaemon(const char *id_name, const char *category, int init_if_not_found) {
    struct ddaemon *d;
    struct dcategory *c;
    if (!category) {
        for (d = daemons; d; d = d->next) {
            if (strcmp(id_name, d->id_name) == 0) {
                return d;
            }
        }
    } else {
        c = find_category(category);
        if (!c)
            return NULL;
        for (d = c->daemons; d; d = d->cnext) {
            if (strcmp(id_name, d->id_name) == 0) {
                return d;
            }
        }
    }

    /* daemon with this id_name doesn't exist yet */
    if (!d && init_if_not_found) {
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
        return d;
    }

    return NULL;
}

struct dcategory *find_category(const char *name) {
    struct dcategory *c;
    for (c = categories; c; c = c->next) {
        if (strcmp(c->name, name) == 0)
            return c;
    }
    return NULL;
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

void free_ddaemons(void) {
    struct ddaemon *d, *n;

    for (d = daemons; d; d = n) {
        n = d->next;
        free_ddaemon(d);
    }
}

struct dcategory *get_categories(void) {
    return categories;
}

int ini_ddaemon_callback(void* user, const char* section, const char* name, const char* value) {
    struct ddaemon *d = find_ddaemon(section, NULL, 1);
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
        /* hacky workaround to avoid reallocing stuff in read-only segments */
        for (int i = 0; i < sizeof(ddaemon_default) / sizeof(char *); i++)
            if (*write_to_str == ((char **)&ddaemon_default)[i]) {
                *write_to_str = NULL;
                break;
            }
        *write_to_str = realloc(*write_to_str, sizeof(char) * (strlen(value) + 1));
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
        add_to_category(value, d);
        return 1;
    }

    report_value(R_WARNING, "Unknown key", name, R_STRING);
    return 1;
}

void init_sigset_sigchld(void) {
    int status;

	/* create sigset for blocking SIGCHLD */
	status = sigemptyset(&sigset_sigchld);
	if (status == -1)
		report(R_FATAL, "Unable to clear out a sigset");
	status = sigaddset(&sigset_sigchld, SIGCHLD);
	if (status == -1)
		report(R_FATAL, "Unable to add signal to a sigset");
}

void launch_ddaemon(struct ddaemon *daemon) {
    pid_t pid;

    if (!daemon)
        return;

    BLOCK_SIGCHLD;
    pid = fork();

    if (pid == 0) { /* child */
        UNBLOCK_SIGCHLD;
        char *args[] = { "/bin/sh", "-c", daemon->launch_cmd, NULL };
        execvp(args[0], args);
        report_value(R_ERROR, "Unable to launch daemon", daemon->launch_cmd, R_STRING);
        exit(EXIT_FAILURE);
    } else if (pid > 0) { /* parent */
        plist_add(pid, daemon);
    } else {
        report(R_FATAL, "Unable to fork into a new process");
    }
    UNBLOCK_SIGCHLD;
}

struct plist *plist_add(pid_t pid, struct ddaemon *ddaemon) {
    struct plist *new_element;

    /* create new element */
    new_element = calloc(1, sizeof(struct plist));
    if (!new_element)
        report(R_FATAL, "Unable to allocate memory for plist");
    new_element->pid = pid;
    new_element->ddaemon = ddaemon;

    BLOCK_SIGCHLD;

    /* add new element to list */
    new_element->next = plist_head;
    plist_head = new_element;

    UNBLOCK_SIGCHLD;
    return new_element;
}

struct plist *plist_get(pid_t pid) {
    struct plist *pl;

    BLOCK_SIGCHLD;

    /* go through list and check if pid matches */
    for (pl = plist_head; pl; pl = pl->next) {
        if (pl->pid == pid) {
            UNBLOCK_SIGCHLD;
            return pl;
        }
    }

    UNBLOCK_SIGCHLD;
    return NULL;
}

struct plist *plist_next_event(struct plist *from) {
    struct plist *pl;

    BLOCK_SIGCHLD;

    /* go through list and check status has changed */
    for (pl = from ? from : plist_head; pl; pl = pl->next) {
        if (pl->status_changed) {
            pl->status_changed = 0;
            UNBLOCK_SIGCHLD;
            return pl;
        }
    }

    UNBLOCK_SIGCHLD;
    return NULL;
}

void plist_free(void) {
    while (plist_head)
        plist_remove(plist_head->pid);
}

void plist_remove(pid_t pid) {
    struct plist *pl, *pl_delete = NULL;

    BLOCK_SIGCHLD;

    /* check if head matches */
    if (plist_head->pid == pid) {
        pl_delete = plist_head;
        plist_head = pl_delete->next;
    } else {
        /* search for the entry before pid */
        for (pl = plist_head; pl && pl->next; pl = pl->next) {
            if (pl->next->pid == pid) {
                pl_delete = pl->next;
                pl->next = pl_delete->next;
            }
        }
    }

    UNBLOCK_SIGCHLD;

    /* free item (NULL anyway if not found) */
    free(pl_delete);
}

struct plist *plist_search(char *id_name, char *category) {
    struct plist *pl;

    BLOCK_SIGCHLD;

    /* go through list and check for matching values */
    for (pl = plist_head; pl; pl = pl->next) {
        /* @TODO check for access on NULL pointers */
        if ((id_name && strcmp(id_name, pl->ddaemon->id_name) == 0)
                || (category && strcmp(category, pl->ddaemon->category->name) == 0)) {
            UNBLOCK_SIGCHLD;
            return pl;
        }
    }

    UNBLOCK_SIGCHLD;
    return NULL;
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
    int status, tested;
    struct ddaemon *d;
    status = printf("%s:\t\t\t(%p)\n", c->name, (void *)c);
    if (status < 0)
        return -1;
    for (d = c->daemons; d; d = d->cnext) {
        status = printf("%s", d->id_name);
        if (status < 0) return -1;
        tested = test_ddaemon(d);
        if (d->cdefault || tested) {
            status = printf("(%s%s)", d->cdefault ? "d" : "", tested ? "t" : "");
            if (status < 0) return -1;
        }
        status = printf(" -> ");
        if (status < 0) return -1;
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

struct ddaemon *select_ddaemon(const char *user_preference, const char *category, int auto_fallback) {
    struct dcategory *c;
    struct ddaemon *d;

    if (user_preference && (d = find_ddaemon(user_preference, category, 0))) {
        return d;
    } else if (!auto_fallback) {
        return NULL;
    } else if (user_preference) {
        report_value(R_WARNING, "Preferred daemon not found - using fallback", user_preference, R_STRING);
    }

    /* preference was not found */
    c = find_category(category);
    if (!c) {
        report_value(R_WARNING, "Unable to launch daemon - category empty", category, R_STRING);
        return NULL;
    }
    for (d = c->daemons; d->cnext; d = d->cnext) {
        if (d->cdefault)
            return d;
    }

    return d;
}

int test_ddaemon(struct ddaemon *daemon) {
    int status;
	sigset_t sigset = {0};
    pid_t pid;
    struct plist *pl;

    if (!daemon || !daemon->test_cmd)
        return 1;

    BLOCK_SIGCHLD;
    pid = fork();

    if (pid == 0) { /* child */
        UNBLOCK_SIGCHLD;
        char *args[] = { "/bin/sh", "-c", daemon->test_cmd, NULL };
        execvp(args[0], args);
        report_value(R_ERROR, "Unable to test daemon", daemon->test_cmd, R_STRING);
        exit(EXIT_FAILURE);
    } else if (pid > 0) { /* parent */
        plist_add(pid, daemon);
    } else {
        report(R_FATAL, "Unable to fork into a new process");
    }

    status = sigemptyset(&sigset);
    if (status == -1)
        report(R_FATAL, "Unable to add signal to sigset");
    /* @TODO add timeout */
    while ((pl = plist_get(pid)) && !pl->status_changed)
        sigsuspend(&sigset);
    status = WIFEXITED(pl->status) && WEXITSTATUS(pl->status) == 0;
    plist_remove(pid);
    UNBLOCK_SIGCHLD;

    return status;
}

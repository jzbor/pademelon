#include "common.h"
#include "signals.h"
#include "desktop-application.h"
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

static void plist_sigchld_handler(int signal);

static struct plist *plist_head = NULL;
static struct sigaction sigaction_sigchld_prev_handler = { .sa_handler = SIG_DFL, .sa_flags = SA_NODEFER|SA_NOCLDSTOP|SA_RESTART};

int block_signal(int signal) {
    int status;
    sigset_t sigset;

	/* create sigset for blocking signal */
	status = sigemptyset(&sigset);
	if (status == -1)
		return 0;
	status = sigaddset(&sigset, signal);
	if (status == -1)
		return 0;

    if (sigprocmask(SIG_BLOCK, &sigset, NULL) == -1)
        return 0;
    return 1;
}

int install_default_sigchld_handler(void) {
    int status;
	struct sigaction sigaction_sigchld_handler = { .sa_handler = SIG_DFL };

	/* handle SIGCHLD*/
	status = sigemptyset(&sigaction_sigchld_handler.sa_mask); // @TODO do I have to block anything here?
	if (status == -1)
		return 0;

	status = sigaction(SIGCHLD, &sigaction_sigchld_handler, &sigaction_sigchld_prev_handler);
	if (status == -1)
		return 0;
    return 1;
}

int install_plist_sigchld_handler(void) {
    int status;
	struct sigaction sigaction_sigchld_handler = { .sa_handler = &plist_sigchld_handler, .sa_flags = SA_NODEFER|SA_NOCLDSTOP|SA_RESTART};

	/* handle SIGCHLD*/
	status = sigfillset(&sigaction_sigchld_handler.sa_mask); // @TODO do I have to block anything here?
	if (status == -1)
		return 0;

	status = sigaction(SIGCHLD, &sigaction_sigchld_handler, &sigaction_sigchld_prev_handler);
	if (status == -1)
		return 0;
    return 1;
}

struct plist *plist_add(pid_t pid, void *content) {
    struct plist *new_element;

    /* create new element */
    new_element = calloc(1, sizeof(struct plist));
    if (!new_element)
        report(R_FATAL, "Unable to allocate memory for plist");
    new_element->pid = pid;
    new_element->content = content;

    block_signal(SIGCHLD);

    /* add new element to list */
    new_element->next = plist_head;
    plist_head = new_element;

    unblock_signal(SIGCHLD);
    return new_element;
}

struct plist *plist_get(pid_t pid) {
    struct plist *pl;

    block_signal(SIGCHLD);

    /* go through list and check if pid matches */
    for (pl = plist_head; pl; pl = pl->next) {
        if (pl->pid == pid) {
            unblock_signal(SIGCHLD);
            return pl;
        }
    }

    unblock_signal(SIGCHLD);
    return NULL;
}

struct plist *plist_next_event(struct plist *from) {
    struct plist *pl;

    block_signal(SIGCHLD);

    /* go through list and check status has changed */
    for (pl = from ? from : plist_head; pl; pl = pl->next) {
        if (pl->status_changed) {
            pl->status_changed = 0;
            unblock_signal(SIGCHLD);
            return pl;
        }
    }

    unblock_signal(SIGCHLD);
    return NULL;
}

void plist_free(void) {
    while (plist_head)
        plist_remove(plist_head->pid);
}

void plist_remove(pid_t pid) {
    struct plist *pl, *pl_delete = NULL;

    block_signal(SIGCHLD);

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
                break;
            }
        }
    }

    unblock_signal(SIGCHLD);

    /* free item (NULL anyway if not found) */
    free(pl_delete);
}

struct plist *plist_search(char *id_name, char *category) {
    struct plist *pl;

    block_signal(SIGCHLD);

    /* go through list and check for matching values */
    for (pl = plist_head; pl; pl = pl->next) {
        /* @TODO check for access on NULL pointers */
        if ((id_name && strcmp(id_name, ((struct dapplication*) pl->content)->id_name) == 0)
                || (category && strcmp(category, ((struct dapplication*) pl->content)->category->name) == 0)) {
            unblock_signal(SIGCHLD);
            return pl;
        }
    }

    unblock_signal(SIGCHLD);
    return NULL;
}

void plist_sigchld_handler(int signal) {
	pid_t pid;
	int status;
	int errno_save = errno;
    struct plist *pl;

	if (signal != SIGCHLD) {
		/* should not happen */
		return;
	}

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        pl = plist_get(pid);
        if (pl) {
            pl->status = status;
            pl->status_changed = 1;
        }
	}

	errno = errno_save;
}

void plist_wait(struct plist *pl, long timeout_milli) {
    int status;
    struct timespec ts = { .tv_sec = 0, .tv_nsec = timeout_milli + 1000 };
    while (!pl->status_changed) {
        status = nanosleep(&ts, &ts);
        if (status == 0 /* time elapsed */
                || (status == -1 && errno != EINTR)) /* real error */
            break;
    }
}

int restore_sigchld_handler(void) {
    int status;
	status = sigaction(SIGCHLD, &sigaction_sigchld_prev_handler, NULL);
	if (status == -1)
		return 0;
    return 1;
}

int unblock_signal(int signal) {
    int status;
    sigset_t sigset;

	/* create sigset for blocking signal */
	status = sigemptyset(&sigset);
	if (status == -1)
		return 0;
	status = sigaddset(&sigset, signal);
	if (status == -1)
		return 0;

    if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) == -1)
        return 0;
    return 1;
}

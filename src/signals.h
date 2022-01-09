#ifndef H_SIGNALS
#define H_SIGNALS

#include <sys/types.h>

struct plist {
    int status; /* as obtained from waitpid */
    int status_changed;
    pid_t pid;
    void *content;
    struct plist *next;
};

int block_signal(int signal);
int install_default_sigchld_handler(void);
int install_plist_sigchld_handler(void);
struct plist *plist_add(pid_t pid, void *content);
void plist_free(void);
struct plist *plist_get(pid_t pid);
struct plist *plist_next_event(struct plist *from);
void plist_remove(pid_t pid);
struct plist *plist_search(char *id_name, char *category);
void plist_wait(struct plist *pl, long timeout_milli);
int restore_sigchld_handler(void);
int unblock_signal(int signal);

#endif /* H_SIGNALS */

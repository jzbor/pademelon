#ifndef H_COMMON
#define H_COMMON

#include <string.h>
#include <stdio.h>

#ifdef DEBUG
#define DBGPRINT(...) \
        do { fprintf(stderr, "%s:%d:%s(): ",__FILE__, __LINE__, __func__);\
             fprintf(stderr, __VA_ARGS__); } while (0)
#else /* DEBUG */
#define DBGPRINT(fmt, ...) do {} while (0)
#endif /* DEBUG */

static inline int STR_STARTS_WITH(const char *s, const char *pre) { return strncmp(pre, s, strlen(pre)) == 0; }
static inline int STR_ENDS_WITH(const char *s, const char *suf) { return strlen(s) >= strlen(suf)
        && strncmp(suf, &s[strlen(s) - strlen(suf)], strlen(suf)) == 0; }
static inline int MAX_INT(int a, int b) { return a > b ? a : b; }
static inline int MIN_INT(int a, int b) { return a < b ? a : b; }
static inline int BET_INT(int x, int a, int b) { return a <= x && x <= b; }

void bye(const char *msg);

void die(const char *msg);

/* returns -1 on error, return code else */
int execute(const char *command);

void init_user_data_path(void);

int str_to_int(const char *str, int *integer);

/*
 * get a user config file or the user config file path
 *
 * if the file or dir is not found NULL is returned
 * if an error occurred NULL is returned and the errno is set
 */
char *system_config_path(char *file);
char *system_data_path(char *file);
char *system_local_data_path(char *file);

/*
 * get a user config file or the user config file path
 *
 * if the file or dir is not found NULL is returned
 * if an error occurred NULL is returned and the errno is set
 */
char *user_config_path(char *file);
char *user_data_path(char *file);

#endif /* H_COMMON */

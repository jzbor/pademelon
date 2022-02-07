#ifndef H_COMMON
#define H_COMMON

#include <string.h>
#include <stdio.h>

#define R_DEBUG                 0
#define R_INFO                  1
#define R_WARNING               2
#define R_ERROR                 3
#define R_FATAL                 4
#define DEFAULT_REPORT_LEVEL    0

#define R_NONE                  0
#define R_POINTER               1
#define R_STRING                2
#define R_INTEGER               3

#define DBGPRINT(fmt, ...) do { \
    fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, __VA_ARGS__); \
        } while (0)

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

/*
 * see report_value
 */
void report(int mode, const char *msg);

/*
 * report information
 *
 * mode: one of R_DEBUG, R_INFO, R_WARNING, R_ERROR, R_FATAL
 * type: one of R_STRING, R_INTEGER
 *
 * this also resets the errno
 *
 * R_FATAL will also end the process with EXIT_FAILURE
 */
void report_value(int mode, const char *msg, const void *value, int type);

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

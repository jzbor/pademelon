#ifndef H_COMMON
#define H_COMMON

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

#define STR_STARTS_WITH(S, PRE) (strncmp((PRE), (S), strlen((PRE))) == 0)
#define STR_ENDS_WITH(S, SUF) (strlen((S)) >= strlen((SUF)) \
        && strncmp((SUF), &(S)[strlen((S)) - strlen((SUF))], strlen((SUF))) == 0)

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

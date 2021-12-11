#ifndef H_COMMON
#define H_COMMON

#define R_DEBUG                 0
#define R_INFO                  1
#define R_WARNING               2
#define R_ERROR                 3
#define R_FATAL                 4
#define DEFAULT_REPORT_LEVEL    0

/*
 * report information
 *
 * mode: one of R_DEBUG, R_INFO, R_WARNING, R_ERROR, R_FATAL
 *
 * R_FATAL will also end the process with EXIT_FAILURE
 */
void report(int mode, char *msg);

/*
 * get a user config file or the user config file path
 *
 * if the file or dir is not found NULL is returned
 * if an error occurred NULL is returned and the errno is set
 */
char *user_config_path(char *file);

/*
 * get a user config file or the user config file path
 *
 * if the file or dir is not found NULL is returned
 * if an error occurred NULL is returned and the errno is set
 */
char *system_config_path(char *file);

#endif /* H_COMMON */

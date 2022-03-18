#ifndef H_DESKTOP_FILES
#define H_DESKTOP_FILES

char **desktop_entry_dirs(void);
struct dapplication *parse_desktop_file(const char *filepath, const char *filename);
struct dapplication *application_by_category(const char *category);
struct dapplication *application_by_name(const char *name, const char *expected_category);

#endif /* H_DESKTOP_FILES */

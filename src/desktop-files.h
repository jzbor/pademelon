#ifndef H_DESKTOP_FILES
#define H_DESKTOP_FILES

char **desktop_entry_dirs(void);
struct dapplication *parse_desktop_file(const char *filepath, const char *filename);

#endif /* H_DESKTOP_FILES */

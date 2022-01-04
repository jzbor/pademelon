#ifndef H_X11_UTILS
#define H_X11_UTILS

int x11_init(void);
int x11_wallpaper(const char *path, int x, int y, int width, int height);
int x11_wallpaper_all(const char *path);
void x11_deinit(void);

#endif /* H_X11_UTILS */

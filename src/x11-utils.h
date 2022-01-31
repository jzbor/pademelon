#ifndef H_X11_UTILS
#define H_X11_UTILS

int x11_init(void);
int x11_screen_has_changed(void);
int x11_keyboard_has_changed(void);
int x11_wallpaper_all(const char *path);
void x11_deinit(void);

#endif /* H_X11_UTILS */

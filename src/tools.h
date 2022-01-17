#ifndef H_PADEMELON_TOOLS
#define H_PADEMELON_TOOLS

#define WALLPAPER_FILE_NAME     "wallpaper"

int tl_backlight_dec(int percentage);
int tl_backlight_inc(int percentage);
int tl_backlight_print(void);
int tl_backlight_set(int percentage);
int tl_launch_application(const char *category);
int tl_load_display_conf(const char *path);
int tl_load_wallpaper(void);
int tl_print_applications(void);
int tl_save_display_conf(void);
int tl_select_application(const char *category);
int tl_set_wallpaper(const char *input_path);
int tl_test_application(const char *id_name);
int tl_volume_dec(int percentage, int play_sound);
int tl_volume_inc(int percentage, int play_sound);
int tl_volume_print(void);
int tl_volume_set(int percentage);

#endif /* H_PADEMELON_TOOLS */

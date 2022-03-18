#include "common.h"
#include "desktop-application.h"
#include "desktop-files.h"
#include "tools.h"
#ifdef X11
#include "x11-utils.h"
#endif /* X11 */
#ifdef CANBERRA
#include <canberra.h>
#endif /* CANBERRA */
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define BUFSIZE                 512
#define UNXRANDR_CMD            "unxrandr"
#define DISPLAY_CONF_FILE       "displayconfiguration"
#define CANBERRA_HINT           "pademelon"
#define CANBERRA_VOLUME_CHANGE  "audio-volume-change"

#ifdef CANBERRA
static void canberra_play_async(const char *sound);
#endif /* CANBERRA */
static int get_pa_volume(int *volume);
static int set_pa_volume(int volume);
static char* wallpaper_path(void);
static int print_category(struct dcategory *c);


#ifdef CANBERRA
static void canberra_play_async(const char *sound) {
    int status;
    status = fork();
    /* status = -1; */
    if (status == 0 || status == -1) { /* child or no fork */
        ca_context *cc;
        ca_context_create (&cc);
        ca_context_play (cc, 0,
                CA_PROP_EVENT_ID, sound,
                CA_PROP_EVENT_DESCRIPTION, CANBERRA_HINT,
                NULL);
    }

    if (status == 0) {/* exit child */
        sleep(1);
        exit(EXIT_SUCCESS);
    }
}
#endif /* CANBERRA */

int get_pa_volume(int *volume) {
    char cmd[] = "pactl get-sink-volume @DEFAULT_SINK@";
    char buffer[100];
    char *from, *to;
    FILE *fp;

    fp = popen(cmd, "r");
    if (!fp)
        return 0;
    if (fgets(buffer, 100, fp) == NULL) {
        pclose(fp);
        return 0;
    }
    pclose(fp);

    from = strchr(buffer, '/');
    if (!from)
        return 0;
    for (from++; from[0] == ' '; from++);
    if (from[0] == '\0')
        return 0;

    to = strchr(from, '%');
    if (!to)
        return 0;
    to[0] = '\0';
    return str_to_int(from, volume);
}

int tl_backlight_dec(int percentage) {
    char cmd_template[] = "xbacklight -dec %d";
    size_t tempsize = strlen(cmd_template) + 5;
    char temp[tempsize];
    percentage = MIN_INT(percentage, 100);
    percentage = MAX_INT(percentage, 0);
    snprintf(temp, tempsize, cmd_template, percentage);
    return system(temp);
}

int tl_backlight_print(void) {
    return system("xbacklight -get");
}

int tl_backlight_inc(int percentage) {
    char cmd_template[] = "xbacklight -inc %d";
    size_t tempsize = strlen(cmd_template) + 5;
    char temp[tempsize];
    percentage = MIN_INT(percentage, 100);
    percentage = MAX_INT(percentage, 0);
    snprintf(temp, tempsize, cmd_template, percentage);
    return system(temp);
}

int tl_backlight_set(int percentage) {
    char cmd_template[] = "xbacklight -set %d";
    size_t tempsize = strlen(cmd_template) + 5;
    char temp[tempsize];
    percentage = MIN_INT(percentage, 100);
    percentage = MAX_INT(percentage, 0);
    snprintf(temp, tempsize, cmd_template, percentage);
    return system(temp);
}

int tl_launch_application(const char *category) {
    struct dcategory *c;
    struct config *cfg;

    if (!category) {
        fprintf(stderr, "select-application: no category supplied\n");
        return EXIT_FAILURE;
    }

    load_applications();
    cfg = load_config();
    if (!cfg) {
        fprintf(stderr, "select-application: unable to load config\n");
        return EXIT_FAILURE;
    }

    c = find_category(category);
    if (!c) {
        fprintf(stderr, "select-application: category not found\n");
        return EXIT_FAILURE;
    }

    if (!c->selected_application) {
        fprintf(stderr, "select-application: no suitable application found\n");
        return EXIT_FAILURE;
    }

    launch_application(c->selected_application);
    return EXIT_SUCCESS;
}

int tl_load_display_conf(const char *path) {
    int status;
    if (!path) {
        path = user_data_path(DISPLAY_CONF_FILE);
        if (!path)
            return EXIT_FAILURE;
    }

    status = execute(path);
    if (status == -1) {
        return EXIT_FAILURE;
    }
    /* else { */
    /*     if (chmod(path, S_IRWXU|S_IRGRP|S_IROTH) == -1) */
    /*         return EXIT_FAILURE; */
    /*     return status; */
    /* } */
    return EXIT_SUCCESS;
}


int tl_load_wallpaper(void) {
    char *path;
#ifndef X11
    fprintf(stderr, "load-wallpaper: missing dependency: x11\n");
#endif /* X11 */
#ifndef IMLIB2
    fprintf(stderr, "load-wallpaper: missing dependency: imlib2\n");
#endif /* IMLIB2 */
#ifdef X11
#ifdef IMLIB2
    path = wallpaper_path();
    /* @TODO check if path exists */
    if (path) {
        if (x11_wallpaper_all(path))
            return EXIT_SUCCESS;
    }
    fprintf(stderr, "load-wallpaper: unable to set wallpaper to all screens\n");
#endif /* IMLIB2 */
#endif /* X11 */
    return EXIT_FAILURE;
}

int tl_print_applications(void) {
    int i;
    struct dcategory *c;
    struct config *cfg;

    cfg = load_config();
    if (!cfg) {
        fprintf(stderr, "select-application: unable to load config\n");
        return EXIT_FAILURE;
    }

    load_applications();

    for (i = 0, c = get_categories(); c[i].name; i++) {
        if (print_category(&c[i]) < 0)
            return EXIT_FAILURE;
        if (printf("\n") < 0)
            return EXIT_FAILURE;
    }
    if (fflush(stdout) == EOF)
        return EXIT_FAILURE;

    free_categories();

    return EXIT_SUCCESS;
}


int tl_save_display_conf(void) {
    int status;
    char *path = user_data_path(DISPLAY_CONF_FILE);
    if (!path)
        return EXIT_FAILURE;

    init_user_data_path();

    char temp[strlen(UNXRANDR_CMD) + strlen(" > ") + strlen(path) + 1];
    strcpy(temp, UNXRANDR_CMD);
    strcat(temp, " > ");
    strcat(temp, path);
    status = execute(temp);
    if (WIFEXITED(status)) {
        if (WEXITSTATUS(status) == 0) {
            if (chmod(path, S_IRWXU) == -1)
                return EXIT_FAILURE;
        }
        return WEXITSTATUS(status);
    } else {
        return EXIT_FAILURE;
    }
}

int tl_select_application(const char *category) {
    struct dcategory *c;
    struct config *cfg;

    if (!category) {
        fprintf(stderr, "select-application: not enough arguments\n");
        return EXIT_FAILURE;
    }

    load_applications();
    cfg = load_config();
    if (!cfg) {
        fprintf(stderr, "select-application: unable to load config\n");
        return EXIT_FAILURE;
    }

    c = find_category(category);
    if (!c) {
        fprintf(stderr, "select-application: category not found\n");
        return EXIT_FAILURE;
    }

    if (!c->selected_application) {
        fprintf(stderr, "select-application: no suitable application found\n");
        return EXIT_FAILURE;
    }

    if (printf("%s\n", c->selected_application->id_name) < 0) {
        perror("select-application: unable to write to stdout");
        return EXIT_FAILURE;
    }
    if (fflush(stderr) == EOF) {
        perror("select-application: unable to flush stdout");
        return EXIT_FAILURE;
    }

    free_categories();
    free_config(cfg);

    return EXIT_SUCCESS;
}

int tl_set_wallpaper(const char *input_path) {
#ifndef X11
    fprintf(stderr, "set-wallpaper: missing dependency: x11\n");
#endif /* X11 */
#ifndef IMLIB2
    fprintf(stderr, "set-wallpaper: missing dependency: imlib2\n");
#endif /* IMLIB2 */
#ifdef X11
#ifdef IMLIB2
    char buffer[BUFSIZE];
    int status;
    size_t n;
    char *path, *dirpath;
    FILE *source, *target;


    if (!input_path) {
        fprintf(stderr, "set-wallpaper: not enough arguments\n");
        return EXIT_FAILURE;
    }

    path = wallpaper_path();
    if (!path) {
        fprintf(stderr, "set-wallpaper: no wallpaper destination available\n");
        return EXIT_FAILURE;
    }

    /* open input file */
    source = fopen(input_path, "r");
    if (!source) {
        fprintf(stderr, "set-wallpaper: unable to open file\n");
        return EXIT_FAILURE;
    }

    /* check if output dir exists */
    dirpath = strdup(path);
    if (!dirpath) {
        fprintf(stderr, "set-wallpaper: unable to allocate enough memory\n");
        fclose(source);
        return EXIT_FAILURE;
    }

    status = mkdir(dirname(dirpath), S_IRWXU|S_IRGRP|S_IROTH);
    if (status == -1 && errno != EEXIST) {
        perror("set-wallpaper: unable to create data dir");
        fclose(source);
        return EXIT_FAILURE;
    }
    free(dirpath);

    /* open output file */
    target = fopen(path, "w");
    if (!target) {
        fprintf(stderr, "set-wallpaper: unable to open target file\n");
        fclose(source);
        return EXIT_FAILURE;
    }

    while ((n = fread(buffer, sizeof(char), sizeof(buffer), source)) > 0) {
        if (fwrite(buffer, sizeof(char), n, target) != n) {
            fprintf(stderr, "set-wallpaper: error while writing to target file\n");
            return EXIT_FAILURE;
        }
    }

    if (ferror(source)) {
        fprintf(stderr, "set-wallpaper: error while reading from source file\n");
        return EXIT_FAILURE;
    }

    fclose(source);
    fclose(target);

    return tl_load_wallpaper();
#endif /* X11 */
#endif /* IMLIB2 */
    return EXIT_FAILURE;
}

int print_category(struct dcategory *c) {
    int status;
    status = printf("%s:\n", c->name);
    if (status < 0)
        return -1;
    status = printf("\tuser config: %s\n", c->user_preference);
    if (status < 0)
        return -1;
    status = printf("\tselected application: %s\n", c->selected_application ? c->selected_application->id_name : "null");
    if (status < 0)
        return -1;
    return 0;
}

int set_pa_volume(int volume) {
    char cmd_template[] = "pactl set-sink-volume @DEFAULT_SINK@ %d%%";
    size_t tempsize = strlen(cmd_template) + 5;
    char temp[tempsize];
    volume = MIN_INT(volume, 100);
    volume = MAX_INT(volume, 0);
    snprintf(temp, tempsize, cmd_template, volume);
    return system(temp);
}

char* wallpaper_path(void) {
    return user_data_path(WALLPAPER_FILE_NAME);
}

int tl_test_application(const char *id_name) {
    struct dapplication *a;

    if (!id_name) {
        fprintf(stderr, "test-application: not enough arguments\n");
        return EXIT_FAILURE;
    }

    load_applications();
    a = application_by_name(id_name, NULL);
    if (!a) {
        fprintf(stderr, "test-application: application not found\n");
        return EXIT_FAILURE;
    }

    if (test_application(a)) {
        /* we don't really care if the print works */
        printf("Application available: (%s, \"%s\")\n", a->id_name, a->display_name);
        fflush(stdout);
        return EXIT_SUCCESS;
    } else {
        /* we don't really care if the print works */
        printf("Application not available: (%s, \"%s\")\n", a->id_name, a->display_name);
        fflush(stdout);
        return EXIT_FAILURE;
    }

    free_categories();
}

int tl_volume_dec(int percentage, int play_sound) {
    int volume, status;
    if (!get_pa_volume(&volume))
        return EXIT_FAILURE;
    else {
#ifdef CANBERRA
        if (play_sound)
            canberra_play_async(CANBERRA_VOLUME_CHANGE);
#endif /* CANBERRA */
        status = set_pa_volume(volume - percentage);
        return status;
    }
}

int tl_volume_inc(int percentage, int play_sound) {
    int volume, status;
    if (!get_pa_volume(&volume))
        return EXIT_FAILURE;
    else {
#ifdef CANBERRA
        if (play_sound) {
            canberra_play_async(CANBERRA_VOLUME_CHANGE);
        }
#endif /* CANBERRA */
        status = set_pa_volume(volume + percentage);
        return status;
    }
}

int tl_volume_mute_input(int i) {
    if (i < 0) {
        return system("pactl set-source-mute @DEFAULT_SOURCE@ toggle");
    } else if (i) {
        return system("pactl set-source-mute @DEFAULT_SOURCE@ 1");
    } else {
        return system("pactl set-source-mute @DEFAULT_SOURCE@ 0");
    }
}

int tl_volume_mute_output(int i) {
    if (i < 0) {
        return system("pactl set-sink-mute @DEFAULT_SINK@ toggle");
    } else if (i) {
        return system("pactl set-sink-mute @DEFAULT_SINK@ 1");
    } else {
        return system("pactl set-sink-mute @DEFAULT_SINK@ 0");
    }
}

int tl_volume_print(void) {
    int volume;
    if (!get_pa_volume(&volume))
        return EXIT_FAILURE;
    else {
        if (printf("%d\n", volume) < 0)
            return EXIT_FAILURE;
        if (fflush(stdout) == EOF)
            return EXIT_FAILURE;
        return EXIT_SUCCESS;
    }
}

int tl_volume_set(int percentage) {
    fprintf(stderr, "%d\n", percentage);
    return set_pa_volume(percentage);
}


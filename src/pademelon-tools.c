#include "tools.h"
#include "cliparse.h"
#ifdef X11
#include "x11-utils.h"
#endif /* X11 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ARG_QUIET_LONG              "--quiet"
#define ARG_QUIET_SHORT             "-q"
#define ARG_MUTE_OUT_LONG           "--mute"
#define ARG_MUTE_OUT_SHORT          "-m"
#define ARG_MUTE_PLACEHOLDER        "0_or_1_or_toggle"
#define ARG_MUTE_IN_LONG            "--mute-input"
#define ARG_MUTE_IN_SHORT           "-n"
#define ARG_DEC_LONG                "--dec"
#define ARG_DEC_SHORT               "-d"
#define ARG_GET_LONG                "--get"
#define ARG_GET_SHORT               "-g"
#define ARG_INC_LONG                "--inc"
#define ARG_INC_SHORT               "-i"
#define ARG_PATH_LONG               "--path"
#define ARG_PATH_PLACEHOLDER        "path"
#define ARG_PATH_SHORT              "-p"
#define ARG_PERCENTAGE_PLACEHOLDER  "percentage"
#define ARG_SET_LONG                "--set"
#define ARG_SET_SHORT               "-s"
#define OP_CATEGORY                 "category"
#define OP_ID_NAME                  "id-name"
#define OP_PATH                     "path"

typedef int (*ToolFunction)(int argc, char *argv[]);
struct clitool {
    CliApplication cliapp;
    CliArgument *args;
    CliOperand *ops;
    ToolFunction execute;
};

static int print_tools(void);
int wr_backlight(int argc, char *argv[]);
int wr_launch_application(int argc, char *argv[]);
int wr_load_display_conf(int argc, char *argv[]);
int wr_load_wallpaper(int argc, char *argv[]);
int wr_print_applications(int argc, char *argv[]);
int wr_save_display_conf(int argc, char *argv[]);
int wr_select_application(int argc, char *argv[]);
int wr_set_wallpaper(int argc, char *argv[]);
int wr_test_application(int argc, char *argv[]);
int wr_volume(int argc, char *argv[]);

const struct clitool ct_last = {0};

CliArgument args_backlight[] = {
    { ArgTypeFlag, ARG_GET_LONG, ARG_GET_SHORT },
    { ArgTypeInteger, ARG_SET_LONG, ARG_SET_SHORT, ARG_PERCENTAGE_PLACEHOLDER },
    { ArgTypeInteger, ARG_INC_LONG, ARG_INC_SHORT, ARG_PERCENTAGE_PLACEHOLDER },
    { ArgTypeInteger, ARG_DEC_LONG, ARG_DEC_SHORT, ARG_PERCENTAGE_PLACEHOLDER },
    {0},
};
CliOperand ops_backlight[] = { {0} };
const struct clitool ct_backlight = {
    .cliapp = { "backlight", "control screen backlight" },
    .args = args_backlight,
    .ops = ops_backlight,
    .execute = wr_backlight,
};

CliArgument args_launch_application[] = { {0} };
CliOperand ops_launch_application[] = { { ArgTypeString, OP_CATEGORY }, {0}  };
const struct clitool ct_launch_application = {
    .cliapp = { "launch-application", "launch pademelons application choice" },
    .args = args_launch_application,
    .ops = ops_launch_application,
    .execute = wr_launch_application,
};

CliArgument args_load_display_conf[] = { { ArgTypeString, ARG_PATH_LONG, ARG_PATH_SHORT, ARG_PATH_PLACEHOLDER }, {0} };
CliOperand ops_load_display_conf[] = { {0} };
const struct clitool ct_load_display_conf = {
    .cliapp = { "load-display-conf", "load last saved display configuration" },
    .args = args_load_display_conf,
    .ops = ops_load_display_conf,
    .execute = wr_load_display_conf,
};

CliArgument args_load_wallpaper[] = { {0} };
CliOperand ops_load_wallpaper[] = { {0} };
const struct clitool ct_load_wallpaper = {
    .cliapp = { "load-wallpaper", "load wallpaper to X11 background" },
    .args = args_load_wallpaper,
    .ops = ops_load_wallpaper,
    .execute = wr_load_wallpaper,
};

CliArgument args_print_applications[] = { {0} };
CliOperand ops_print_applications[] = { {0} };
const struct clitool ct_print_applications = {
    .cliapp = { "print-applications", "print installed application files" },
    .args = args_print_applications,
    .ops = ops_print_applications,
    .execute = wr_print_applications,
};

CliArgument args_save_display_conf[] = { {0} };
CliOperand ops_save_display_conf[] = { {0} };
const struct clitool ct_save_display_conf = {
    .cliapp = { "save-display-conf", "save current display configuration" },
    .args = args_save_display_conf,
    .ops = ops_save_display_conf,
    .execute = wr_save_display_conf,
};

CliArgument args_select_application[] = { {0} };
CliOperand ops_select_application[] = { { ArgTypeString, OP_CATEGORY }, {0} };
const struct clitool ct_select_application = {
    .cliapp = { "select-application", "get pademelons chosen application for the category" },
    .args = args_select_application,
    .ops = ops_select_application,
    .execute = wr_select_application,
};

CliArgument args_set_wallpaper[] = { {0} };
CliOperand ops_set_wallpaper[] = { { ArgTypeString, OP_PATH }, {0} };
const struct clitool ct_set_wallpaper = {
    .cliapp = { "set-wallpaper", "set the wallpaper" },
    .args = args_set_wallpaper,
    .ops = ops_set_wallpaper,
    .execute = wr_set_wallpaper,
};

CliArgument args_test_application[] = { {0} };
CliOperand ops_test_application[] = { { ArgTypeString, OP_ID_NAME }, {0} };
const struct clitool ct_test_application = {
    .cliapp = { "test-application", "test if an application is installed and available" },
    .args = args_test_application,
    .ops = ops_test_application,
    .execute = wr_test_application,
};

CliArgument args_volume[] = {
    { ArgTypeFlag, ARG_GET_LONG, ARG_GET_SHORT },
    { ArgTypeInteger, ARG_SET_LONG, ARG_SET_SHORT, ARG_PERCENTAGE_PLACEHOLDER },
    { ArgTypeInteger, ARG_INC_LONG, ARG_INC_SHORT, ARG_PERCENTAGE_PLACEHOLDER },
    { ArgTypeInteger, ARG_DEC_LONG, ARG_DEC_SHORT, ARG_PERCENTAGE_PLACEHOLDER },
    { ArgTypeFlag, ARG_QUIET_LONG, ARG_QUIET_SHORT },
    { ArgTypeString, ARG_MUTE_OUT_LONG, ARG_MUTE_OUT_SHORT, ARG_MUTE_PLACEHOLDER },
    { ArgTypeString, ARG_MUTE_IN_LONG, ARG_MUTE_IN_SHORT, ARG_MUTE_PLACEHOLDER},
    {0},
};
CliOperand ops_volume[] = { {0} };
const struct clitool ct_volume = {
    .cliapp = { "volume", "control pulse audio master volume" },
    .args = args_volume,
    .ops = ops_volume,
    .execute = wr_volume,
};

struct clitool clitools[] = {
    ct_backlight,
    ct_launch_application,
    ct_load_display_conf,
    ct_load_wallpaper,
    ct_print_applications,
    ct_save_display_conf,
    ct_select_application,
    ct_set_wallpaper,
    ct_test_application,
    ct_volume,
    ct_last,
};

const char *binary_name = "pademelon-tools";

int print_tools(void) {
    int i, status;
    status = printf("These are the available tools:\n\n");
    if (status < 0)
        return EXIT_FAILURE;
    for (i = 0; clitools[i].cliapp.name != NULL; i++) {
        status = printf("%-20s\t%s\n", clitools[i].cliapp.name, clitools[i].cliapp.description);
        if (status < 0)
            return EXIT_FAILURE;
        fflush(stderr);
        if (status < 0)
            return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int wr_backlight(int argc, char *argv[]) {
    char usage_name[strlen(binary_name) + strlen(argv[0]) + 2];
    int status;
    ArgValue *val;
    CliError err;
    snprintf(usage_name, sizeof(usage_name), "%s %s", binary_name, argv[0]);
    status = cli_parse(argc, argv, ct_backlight.cliapp,
            ct_backlight.args, ct_backlight.ops, &err);
    if (!status) {
        if (err == CliErrHelp) {
            cli_print_help(ct_backlight.cliapp, ct_backlight.args, ct_backlight.ops);
            return EXIT_SUCCESS;
        } else {
            cli_print_error(err);
            cli_print_usage(usage_name, ct_backlight.cliapp, ct_backlight.args,
                    ct_backlight.ops);
            return EXIT_FAILURE;
        }
    }

    ;
    if ((val = cli_get_argument(ARG_GET_LONG, ct_backlight.args)) && val->f) {
        return tl_backlight_print();
    } else if ((val = cli_get_argument(ARG_SET_LONG, ct_backlight.args))) {
        return tl_backlight_set(val->i);
    } else if ((val = cli_get_argument(ARG_INC_LONG, ct_backlight.args))) {
        return tl_backlight_inc(val->i);
    } else if ((val = cli_get_argument(ARG_DEC_LONG, ct_backlight.args))) {
        return tl_backlight_dec(val->i);
    }

    cli_print_usage(usage_name, ct_backlight.cliapp, ct_backlight.args,
            ct_backlight.ops);
    return EXIT_FAILURE;
}

int wr_launch_application(int argc, char *argv[]) {
    char usage_name[strlen(binary_name) + strlen(argv[0]) + 2];
    int status;
    OpValue *val_category;
    CliError err;
    snprintf(usage_name, sizeof(usage_name), "%s %s", binary_name, argv[0]);
    status = cli_parse(argc, argv, ct_launch_application.cliapp,
            ct_launch_application.args, ct_launch_application.ops, &err);
    if (!status) {
        if (err == CliErrHelp) {
            cli_print_help(ct_launch_application.cliapp, ct_launch_application.args, ct_launch_application.ops);
            return EXIT_SUCCESS;
        } else {
            cli_print_error(err);
            cli_print_usage(usage_name, ct_launch_application.cliapp, ct_launch_application.args,
                    ct_launch_application.ops);
            return EXIT_FAILURE;
        }
    }

    val_category = cli_get_operand(OP_CATEGORY, ct_launch_application.ops);
    if (!val_category) {
        fprintf(stderr, "Error: missing operand\n");
        return EXIT_FAILURE;
    }
    return tl_launch_application(val_category->s);
}

int wr_load_display_conf(int argc, char *argv[]) {
    char usage_name[strlen(binary_name) + strlen(argv[0]) + 2];
    int status;
    const char *path = NULL;
    ArgValue *val_path;
    CliError err;
    snprintf(usage_name, sizeof(usage_name), "%s %s", binary_name, argv[0]);
    status = cli_parse(argc, argv, ct_load_display_conf.cliapp,
            ct_load_display_conf.args, ct_load_display_conf.ops, &err);
    if (!status) {
        if (err == CliErrHelp) {
            cli_print_help(ct_load_display_conf.cliapp, ct_load_display_conf.args, ct_load_display_conf.ops);
            return EXIT_SUCCESS;
        } else {
            cli_print_error(err);
            cli_print_usage(usage_name, ct_load_display_conf.cliapp, ct_load_display_conf.args,
                    ct_load_display_conf.ops);
            return EXIT_FAILURE;
        }
    }

    val_path = cli_get_argument(ARG_PATH_LONG, ct_load_display_conf.args);
    if (val_path)
        path = val_path->s;
    return tl_load_display_conf(path);
}

int wr_load_wallpaper(int argc, char *argv[]) {
    char usage_name[strlen(binary_name) + strlen(argv[0]) + 2];
    int status;
    CliError err;
    snprintf(usage_name, sizeof(usage_name), "%s %s", binary_name, argv[0]);
    status = cli_parse(argc, argv, ct_load_wallpaper.cliapp,
            ct_load_wallpaper.args, ct_load_wallpaper.ops, &err);
    if (!status) {
        if (err == CliErrHelp) {
            cli_print_help(ct_load_wallpaper.cliapp, ct_load_wallpaper.args, ct_load_wallpaper.ops);
            return EXIT_SUCCESS;
        } else {
            cli_print_error(err);
            cli_print_usage(usage_name, ct_load_wallpaper.cliapp, ct_load_wallpaper.args,
                    ct_load_wallpaper.ops);
            return EXIT_FAILURE;
        }
    }

    return tl_load_wallpaper();
}

int wr_print_applications(int argc, char *argv[]) {
    char usage_name[strlen(binary_name) + strlen(argv[0]) + 2];
    int status;
    CliError err;
    snprintf(usage_name, sizeof(usage_name), "%s %s", binary_name, argv[0]);
    status = cli_parse(argc, argv, ct_print_applications.cliapp,
            ct_print_applications.args, ct_print_applications.ops, &err);
    if (!status) {
        if (err == CliErrHelp) {
            cli_print_help(ct_print_applications.cliapp, ct_print_applications.args, ct_print_applications.ops);
            return EXIT_SUCCESS;
        } else {
            cli_print_error(err);
            cli_print_usage(usage_name, ct_print_applications.cliapp, ct_print_applications.args,
                    ct_print_applications.ops);
            return EXIT_FAILURE;
        }
    }

    return tl_print_applications();
}

int wr_save_display_conf(int argc, char *argv[]) {
    char usage_name[strlen(binary_name) + strlen(argv[0]) + 2];
    int status;
    CliError err;
    snprintf(usage_name, sizeof(usage_name), "%s %s", binary_name, argv[0]);
    status = cli_parse(argc, argv, ct_save_display_conf.cliapp,
            ct_save_display_conf.args, ct_save_display_conf.ops, &err);
    if (!status) {
        if (err == CliErrHelp) {
            cli_print_help(ct_save_display_conf.cliapp, ct_save_display_conf.args, ct_save_display_conf.ops);
            return EXIT_SUCCESS;
        } else {
            cli_print_error(err);
            cli_print_usage(usage_name, ct_save_display_conf.cliapp, ct_save_display_conf.args,
                    ct_save_display_conf.ops);
            return EXIT_FAILURE;
        }
    }

    return tl_save_display_conf();
}

int wr_set_wallpaper(int argc, char *argv[]) {
    char usage_name[strlen(binary_name) + strlen(argv[0]) + 2];
    int status;
    OpValue *val_category;
    CliError err;
    snprintf(usage_name, sizeof(usage_name), "%s %s", binary_name, argv[0]);
    status = cli_parse(argc, argv, ct_set_wallpaper.cliapp,
            ct_set_wallpaper.args, ct_set_wallpaper.ops, &err);
    if (!status) {
        if (err == CliErrHelp) {
            cli_print_help(ct_set_wallpaper.cliapp, ct_set_wallpaper.args, ct_set_wallpaper.ops);
            return EXIT_SUCCESS;
        } else {
            cli_print_error(err);
            cli_print_usage(usage_name, ct_set_wallpaper.cliapp, ct_set_wallpaper.args,
                    ct_set_wallpaper.ops);
            return EXIT_FAILURE;
        }
    }

    val_category = cli_get_operand(OP_PATH, ct_set_wallpaper.ops);
    if (!val_category) {
        fprintf(stderr, "Error: missing operand\n");
        return EXIT_FAILURE;
    }
    return tl_set_wallpaper(val_category->s);
}

int wr_select_application(int argc, char *argv[]) {
    char usage_name[strlen(binary_name) + strlen(argv[0]) + 2];
    int status;
    OpValue *val_category;
    CliError err;
    snprintf(usage_name, sizeof(usage_name), "%s %s", binary_name, argv[0]);
    status = cli_parse(argc, argv, ct_select_application.cliapp,
            ct_select_application.args, ct_select_application.ops, &err);
    if (!status) {
        if (err == CliErrHelp) {
            cli_print_help(ct_select_application.cliapp, ct_select_application.args, ct_select_application.ops);
            return EXIT_SUCCESS;
        } else {
            cli_print_error(err);
            cli_print_usage(usage_name, ct_select_application.cliapp, ct_select_application.args,
                    ct_select_application.ops);
            return EXIT_FAILURE;
        }
    }

    val_category = cli_get_operand(OP_CATEGORY, ct_select_application.ops);
    if (!val_category) {
        fprintf(stderr, "Error: missing operand\n");
        return EXIT_FAILURE;
    }
    return tl_select_application(val_category->s);
}

int wr_test_application(int argc, char *argv[]) {
    char usage_name[strlen(binary_name) + strlen(argv[0]) + 2];
    int status;
    OpValue *val_id_name;
    CliError err;
    snprintf(usage_name, sizeof(usage_name), "%s %s", binary_name, argv[0]);
    status = cli_parse(argc, argv, ct_test_application.cliapp,
            ct_test_application.args, ct_test_application.ops, &err);
    if (!status) {
        if (err == CliErrHelp) {
            cli_print_help(ct_test_application.cliapp, ct_test_application.args, ct_test_application.ops);
            return EXIT_SUCCESS;
        } else {
            cli_print_error(err);
            cli_print_usage(usage_name, ct_test_application.cliapp, ct_test_application.args,
                    ct_test_application.ops);
            return EXIT_FAILURE;
        }
    }

    val_id_name = cli_get_operand(OP_ID_NAME, ct_test_application.ops);
    if (!val_id_name) {
        fprintf(stderr, "Error: missing operand\n");
        return EXIT_FAILURE;
    }
    return tl_test_application(val_id_name->s);
}

int wr_volume(int argc, char *argv[]) {
    char usage_name[strlen(binary_name) + strlen(argv[0]) + 2];
    int status, play_sound;
    ArgValue *val;
    CliError err;
    snprintf(usage_name, sizeof(usage_name), "%s %s", binary_name, argv[0]);
    status = cli_parse(argc, argv, ct_volume.cliapp,
            ct_volume.args, ct_volume.ops, &err);
    if (!status) {
        if (err == CliErrHelp) {
            cli_print_help(ct_volume.cliapp, ct_volume.args, ct_volume.ops);
            return EXIT_SUCCESS;
        } else {
            cli_print_error(err);
            cli_print_usage(usage_name, ct_volume.cliapp, ct_volume.args,
                    ct_volume.ops);
            return EXIT_FAILURE;
        }
    }

    play_sound = !((val = cli_get_argument(ARG_QUIET_LONG, ct_volume.args)) && val->f);

    if ((val = cli_get_argument(ARG_GET_LONG, ct_volume.args)) && val->f) {
        status = tl_volume_print();
    } else if ((val = cli_get_argument(ARG_SET_LONG, ct_volume.args))) {
        status = tl_volume_set(val->i);
    } else if ((val = cli_get_argument(ARG_INC_LONG, ct_volume.args))) {
        status = tl_volume_inc(val->i, play_sound);
#ifdef CANBERRA
        sleep(1);
#endif /* CANBERRA */
    } else if ((val = cli_get_argument(ARG_DEC_LONG, ct_volume.args))) {
        status = tl_volume_dec(val->i, play_sound);
#ifdef CANBERRA
        sleep(1);
#endif /* CANBERRA */
    } else if ((val = cli_get_argument(ARG_MUTE_OUT_LONG, ct_volume.args)) && val->s) {
        if (strcmp(val->s, "mute") == 0
                || strcmp(val->s, "1") == 0
                || strcmp(val->s, "true") == 0
                || strcmp(val->s, "True") == 0) {
            status = tl_volume_mute_output(1);
        } else if (strcmp(val->s, "unmute") == 0
                || strcmp(val->s, "0") == 0
                || strcmp(val->s, "false") == 0
                || strcmp(val->s, "False") == 0) {
            status = tl_volume_mute_output(0);
        } else if (strcmp(val->s, "toggle") == 0
                || strcmp(val->s, "t") == 0) {
            status = tl_volume_mute_output(-1);
        } else {
            fprintf(stderr, "Value '%s' not valid for option '%s'\n", val->s, ARG_MUTE_OUT_LONG);
            return EXIT_FAILURE;
        }
    } else if ((val = cli_get_argument(ARG_MUTE_IN_LONG, ct_volume.args)) && val->s) {
        if (strcmp(val->s, "mute") == 0
                || strcmp(val->s, "1") == 0
                || strcmp(val->s, "true") == 0
                || strcmp(val->s, "True") == 0) {
            status = tl_volume_mute_input(1);
        } else if (strcmp(val->s, "unmute") == 0
                || strcmp(val->s, "0") == 0
                || strcmp(val->s, "false") == 0
                || strcmp(val->s, "False") == 0) {
            status = tl_volume_mute_input(0);
        } else if (strcmp(val->s, "toggle") == 0
                || strcmp(val->s, "t") == 0) {
            status = tl_volume_mute_input(-1);
        } else {
            fprintf(stderr, "Value '%s' not valid for option '%s'\n", val->s, ARG_MUTE_IN_LONG);
            return EXIT_FAILURE;
        }
    } else {
        cli_print_usage(usage_name, ct_volume.cliapp, ct_volume.args,
                ct_volume.ops);
        return EXIT_FAILURE;
    }

    return status;
}


int main(int argc, char *argv[]) {
    int i, status, tool_found;
    status = EXIT_FAILURE;
    tool_found = 0;

    if (argc < 2) {
        status = print_tools();
        exit(status);
    }

    binary_name = argv[0];

#ifdef X11
    x11_init();
#endif /* X11 */

    for (i = 0; clitools[i].cliapp.name != NULL; i++) {
        if (strcmp(clitools[i].cliapp.name, argv[1]) == 0) {
            status = clitools[i].execute(argc - 1, &argv[1]);
            tool_found = 1;
            break;
        }
    }

    if (!tool_found)
        status = print_tools();

#ifdef X11
    x11_deinit();
#endif /* X11 */

    return status;
}

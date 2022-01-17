#include "tools.h"
#include "cliparse.h"
#ifdef X11
#include "x11-utils.h"
#endif /* X11 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARG_PATH_LONG           "--path"
#define ARG_PATH_SHORT          "-p"
#define ARG_PATH_PLACEHOLDER    "path"
#define OP_CATEGORY             "category"
#define OP_ID_NAME              "id-name"
#define OP_PATH                 "path"

typedef int (*ToolFunction)(int argc, char *argv[]);
struct clitool {
    CliApplication cliapp;
    CliArgument *args;
    CliOperand *ops;
    ToolFunction execute;
};

static int print_tools(void);
int wr_launch_application(int argc, char *argv[]);
int wr_load_display_conf(int argc, char *argv[]);
int wr_load_wallpaper(int argc, char *argv[]);
int wr_print_applications(int argc, char *argv[]);
int wr_save_display_conf(int argc, char *argv[]);
int wr_select_application(int argc, char *argv[]);
int wr_set_wallpaper(int argc, char *argv[]);
int wr_test_application(int argc, char *argv[]);

const struct clitool ct_last = {0};

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
CliOperand ops_set_wallpaper[] = { { ArgTypeString, OP_PATH } };
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

struct clitool clitools[] = {
    ct_launch_application,
    ct_launch_application,
    ct_load_display_conf,
    ct_load_wallpaper,
    ct_print_applications,
    ct_save_display_conf,
    ct_select_application,
    ct_set_wallpaper,
    ct_test_application,
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

int wr_launch_application(int argc, char *argv[]) {
    int status;
    OpValue *val_category;
    CliError err;
    status = cli_parse(argc, argv, ct_launch_application.cliapp,
            ct_launch_application.args, ct_launch_application.ops, &err);
    if (!status) {
        if (err == CliErrHelp) {
            cli_print_help(ct_launch_application.cliapp, ct_launch_application.args, ct_launch_application.ops);
            return EXIT_SUCCESS;
        } else {
            cli_print_error(err);
            cli_print_usage(binary_name, ct_launch_application.cliapp, ct_launch_application.args,
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
    int status;
    const char *path = NULL;
    ArgValue *val_path;
    CliError err;
    status = cli_parse(argc, argv, ct_load_display_conf.cliapp,
            ct_load_display_conf.args, ct_load_display_conf.ops, &err);
    if (!status) {
        if (err == CliErrHelp) {
            cli_print_help(ct_load_display_conf.cliapp, ct_load_display_conf.args, ct_load_display_conf.ops);
            return EXIT_SUCCESS;
        } else {
            cli_print_error(err);
            cli_print_usage(binary_name, ct_load_display_conf.cliapp, ct_load_display_conf.args,
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
    int status;
    CliError err;
    status = cli_parse(argc, argv, ct_load_wallpaper.cliapp,
            ct_load_wallpaper.args, ct_load_wallpaper.ops, &err);
    if (!status) {
        if (err == CliErrHelp) {
            cli_print_help(ct_load_wallpaper.cliapp, ct_load_wallpaper.args, ct_load_wallpaper.ops);
            return EXIT_SUCCESS;
        } else {
            cli_print_error(err);
            cli_print_usage(binary_name, ct_load_wallpaper.cliapp, ct_load_wallpaper.args,
                    ct_load_wallpaper.ops);
            return EXIT_FAILURE;
        }
    }

    return tl_load_wallpaper();
}

int wr_print_applications(int argc, char *argv[]) {
    int status;
    CliError err;
    status = cli_parse(argc, argv, ct_print_applications.cliapp,
            ct_print_applications.args, ct_print_applications.ops, &err);
    if (!status) {
        if (err == CliErrHelp) {
            cli_print_help(ct_print_applications.cliapp, ct_print_applications.args, ct_print_applications.ops);
            return EXIT_SUCCESS;
        } else {
            cli_print_error(err);
            cli_print_usage(binary_name, ct_print_applications.cliapp, ct_print_applications.args,
                    ct_print_applications.ops);
            return EXIT_FAILURE;
        }
    }

    return tl_print_applications();
}

int wr_save_display_conf(int argc, char *argv[]) {
    int status;
    CliError err;
    status = cli_parse(argc, argv, ct_save_display_conf.cliapp,
            ct_save_display_conf.args, ct_save_display_conf.ops, &err);
    if (!status) {
        if (err == CliErrHelp) {
            cli_print_help(ct_save_display_conf.cliapp, ct_save_display_conf.args, ct_save_display_conf.ops);
            return EXIT_SUCCESS;
        } else {
            cli_print_error(err);
            cli_print_usage(binary_name, ct_save_display_conf.cliapp, ct_save_display_conf.args,
                    ct_save_display_conf.ops);
            return EXIT_FAILURE;
        }
    }

    return tl_save_display_conf();
}

int wr_set_wallpaper(int argc, char *argv[]) {
    int status;
    OpValue *val_category;
    CliError err;
    status = cli_parse(argc, argv, ct_set_wallpaper.cliapp,
            ct_set_wallpaper.args, ct_set_wallpaper.ops, &err);
    if (!status) {
        if (err == CliErrHelp) {
            cli_print_help(ct_set_wallpaper.cliapp, ct_set_wallpaper.args, ct_set_wallpaper.ops);
            return EXIT_SUCCESS;
        } else {
            cli_print_error(err);
            cli_print_usage(binary_name, ct_set_wallpaper.cliapp, ct_set_wallpaper.args,
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
    int status;
    OpValue *val_category;
    CliError err;
    status = cli_parse(argc, argv, ct_select_application.cliapp,
            ct_select_application.args, ct_select_application.ops, &err);
    if (!status) {
        if (err == CliErrHelp) {
            cli_print_help(ct_select_application.cliapp, ct_select_application.args, ct_select_application.ops);
            return EXIT_SUCCESS;
        } else {
            cli_print_error(err);
            cli_print_usage(binary_name, ct_select_application.cliapp, ct_select_application.args,
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
    int status;
    OpValue *val_id_name;
    CliError err;
    status = cli_parse(argc, argv, ct_test_application.cliapp,
            ct_test_application.args, ct_test_application.ops, &err);
    if (!status) {
        if (err == CliErrHelp) {
            cli_print_help(ct_test_application.cliapp, ct_test_application.args, ct_test_application.ops);
            return EXIT_SUCCESS;
        } else {
            cli_print_error(err);
            cli_print_usage(binary_name, ct_test_application.cliapp, ct_test_application.args,
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

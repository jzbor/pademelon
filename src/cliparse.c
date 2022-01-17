#include "cliparse.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int str_to_int(const char *str, int *integer);

static inline int count_args(CliArgument *arguments) {
    int nargs;
    for (nargs = 0; memcmp((char*) &arguments[nargs], (char *) &((CliArgument){0}), sizeof(CliArgument)) != 0; nargs++);
    return nargs;
}

static inline int count_ops(CliOperand *operands) {
    int nops;
    for (nops = 0; strncmp((char*) &operands[nops], (char *) &((CliOperand){0}), sizeof(CliOperand)) != 0; nops++);
    return nops;
}

ArgValue *cli_get_argument(char *long_name, CliArgument *arguments) {
    int i;
    int nargs = count_args(arguments);
    for (i = 0; i < nargs; i++) {
        if (strcmp(long_name, arguments[i].long_name) == 0) {
            if (arguments[i].is_set)
                return &arguments[i].value;
            else
                return NULL;
        }
    }
    return NULL;
}

OpValue *cli_get_operand(char *placeholder, CliOperand *operands) {
    int i;
    int nops = count_ops(operands);
    for (i = 0; i < nops; i++) {
        if (strcmp(placeholder, operands[i].placeholder) == 0)
            return &operands[i].value;
    }
    return NULL;
}

int cli_parse(int argc, char *argv[], CliApplication cli_application, CliArgument *arguments,
        CliOperand *operands, CliError *err) {
    int i, j, nargs, nops, first_operand;
    int argv_used[argc];

    /* sanity checks */
    if (argc < 1 || !argv)
        return 0;

    /* init argv_used */
    memset(argv_used, 0, sizeof(int) * argc);
    argv_used[0] = 1; /* binary name */

    /* get argument and operand count */
    nargs = count_args(arguments);
    nops = count_ops(operands);

    /* parse arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0
                || strcmp(argv[i], "-h") == 0) {
            *err = CliErrHelp;
            return 0;
        }
        for (j = 0; j < nargs; j++) {
            if (strcmp(argv[i], arguments[j].long_name) == 0
                    || strcmp(argv[i], arguments[j].short_name) == 0) {
                if (arguments[j].type == ArgTypeFlag) {
                    argv_used[i] = 1;
                    arguments[j].value.f = 1;
                    arguments[j].is_set = 1;
                } else if (arguments[j].type == ArgTypeInteger) {
                    argv_used[i] = 1;
                    i++;
                    if (!str_to_int(argv[i], &arguments[j].value.i)) {
                        *err = CliErrInvalidInt;
                        return 0;
                    }
                    argv_used[i] = 1;
                    arguments[j].is_set = 1;
                } else if (arguments[j].type == ArgTypeString) {
                    if (i == argc - 1) {
                        *err = CliErrMissingArgVal;
                        return 0;
                    }
                    argv_used[i] = 1;
                    i++;
                    argv_used[i] = 1;
                    arguments[j].value.s = argv[i];
                    arguments[j].is_set = 1;
                } else {
                    *err = CliErrInvalidType;
                    return 0;
                }
            }
        }
    }

    /* skip operand parsing if none are specified */
    if (nops > 0) {
        /* find index of first operand */
        first_operand = -1;
        for (i = 1; i < argc; i++) {
            if (!argv_used[i]) {
                first_operand = i;
            } else if (first_operand != -1) {
                *err = CliErrOperandNotLast;
                return 0;
            }
        }
        if (argc - first_operand != nops) {
            *err = CliErrWrongNoOperands;
            return 0;
        }

        /* read operands */
        for (i = 0; i < nops; i++) {
            if (operands[i].type == ArgTypeString) {
                operands[i].value.s = argv[first_operand + i];
                argv_used[first_operand + i] = 1;
            } else if (operands[i].type == ArgTypeInteger) {
                if (str_to_int(argv[first_operand + i], &operands[i].value.i)) {
                    *err = CliErrInvalidInt;
                    return 0;
                }
                argv_used[first_operand + i] = 1;
            } else {
                *err = CliErrInvalidType;
                return 0;
            }
        }
    }

    for (i = 1; i < argc; i++) {
        if (!argv_used[i]) {
            *err = CliErrUnknownArg;
            return 0;
        }
    }

    return 1;
}

void cli_print_error(CliError err) {
    char *msg;
    switch (err) {
        case CliErrInvalidType:
            msg = "invalid type requested by application";
            break;
        case CliErrMissingArgVal:
            msg = "missing value for argument";
            break;
        case CliErrOperandNotLast:
            msg = "operands must be specified last";
            break;
        case CliErrUnknownArg:
            msg = "unknown argument";
            break;
        case CliErrWrongNoOperands:
            msg = "wrong operand or not enough operands";
            break;
        default:
            msg = "unknown error";
    }

    fprintf(stderr, "cliparse: An error occured: %s\n", msg);
}

void cli_print_help(CliApplication cli_application, CliArgument *arguments, CliOperand *operands) {
    int i, nargs, nops;
    printf("\n%s - %s\n", cli_application.name, cli_application.description);

    /* get argument and operand count */
    nargs = count_args(arguments);
    nops = count_ops(operands);

    printf("\nArguments: \n");
    for (i = 0; i < nargs; i++) {
        if (arguments[i].placeholder) {
            printf("\t%-5s\t%s <%s>\n", arguments[i].short_name ? arguments[i].short_name : "",
                    arguments[i].long_name ? arguments[i].long_name : "",
                    arguments[i].placeholder);
        } else {
            printf("\t%-5s\t%s\n", arguments[i].short_name ? arguments[i].short_name : "",
                    arguments[i].long_name ? arguments[i].long_name : "");

        }
    }

    printf("\nOperands: \n");
    for (i = 0; i < nops; i++)
        printf("\t%s\n", operands[i].placeholder);

    fflush(stdout);
}

void cli_print_usage(const char *binary_name, CliApplication cli_application, CliArgument *arguments,
        CliOperand *operands) {
    int i, nargs, nops;
    printf("\nUsage:\n%s ", binary_name);

    /* get argument and operand count */
    nargs = count_args(arguments);
    nops = count_ops(operands);

    for (i = 0; i < nargs; i++) {
        if (arguments[i].placeholder) {
            printf("[ %s/%s <%s> ] ", arguments[i].short_name ? arguments[i].short_name : "",
                    arguments[i].long_name ? arguments[i].long_name : "", arguments[i].placeholder);
        } else {
            printf("[ %s | %s ] ", arguments[i].short_name ? arguments[i].short_name : "",
                    arguments[i].long_name ? arguments[i].long_name : "");
        }
    }

    for (i = 0; i < nops; i++)
        printf("<%s> ", operands[i].placeholder);

    printf("\n");
    fflush(stdout);
}

int str_to_int(const char *str, int *integer) {
    char *endptr;
    long ltemp;
    int val;
    errno = 0;
    ltemp = strtol(str, &endptr, 0);
    val = ltemp;
    if (val != ltemp || (ltemp == 0 && errno != 0) || str == endptr || (*endptr) != '\0') {
        return 0;
    }
    *integer = val;
    return 1;
}

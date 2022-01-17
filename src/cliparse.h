#ifndef H_CLIPARSE
#define H_CLIPARSE

typedef enum {
    ArgTypeInteger, ArgTypeString, ArgTypeFlag,
} ArgType;
typedef ArgType OpType;

typedef enum {
    CliErrHelp, CliErrInvalidInt, CliErrInvalidType, CliErrMissingArgVal, CliErrOperandNotLast,
    CliErrUnknownArg, CliErrWrongNoOperands,
} CliError;

typedef union {
    int i;
    const char *s;
    int f;
} ArgValue;
typedef ArgValue OpValue;

typedef struct {
    ArgType type;
    const char *long_name, *short_name, *placeholder;
    int is_set;
    ArgValue value;
} CliArgument;

typedef struct {
    ArgType type;
    const char *placeholder;
    ArgValue value;
} CliOperand;

typedef struct {
    const char *name, *description;
} CliApplication;


ArgValue *cli_get_argument(char *long_name, CliArgument *arguments);
ArgValue *cli_get_operand(char *placeholder, CliOperand *operands);
int cli_parse(int argc, char *argv[], CliApplication cli_application, CliArgument *arguments,
        CliOperand *operands, CliError *err);
void cli_print_error(CliError err);
void cli_print_help(CliApplication cli_application, CliArgument *arguments, CliOperand *operands);
void cli_print_usage(const char *binary_name, CliApplication cli_application, CliArgument *arguments,
        CliOperand *operands);

#endif /* H_CLIPARSE */


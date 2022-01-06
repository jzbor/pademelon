#include "tools.h"
#include "x11-utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int (*ToolFunction)(int argc, char *argv[]);
struct tool {
    const char *name, *description;
    ToolFunction execute;
};

static int print_tools(void);

struct tool tools[] = {
    { "load-display-conf",  "load last saved display configuration",    tl_load_display_conf },
    { "load-wallpaper",     "load wallpaper",                           tl_load_wallpaper },
    { "print-daemons",      "print installed daemon files",             tl_print_daemons },
    { "save-display-conf",  "save current display configuration",       tl_save_display_conf },
    { "set-wallpaper",      "set the wallpaper",                        tl_set_wallpaper },
    { NULL,                 NULL,                                       NULL },
};

int print_tools(void) {
    int i, status;
    status = printf("These are the available tools:\n\n");
    if (status < 0)
        return EXIT_FAILURE;
    for (i = 0; tools[i].name != NULL; i++) {
        status = printf("%-20s\t%s\n", tools[i].name, tools[i].description);
        if (status < 0)
            return EXIT_FAILURE;
        fflush(stderr);
        if (status < 0)
            return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;

}

int main(int argc, char *argv[]) {
    int i, status, tool_found;
    status = EXIT_FAILURE;
    tool_found = 0;

    if (argc < 2) {
        status = print_tools();
        exit(status);
    }

    x11_init();

    for (i = 0; tools[i].name != NULL; i++) {
        if (strcmp(tools[i].name, argv[1]) == 0) {
            status = tools[i].execute(argc - 2, &argv[2]);
            tool_found = 1;
            break;
        }
    }

    if (!tool_found)
        status = print_tools();

    x11_deinit();

    return status;
}

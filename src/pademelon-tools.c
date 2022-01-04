#include "tools.h"
#include "x11-utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int (*ToolFunction)(int argc, char *argv[]);
struct tool {
    const char *name;
    ToolFunction execute;
};

struct tool tools[] = {
    { "load-wallpaper", tl_load_wallpaper },
    { "set-wallpaper", tl_set_wallpaper },
    { NULL, NULL },
};


int main(int argc, char *argv[]) {
    int i, status;
    status = EXIT_FAILURE;

    if (argc < 2) {
        fprintf(stderr, "%s requires at least one argument \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    x11_init();

    for (i = 0; tools[i].name != NULL; i++) {
        if (strcmp(tools[i].name, argv[1]) == 0) {
            status = tools[i].execute(argc - 2, &argv[2]);
            break;
        }
    }

    x11_deinit();

    return status;
}

#include <stddef.h>
#include <stdlib.h>
#include "common.h"

int main(int argc, char *argv[]) {
    char *sysconf, *userconf;
    sysconf = system_config_path("daemon.conf");
    userconf = user_config_path("blabla.conf");
    report(R_DEBUG, "Config paths:");
    report(R_DEBUG, sysconf);
    report(R_DEBUG, userconf);

    free(sysconf);
    free(userconf);

}

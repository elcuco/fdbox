#include <stdlib.h>
#include <stdio.h>

#include "fdbox.h"
#include "dos/cls.h"

/*
This file is part of fdbox
For license - read license.txt
*/

int command_cls(int argc, char* argv[]) {
#ifdef __linux__
    printf("\e[1;1H\e[2J");
#endif

#ifdef WIN32
    printf("\e[1;1H\e[2J");
#endif

    return EXIT_FAILURE;
}

const char* help_cls() {
    return "Clear screen";
}

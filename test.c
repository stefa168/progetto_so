//#define DEBUG

#include <stdio.h>
#include "settings_reader.h"




int main(int argc, char *argv[]) {
    int settings[7];
    int i;

    readConfiguration(settings, argc, argv);

    for (i = 0; i < 7; i++) {
        printf("%d\n", settings[i]);
    }

    return 0;
}


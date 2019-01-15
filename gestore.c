#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include "gestore.h"
#include "settings_reader.h"

int main(int argc, char *argv[]) {
    SettingsData *settings;

    settings = readConfiguration(argc, argv);

//    printf("Trovate le impostazioni:\n\t- ");

    // Gestire prima della logica degli studenti tutta la parte della memoria e il comportamento del gestore

}
#define DEBUG
#include "gestore.h"
#include "settings_reader.h"

#define SETTINGS_NUM 8

int main(int argc, char *argv[]) {
    int settings[SETTINGS_NUM];

    readConfiguration(settings, SETTINGS_NUM, argc, argv);

    // Gestire prima della logica degli studenti tutta la parte della memoria e il comportamento del gestore



}
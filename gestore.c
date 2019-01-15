#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include "gestore.h"
#include "settings_reader.h"

int main(int argc, char *argv[]) {
    SettingsData *settings;

#ifdef DEBUG
    int i;
#endif

    settings = readConfiguration(argc, argv);

#ifdef DEBUG
    printf("Trovate le impostazioni:\n"
           "\t- pop_size = %d\n"
           "\t- sim_duration = %d\n"
           "\t- AdE_min = %d\n"
           "\t- AdE_max = %d\n"
           "\t- minGroupPref = %d\n"
           "\t- maxGroupPref = %d\n"
           "\t- nof_invites = %d\n"
           "\t- nof_refuse = %d\n"
           "\t- settingsCount = %d\n"
           "\t- numOfPreferences = %d\n"
           "\t- prefPerc = [", settings->pop_size, settings->sim_duration, settings->AdE_min, settings->AdE_max,
           settings->minGroupPref, settings->maxGroupPref, settings->nof_invites, settings->nof_refuse,
           settings->settingsCount, settings->numOfPreferences);

    for (i = 0; i < settings->numOfPreferences; i++) {
        if (i == settings->numOfPreferences - 1) {
            printf("%d]\n", settings->preferencePercentages[i]);
        } else {
            printf("%d,", settings->preferencePercentages[i]);
        }
    }
#endif

    // Gestire prima della logica degli studenti tutta la parte della memoria e il comportamento del gestore

    return 0;
}
#ifndef ESERCIZI_SO_SETTINGS_READER_H
#define ESERCIZI_SO_SETTINGS_READER_H

#define MAX_LEN 128
#define NOF_SETTINGS 8
#define DEBUG

typedef struct SettingsData {
    int settingsCount;

    int pop_size;
    int sim_duration;

    int AdE_min;
    int AdE_max;

    int minGroupPref;
    int maxGroupPref;

    int nof_invites;
    int nof_refuse;

    int numOfPreferences;

    int *preferencePercentages;
} SettingsData;

SettingsData *readConfiguration(int argc, char *argv[]);

void setSettingsValue(SettingsData *data, int index, int value);

int getSettingsValue(SettingsData *data, int index);

#endif

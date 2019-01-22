#ifndef PROGETTO_SO_SETTINGS_READER_H
#define PROGETTO_SO_SETTINGS_READER_H

#include "types.h"

#define MAX_LEN 128
#define NOF_SETTINGS 8
#define MAX_PREFERENCES 16

//#define S_R_DEBUG

SettingsData *readConfiguration(int argc, char *argv[]);

void setSettingsValue(SettingsData *data, int index, int value);

int getSettingsValue(SettingsData *data, int index);

bool validateSettings(SettingsData *data); // todo

#endif

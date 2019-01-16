#ifndef PROGETTO_SO_GESTORE_H
#define PROGETTO_SO_GESTORE_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <unistd.h>
#include "utilities/settings_reader.h"
#include "utilities/macros.h"
#include "utilities/ipc_utils.h"
#include "utilities/types.h"

void printFoundSettings(SettingsData *settings);

#endif

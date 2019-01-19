#ifndef PROGETTO_SO_GESTORE_H
#define PROGETTO_SO_GESTORE_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "utilities/settings_reader.h"
#include "utilities/macros.h"
#include "utilities/ipc_utils.h"
#include "utilities/types.h"
#include "utilities/utils.h"

#define STUDENT_PATH "Studente"

void printFoundSettings(SettingsData *settings);

#endif

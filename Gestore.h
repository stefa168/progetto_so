#ifndef PROGETTO_SO_GESTORE_H
#define PROGETTO_SO_GESTORE_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "utilities/settings_reader.h"
#include "utilities/macros.h"
#include "utilities/ipc_utils.h"
#include "utilities/types.h"
#include "utilities/utils.h"

#define STUDENT_PATH "Studente.r"

void printFoundSettings(SettingsData *settings);

void abortSimulationOnSignal(int sigid);

void raiseSignalToStudents(int sigid);

void instantiateChildren();

void calculateStudentsMarks();

void calculatePadding(int valueToPrint, int maxSize, int *leftPadding, int *rightPadding);

void printSimulationResults();

void waitForZombieChildren();

void freeAllocatedMemory();

void closeIPC();

#endif

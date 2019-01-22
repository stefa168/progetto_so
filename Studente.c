#include "Studente.h"

int myID;
int sharedMemoryID, semaphoresID;
SimulationData *simulationData;
StudentData *this;
SettingsData *settings;

int main(int argc, char *argv[]) {
    semaphoresID = getSemaphoresID();
    sharedMemoryID = getSharedMemoryID();
    simulationData = attachSharedMemory(sharedMemoryID);

    myID = (int) strtol(argv[1], NULL, 10);

    this = &simulationData->students[myID];
    settings = &simulationData->settings;


    PRINT_IF_ERRNO_EXIT(-1)

    initializeStudent();

    reserveSemaphore(semaphoresID, SEMAPHORE_CAN_PRINT);

    printf("[%d]: Sono lo studente %d\n", getpid(), myID);
    printf("[%d]Il mio voto di ARCH e' %d e preferisco stare con %d studenti in gruppo.\n", getpid(), this->voto_AdE,
           this->nofElemsPref);
    printf("[%d]Tutto qui.\n\n", getpid());

    releaseSemaphore(semaphoresID, SEMAPHORE_CAN_PRINT);

    reserveSemaphore(semaphoresID, SEMAPHORE_EVERYONE_READY);
    waitForZero(semaphoresID, SEMAPHORE_EVERYONE_READY, 0);
    return 0;
}

void initializeStudent() {
    int i, minPref, maxPref;
    int numOfPreferences = simulationData->settings.numOfPreferences;
    int *prefValues = calloc((size_t) numOfPreferences, sizeof(int));
    int *myPref = &(this->nofElemsPref);

    minPref = simulationData->settings.minGroupPref;
    maxPref = simulationData->settings.maxGroupPref;

    for (i = 0; i < maxPref - minPref + 1; i++) {
        prefValues[i] = minPref + i;
    }

    initRandom((unsigned int) getpid());


#if PREFERENCE_LOGIC == 0
    *myPref = getWeightedRand(numOfPreferences, prefValues, simulationData->settings.preferencePercentages);
    this->nofElemsPref = myPref;
#else
    *myPref = getWeighted(settings->pop_size, myID, numOfPreferences, prefValues, settings->preferencePercentages);
#endif


}

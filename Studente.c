#include "Studente.h"

int myID;
int sharedMemoryID, semaphoresID;
SimulationData *simulationData;
StudentData *this;

int main(int argc, char *argv[]) {
    semaphoresID = getSemaphoresID();
    sharedMemoryID = getSharedMemoryID();
    simulationData = attachSharedMemory(sharedMemoryID);

    myID = (int) strtol(argv[1], NULL, 10);

    PRINT_IF_ERRNO_EXIT(-1)

    printf("%d\n", simulationData->settings.numOfPreferences);
    printf("%d\n", simulationData->settings.preferencePercentages[0]);

    initializeStudent();

    reserveSemaphore(semaphoresID, SEMAPHORE_CAN_PRINT);

    printf("[%d]: Sono lo studente %d\n", getpid(), myID);
    printf("[%d]Il mio voto di ARCH e' %d e preferisco stare con %d studenti in gruppo.\n", this->voto_AdE,
           this->nofElemsPref);
    printf("[%d]Tutto qui.\n");

    releaseSemaphore(semaphoresID, SEMAPHORE_CAN_PRINT);

    reserveSemaphore(semaphoresID, SEMAPHORE_EVERYONE_READY);
    return 0;
}

void initializeStudent() {
    int numOfPreferences = simulationData->settings.numOfPreferences;
    int *prefValues = calloc((size_t) numOfPreferences, sizeof(int));
    int i, minPref, maxPref, myPref;

    minPref = simulationData->settings.minGroupPref;
    maxPref = simulationData->settings.maxGroupPref;

    for (i = 0; i < maxPref - minPref + 1; i++) {
        prefValues[i] = minPref + i;
    }

    initRandom((unsigned int) getpid());

    this = &simulationData->students[myID];

    myPref = getWeightedRand(numOfPreferences, prefValues, simulationData->settings.preferencePercentages);
    this->nofElemsPref = myPref;
}

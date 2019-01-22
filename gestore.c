#define DEBUG_GESTORE

//#include "gestore.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "utilities/settings_reader.h"
#include "utilities/macros.h"
#include "utilities/ipc_utils.h"
#include "utilities/utils.h"

#define STUDENT_PATH "Studente"

void printFoundSettings(SettingsData *settings);

int *childrenPIDs;
int childrenCounter = 0;

int semid, shmid;
SettingsData *settings;
SimulationData *simulationData;

void instantiateChildren();

void closeIPC() {
    destroySemaphores(semid);

    detachSharedMemory(simulationData);

    destroySharedMemory(shmid);
}

int terminateZombieChildrens(bool block) {
    int childrenReturnStatus, childrenPid;
    int options = 0;
    int caughtZombiesCount = 0;

    /* Se non vogliamo che non sia bloccante, impostiamo la flag apposita. */
    if (!block) {
        options |= WNOHANG;
    }

    do {
        /*
         * Con -1 facciamo aspettare qualsiasi processo figlio.
         * Continuiamo a farlo fino a quando waitpid non ritorna 0, il che significa che esistono processi figli ma
         * nessuno è ancora zombie.
         */
        childrenPid = waitpid(-1, &childrenReturnStatus, options);
        if (childrenPid > 0) {
            printf("[!] Terminato con successo il processo zombie %d con stato %d\n", childrenPid,
                   childrenReturnStatus);
            caughtZombiesCount++;
        } else if (childrenPid < 0) {
            PRINT_ERRNO
            errno = 0;
        }
    } while (childrenPid > 0);

    return caughtZombiesCount;
}

void terminateSimulation(int sigid) {
//    int i;

    static bool terminating = false;

    if (!terminating) {
        terminating = true;
    } else {
        return;
    }

    int i, childrenReturnStatus, childrenPid;

    printf("Iniziata terminazione del programma con segnale %d\n\n", sigid);

    for (i = 0; i < childrenCounter; i++) {
        kill(childrenPIDs[i], SIGTERM);
        if (errno) {
            PRINT_ERRNO
            errno = 0;
        }
    }

    while (childrenCounter > 0) {
        childrenPid = wait(&childrenReturnStatus);
        printf("[!] %d Terminato con successo il PID %d con stato %d\n", childrenCounter, childrenPid,
               childrenReturnStatus);
        childrenCounter--;
    }

    closeIPC();

    exit(0);
}

void printSignal(int sig) {
    printf("Segnale ricevuto = %d\n", sig);
}

int main(int argc, char *argv[]) {
    settings = readConfiguration(argc, argv);

    signal(SIGINT, terminateSimulation);
    signal(SIGTERM, terminateSimulation);
    /* Utilizzare questo segnale durante lo sviluppo solo se ci sono problemi; può provocare la terminazione della
     * simulazione prima del previsto */
    signal(SIGCHLD, terminateSimulation);
    signal(SIGSEGV, terminateSimulation);

    printFoundSettings(settings);

    initRandom((unsigned int) getpid());

    childrenPIDs = calloc((size_t) settings->pop_size, sizeof(int));

    shmid = createSharedMemory(sizeof(SettingsData) +
                               settings->numOfPreferences * sizeof(int) +
                               settings->pop_size * sizeof(StudentData));

    simulationData = attachSharedMemory(shmid);

    /* Copiamo le impostazioni nella memoria condivisa. */
    memcpy(&simulationData->settings, settings, sizeof(SettingsData) + settings->numOfPreferences * sizeof(int));

    semid = createSemaphores(settings->pop_size);

    initializeSemaphore(semid, SEMAPHORE_EVERYONE_READY, settings->pop_size, 0);
    initializeSemaphore(semid, SEMAPHORE_CAN_PRINT, 1, 0);

    instantiateChildren();

    waitForZero(semid, SEMAPHORE_EVERYONE_READY, 0);

    closeIPC();

    return 0;
}

void instantiateChildren() {
    char childrenIDString[16] = {0};
    for (childrenCounter = 0; childrenCounter < settings->pop_size; childrenCounter++) {
        switch (childrenPIDs[childrenCounter] = fork()) {
            case -1: {
                // todo killare tutti i figli
                PRINT_ERRNO_EXIT(-1)
            }

            case 0: {
                /* Siamo un figlio, avviamo execv */
                sprintf(childrenIDString, "%d", childrenCounter);
                execl(STUDENT_PATH, STUDENT_PATH, childrenIDString);
                // todo verificare cosa fare se ci sono problemi
                //todo intercettare il segnale dei figli nel padre
                PRINT_ERRNO_EXIT(-1)
            }
            default: {
                //todo manage new children
                simulationData->students[childrenCounter].voto_AdE = getRandomRange(settings->AdE_min,
                                                                                    settings->AdE_max);
                printf("Inizializzato processo con PID %d. Il suo voto è%d.\n\n",
                       childrenPIDs[childrenCounter],
                       simulationData->students[childrenCounter].voto_AdE);


                break;
            }
        }
    }
}

#ifdef DEBUG_GESTORE

void printFoundSettings(SettingsData *settings) {
    int i;

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
            printf("%d]\n\n", settings->preferencePercentages[i]);
        } else {
            printf("%d,", settings->preferencePercentages[i]);
        }
    }
}

#endif
#define DEBUG_GESTORE

#include "gestore.h"

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

void terminateSimulation(int sigid) {
    int i, childrenReturnStatus, childrenPid;

    printf("Iniziata terminazione del programma con segnale %d\n\n", sigid);

    for (i = 0; i < childrenCounter; i++) {
        kill(childrenPIDs[i], SIGTERM);
        if (errno) {
            PRINT_ERRNO
            errno = 0;
        }
    }

    childrenCounter--;

    while (childrenCounter > 0) {
        childrenPid = wait(&childrenReturnStatus);
        printf("[!] Terminato con successo il PID %d con stato %d\n", childrenPid, childrenReturnStatus);
        childrenCounter--;
    }

    closeIPC();

    exit(0);
}

int main(int argc, char *argv[]) {
//    int *test;
//    int i, minPref, maxPref, myPref;
//    int *prefValues;

//    printf("%d", settings->preferencePercentages[0]);

//    int *originalPointer;

    int i;

    SettingsData *simSettings;
    SimulationData *data;
    size_t size = 0;

    settings = readConfiguration(argc, argv);

//    prefValues = calloc((size_t) settings->numOfPreferences, sizeof(int));

    //todo validare le impostazioni

    initRandom((unsigned int) getpid());

    childrenPIDs = calloc((size_t) settings->pop_size, sizeof(int));

#ifdef DEBUG_GESTORE
    printFoundSettings(settings);
#endif


    size += sizeof(SettingsData);
    size += settings->numOfPreferences * sizeof(int);
    size += settings->pop_size * sizeof(StudentData);

    printf("%d %d %d %d\n", sizeof(SettingsData), sizeof(int), sizeof(StudentData), size);

    shmid = createSharedMemory(size);

    simulationData = attachSharedMemory(shmid);

    data = simulationData;

//    printf("%p %p\n", &(simulationData->settings.preferencePercentages),
//           simulationData->settings.preferencePercentages);

    simSettings = &simulationData->settings;
    simSettings->numOfPreferences = settings->numOfPreferences;
    simSettings->pop_size = settings->pop_size;
    simSettings->maxGroupPref = settings->maxGroupPref;
    simSettings->minGroupPref = settings->minGroupPref;
    simSettings->AdE_max = settings->AdE_max;
    simSettings->AdE_min = settings->AdE_min;
    simSettings->settingsCount = settings->settingsCount;
    simSettings->nof_refuse = settings->nof_refuse;
    simSettings->nof_invites = settings->nof_invites;
    simSettings->sim_duration = settings->sim_duration;



//    for (i = 0; i <)

//    originalPointer = settings->preferencePercentages;

    /* Copiamo le impostazioni nella memoria condivisa. */
    // todo risolvere il problema del segfault
    //  avviene perchè non possiamo "creare" un puntatore nella memoria convisa. L'unica soluzione è creare al suo
    //  posto un array sufficientemente grande per contenere un po' di percentuali (immaginiamo 16, tanto usiamo define)
    //  in quel modo non ci sono problemi e la memoria essendo di dati e non di puntatori a dati è a posto.
    //memcpy(&simulationData->settings, settings, sizeof(SettingsData));
    memcpy(simulationData->settings.preferencePercentages, settings->preferencePercentages,
           (size_t) settings->numOfPreferences);

    printf("%d\n", simulationData->settings.preferencePercentages[0]);

//    settings->preferencePercentages = originalPointer;

//    printf(" %d\n", (unsigned int) &(simulationData->settings.preferencePercentages));

    // Gestire prima della logica degli studenti tutta la parte della memoria e il comportamento del gestore
    semid = createSemaphores(settings->pop_size);

    initializeSemaphore(semid, SEMAPHORE_EVERYONE_READY, settings->pop_size, 0);
    initializeSemaphore(semid, SEMAPHORE_CAN_PRINT, 1, 0);

    signal(SIGINT, terminateSimulation);
    signal(SIGTERM, terminateSimulation);
    signal(SIGCHLD, terminateSimulation);
    signal(SIGSEGV, terminateSimulation);

    printf("PID PADRE = %d; Semid = %d; PrefPointer = %p %p\n", getpid(), semid,
           &(simulationData->settings.preferencePercentages), simulationData->settings.preferencePercentages);


//    minPref = simulationData->settings.minGroupPref;
//    maxPref = simulationData->settings.maxGroupPref;
//
//    for (i = 0; i < maxPref - minPref + 1; i++) {
//        prefValues[i] = minPref + i;
//    }

//    for(i=0; i<25;i++)
//    printf("Weighted Rand %d\n", getWeightedRand(3, prefValues, settings->preferencePercentages));


    instantiateChildren();

    // todo waitforzero
    waitForZero(semid, SEMAPHORE_EVERYONE_READY, 0);

    destroySemaphores(semid);

    detachSharedMemory(simulationData);

    destroySharedMemory(shmid);


    // IPC CREAT e EXCL insieme creano la coda se non esiste, ma se esiste lanciano un errore.
//    msgID = msgget(IPC_SIM_KEY, IPC_CREAT | IPC_EXCL | S_IWUSR | S_IRUSR);
//
//    if (msgID == -1) {
//        PRINT_ERROR("Errore durante l'apertura della coda di messaggi\n")
//        PRINT_ERRNO_EXIT(-1)
//    } else {
//        printf("Coda di messaggi aperta correttamente con ID %d\n", msgID);
//    }
//
//    // msgctl serve anche per cancellare la coda.
//    // consigliato ipcs per vedere se ci sono delle cose che non vogliamo
//    // consigliato ipcrm --all=msg per rimuovere tutti i messaggi
//    if (msgctl(msgID, IPC_RMID, NULL)) {
//        fprintf(stderr, "%s:%d: Errore durante la chiusura della coda di messaggi %d\n", __FILE__, __LINE__, msgID);
//        PRINT_ERRNO_EXIT(-1)
//    } else {
//        printf("Coda di messaggi %d chiusa correttamente.\n", msgID);
//    }

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
                printf("Inizializzato processo con PID %d\n\n", childrenPIDs[childrenCounter]);
                simulationData->students[childrenCounter].voto_AdE = getRandomRange(settings->AdE_min,
                                                                                    settings->AdE_max);

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
            printf("%d]\n", settings->preferencePercentages[i]);
        } else {
            printf("%d,", settings->preferencePercentages[i]);
        }
    }
}

#endif
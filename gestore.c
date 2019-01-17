#define DEBUG_GESTORE

#include "gestore.h"

int main(int argc, char *argv[]) {
    SettingsData *settings;
    int semid;
    int *childrenPIDs;
    int childrenCounter = 0;
    char childrenIDString[16] = {0};

    settings = readConfiguration(argc, argv);

    childrenPIDs = calloc((size_t) settings->pop_size, sizeof(int));

#ifdef DEBUG_GESTORE
    printFoundSettings(settings);
#endif

    // Gestire prima della logica degli studenti tutta la parte della memoria e il comportamento del gestore
    semid = createSemaphores(1);
    initializeSemaphore(semid, SEMAPHORE_EVERYONE_READY, settings->pop_size, 0);
    printf("PID PADRE = %d; Semid = %d\n",getpid(), semid);

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
                break;
            }
        }
    }

    // todo waitforzero
    waitForZero(semid, SEMAPHORE_EVERYONE_READY, 0);

    destroySemaphores(semid);




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
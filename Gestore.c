#define DEBUG_GESTORE

#include "Gestore.h"

int *childrenPIDs;
int childrenCounter = 0;

int semid, shmid, msgid;
SettingsData *settings;
SimulationData *simulationData;

int *ade_marks, *so_marks;
float ade_mean = 0, so_mean = 0;

int main(int argc, char *argv[]) {
    /* Leggiamo le impostazioni */
    settings = readConfiguration(argc, argv);

    validateSettings(settings);

    /* Inizializziamo i segnali. Se per caso uno di questi è attivato, la simulazione deve interrompersi. */
    signal(SIGINT, abortSimulationOnSignal);
    signal(SIGTERM, abortSimulationOnSignal);
    signal(SIGCHLD, abortSimulationOnSignal);

    /* Stampiamo le impostazioni che abbiamo letto dal file di configurazione */
    printFoundSettings(settings);

    /* Inizializziamo getrand */
    initRandom((unsigned int) getpid());

    /* Ora che abbiamo le impostazioni, inizializziamo il vettore che contiene i pid dei processi figli */
    childrenPIDs = calloc(settings->pop_size, sizeof(int));

    /*
     * Inizializziamo la memoria condivisa. La sua dimensione è data da:
     *  - la dimensione della struttura che contiene le impostazioni;
     *  - la dimensione del vettore contenente le % delle preferenze
     *  - la dimensione della struttura contenente le informazioni degli studenti per il numero di studenti.
     */
    shmid = createSharedMemory(sizeof(SettingsData) +
                               MAX_PREFERENCES * sizeof(int) +
                               settings->pop_size * sizeof(StudentData));

    /* Otteniamo il puntatore alla memoria condivisa */
    simulationData = attachSharedMemory(shmid);

    /* Copiamo le impostazioni nella memoria condivisa. */
    memcpy(&simulationData->settings, settings, sizeof(SettingsData) + settings->numOfPreferences * sizeof(int));

    /* Inizializziamo semafori e coda di mesaggi. */
    semid = createSemaphores(settings->pop_size);

    initializeSemaphore(semid, SEMAPHORE_EVERYONE_READY, settings->pop_size + 1);
    initializeSemaphore(semid, SEMAPHORE_EVERYONE_ENDED, settings->pop_size);
    initializeSemaphore(semid, SEMAPHORE_MARKS_AVAILABLE, 1);
    initializeSemaphore(semid, SEMAPHORE_CAN_PRINT, 1);

    msgid = createMessageQueue();

    printf("[GESTORE] Genero i processi figlio. Io sono il processo %d\n", getpid());
    instantiateChildren();

    /* Attendiamo che siano tutti pronti. */
    reserveSemaphore(semid, SEMAPHORE_EVERYONE_READY);
    waitForZero(semid, SEMAPHORE_EVERYONE_READY);

    printf("\n\n"
           "############################# GESTORE #############################\n"
           "#####  Inizia la simulazione. Da questo momento sono in sleep #####\n"
           "###################################################################\n"
           "\t\tDurata prevista: %d secondi.\n\n", settings->sim_duration);

    /* Mandiamo in sleep il gestore. Se questa linea è rimossa, l'intera simulazione non può aver luogo. */
    sleep((unsigned int) settings->sim_duration);

    printf("\n\n"
           "################################### GESTORE ####################################\n"
           "####  Tempo scaduto; la simulazione termina. Invio segnale di terminazione  ####\n"
           "################################################################################\n\n");

    /* SIGUSR1 è utilizzato per indicare ai processi figli la fine della simulazione, così da uscire dal while(true) */
    raiseSignalToStudents(SIGUSR1);

    printf("[GESTORE] Attendo che tutti i processi figli terminino la simulazione...\n");

    waitForZero(semid, SEMAPHORE_EVERYONE_ENDED);

    /*
     * Da questo momento i figli non dovrebbero più dipendere l'uno dall'altro per cui non ci sono problemi per quando
     * terminano visto che, inoltre, il segnale farebbe terminare la simulazione all'improvviso quando in realtà
     * non è necessario
     */
    signal(SIGCHLD, SIG_IGN);

    /* Calcoliamo i voti degli studenti. */
    calculateStudentsMarks();

    printf("\n[GESTORE] Voti pronti, avviso gli studenti!\n");
    reserveSemaphore(semid, SEMAPHORE_MARKS_AVAILABLE);

    /* Attendiamo con una wait che tutti i processi finiscano di stampare. da questo momento il programma termina. */
    waitForZombieChildren();

    printf("\n\n"
           "######################### GESTORE #########################\n"
           "####  Tutti gli studenti hanno terminato l'esecuzione  ####\n"
           "###########################################################\n\n");


    printf("Ecco i risultati: \n");

    printSimulationResults();

    freeAllocatedMemory();

    closeIPC();

    return 0;
}

/*
 * Questa funzione calcola i voti di SO, scansionando ogni studente, recuperando il punteggio migliore di AdE e
 * assegnandolo, se necessario aggiungendo la penalità, al campo nella struttura dedicata ai dati degli studenti.
 */
void calculateStudentsMarks() {
    so_marks = calloc(settings->AdE_max + 1, sizeof(int));

    /* Aggiunte per comodità; avremmo dovuto scrivere delle linee molto più lunghe senza questi puntatori */
    StudentData *currentStudent;
    StudentData *groupOwner;
    int i, mark;

    for (i = 0; i < settings->pop_size; i++) {
        mark = 0;
        currentStudent = &simulationData->students[i];
        groupOwner = &simulationData->students[currentStudent->groupOwnerID];

        /* Solo se il capogruppo ha chiuso il gruppo possiamo dare un voto allo studente attualmente considerato. */
        if (groupOwner->groupClosed) {
            /* Il voto massimo è tenuto aggiornato solo dal capogruppo che punta allo studente con il voto ade più alto. */
            mark = simulationData->students[groupOwner->bestMarkID].voto_AdE;
            /* Applichiamo la penalità se lo studente fa parte di un gruppo il cui numero è != dalla sua preferenza */
            if (groupOwner->studentsCount != currentStudent->nofElemsPref) {
                printf("\n[CALCOLO VOTI] Lo studente %d prenderebbe il voto %d, ma ha una preferenza diversa (%d nel gruppo invece di %d)\n",
                       i, mark, groupOwner->studentsCount, currentStudent->nofElemsPref);
                mark -= GROUP_PENALTY;
            } else {
                printf("\n[CALCOLO VOTI] Lo studente %d prende il voto %d, senza penalità.\n", i, mark);
            }

        } else {
            printf("[CALCOLO VOTI] Il capogruppo %d non ha chiuso il gruppo, per cui %d prende 0.\n",
                   currentStudent->groupOwnerID, i);
        }

        /* Incrementiamo il contatore degli studenti che hanno conseguito un certo voto */
        so_marks[mark]++;

        printf("\t\t\t\tAltri %d studenti hanno preso lo stesso voto.\n", so_marks[mark] - 1);

        currentStudent->voto_SO = mark;
        so_mean += (float) mark;
    }

    so_mean /= (float) (settings->pop_size);
}

/*
 * Questa funzione, dato un certo valore, e un numero di caratteri massimo, calcolerà la spaziatura destra e sinistra,
 * in modo da mantenere centrata (se non si supera maxSize) la scritta del numero.
 */
void calculatePadding(int valueToPrint, int maxSize, int *leftPadding, int *rightPadding) {
    char numString[17] = {0};
    int stringLength;

    sprintf(numString, "%d", valueToPrint);
    stringLength = strlen(numString);

    *leftPadding = (maxSize - stringLength) / 2;
    *rightPadding = maxSize - *leftPadding - stringLength;
}

/* Stampiamo i risultati della simulazione, con delle tabelle. I voti non ottenibili di AdE non sono stampati. */
void printSimulationResults() {
    int i;
    int leftPadding[3] = {0}, rightPadding[3] = {0};

    printf("\n"
           "╔══════════╦════════╦══════════╦════════╗\n"
           "║ VOTO ADE ║ NUMERO ║ VOTO  SO ║ NUMERO ║\n");

    for (i = 0; i < settings->AdE_min; i++) {
        if (so_marks[i] != 0) {


            calculatePadding(i, 10, &leftPadding[0], &rightPadding[0]);
            calculatePadding(so_marks[i], 8, &leftPadding[2], &rightPadding[2]);


            printf("╠══════════╬════════╬══════════╬════════╣\n"
                   "║          ║        ║%*c%d%*c║%*c%d%*c║\n",
                   leftPadding[0], ' ', i, rightPadding[0], ' ',
                   leftPadding[2], ' ', so_marks[i], rightPadding[2], ' ');
        }
    }

    for (; i <= settings->AdE_max; i++) {
        if (so_marks[i] != 0 || ade_marks[i - settings->AdE_min] != 0) {
            printf("╠══════════╬════════╬══════════╬════════╣\n");
            calculatePadding(i, 10, &leftPadding[0], &rightPadding[0]);
            calculatePadding(ade_marks[i - settings->AdE_min], 8, &leftPadding[1], &rightPadding[1]);
            calculatePadding(so_marks[i], 8, &leftPadding[2], &rightPadding[2]);


            printf("║%*c%d%*c║%*c%d%*c║%*c%d%*c║%*c%d%*c║\n",
                   leftPadding[0], ' ', i, rightPadding[0], ' ',
                   leftPadding[1], ' ', ade_marks[i - settings->AdE_min], rightPadding[1], ' ',
                   leftPadding[0], ' ', i, rightPadding[0], ' ',
                   leftPadding[2], ' ', so_marks[i], rightPadding[2], ' ');
        }
    }

    printf("╚══════════╩════════╩══════════╩════════╝\n\n");

    /* 0394 è il simbolo unicode per il delta! */
    printf(" ─ La media dei voti di Architettura degli Elaboratori e' %.2f;\n"
           " ─ La media dei voti del Laboratorio di Sistemi Operativi e' %.2f;\n"
           " ─ \u0394 = %.2f\n\n", ade_mean, so_mean, so_mean - ade_mean);
}

void raiseSignalToStudents(int sigid) {
    int i;
    for (i = 0; i < settings->pop_size; i++) {
        printf("[GESTORE] Invio segnale %s al processo con PID %d\n", strsignal(sigid), childrenPIDs[i]);
        kill(childrenPIDs[i], sigid);
        if (errno) {
            PRINT_ERRNO
            errno = 0;
        }
    }
}

void closeIPC() {
    destroySemaphores(semid);

    destroyMessageQueue(msgid);

    detachSharedMemory(simulationData);

    destroySharedMemory(shmid);
}

void abortSimulationOnSignal(int sigid) {
    /* Variabile statica usata per evitare di eseguire più di una volta la terminazione del programma */
    static bool terminating = false;

    if (!terminating) {
        terminating = true;
    } else {
        return;
    }

    printf("\n\n#!#!#! [GESTORE] Ricevuto segnale %d (%s). Termino il programma.\n\n", sigid, strsignal(sigid));

    /* Inviamo a tutti i processi il segnale di terminazione. */
    raiseSignalToStudents(SIGTERM);

    waitForZombieChildren();

    freeAllocatedMemory();

    closeIPC();

    exit(sigid);
}

void freeAllocatedMemory() {
    free(settings);
    free(childrenPIDs);
    free(ade_marks);
    free(so_marks);
}

void waitForZombieChildren() {
    int childrenStatus, childrenPID;

    while (childrenCounter > 0) {
        childrenPID = wait(&childrenStatus);
        switch (errno) {
            /* Errno con errore ECHILD sta a indicare che non ha processi figli il padre. */
            case ECHILD: {
                printf("\n[GESTORE] Finiti i processi Zombie, termino.\n");
                childrenCounter = 0;
                errno = 0;
                break;
            }
            case 0: {
                childrenCounter--;
                printf("#!#!#! [GESTORE] Terminato processo %d con stato %d. Ne mancano %d\n", childrenPID,
                       childrenStatus, childrenCounter);
                break;
            }
                /* Se avviene questo errore, la wait è stata interrotta da un segnale. Riproviamo la wait in questo caso. */
            case EINTR: {
                PRINT_ERRNO
                errno = 0;
                break;
            }
            case EINVAL: {
                /* Non dovrebbe capitare; non stiamo utilizzando delle opzioni strane. */
                PRINT_ERRNO_EXIT(-1)
                break;
            }
            default: {
                /* Se siamo qui qualcosa di grave è successo. */
                PRINT_ERRNO_EXIT(-1)
            }
        }
    }
}

void instantiateChildren() {
    /* Allochiamo spazio per settings->AdE_max - settings->AdE_min + 1 contatori per i voti degli studenti. */
    char childrenIDString[16] = {0};
    int currentStudentAdEMark;

    ade_marks = calloc(settings->AdE_max - settings->AdE_min + 1, sizeof(int));

    for (childrenCounter = 0; childrenCounter < settings->pop_size; childrenCounter++) {
        switch (childrenPIDs[childrenCounter] = fork()) {
            case -1: {
                abortSimulationOnSignal(0);
                PRINT_ERRNO_EXIT(-1)
            }

            case 0: {
                /* Siamo un figlio, avviamo execl */
                sprintf(childrenIDString, "%d", childrenCounter);
                execl(STUDENT_PATH, STUDENT_PATH, childrenIDString);
                PRINT_ERRNO_EXIT(-1)
            }
            default: {
                currentStudentAdEMark = getRandomRange(settings->AdE_min, settings->AdE_max);

                /* Incrementiamo il contatore del voto che ha ottenuto lo studente. */
                ade_marks[currentStudentAdEMark - settings->AdE_min]++;

                simulationData->students[childrenCounter].voto_AdE = currentStudentAdEMark;

                printf("[GESTORE] Inizializzato processo con PID %d. Il suo voto è %d. Altri %d studenti hanno lo stesso voto.\n\n",
                       childrenPIDs[childrenCounter], currentStudentAdEMark,
                       ade_marks[currentStudentAdEMark - settings->AdE_min]);

                ade_mean += currentStudentAdEMark;

                break;
            }
        }
    }

    ade_mean /= (float) (settings->pop_size);
}

#ifdef DEBUG_GESTORE

void printFoundSettings(SettingsData *foundSettings) {
    int i;

    printf("[GESTORE] Trovate le impostazioni:\n"
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
           "\t- prefPerc = [",
           foundSettings->pop_size,
           foundSettings->sim_duration,
           foundSettings->AdE_min,
           foundSettings->AdE_max,
           foundSettings->minGroupPref,
           foundSettings->maxGroupPref,
           foundSettings->nof_invites,
           foundSettings->nof_refuse,
           foundSettings->settingsCount,
           foundSettings->numOfPreferences);

    for (i = 0; i < foundSettings->numOfPreferences; i++) {
        if (i == foundSettings->numOfPreferences - 1) {
            printf("%d]\n\n", foundSettings->preferencePercentages[i]);
        } else {
            printf("%d,", foundSettings->preferencePercentages[i]);
        }
    }
}

#endif

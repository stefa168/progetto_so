#include "Studente.h"

int myID;
int sharedMemoryID, semaphoresID;
SimulationData *simulationData;
StudentData *this;
SettingsData *settings;

void simulationEnd(int sigid);

void simulationAlmostEnded(int sigid);

void abortSimulation(int sigid);

int main(int argc, char *argv[]) {
    semaphoresID = getSemaphoresID();
    sharedMemoryID = getSharedMemoryID();
    simulationData = attachSharedMemory(sharedMemoryID);

    /* Essenziale che innanzitutto salviamo il nostro ID!!! */
    myID = (int) strtol(argv[1], NULL, 10);

    /* Mi collego alla zona di memoria in cui si trovano le informazioni della specifica istanza dello studente */
    this = &simulationData->students[myID];
    /* Recuperiamo anche le impostazioni */
    settings = &simulationData->settings;

    reserveSemaphore(semaphoresID, SEMAPHORE_CAN_PRINT);

    /* Inizializziamo lo studente */
    initializeStudent();

    initializeStudentSemaphore(semaphoresID, myID);

    signal(SIGUSR1, simulationEnd);
    signal(SIGALRM, simulationAlmostEnded);

//    reserveSemaphore(semaphoresID, SEMAPHORE_CAN_PRINT);
    printf("[%d]\n\tSono lo studente %d\n", getpid(), myID);
    printf("\tIl mio voto di ARCH e' %d e preferisco stare con %d studenti in gruppo.\n", this->voto_AdE,
           this->nofElemsPref);
    printf("\tTutto qui.\n\n");
    releaseSemaphore(semaphoresID, SEMAPHORE_CAN_PRINT);

    reserveSemaphore(semaphoresID, SEMAPHORE_EVERYONE_READY);
    waitForZero(semaphoresID, SEMAPHORE_EVERYONE_READY);

    /*
     * Impostiamo un segnale di allarme per quando manca poco tempo in modo che chiudiamo il gruppo se non l'abbiamo
     * già fatto per alzare il voto complessivo e non far prendere uno 0
     */
    alarm(settings->sim_duration - 1);

    while (true) {
        // todo simulazione dello studente
    }

    return 1;
}

void simulationAlmostEnded(int sigid) {
    printf("[%d-%d] Manca poco, meglio chiudere il gruppo.\n", getpid(), myID);

    //todo chiudere effettivamente il gruppo con una zona critica.
    this->groupClosed = true;
    // todo andare in una wait per il resto della simulazione
}

void simulationEnd(int sigid) {
    printf("[%d-%d] Ricevuto segnale di termine simulazione.\n", getpid(), myID);

    /*
     * Attendiamo a questo punto che tutti gli studenti terminino la simulazione
     */
    reserveSemaphore(semaphoresID, SEMAPHORE_EVERYONE_ENDED);
    waitForZero(semaphoresID, SEMAPHORE_EVERYONE_ENDED);

    /*
     * A questo punto la simulazione è terminata. Manca solo che il gestore calcoli i voti.
     */
    waitForZero(semaphoresID, SEMAPHORE_MARKS_AVAILABLE_ID);

    reserveSemaphore(semaphoresID, SEMAPHORE_CAN_PRINT);
    printf("[%d-%d]: Il mio voto è %d\n", getpid(), myID, this->voto_SO);
    releaseSemaphore(semaphoresID, SEMAPHORE_CAN_PRINT);

    detachSharedMemory(simulationData);

    exit(0);

}

void initializeStudent() {
    int i, minPref, maxPref;
    int numOfPreferences = simulationData->settings.numOfPreferences;
    int *prefValues = calloc((size_t) numOfPreferences, sizeof(int));
    int *myPref = &(this->nofElemsPref);

    minPref = simulationData->settings.minGroupPref;
    maxPref = simulationData->settings.maxGroupPref;

    /*
     * Sfortunatamente è necessario generare sul momento i vari valori delle preferenze perchè non è possibile
     * posizionare più di un vettore in una struttura. Avremmo potuto unire i due vettori, ma abbiamo preferito
     * mantenere tutto il più possibile strutturato.
     */
    for (i = 0; i < maxPref - minPref + 1; i++) {
        prefValues[i] = minPref + i;
    }

    initRandom((unsigned int) getpid());


#if PREFERENCE_LOGIC == 0
    *myPref = getWeightedRand(numOfPreferences, prefValues, simulationData->settings.preferencePercentages);
#else
    *myPref = getWeighted(settings->pop_size, myID, numOfPreferences, prefValues, settings->preferencePercentages);
#endif

    /* Inizializziamo il resto dei valori che ogni studente ha per indicare le informazioni di un gruppo. */
    this->studentsCount = 1;
    this->bestMarkID = myID;
    this->groupClosed = false;
    /* Questo valore sarà l'unico aggiornato se lo studente viene invitato in un gruppo. Serve per calcolare il voto! */
    this->groupOwnerID = myID;

    /* Non dimentichiamo di liberare la memoria che è utilizzata durante l'esecuzione del metodo */
    free(prefValues);
}

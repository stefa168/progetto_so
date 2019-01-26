#include "Studente.h"

int myID;
int sharedMemoryID, semaphoresID, messageQueueID;
SimulationData *simulationData;
StudentData *this;
SettingsData *settings;

int main(int argc, char *argv[]) {
    semaphoresID = getSemaphoresID();
    sharedMemoryID = getSharedMemoryID();
    simulationData = attachSharedMemory(sharedMemoryID);
    messageQueueID = getMessageQueue();

    /* Essenziale che innanzitutto salviamo il nostro ID!!! */
    myID = (int) strtol(argv[1], NULL, 10);

    /* Mi collego alla zona di memoria in cui si trovano le informazioni della specifica istanza dello studente */
    this = &simulationData->students[myID];
    /* Recuperiamo anche le impostazioni */
    settings = &simulationData->settings;

    /* Inizializziamo lo studente */
    initializeStudent();

    initializeStudentSemaphore(semaphoresID, myID);

    signal(SIGUSR1, simulationEnd);
    signal(SIGALRM, simulationAlmostEnded);

    /* Zona critica necessaria per informare che lo studente è partito correttamente. */
    reserveSemaphore(semaphoresID, SEMAPHORE_CAN_PRINT);
    printf("[%d]\n\tSono lo studente %d\n", getpid(), myID);
    printf("\tIl mio voto di ARCH e' %d e preferisco stare con %d studenti in gruppo.\n", this->voto_AdE,
           this->nofElemsPref);
    printf("\tTutto qui.\n\n");
    releaseSemaphore(semaphoresID, SEMAPHORE_CAN_PRINT);

    /* Attendiamo che tutti siano pronti prima di partire con la simulazione! */
    reserveSemaphore(semaphoresID, SEMAPHORE_EVERYONE_READY);
    waitForZero(semaphoresID, SEMAPHORE_EVERYONE_READY);

    /*
     * Impostiamo un segnale di allarme per quando manca poco tempo in modo che chiudiamo il gruppo se non l'abbiamo
     * già fatto per alzare il voto complessivo e non far prendere uno 0
     */
    alarm(settings->sim_duration - 1);

    while (true) {
        if (checkForMessages(false)) {
            stopAcceptingInvites();
        }

//        if(!trySendingInvites()) {
//            checkForMessages(true);
//        }
    }

    /* Se siamo qui vuol dire che qualcosa di grave è capitato. Dobbiamo terminare. */
    return 1;
}

bool amIOwner() {
    return this->groupOwnerID == myID;
}

bool amInGroup() {
    return this->status == IN_GROUP;
}

bool wantToCloseGroup() {
    return amIOwner() && this->nofElemsPref == this->studentsCount;//todo chiudo solo se non attendo più risposte
}

int getGroupParticipantCount() {
    return simulationData->students[this->groupOwnerID].studentsCount;
}

void acknowledgeInvite(int fromID) {
    //todo
}

void acknowledgeInviteAccepted(int fromID) {
    //todo
}

void acknowledgeInviteRejected(int fromID) {
    // todo
}

void stopAcceptingInvites() {
    SimMessage message;
    printf("[%d-%d] Smetto di interagire, eccetto per rifiutare inviti.\n", getpid(), myID);

    /*
     * Nel caso in cui utilizziamo getMessage in modalità bloccante, ritornerà sempre true dopo aver ricevuto un messaggio
     * per cui rimarrà qui fino alla fine della simulazione.
     */
    while (true) {
        getMessage(messageQueueID, &message, myID, true);
        switch (message.type) {

            case INVITE: {
                printf("[%d-%d] Ho ricevuto un invito inatteso da %d; devo rifiutarlo.\n", getpid(), myID,
                       message.from);
                sendMessage(messageQueueID, message.from, REJECT_INVITE, true);
                break;
            }
            case ACCEPT_INVITE: {
                printf("[%d-%d] Ho ricevuto un'accettazione inattesa da %d.\n", getpid(), myID, message.from);
                break;
            }
            case REJECT_INVITE: {
                printf("[%d-%d] Ho ricevuto un rifiuto inatteso da %d.\n", getpid(), myID, message.from);
                break;
            }

        }
    }
}

void closeMyGroup() {
    /*
     * Chiudiamo il gruppo solo se:
     *  - siamo da soli e
     *  - non stiamo attendendo conferma di inclusione in un gruppo
     *  - il gruppo non è stato già chiuso.
     */
    if (!amInGroup() && this->status != WAITING_CONFIRM && !this->groupClosed) {
        printf("[%d-%d] Chiudo il gruppo con %d partecipanti su %d\n", getpid(), myID, this->studentsCount,
               this->nofElemsPref);
        this->groupClosed = true;
    }
}

bool checkForMessages(bool hasToWait) {
    SimMessage message;
    bool canStopChecking = false;
    bool alreadyReadAMessage = false;

    /*
     * Cerchiamo di ricevere un messaggio. Il comportamento è il seguente:
     * Continuiamo a cercare messaggi se il processo deve ancora far parte di un gruppo o il capogruppo non l'ha chiuso
     * Inoltre, continuiamo solo se dobbiamo gestire un messaggio. Nel caso in cui siamo obbligati ad attendere per un
     * messaggio, attenderemo solo per il primo, in modo da non rimanere bloccati nel loop nel caso in cui il processo
     * continui a ricevere messaggi.
     */
    while (!canStopChecking && getMessage(messageQueueID, &message, myID, hasToWait && alreadyReadAMessage)) {

        reserveStudentSemaphore(semaphoresID, myID);

        switch (message.type) {

            case INVITE: {
                acknowledgeInvite(message.from);
                break;
            }
            case ACCEPT_INVITE: {
                acknowledgeInviteAccepted(message.from);
                break;
            }
            case REJECT_INVITE: {
                acknowledgeInviteRejected(message.from);
                break;
            }
        }

        if (wantToCloseGroup()) {
            closeMyGroup();
            canStopChecking = true;
        }

        releaseStudentSemaphore(semaphoresID, myID);

        alreadyReadAMessage = true;

    }

    return canStopChecking;

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
    this->status = AVAILABLE;

    /* Non dimentichiamo di liberare la memoria che è utilizzata durante l'esecuzione del metodo */
    free(prefValues);
}

const char *getStatusString() {
    if (amIOwner()) {
        if (this->groupClosed) {
            return "OWNER CHIUSO";
        } else {
            return "OWNER APERTO";
        }
    } else if (amInGroup()) {
        return "IN GRUPPO";
    } else if (this->status == WAITING_CONFIRM) {
        return "IN ATTESA DI CONFERMA";
    } else {
        return "DISPONIBILE";
    }
}

void simulationAlmostEnded(int sigid) {
//    reserveSemaphore(semaphoresID, SEMAPHORE_CAN_PRINT);
    printf("[%d-%d] Manca poco. Il mio stato e' %s. Gruppo: %d studenti su %d.\n",
           getpid(), myID, getStatusString(), getGroupParticipantCount(), this->nofElemsPref);
//    releaseSemaphore(semaphoresID, SEMAPHORE_CAN_PRINT);

    reserveStudentSemaphore(semaphoresID, myID);
    closeMyGroup();
    releaseStudentSemaphore(semaphoresID, myID);

    stopAcceptingInvites();
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
    waitForZero(semaphoresID, SEMAPHORE_MARKS_AVAILABLE);

    reserveSemaphore(semaphoresID, SEMAPHORE_CAN_PRINT);
    printf("[%d-%d]: Il mio voto è %d\n", getpid(), myID, this->voto_SO);
    releaseSemaphore(semaphoresID, SEMAPHORE_CAN_PRINT);

    detachSharedMemory(simulationData);

    exit(0);

}

bool getMessage(int msgqid, SimMessage *msgPointer, int msgType, bool hasToWaitForMessage) {
    int flags = 0;

    if (!hasToWaitForMessage) {
        flags |= IPC_NOWAIT;
    }

    msgrcv(msgqid, msgPointer, sizeof(SimMessage) - sizeof(long), msgType, flags);


    if (errno == ENOMSG) {
//            printf("[%d-%d] Nessun messaggio da ricevere\n", getpid(), myID);
        errno = 0;
        return false;
    } else if (errno == EINTR) {
        printf("[%d-%d] Attesa ricezione messaggio interrotta\n", getpid(), myID);
        errno = 0;
        return false;
    } else if (errno) {
        PRINT_IF_ERRNO_EXIT(-1)
    }

    PRINT_IF_ERRNO_EXIT(-1)

    return true;
}

bool sendMessage(int msgqid, int toStudentID, MessageType type, bool mustSend) {
    SimMessage message;
    int flags = 0;


    if (!mustSend) {
        flags |= IPC_NOWAIT;
    }

    message.type = type;
    message.from = myID;
    message.mType = toStudentID;

    msgsnd(msgqid, &message, sizeof(SimMessage) - sizeof(long), flags);

    if (errno) {
        if (errno == EAGAIN) {
            printf("[%d-%d] Non abbastanza spazio per inviare il messaggio.\n", getpid(), myID);
            errno = 0;
            return false;
        } else if (errno == EINTR) {
            printf("[%d-%d] Attesa invio messaggio interrotta\n", getpid(), myID);
            errno = 0;
            return false;
        } else {
            PRINT_IF_ERRNO_EXIT(-1)
        }
    }

    PRINT_IF_ERRNO_EXIT(-1)

    return true;

}
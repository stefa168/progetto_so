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

        if (!trySendingInvites()) {
            checkForMessages(true);
        }
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
    while (getMessage(messageQueueID, &message, myID, true)) {
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


    /*
     * Se errno è ENOMSG, vuol dire che non ci sono messaggi in coda e noi abbiamo cercato di ricevere un messaggio
     * senza attesa
     */
    if (errno == ENOMSG) {
        /* Linea commentata perchè avviene troppo spesso*/
//            printf("[%d-%d] Nessun messaggio da ricevere\n", getpid(), myID);
        errno = 0;
        return false;
    } else if (errno == EINTR) { /* Improbabile che finiamo qui; probabilmente veniamo interrotti perchè a fine sim. */
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

int invitesAvailable() {
    return settings->nof_invites - this->invitesSent;
}

bool hasToInviteMoreStudents() {
    if (this->status == AVAILABLE || (amIOwner() && !this->groupClosed)) {
        return this->voto_AdE >= MIN_GRADE &&
               this->invitesSent < settings->nof_invites &&
               this->studentsCount + this->invitesPending < this->nofElemsPref;
    } else {
        return false;
    }
}

bool shouldInviteStudent(int id, int iterations) {
    StudentData *candidate = &simulationData->students[id];
    int penalty = 0;
    int bestGroupMark = simulationData->students[this->bestMarkID].voto_AdE;

    if (this->nofElemsPref != candidate->nofElemsPref) {
        penalty = GROUP_PENALTY;
    }

    if (penalty > 0 && iterations <= MAXIMUM_ITERATIONS / 2) {
        return false;
    } else if (iterations == MAXIMUM_ITERATIONS - 1) {
        /*
         * Altrimenti, se siamo all'ultima iterazione vuol dire che abbiamo problemi di invito, quindi probabilmente
         * ci conviene invitare comunque
         */
    } else {
        /* Questo è il caso più particolare:
         * Potrebbe esserci la penalità, ma solo se abbiamo superato un minimo di tentativi.
         * dobbiamo
         */
        if (bestGroupMark < candidate->voto_AdE) {
            bestGroupMark = candidate->voto_AdE;
        }

        /* Supponiamo che:
         *  - bestGroupMark = 27
         *  - settings->AdE_max = 30
         *  - settings->AdE_min = 18
         *  - candidate->voto_AdE = 27
         *
         *  Allora, |48 - 27 - 27| = 6.
         *
         *  Se invece candidate->voto_AdE = 18
         *  |48 - 27 - 18| = 3.
         *
         *  Questa logica permette di spingere gli studenti con voti molto diversi a invitarsi,
         *  in modo da incrementare la media finale.
         *  Lo studente diventa meno rigido nella scelta più sono i tentativi effettuati per cercare un membro.
         */
        // todo verificare se è meglio penalizzare o no per la differenza di preferenza. Probabile penalizzare. (+)
        return abs(settings->AdE_max + settings->AdE_min - bestGroupMark - candidate->voto_AdE) - penalty <=
               2 * iterations;
    }

}

int lookForStudentToInvite() {
    int iterations, i, j;

    for (iterations = 0; iterations < MAXIMUM_ITERATIONS; iterations++) {
        for (i = myID, j = 0; j < settings->pop_size; i++, j++, i %= settings->pop_size) {
            /*
             * Dobbiamo essere sicuri di non cercare di invitare noi stessi o qualcuno dell'altro turno, che chi stiamo
             * cercando di invitare sia disponibile per l'invito e non stia attendendo di poter creare un gruppo e che
             * convenga invitare la persona in funzione di quanto incrementeremo la media finale.
             */
            if (i != myID && i % 2 == myID % 2 &&
                simulationData->students[i].status == AVAILABLE && simulationData->students[i].invitesPending == 0 &&
                shouldInviteStudent(i, iterations)) {
                /*
                 * Blocchiamo il cambiamento di stato dello studente in modo da evitare che qualche altro processo
                 * cerchi di invitare il nostro potenziale membro di gruppo e che non cambi lo stato interno dello s.
                 */
                reserveStudentSemaphore(semaphoresID, i);

                /*
                 * Verifichiamo di nuovo che il suo stato non sia cambiato, altrimenti non possiamo invitarlo!
                 * Se non è cambiato nulla, invitiamolo.
                 */
                if (simulationData->students[i].status == AVAILABLE &&
                    simulationData->students[i].invitesPending == 0) {
                    return i;
                } else {
                    releaseStudentSemaphore(semaphoresID, i);
                }

            }
        }
    }

    /* Non abbiamo trovato nessun candidato. */
    return -1;
}

/**
 * @brief Questa funzione cerca uno studente a cui inviare un invito.
 * @return Restituisce true se è riuscita ad inviare un invito ad uno studente, false altrimenti.
 */
bool trySendingInvites() {
    int foundStudentID;
    bool sentInvite = false;

    if (!hasToInviteMoreStudents()) {
        printf("[%d-%d] Non ho più bisogno di invitare. Gruppo: %d studenti di %d (%d in attesa di conferma). Ho consumato %d inviti.\n",
               getpid(), myID, getGroupParticipantCount(), this->nofElemsPref, this->invitesPending,
               invitesAvailable());
    } else {
        foundStudentID = lookForStudentToInvite();
        if (foundStudentID != -1) {
            sentInvite = sendInvite(foundStudentID);
            releaseStudentSemaphore(semaphoresID, foundStudentID);
            return sentInvite;
        } else {
            printf("[%d,%d] Non sono piu' disponibili studenti da invitare...\n", getpid(), myID);
            return false;
        }
    }
}

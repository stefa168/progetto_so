#include "Studente.h"

int myID;
int sharedMemoryID, semaphoresID, messageQueueID;
SimulationData *simulationData;
StudentData *this;
SettingsData *settings;
int *studentsAlreadyInvited;

int main(int argc, char *argv[]) {
    SimMessage message;
    bool gotMessage;

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
    signal(SIGTERM, abortSimulation);

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
    return this->status == GROUP_OWNER;
}

bool amInGroup() {
    return this->status == IN_GROUP || this->status == GROUP_OWNER;
}

bool wantToCloseGroup() {
    return amIOwner() &&
           !this->groupClosed &&
           this->nofElemsPref == this->studentsCount &&
           this->invitesPending == 0;
}

bool hasAlreadyInvited(int studentID) {
    int i;
    for (i = 0; i < settings->nof_invites; i++) {
        if (studentsAlreadyInvited[i] == studentID) {
            return true;
        }
    }

    return false;
}

int getGroupParticipantCount() {
    return simulationData->students[this->groupOwnerID].studentsCount;
}

bool acceptInvite(int fromID) {
    printf("[%d-%d] Accetto un invito di %d, con %d rifiuti disponibili.\n", getpid(), myID, fromID,
           this->rejectsAvailable);
    this->status = WAITING_CONFIRM;
    sendMessage(messageQueueID, fromID, ACCEPT_INVITE, true);
    return true;
}

bool rejectInvite(int fromID) {
    printf("[%d-%d] Rifiuto un invito di %d. %d rifiuti disponibili.\n", getpid(), myID, fromID,
           this->rejectsAvailable - 1);
    this->rejectsAvailable--;
    sendMessage(messageQueueID, fromID, REJECT_INVITE, true);
    return false;
}

bool genericRejectInvite(int fromID) {
    printf("[%d-%d] Rifiuto un invito da %d. Stato: %s\n", getpid(), myID, fromID, getStatusString());
    sendMessage(messageQueueID, fromID, REJECT_INVITE, true);
    return false;
}

bool acknowledgeInvite(int fromID) {
    bool accepted;
    int penalty = 0, possibleMark, groupMark;
    /* Se non siamo disponibili o stiamo attendendo risposta non possiamo accettare di far parte di un gruppo. */
    if (this->status != AVAILABLE || this->invitesPending > 0) {
        return genericRejectInvite(fromID);
    } else if (this->rejectsAvailable == 0) {
        printf("[%d-%d] Non ho più rifiuti disponibili, per cui accetto per forza l'invito da %d\n", getpid(), myID,
               fromID);
        return acceptInvite(fromID);
    } else {
        if (this->nofElemsPref != simulationData->students[fromID].nofElemsPref) {
            penalty = GROUP_PENALTY;
        }

        groupMark = simulationData->students[getGroupBestID(fromID)].voto_AdE;

        if (this->voto_AdE > groupMark) {
            possibleMark = this->voto_AdE;
        } else {
            possibleMark = groupMark;
        }

        accepted = (possibleMark - penalty) > (settings->AdE_min + settings->AdE_max) / 2;

        if (accepted) {
            return acceptInvite(fromID);
        } else {
            return rejectInvite(fromID);
        }
    }
}

void acknowledgeInviteAccepted(int fromID) {
    StudentData *newMember;

    printf("[%d-%d] Lo studente %d si e' aggiunto al mio gruppo!\n", getpid(), myID, fromID);
    if (this->status != GROUP_OWNER) {
        this->status = GROUP_OWNER;
        printf("[%d-%d] Ora sono il capogruppo.\n", getpid(), myID);
    }

    acknowledgeAnswer(fromID);
    newMember = &simulationData->students[fromID];

    this->studentsCount++;

    if (simulationData->students[this->bestMarkID].voto_AdE < newMember->voto_AdE) {
        this->bestMarkID = fromID;
    }

    newMember->status = IN_GROUP;
    newMember->groupOwnerID = myID;
}

void acknowledgeInviteRejected(int fromID) {
    printf("[%d-%d] Lo studente %d ha rifiutato un mio invito.\n", getpid(), myID, fromID);
    acknowledgeAnswer(fromID);
}

void acknowledgeAnswer(int fromID) {
    printf("[%d-%d] Ho ricevuto una risposta da %d che%s aspettavo.\n", getpid(), myID, fromID,
           hasAlreadyInvited(fromID) ? "" : " NON");
    this->invitesPending--;
    removePendingInvite(fromID);
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
                genericRejectInvite(message.from);
                break;
            }
            case ACCEPT_INVITE: {
                printf("[%d-%d] Ho ricevuto un'accettazione inattesa da %d.\n", getpid(), myID, message.from);
//                acknowledgeInviteAccepted(message.from);
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

//    if (!(amInGroup()) && this->status != WAITING_CONFIRM && !this->groupClosed) {
    if (this->status != IN_GROUP &&
        this->status != WAITING_CONFIRM &&
        !(amIOwner() && this->groupClosed)) {
        printf("[%d-%d] Chiudo il gruppo con %d partecipanti su %d\n", getpid(), myID, this->studentsCount,
               this->nofElemsPref);
        this->groupClosed = true;
    } else {
        printf("[%d-%d] %d, %d\n", getpid(), myID, this->status, !this->groupClosed);
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
    while (!canStopChecking && getMessage(messageQueueID, &message, myID, hasToWait && !alreadyReadAMessage)) {

//        reserveStudentSemaphore(semaphoresID, myID);
        beginCriticalSection(myID);

        switch (message.type) {

            case INVITE: {
                if (acknowledgeInvite(message.from)) {
                    canStopChecking = true; //todo vedere se comprimere ad una sola riga
                }
                printf("[%d-%d] Ho ricevuto un invito da %d. %s.\n", getpid(), myID, message.from,
                       canStopChecking ? "Accettato" : "Rifiutato");
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

//        releaseStudentSemaphore(semaphoresID, myID);
        endCriticalSection(myID);
        alreadyReadAMessage = true;

    }

    return canStopChecking;

}

void initializeStudent() {
    int i, minPref, maxPref;
    int numOfPreferences = simulationData->settings.numOfPreferences;
    int *prefValues = calloc(numOfPreferences, sizeof(int));
    int *myPref = &(this->nofElemsPref);

    studentsAlreadyInvited = calloc(settings->nof_invites, sizeof(int));
    for (i = 0; i < settings->nof_invites; i++) {
        studentsAlreadyInvited[i] = -1;
    }

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
    this->rejectsAvailable = settings->nof_refuse;
    this->invitesPending = 0;
    this->invitesSent = 0;

    /* Non dimentichiamo di liberare la memoria che è utilizzata durante l'esecuzione del metodo */
    free(prefValues);
}

const char *getStatusString() {
    if (amIOwner()) {
        if (this->groupClosed) {
            return "CAPOGRUPPO CHIUSO";
        } else {
            return "CAPOGRUPPO APERTO";
        }
    } else if (this->status == IN_GROUP) {
        return "IN GRUPPO";
    } else if (this->status == WAITING_CONFIRM) {
        return "IN ATTESA";
    } else {
        return "DISPONIBILE";
    }

    /*if (amIOwner()) {
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
    }*/
}

void simulationAlmostEnded(int sigid) {
//    reserveSemaphore(semaphoresID, SEMAPHORE_CAN_PRINT);
    printf("[%d-%d] Manca poco. Il mio stato e' %s. Gruppo: %d studenti su %d.\n",
           getpid(), myID, getStatusString(), getGroupParticipantCount(), this->nofElemsPref);
//    releaseSemaphore(semaphoresID, SEMAPHORE_CAN_PRINT);

//    reserveStudentSemaphore(semaphoresID, myID);
    beginCriticalSection(myID);
    closeMyGroup();
//    releaseStudentSemaphore(semaphoresID, myID);
    endCriticalSection(myID);
    stopAcceptingInvites();
}

void freeMemory() {
    free(studentsAlreadyInvited);
}

void beginCriticalSection(int studentID) {
    reserveStudentSemaphore(semaphoresID, studentID);
    blockSignal(SIGUSR1);
    blockSignal(SIGALRM);
}

void endCriticalSection(int studentID) {
    releaseStudentSemaphore(semaphoresID, studentID);
    unblockSignal(SIGUSR1);
    unblockSignal(SIGALRM);
}

void blockSignal(int sigid) {
    sigset_t sigset;
    sigemptyset(&sigset);

    sigaddset(&sigset, sigid);

    sigprocmask(SIG_BLOCK, &sigset, NULL);

    PRINT_IF_ERRNO_EXIT(-1)
}

void unblockSignal(int sigid) {
    sigset_t sigset;
    sigemptyset(&sigset);

    sigaddset(&sigset, sigid);

    sigprocmask(SIG_UNBLOCK, &sigset, NULL);

    PRINT_IF_ERRNO_EXIT(-1)
}

void simulationEnd(int sigid) {
    printf("[%d-%d] Ricevuto segnale di fine simulazione.\n", getpid(), myID);

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

    freeMemory();

    exit(0);

}

void abortSimulation(int sigid) {
    printf("[%d-%d] Ricevuto segnale di termine improvviso della simulazione.\n", getpid(), myID);

    detachSharedMemory(simulationData);

    freeMemory();

    exit(5);
}

bool getMessage(int msgqid, SimMessage *msgPointer, int msgType, bool hasToWaitForMessage) {
    int flags = 0;

    if (!hasToWaitForMessage) {
        flags |= IPC_NOWAIT;
    }

    msgrcv(msgqid, msgPointer, sizeof(SimMessage) - sizeof(long), msgType + 1, flags);


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
    message.mType = toStudentID + 1;

//    printf("[%d-%d] Invio un messaggio. Dimensione: %ld\n", getpid(), myID, sizeof(SimMessage) - sizeof(long));

    msgsnd(msgqid, &message, sizeof(SimMessage) - sizeof(long), flags);

    if (errno == EAGAIN) {
        printf("[%d-%d] Non abbastanza spazio per inviare il messaggio.\n", getpid(), myID);
        errno = 0;
        return false;
    } else if (errno == EINTR) {
        printf("[%d-%d] Attesa invio messaggio interrotta\n", getpid(), myID);
        errno = 0;
        return false;
    } else if (errno) {
        PRINT_ERRNO
        fprintf(stderr, "Dimensione del messaggio: %ld\n", (sizeof(SimMessage) - sizeof(long)));
        exit(-1);
    }

    return true;

}

int invitesAvailable() {
    return settings->nof_invites - this->invitesSent > 0;
}

bool hasToInviteMoreStudents() {
    if (this->status == AVAILABLE || (amIOwner() && !this->groupClosed)) {
        return this->voto_AdE >= MIN_GRADE &&
               //               this->invitesSent < settings->nof_invites &&
               invitesAvailable() &&
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

    /* Se non abbiamo ancora fatto troppe ricerche, evitiamo di invitare quando potrebbe esserci una scelta migliore. */
    if (penalty > 0 && iterations <= MAXIMUM_ITERATIONS / 2) {
        return false;
    } else if (iterations == MAXIMUM_ITERATIONS - 1) {
        /*
         * Altrimenti, se siamo all'ultima iterazione vuol dire che abbiamo problemi di invito, quindi probabilmente
         * ci conviene invitare comunque
         */
        return true;
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
        return abs(settings->AdE_max + settings->AdE_min - bestGroupMark - candidate->voto_AdE) + penalty <=
               2 * iterations;
    }

}

int getGroupBestID(int studentID) {
    return simulationData->students[simulationData->students[studentID].groupOwnerID].bestMarkID;
}

int lookForStudentToInvite() {
    int iterations, i, j;

    for (iterations = 0; iterations < MAXIMUM_ITERATIONS; iterations++) {
        for (i = myID, j = 0; j < settings->pop_size; i++, j++, i %= settings->pop_size) {
            /*
             * Dobbiamo essere sicuri di non cercare di invitare noi stessi o qualcuno dell'altro turno, oltre a non
             * esser stato già invitato da noi! Chi stiamo cercando di invitare deve essere disponibile per l'invito e
             * non deve star attendendo di poter creare un gruppo e deve convenire invitare la persona in funzione di
             * quanto incrementeremo la media finale.
             */
            if (i != myID && i % 2 == myID % 2 && !hasAlreadyInvited(i) &&
                simulationData->students[i].status == AVAILABLE && simulationData->students[i].invitesPending == 0 &&
                shouldInviteStudent(i, iterations)) {
                /*
                 * Blocchiamo il cambiamento di stato dello studente in modo da evitare che qualche altro processo
                 * cerchi di invitare il nostro potenziale membro di gruppo e che non cambi lo stato interno dello s.
                 */
//                reserveStudentSemaphore(semaphoresID, i);
                beginCriticalSection(i);

                /*
                 * Verifichiamo di nuovo che il suo stato non sia cambiato, altrimenti non possiamo invitarlo!
                 * Se non è cambiato nulla, invitiamolo.
                 */
                if (simulationData->students[i].status == AVAILABLE &&
                    simulationData->students[i].invitesPending == 0) {
                    return i;
                } else {
//                    releaseStudentSemaphore(semaphoresID, i);
                    endCriticalSection(i);
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
    bool inviteSent;

    if (!hasToInviteMoreStudents()) {
        /* Commentato perchè avviene troppo spesso */
        /*printf("[%d-%d] Non ho più bisogno di invitare. Gruppo: %d studenti di %d (%d in attesa di conferma). Ho consumato %d inviti.\n",
               getpid(), myID, getGroupParticipantCount(), this->nofElemsPref, this->invitesPending,
               invitesAvailable());*/
        return false;
    } else {
        foundStudentID = lookForStudentToInvite();
        if (foundStudentID != -1) {
            inviteSent = sendInvite(foundStudentID);
//            releaseStudentSemaphore(semaphoresID, foundStudentID);
            endCriticalSection(foundStudentID);
            return inviteSent;
        } else {
//            printf("[%d,%d] Non sono piu' disponibili studenti da invitare...\n", getpid(), myID);
            return false;
        }
    }
}


void addPendingInvite(int studentID) {
    int i;
    for (i = 0; i < settings->nof_invites; i++) {
        if (studentsAlreadyInvited[i] == -1) {
            studentsAlreadyInvited[i] = studentID;
            return;
        }
    }

    PRINT_ERROR_EXIT("Aggiunta invito oltre il limite", -1)
}

void removePendingInvite(int studentID) {
    int i;
    for (i = 0; i < settings->nof_invites; i++) {
        if (studentsAlreadyInvited[i] == studentID) {
            studentsAlreadyInvited[i] = -1;
            return;
        }
    }

    PRINT_ERROR_EXIT("Rimozione invito oltre il limite", -1)
}

bool sendInvite(int toStudentID) {
    printf("[%d-%d] Invio un invito a %d. Gruppo: %d studenti di %d (%d in attesa di conferma) Voto gruppo %d.\n",
           getpid(), myID, toStudentID, getGroupParticipantCount(), this->nofElemsPref, this->invitesPending + 1,
           simulationData->students[getGroupBestID(myID)].voto_AdE);

    this->invitesSent++;
    this->invitesPending++;
    addPendingInvite(toStudentID);
    return sendMessage(messageQueueID, toStudentID, INVITE, false);
}
#ifndef PROGETTO_SO_STUDENTE_H
#define PROGETTO_SO_STUDENTE_H

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include "utilities/types.h"
#include "utilities/ipc_utils.h"
#include "utilities/utils.h"

/*
 * 0: La preferenza dello studente è scelta a caso, utilizzando come % quelle inserite nelle impostazioni
 * 1: La preferenza dello studente è definita in funzione del numero di matricola e delle % che dividono gli studenti
 *    per "gruppi" di preferenze.
 *
 * Sono state implementate entrambe le versioni data la presenza di un'inconsistenza tra il testo che descrive il
 * progetto e le note a piè di pagina che indicano il contrario.
 */
#define PREFERENCE_LOGIC 1

#define MIN_GRADE 21
#define MAXIMUM_ITERATIONS 8

void initializeStudent();

bool amIOwner();

bool amInGroup();

bool wantToCloseGroup();

bool hasAlreadyInvited(int studentID);

bool getMessage(int msgqid, SimMessage *msgPointer, int msgType, bool hasToWaitForMessage);

bool sendMessage(int msgqid, int toStudentID, MessageType type, bool mustSend);

void simulationEnd(int sigid);

void simulationAlmostEnded(int sigid);

void abortSimulation(int sigid);

void stopAcceptingInvites();

bool checkForMessages(bool hasToWait);

bool trySendingInvites();

void addPendingInvite(int studentID);

void removePendingInvite(int studentID);

bool sendInvite(int toStudentID);

int getGroupParticipantCount();

int getGroupBestID(int studentID);

bool acknowledgeInvite(int fromID);

void acknowledgeInviteAccepted(int fromID);

bool acceptInvite(int fromID);

bool rejectInvite(int fromID);

void acknowledgeInviteRejected(int fromID);

void acknowledgeAnswer(int fromID);

const char *getStatusString();

void freeMemory();

void beginCriticalSection(int studentID);

void endCriticalSection(int studentID);

void blockSignal(int sigid);

void unblockSignal(int sigid);

#endif

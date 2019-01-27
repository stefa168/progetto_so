#ifndef PROGETTO_SO_TYPES_H
#define PROGETTO_SO_TYPES_H

#include <stdbool.h>

#define MAX_PREFERENCES 16

typedef enum {
    INVITE,
    ACCEPT_INVITE,
    REJECT_INVITE
} MessageType;

typedef struct {
    long mType;
    int from;
    MessageType type;
} SimMessage;

/*
 * I vari semafori che utilizziamo durante l'esecuzione della simulazione.
 * EVERYONE READY serve per essere sicuri che tutti gli studenti siano pronti per iniziare la simulazione.
 * EVERYONE_ENDED serve alla fine della simulazione per attendere che tutti gli studenti abbiano terminato il lavoro.
 * MARKS_AVAILABLE serve per segnalare ai figli che i voti sono pronti per essere stampati
 * CAN_PRINT serve per evitare interleaving tra i processi che stampano.
 * STUDENT serve ad indicare che vi è un'interazione con un semaforo di uno specifico studente.
 */
typedef enum {
    SEMAPHORE_EVERYONE_READY,
    SEMAPHORE_EVERYONE_ENDED,
    SEMAPHORE_MARKS_AVAILABLE,
    SEMAPHORE_CAN_PRINT,
    SEMAPHORE_STUDENT
} SemaphoreType;

typedef struct {
    int settingsCount;

    int pop_size;
    int sim_duration;

    int AdE_min;
    int AdE_max;

    int minGroupPref;
    int maxGroupPref;

    int nof_invites;
    int nof_refuse;

    int numOfPreferences;
    int preferencePercentages[MAX_PREFERENCES];
} SettingsData;

typedef enum {
    AVAILABLE,
    IN_GROUP,
    WAITING_CONFIRM,
    GROUP_OWNER
} StudentStatus;

typedef struct {
    int groupOwnerID;
    int bestMarkID;
    int studentsCount;
    int voto_AdE;
    int voto_SO;
    int nofElemsPref;
    int invitesSent;
    int invitesPending;
    int rejectsAvailable;
    StudentStatus status;
    bool groupClosed;
} StudentData;

typedef struct {
    SettingsData settings;
    StudentData students[];
} SimulationData;

#endif
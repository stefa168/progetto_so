#ifndef PROGETTO_SO_TYPES_H
#define PROGETTO_SO_TYPES_H

#include <stdbool.h>

typedef enum {
    INVITE,
    ACCEPT,
    REJECT
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
 * STUDENT serve ad indicare che vi è un'interazione con un semaforo di uno specifico studente.
 */
typedef enum {
    SEMAPHORE_EVERYONE_READY,
    SEMAPHORE_EVERYONE_ENDED,
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
    int preferencePercentages[];
} SettingsData;

typedef struct {
    int groupOwnerID;
    int bestMarkID;
    int studentsCount;
    int voto_AdE;
    int nofElemsPref;
    bool groupClosed;
} StudentData;

typedef struct {
    SettingsData settings;
    StudentData students[];
} SimulationData;

#endif
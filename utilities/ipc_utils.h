#ifndef PROGETTO_SO_IPC_UTILS_H
#define PROGETTO_SO_IPC_UTILS_H

//#define IPC_MEM_DEBUG
//#define IPC_SEM_DEBUG
//#define IPC_MSG_DEBUG

#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <unistd.h>
#include "types.h"
#include "macros.h"

/**
 * @def Chiave univoca per accedere ai dati IPC della simulazione.
 */
#define IPC_SIM_KEY 34197
#define GLOBAL_SEMAPHORE_COUNT 4

/*
 * Semaforo utilizzato per far attendere a tutti i processi figli e al processo padre che tutti siano pronti prima
 * di iniziare la simulazione
 */
#define SEMAPHORE_EVERYONE_READY_ID 0

/*
 * Semaforo utilizzato per attendere che tutti gli studenti terminino la simulazione.
 */
#define SEMAPHORE_EVERYONE_ENDED_ID 1

/*
 * Semaforo utilizzato per avvisare gli studenti della disponibilità dei voti.
 */
#define SEMAPHORE_MARKS_AVAILABLE_ID 2

/*
 * Semaforo utilizzato per evitare possibile interleaving delle stampe di processi differenti.
 */
#define SEMAPHORE_CAN_PRINT_ID 3

/*
 * Questo valore indica la base con la quale iniziano i semafori dei singoli studenti. Servono per evitare che gli altri
 * studenti possano interagire con uno studente che potrebbe cambiare stato (e quindi entrare in un gruppo o chiuderlo)
 */
#define SEMAPHORE_STUDENT_BASE_ID 4

int createMessageQueue();

int getMessageQueue();

void destroyMessageQueue(int id);

/**********************************************************************************************************************/

/**
 * @brief Richiedi i semafori al SO
 * @param count Quanti semafori servono
 * @return L'ID dell'aggregato di semafori ottenuto.
 */
int createSemaphores(int count);


/**
 * @brief Inizializza il semaforo dello studente studentID.
 * @param id ID del vettore di semafori
 * @param studentID ID dello studente il cui semaforo deve essere inizializzato.
 */
void initializeStudentSemaphore(int id, int studentID);

/**
 * @brief Inizializza uno specifico semaforo con 'count' risorse.
 * @param id L'ID dell'aggregato di semafori
 * @param which Quale semaforo è da modificare.
 *        Se è passato SEMAPHORE_STUDENT, ci si aspetta che il campo studentID contenga l'ID dello studente.
 *        Altrimenti, il campo è semplicemente ignorato.
 * @param count Con quante risorse deve essere inizializzato il semaforo.
 */
void initializeSemaphore(int id, SemaphoreType which, int count);

/**
 * @return ID dell'aggregato di semafori legato alla chiave IPC_SIM_KEY
 */
int getSemaphoresID();

/**
 * @brief Effettua un'operazione di riserva su un semaforo.
 *        Se usato con i semafori degli studenti, lancia un errore. Usare il metodo apposito.
 * @param id ID dell'aggregato di semafori
 * @param which Quale semaforo riservare
 */
void reserveSemaphore(int id, SemaphoreType which);

/**
 * @brief Effettua un'operazione di rilasio su un semaforo.
 *        Se usato con i semafori degli studenti, lancia un errore. Usare il metodo apposito.
 * @param id ID dell'aggregato di semafori
 * @param which Quale semaforo rilasciare
 */
void releaseSemaphore(int id, SemaphoreType which);

/**
 * @brief Effettua un'operazione di riserva su un semaforo studente.
 * @param id ID dell'aggregato di semafori
 * @param studentID ID dello studente il cui semaforo è da riservare.
 */
void reserveStudentSemaphore(int id, int studentID);

/**
 * @brief Effettua un'operazione di rilascio su un semaforo studente.
 * @param id ID dell'aggregato di semafori
 * @param studentID ID dello studente il cui semaforo è da rilasciare.
 */
void releaseStudentSemaphore(int id, int studentID);

/**
 * @brief Blocca un processo e fallo attendere fino a quando un semaforo specificato non è a 0.
 * @param id ID dell'aggregato di semafori
 * @param which Per quale semaforo attendere
 * @param studentID Opzionale //TODO
 */
void waitForZero(int id, SemaphoreType which);

/**
 * @brief Cancella l'aggregato di semafori.
 * @param id L'ID dell'aggregato di semafori.
 */
void destroySemaphores(int id);

/**********************************************************************************************************************/

/**
 * @brief Richiede l'allocazione al SO di una zona di memoria collegata ad IPC_SEM_KEY di dimensione size byte.
 * @param size Quanti byte allocare
 * @return L'ID della zona di memoria
 */
int createSharedMemory(size_t size);

/**
 * @brief Restituisce l'ID di una zona di memoria già allocata e collegata ad IPC_SEM_KEY
 * @return L'ID della zona di memoria
 */
int getSharedMemoryID();

/**
 * @brief Crea un puntatore per collegare un processo alla memoria condivisa.
 * @param id L'ID della zona di memoria condivisa
 * @return Puntatore alla zona di memoria condivisa
 */
SimulationData *attachSharedMemory(int id);

/**
 * @brief Scollega la memoria condivisa dal processo
 * @param addr Il puntatore da scollegare
 */
void detachSharedMemory(void *addr);

/**
 * @brief Fa distruggere al SO la zona di memoria condivisa collegata all'ID id
 * @param id ID della zona di memoria condivisa da distruggere.
 */
void destroySharedMemory(int id);

#endif


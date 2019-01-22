#ifndef PROGETTO_SO_IPC_UTILS_H
#define PROGETTO_SO_IPC_UTILS_H

//#define IPC_MEM_DEBUG
//#define IPC_SEM_DEBUG

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
#define GLOBAL_SEMAPHORE_COUNT 0

#define SEMAPHORE_EVERYONE_READY_ID 0
#define SEMAPHORE_EVERYONE_ENDED_ID 1
#define SEMAPHORE_CAN_PRINT_ID 4
#define SEMAPHORE_STUDENT_BASE_ID 5

int createMessageQueue();

int getMessageQueue(int key);

void destroyMessageQueue(int id);

/**********************************************************************************************************************/

/**
 * @brief Richiedi i semafori al SO
 * @param count Quanti semafori servono
 * @return L'ID dell'aggregato di semafori ottenuto.
 */
int createSemaphores(int count);

/**
 * @brief Inizializza i semafori corrispondenti all'ID passato.
 *        Dobbiamo dedicare a ogni studente un semaforo per alcune operazioni importanti.
 * @param id ID dell'aggregato di semafori
 * @param numOfStudents Quanti studenti abbiamo nella simulazione.
 */
void initializeSemaphores(int id, int numOfStudents);

/**
 * @brief Inizializza uno specifico semaforo con 'count' risorse.
 * @param id L'ID dell'aggregato di semafori
 * @param which Quale semaforo è da modificare.
 *        Se è passato SEMAPHORE_STUDENT, ci si aspetta che il campo studentID contenga l'ID dello studente.
 *        Altrimenti, il campo è semplicemente ignorato.
 * @param count Con quante risorse deve essere inizializzato il semaforo.
 * @param studentID Necessario solo se 'which' è SEMAPHORE_STUDENT.
 *                  In questo caso, DEVE essere passato l'ID dello studente.
 */
void initializeSemaphore(int id, SemaphoreType which, int count, int studentID);

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

///**
// * @brief Questa funzione è bloccante e fa attendere a un processo che un semaforo raggiunga un certo valore value.
// *        Se which è SEMAPHORE_STUDENT, è necessario passarne l'ID, altrimenti il campo è ignorato.
// * @param id ID dell'aggregato di semafori
// * @param which Per quale semaforo attendere
// * @param value Fino a che valore attendere
// * @param studentID ID dello studente il cui semaforo deve raggiungere value per sbloccare il processo.
// */
//void waitForSemaphore(int id, SemaphoreType which, int value, int studentID);

/**
 * @brief Blocca un processo e fallo attendere fino a quando un semaforo specificato non è a 0.
 * @param id ID dell'aggregato di semafori
 * @param which Per quale semaforo attendere
 * @param studentID Opzionale //TODO
 */
void waitForZero(int id, SemaphoreType which, int studentID);

/**
 * @brief Cancella l'aggregato di semafori.
 * @param id L'ID dell'aggregato di semafori.
 */
void destroySemaphores(int id);

/**********************************************************************************************************************/

int createSharedMemory(size_t size);

int getSharedMemoryID();

SimulationData *attachSharedMemory(int id);

void detachSharedMemory(void *addr);

void destroySharedMemory(int id);

#endif


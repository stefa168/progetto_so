#ifndef ESERCIZI_SO_IPC_UTILS_H
#define ESERCIZI_SO_IPC_UTILS_H

/**
 * @def Chiave univoca per accedere ai dati IPC della simulazione.
 */
#define IPC_OWN_KEY 34197

//messaggi
int createMessageQueue();

int getMessageQueue(int key);

/*
 * I vari semafori che utilizziamo durante l'esecuzione della simulazione.
 * EVERYONE READY serve per essere sicuri che tutti gli studenti siano pronti per iniziare la simulazione.
 * EVERYONE_ENDED serve alla fine della simulazione per attendere che tutti gli studenti abbiano terminato il lavoro.
 * STUDENT serve ad indicare che vi è un'interazione con un semaforo di uno specifico studente.
 */
typedef enum SemaphoreType {
    SEMAPHORE_EVERYONE_READY,
    SEMAPHORE_EVERYONE_ENDED,
    SEMAPHORE_STUDENT
} SemaphoreType;

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
 * @brief Cancella l'aggregato di semafori.
 * @param id L'ID dell'aggregato di semafori.
 */
void deleteSemaphores(int id);

// dati

#endif
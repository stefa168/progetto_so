#ifndef ESERCIZI_SO_IPC_UTILS_H
#define ESERCIZI_SO_IPC_UTILS_H

/**
 * @def Chiave univoca per accedere ai dati IPC della simulazione.
 */
#define IPC_SIM_KEY 34197

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
 * @brief Questa funzione è bloccante e fa attendere a un processo che un semaforo raggiunga un certo valore value.
 *        Se which è SEMAPHORE_STUDENT, è necessario passarne l'ID, altrimenti il campo è ignorato.
 * @param id ID dell'aggregato di semafori
 * @param which Per quale semaforo attendere
 * @param value Fino a che valore attendere
 * @param studentID ID dello studente il cui semaforo deve raggiungere value per sbloccare il processo.
 */
void waitForSemaphore(int id, SemaphoreType which, int value, int studentID);

/**
 * @brief Cancella l'aggregato di semafori.
 * @param id L'ID dell'aggregato di semafori.
 */
void deleteSemaphores(int id);

// dati

#endif
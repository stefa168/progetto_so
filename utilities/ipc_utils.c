#include "ipc_utils.h"

/**********************************************************************************************************************/

int createSemaphores(int count) {
    int id = semget(IPC_SIM_KEY, count + GLOBAL_SEMAPHORE_COUNT, IPC_CREAT | IPC_EXCL | 0666);

    PRINT_IF_ERRNO_EXIT(-1)

#ifdef IPC_SEM_DEBUG
    printf("[SEMAFORI]: Creato il vettore di semafori con chiave %d\n", IPC_SIM_KEY);
#endif

    return id;
}

void initializeSemaphore(int id, SemaphoreType which, int count, int studentID) {
    switch (which) {

        case SEMAPHORE_EVERYONE_READY: {
            semctl(id, SEMAPHORE_EVERYONE_READY_ID, SETVAL, count);
            PRINT_IF_ERRNO_EXIT(-1)
#ifdef IPC_SEM_DEBUG
            printf("[SEMAFORI]: Semaforo EVERYONE_READY inizializzato a valore %d\n", count);
#endif
            break;
        }
        case SEMAPHORE_EVERYONE_ENDED: {
            PRINT_ERRNO_EXIT(-1)
            break;
        }
        case SEMAPHORE_CAN_PRINT: {
            semctl(id, SEMAPHORE_CAN_PRINT_ID, SETVAL, count);
            PRINT_IF_ERRNO_EXIT(-1)
#ifdef IPC_SEM_DEBUG
            printf("[SEMAFORI]: Semaforo CAN_PRINT inizializzato a valore %d\n", count);
#endif
            break;
        }

        case SEMAPHORE_STUDENT: {
            PRINT_ERRNO_EXIT(-1)
            break;
        }

        default: PRINT_ERRNO_EXIT(-1)
    }
}

void destroySemaphores(int id) {
    semctl(id, 0, IPC_RMID);
    if (errno) {
        PRINT_ERROR("!!! [SEMAFORI] !!! : Distruzione del vettore di semafori fallita.\n")
        errno = 0;
    } else {
#ifdef IPC_SEM_DEBUG
        printf("[SEMAFORI]: Vettore di semafori correttamente distrutto.\n");
#endif
    }
}

void waitForZero(int id, SemaphoreType which, int studentID) {
    struct sembuf myOp;
    int semaphoreID = -1;
    switch (which) {

        case SEMAPHORE_EVERYONE_READY: {
            semaphoreID = SEMAPHORE_EVERYONE_READY_ID;
            break;
        }
        case SEMAPHORE_EVERYONE_ENDED: {
            semaphoreID = SEMAPHORE_EVERYONE_ENDED_ID;
            break;
        }
        default: PRINT_ERRNO_EXIT(-1)
    }

    myOp.sem_num = (unsigned short) semaphoreID;
    myOp.sem_op = 0;
    myOp.sem_flg = 0;

#ifdef IPC_SEM_DEBUG
    printf("[SEMAFORI] %d: Inizio attesa a 0 del semaforo %d\n", getpid(), semaphoreID);
#endif

    semop(id, &myOp, 1);

    PRINT_IF_ERRNO_EXIT(-1)

#ifdef IPC_SEM_DEBUG
    printf("[SEMAFORI] %d: Finita attesa a 0 del semaforo %d\n", getpid(), semaphoreID);
#endif
}

int getSemaphoresID() {
    int id = semget(IPC_SIM_KEY, 0, 0666);
    PRINT_IF_ERRNO_EXIT(-1)
    return id;
}

void reserveSemaphore(int id, SemaphoreType which) {
    struct sembuf myOp;
    int semaphoreID = -1;
    switch (which) {

        case SEMAPHORE_EVERYONE_READY: {
            semaphoreID = SEMAPHORE_EVERYONE_READY_ID;
            break;
        }
        case SEMAPHORE_EVERYONE_ENDED: {
            PRINT_ERRNO_EXIT(-1)
            break;
        }

        case SEMAPHORE_CAN_PRINT: {
            semaphoreID = SEMAPHORE_CAN_PRINT_ID;
            break;
        }
        default: PRINT_ERRNO_EXIT(-1)
    }

    myOp.sem_num = (unsigned short) semaphoreID;
    myOp.sem_op = -1;
    myOp.sem_flg = 0;

#ifdef IPC_SEM_DEBUG
    printf("[SEMAFORI] %d: Tentativo di riserva del semaforo %d\n", getpid(), semaphoreID);
#endif
    semop(id, &myOp, 1);

    PRINT_IF_ERRNO_EXIT(-1)
#ifdef IPC_SEM_DEBUG
    printf("[SEMAFORI] %d: Riserva del semaforo %d riuscita\n", getpid(), semaphoreID);
#endif
}


void releaseSemaphore(int id, SemaphoreType which) {
    struct sembuf myOp;
    int semaphoreID = -1;
    switch (which) {
        case SEMAPHORE_CAN_PRINT: {
            semaphoreID = SEMAPHORE_CAN_PRINT_ID;
            break;
        }
        default: PRINT_ERRNO_EXIT(-1)
    }

    myOp.sem_num = (unsigned short) semaphoreID;
    myOp.sem_op = +1;
    myOp.sem_flg = 0;

#ifdef IPC_SEM_DEBUG
    printf("[SEMAFORI] %d: Tentativo di rilascio del semaforo %d\n", getpid(), semaphoreID);
#endif
    semop(id, &myOp, 1);

    PRINT_IF_ERRNO_EXIT(-1)
#ifdef IPC_SEM_DEBUG
    printf("[SEMAFORI] %d: Rilascio del semaforo %d riuscito\n", getpid(), semaphoreID);
#endif
}

/**********************************************************************************************************************/

int createSharedMemory(size_t size) {
    int id = shmget(IPC_SIM_KEY, size, IPC_CREAT | IPC_EXCL | 0666);

    PRINT_IF_ERRNO_EXIT(-1)

#ifdef IPC_MEM_DEBUG
    printf("[MEMORIA CONDIVISA] %d: Creata con successo con la chiave %d\n", getpid(), IPC_SIM_KEY);
#endif

    return id;
}

int getSharedMemoryID() {
    int id = shmget(IPC_SIM_KEY, 0, 0666);

    PRINT_IF_ERRNO_EXIT(-1)

#ifdef IPC_MEM_DEBUG
    printf("[MEMORIA CONDIVISA] %d: Ottenuto l'ID %d associato alla chiave %d\n", getpid(), id, IPC_SIM_KEY);
#endif

    return id;
}

SimulationData *attachSharedMemory(int id) {
    SimulationData *addr = shmat(id, NULL, 0666);

    PRINT_IF_ERRNO_EXIT(-1)
#ifdef IPC_MEM_DEBUG
    printf("[MEMORIA CONDIVISA] %d: Collegamento alla zona associata alla chiave %d riuscita\n", getpid(), IPC_SIM_KEY);
#endif

    return addr;
}

void detachSharedMemory(void *addr) {
    shmdt(addr);

    PRINT_IF_ERRNO_EXIT(-1)
#ifdef IPC_MEM_DEBUG
    printf("[MEMORIA CONDIVISA] %d: Zona associata alla chiave %d scollegata\n", getpid(), IPC_SIM_KEY);
#endif
}

void destroySharedMemory(int id) {
    shmctl(id, IPC_RMID, NULL);
    // todo debug
    PRINT_IF_ERRNO_EXIT(-1)

#ifdef IPC_MEM_DEBUG
    printf("[MEMORIA CONDIVISA] %d: Zona associata alla chiave %d distrutta\n", getpid(), IPC_SIM_KEY);
#endif
}








#include "ipc_utils.h"

/**********************************************************************************************************************/

int createSemaphores(int count) {
    int id = semget(IPC_SIM_KEY, count + GLOBAL_SEMAPHORE_COUNT, IPC_CREAT | IPC_EXCL | 0660);

    PRINT_IF_ERRNO_EXIT(-1)

    return id;
}

void initializeSemaphore(int id, SemaphoreType which, int count, int studentID) {
    switch (which) {

        case SEMAPHORE_EVERYONE_READY: {
            semctl(id, 0, SETVAL, count);
            PRINT_IF_ERRNO_EXIT(-1)
            printf("[SEMAFORI]: Semaforo EVERYONE_READY inizializzato a valore %d\n", count);
            break;
        }
        case SEMAPHORE_EVERYONE_ENDED: {
            PRINT_ERRNO_EXIT(-1)
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
    PRINT_IF_ERRNO_EXIT(-1)
    printf("[SEMAFORI]: Aggregato di semafori correttamente distrutto.\n");
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

    printf("[SEMAFORI] %d: Inizio attesa a 0 del semaforo %d\n", getpid(), semaphoreID);
    semop(id, &myOp, 1);

    PRINT_IF_ERRNO_EXIT(-1)
    
    printf("[SEMAFORI] %d: Finita attesa a 0 del semaforo %d\n", getpid(), semaphoreID);
}

int getSemaphoresID() {
    int id = semget(IPC_SIM_KEY, 0, 0660);
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
        default: PRINT_ERRNO_EXIT(-1)
    }

    myOp.sem_num = (unsigned short) semaphoreID;
    myOp.sem_op = -1;
    myOp.sem_flg = 0;

    printf("[SEMAFORI] %d: Tentativo di riserva del semaforo %d\n", getpid(), semaphoreID);
    semop(id, &myOp, 1);

    PRINT_IF_ERRNO_EXIT(-1)
    printf("[SEMAFORI] %d: Riserva del semaforo %d riuscita\n", getpid(), semaphoreID);
}



/**********************************************************************************************************************/


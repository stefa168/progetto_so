

#include <stdio.h>
#include "utilities/types.h"
#include "utilities/ipc_utils.h"

int main(int argc, char *argv[]) {
    int semid = getSemaphoresID();

    printf("PID %d: Sono lo studente %s\n", getpid(), argv[1]);

    reserveSemaphore(semid, SEMAPHORE_EVERYONE_READY);
    return 0;
}

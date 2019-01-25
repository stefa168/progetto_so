#ifndef PROGETTO_SO_MACROS_H
#define PROGETTO_SO_MACROS_H

/*
 * Queste due inclusioni sono essenziali per permettere a qualsiasi file che usa queste macro di funzionare
 * senza dare errori.
 */
#include <errno.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Grazie Professore */
#define PRINT_ERRNO fprintf(stderr, "[%d] %s:%d: Errore #%3d \"%s\"\n", getpid(), __FILE__, __LINE__, errno, strerror(errno));
#define PRINT_ERRNO_EXIT(e) {PRINT_ERRNO exit(e);}
#define PRINT_IF_ERRNO_EXIT(e) if(errno) {PRINT_ERRNO_EXIT(e)}
#define PRINT_ERROR(x) fprintf(stderr, "[%d] %s:%d: %s",getpid(),  __FILE__, __LINE__, x);
#define PRINT_ERROR_EXIT(x, e) PRINT_ERROR(x) exit(e);

#define GROUP_PENALTY 3

#endif

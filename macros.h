#ifndef ESERCIZI_SO_MACROS_H
#define ESERCIZI_SO_MACROS_H

/* Grazie Professore */
#define PRINT_ERRNO fprintf(stderr, "%s:%d: Errore #%3d \"%s\"\n", __FILE__, __LINE__, errno, strerror(errno));
#define PRINT_ERRNO_EXIT(e) {PRINT_ERRNO exit(e);}
#define PRINT_IF_ERRNO_EXIT(e) if(errno) {PRINT_ERRNO_EXIT(e)}
#define PRINT_ERROR(x) fprintf(stderr, "%s:%d: %s", __FILE__, __LINE__, x);
#define PRINT_ERROR_EXIT(x, e) PRINT_ERROR(x) exit(e);

#endif

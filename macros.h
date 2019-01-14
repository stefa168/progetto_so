#ifndef ESERCIZI_SO_MACROS_H
#define ESERCIZI_SO_MACROS_H

/* Grazie Professore */
#define PRINT_ERRNO fprintf(stderr, "%s:%d: Errore #%3d \"%s\"\n", __FILE__, __LINE__, errno, strerror(errno));
#define PRINT_ERROR(x) fprintf(stderr, "%s", x);

#endif

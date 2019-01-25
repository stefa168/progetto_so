#ifndef PROGETTO_SO_STUDENTE_H
#define PROGETTO_SO_STUDENTE_H

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include "utilities/types.h"
#include "utilities/ipc_utils.h"
#include "utilities/utils.h"

/*
 * 0: La preferenza dello studente è scelta a caso, utilizzando come % quelle inserite nelle impostazioni
 * 1: La preferenza dello studente è definita in funzione del numero di matricola e delle % che dividono gli studenti
 *    per "gruppi" di preferenze.
 *
 * Sono state implementate entrambe le versioni data la presenza di un'inconsistenza tra il testo che descrive il
 * progetto e le note a piè di pagina che indicano il contrario.
 */
#define PREFERENCE_LOGIC 1

void initializeStudent();

#endif

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

/* Grazie Professore */
#define PRINT_ERRNO fprintf(stderr, "%s:%d: Errore #%3d \"%s\"\n", __FILE__, __LINE__, errno, strerror(errno));
#define PRINT_ERROR(x) fprintf(stderr, "%s", x);
#define MAX_LEN 128

int readConfiguration(int *settings, int argc, char *argv[]);

int main(int argc, char *argv[]) {
    int settings[7];

    readConfiguration(settings, argc, argv);

    return 0;
}

int readConfiguration(int *settings, int argc, char *argv[]) {
    char *path = "opt.conf";
    int configFD;
    FILE *configFile;
    int i, j, k;

    char defaultSettingsString[] = "# numero di studenti\n"
                                   "pop_size 5\n"
                                   "# voto minimo generabile per ade\n"
                                   "voto_ade_min 18\n"
                                   "# voto massimo generabile per ade\n"
                                   "voto_ade_max 30\n"
                                   "# preferenza minima per i gruppi\n"
                                   "nof_elems_min 2\n"
                                   "# preferenza massima per i gruppi\n"
                                   "nof_elems_max 4\n"
                                   "# durata della simulazione in secondi\n"
                                   "sim_time 5\n"
                                   "# massimo numero di rifiuti che può fare uno studente\n"
                                   "max_rejext 3\n";

    const char *settingsNames[] = {
            "pop_size",
            "voto_ade_min",
            "voto_ade_max",
            "nof_elems_min",
            "nof_elems_max",
            "sim_time",
            "max_reject"
    };

    char readContent[MAX_LEN] = {0};
    char readChar;
    int settingsFoundCount = 0;
    /*
     * Stato del lettore del file.
     * 0: leggendo caratteri dell'ID
     * 1: trovato uno spazio
     * 2: leggo un numero relativo all'ID precedente.
     */
    int state = 0;

    /* Il secondo argomento è la path del file di configurazione.
     * Se l'abbiamo, modifichiamo la path di default.
     */
    if (argc > 1) {
        path = argv[1];
    }

    /*
     * Proviamo ad aprire il file in sola lettura; se esiste dovremmo ottenere il fd.
     */

    if ((configFD = open(path, O_RDONLY)) == -1) {
        switch (errno) {
            case 0:
                /* Tutto a posto, niente da fare*/
                break;

            case ENOENT: {
                printf("Il file di configurazione non esiste. Sara' creato\n");
                /*
                 * Apriamo il file in modalità sola scrittura
                 * Lo creiamo se non esiste
                 * Se esiste è sovrascritto con lunghezza 0
                 *
                 * Con le due ultime flag impostiamo che l'utente ha accesso R/W.
                 */
                if ((configFD = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) == -1) {
                    PRINT_ERRNO
                }

                write(configFD, defaultSettingsString, strlen(defaultSettingsString));
                /* Ricordiamoci di chiudere il file */
                close(configFD);

                /* Riapriamo il file per poter leggere le impostazioni */
                if ((configFD = open(path, O_RDONLY)) == -1) {
                    PRINT_ERRNO
                }
                break;
            }

            default:
                PRINT_ERRNO
                exit(-1);

        }
    }

    /* Passiamo ad usare un FILE per poter usare qualche funzione più ad alto livello. */
    configFile = fdopen(configFD, O_RDONLY);

    for (i = 0; i < MAX_LEN && settingsFoundCount < 7; i++) {
        readChar = (char) fgetc(configFile);

        /* Se abbiamo letto EOF potremmo essere arrivati alla fine del file o potrebbe essersi verificato un errore.*/
        if (readChar == EOF) {
            if (errno == 0) {
                /* Siamo arrivati alla fine del file. Se non abbiamo ancora trovato tutto, lanciamo un errore. */
                if (settingsFoundCount == 6 && state == 2) {
                    settings[settingsFoundCount] = (int) strtol(readContent, NULL, 10);
                    if (errno != 0) {
                        PRINT_ERRNO
                        exit(-1);
                    } else {
                        settingsFoundCount++;
                        continue;
                    }
                } else {
                    PRINT_ERROR(
                            "Non sono stati inseriti tutti i parametri di configurazione. Se necessario, eliminare opt.conf")
                    exit(-1);
                }
            } else {
                PRINT_ERRNO
                exit(-1);
            }
            // TODO RIFARE
        } else if ('a' <= readChar && readChar <= 'z' && state == 0) {
            /* Aggiungiamo il carattere all'array */
        } else if ('0' <= readChar && readChar <= '9' && state != 0) {
            if (state == 1) {
                state = 2;
            } else if (state == 2) {

            }
        } else if (readChar == ' ' && state != 2) {
            /* Lo spazio cambia lo stato.*/
            for (j = 0; j < sizeof(settingsNames); j++) {
                if (strncmp(readContent, settingsNames[j], strlen(settingsNames[j])) == 0) {

                }
            }
            state = 1;

        } else if (readChar == '\n') {
            /* Se andiamo a capo resettiamo quello che abbiamo letto. */
            for (i = 0; i < MAX_LEN; i++) {
                readContent[i] = '\0';
            }
            state = 0;
        } else if (readChar == '#') {
            /* Abbiamo trovato un commento; ignoriamo il resto della riga. */
            do {
                readChar = (char) fgetc(configFile);
            } while (readChar != '\n');

            /* Cancelliamo quello che abbiamo letto */
            for (i = 0; i < MAX_LEN; i++) {
                readContent[i] = '\0';
            }
            state = 0;
        } else {
            /* C'è qualcosa che non va con il file. Evitiamo di continuare. */
            PRINT_ERROR("Struttura di opt.conf non riconoscibile. Se necessario, eliminare opt.conf")
            exit(-1);
        }
    }


    return 0;
}

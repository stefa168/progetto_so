#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include "macros.h"
#include "settings_reader.h"

#define MAX_LEN 128

int readConfiguration(int *settings, int argc, char *argv[]) {
    char *path = "opt.conf";
    int configFD;
    struct _IO_FILE *configFile;
    int i;

    char defaultSettingsString[] = "5\n"
                                   "18\n"
                                   "30\n"
                                   "2\n"
                                   "4\n"
                                   "5\n"
                                   "3";

    char readContent[MAX_LEN] = {0};
    int readContentSize = 0;
    char readChar;
    int state = 0;
    int settingsFoundCount = 0;

    /* Il secondo argomento è la path del file di configurazione.
     * Se l'abbiamo, modifichiamo la path di default.
     */
    if (argc > 1) {
        path = argv[1];
    }

    /*
     * Proviamo ad aprire il file in sola lettura; se esiste dovremmo ottenere il fd.
     */
    configFD = open(path, O_RDONLY);
    if (configFD == -1) {
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
                    exit(-1);
                }

                write(configFD, defaultSettingsString, strlen(defaultSettingsString));
                /* Ricordiamoci di chiudere il file */
                close(configFD);

                /* Riapriamo il file per poter leggere le impostazioni */
                /*if ((configFD = open(path, O_RDONLY)) == -1) {
                    PRINT_ERRNO
                    exit(-1);
                }*/
                break;
            }

            default:
                PRINT_ERRNO
                exit(-1);
        }
    }

    /* Passiamo ad usare un FILE per poter usare qualche funzione più ad alto livello. */
    configFile = fopen(path, "r");

    if (configFile == 0) {
        PRINT_ERRNO
        exit(-1);
    }

    while (settingsFoundCount < 7) {
        readChar = (char) fgetc(configFile);
        if (readContentSize > MAX_LEN) {
            PRINT_ERROR("Numero massimo di caratteri superato")
            exit(-1);
        }

        switch (state) {
            case 0: {
                if (readChar == ' ') {
                    state = 0;
                } else if ('0' <= readChar && readChar <= '9') {
                    readContent[readContentSize] = readChar;
                    readContentSize++;
                    state = 1;
                } else if (readChar == EOF) {
                    PRINT_ERRNO
                    exit(-1);
                } else {
                    fprintf(stderr, "0: Trovato carattere non consentito ('%c')", readChar);
                    exit(-1);
                }
                break;
            }
            case 1: {
                if ('0' <= readChar && readChar <= '9') {
                    readContent[readContentSize] = readChar;
                    readContentSize++;
                } else if (readChar == ' ' || readChar == '\n' || readChar == EOF) {
                    if (errno != 0) {
                        PRINT_ERRNO
                        exit(-1);
                    } else if (settingsFoundCount != 6 && readChar == EOF) {
                        PRINT_ERROR("Mancano delle impostazioni. Valutare se cancellare opt.conf")
                        exit(-1);
                    }

                    settings[settingsFoundCount] = (int) strtol(readContent, NULL, 10);
#ifdef DEBUG
                    printf("Trovato %d\n", settings[settingsFoundCount]);
#endif
                    settingsFoundCount++;

                    for (i = 0; i < MAX_LEN; i++) {
                        readContent[i] = 0;
                    }
                    readContentSize = 0;
                    state = 0;
                } else {
                    fprintf(stderr, "1: Trovato carattere non consentito ('%c')", readChar);
                    exit(-1);
                }
                break;
            }
        }
    }

    return 0;
}
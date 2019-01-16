#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include "../macros.h"
#include "settings_reader.h"

SettingsData *readConfiguration(int argc, char *argv[]) {
    char *path = "opt.conf";
    int configFD;
    struct _IO_FILE *configFile;
    int i;
    int settingsNum = NOF_SETTINGS;

    SettingsData *settingsData = malloc(sizeof(SettingsData));

    char defaultSettingsString[] = "# Impostazioni della simulazione.\n"
                                   "# NOTA BENE: tutti i valori devono essere INTERI e devono essere solo composti da cifre decimali.\n"
                                   "#            L'ordine delle impostazioni è IMPORTANTE! In caso di problemi, è sufficiente eliminare\n"
                                   "#            questo file per farne generare uno nuovo coi valori di default.\n"
                                   "\n"
                                   "# Numero di Studenti\n"
                                   "5\n"
                                   "\n"
                                   "# tempo della simulazione\n"
                                   "5\n"
                                   "\n"
                                   "# Voto AdE minimo\n"
                                   "18\n"
                                   "# Voto AdE massimo\n"
                                   "30\n"
                                   "\n"
                                   "# Seguono due impostazioni per indicare il range (estremi inclusi) delle possibili preferenze degli studenti.\n"
                                   "# Numero di elementi minimo del gruppo. Non è possibile indicare meno di 1.\n"
                                   "2\n"
                                   "# Numero di elementi massimo del gruppo. Massimo 10.\n"
                                   "4\n"
                                   "\n"
                                   "# Numero massimo di inviti inviabili\n"
                                   "5\n"
                                   "# Numero massimo di rifiuti\n"
                                   "3\n"
                                   "\n"
                                   "# Seguono le percentuali delle preferenze. Se devo scrivere 50% (0.5), indicherò 50.\n"
                                   "# 2 componenti\n"
                                   "25\n"
                                   "# 3 componenti\n"
                                   "25\n"
                                   "# 4 componenti\n"
                                   "50";

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
                printf("[Lettore Impostazioni] W: Il file di configurazione non esiste. Sara' creato\n");
                /*
                 * Apriamo il file in modalità sola scrittura
                 * Lo creiamo se non esiste
                 * Se esiste è sovrascritto con lunghezza 0
                 *
                 * Con le due ultime flag impostiamo che l'utente ha accesso R/W.
                 */
                if ((configFD = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) == -1) {
                    PRINT_ERRNO_EXIT(-1)
                }

                write(configFD, defaultSettingsString, strlen(defaultSettingsString));
                break;
            }

            default: PRINT_ERRNO_EXIT(-1)
        }
    }
    close(configFD);

    /* Passiamo ad usare un FILE per poter usare qualche funzione più ad alto livello. */
    configFile = fopen(path, "r");

    if (configFile == 0) {
        PRINT_ERRNO_EXIT(-1)
    }

    settingsData->settingsCount = NOF_SETTINGS;

    while (settingsFoundCount < settingsNum) {
        readChar = (char) fgetc(configFile);
        if (readContentSize > MAX_LEN) {
            PRINT_ERROR("[Lettore Impostazioni] E: Numero massimo di caratteri superato")
            exit(-1);
        }

        switch (state) {
            case 0: {
                if (readChar == ' ' || readChar == '\n') {
                    state = 0;
                } else if ('0' <= readChar && readChar <= '9') {
                    readContent[readContentSize] = readChar;
                    readContentSize++;
                    state = 1;
                } else if (readChar == '#') {
                    state = 2;
                } else if (readChar == EOF) {
                    PRINT_ERRNO_EXIT(-1)
                } else {
                    fprintf(stderr, "[Lettore Impostazioni] E: (0) Trovato carattere non consentito ('%c' %d)",
                            readChar, readChar);
                    exit(-1);
                }
                break;
            }
            case 1: {
                if ('0' <= readChar && readChar <= '9') {
                    readContent[readContentSize] = readChar;
                    readContentSize++;
                } else if (readChar == ' ' || readChar == '\n' || readChar == EOF) {
                    if (errno) {
                        PRINT_ERRNO_EXIT(-1)
                    } else if (settingsFoundCount != (settingsNum - 1) && readChar == EOF) {
                        PRINT_ERROR(
                                "[Lettore Impostazioni] E: Mancano delle impostazioni. Valutare se cancellare opt.conf")
                        exit(-1);
                    }

                    setSettingsValue(settingsData, settingsFoundCount, (int) strtol(readContent, NULL, 10));
                    PRINT_IF_ERRNO_EXIT(-1)

#ifdef S_R_DEBUG
                    printf("Trovato %d\n", getSettingsValue(settingsData, settingsFoundCount));
#endif

                    /* Se per caso abbiamo letto la preferenza minima e massima di componenti dei gruppi, aggiorniamo
                     * il numero di cicli da fare per rispecchiare la necessità di leggere anche quei valori dal file
                     * delle impostazioni (si trovano al fondo, dopo NOF_SETTINGS altri valori)
                     */
                    if (settingsFoundCount == 5) {
                        settingsData->numOfPreferences = settingsData->maxGroupPref - settingsData->minGroupPref + 1;

#ifdef S_R_DEBUG
                        printf("Aggiornato il numero di impostazioni da leggere da %d a %d\n", settingsNum,
                               settingsNum + settingsData->numOfPreferences);
#endif

                        settingsNum += settingsData->numOfPreferences;

                        settingsData->preferencePercentages = calloc((size_t) settingsData->numOfPreferences,
                                                                     sizeof(int));
                    }

                    settingsFoundCount++;

                    for (i = 0; i < readContentSize; i++) {
                        readContent[i] = 0;
                    }
                    readContentSize = 0;
                    state = 0;
                } else {
                    fprintf(stderr, "[Lettore Impostazioni] E: (1) Trovato carattere non consentito ('%c', %d)",
                            readChar, readChar);
                    exit(-1);
                }
                break;
            }

                /* Ignoriamo i commenti. */
            case 2: {
                if (readChar == '\n') {
                    state = 0;
                } else if (readChar == EOF) {
                    PRINT_IF_ERRNO_EXIT(-1)
                    PRINT_ERROR_EXIT("[Lettore Impostazioni] E?: (2) Raggiunta la fine del file in un commento", -1)
                }
            }
        }
    }

    fclose(configFile);
    return settingsData;
}

void setSettingsValue(SettingsData *data, int index, int value) {
    switch (index) {
        case 0:
            data->pop_size = value;
            break;
        case 1:
            data->sim_duration = value;
            break;
        case 2:
            data->AdE_min = value;
            break;
        case 3:
            data->AdE_max = value;
            break;
        case 4:
            data->minGroupPref = value;
            break;
        case 5:
            data->maxGroupPref = value;
            break;
        case 6:
            data->nof_invites = value;
            break;
        case 7:
            data->nof_refuse = value;
            break;
        default: {
            if (index > data->settingsCount + data->numOfPreferences - 1) {
                PRINT_ERROR_EXIT(
                        "[Lettore Impostazioni] E: Tentata una scrittura a un'indice delle impostazioni errato.", -1)
            } else {
                data->preferencePercentages[index - data->settingsCount] = value;
                PRINT_IF_ERRNO_EXIT(-1)
            }
        }
    }
}

int getSettingsValue(SettingsData *data, int index) {
    switch (index) {
        case 0:
            return data->pop_size;
        case 1:
            return data->sim_duration;
        case 2:
            return data->AdE_min;
        case 3:
            return data->AdE_max;
        case 4:
            return data->minGroupPref;
        case 5:
            return data->maxGroupPref;
        case 6:
            return data->nof_invites;
        case 7:
            return data->nof_refuse;
        default: {
            if (index > data->settingsCount + data->numOfPreferences - 1) {
                PRINT_ERROR_EXIT("[Lettore Impostazioni] E: Tentata una lettura a un'indice delle impostazioni errato.",
                                 -1)
            } else {
                return data->preferencePercentages[index - data->settingsCount - 1];
            }
        }

    }
}

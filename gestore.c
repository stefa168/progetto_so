#define DEBUG_GESTORE

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <unistd.h>
#include "gestore.h"
#include "utilities/settings_reader.h"
#include "macros.h"
#include "utilities/ipc_utils.h"


int main(int argc, char *argv[]) {
    SettingsData *settings;
    int msgID;
    int i;

    settings = readConfiguration(argc, argv);

#ifdef DEBUG_GESTORE
    printf("Trovate le impostazioni:\n"
           "\t- pop_size = %d\n"
           "\t- sim_duration = %d\n"
           "\t- AdE_min = %d\n"
           "\t- AdE_max = %d\n"
           "\t- minGroupPref = %d\n"
           "\t- maxGroupPref = %d\n"
           "\t- nof_invites = %d\n"
           "\t- nof_refuse = %d\n"
           "\t- settingsCount = %d\n"
           "\t- numOfPreferences = %d\n"
           "\t- prefPerc = [", settings->pop_size, settings->sim_duration, settings->AdE_min, settings->AdE_max,
           settings->minGroupPref, settings->maxGroupPref, settings->nof_invites, settings->nof_refuse,
           settings->settingsCount, settings->numOfPreferences);

    for (i = 0; i < settings->numOfPreferences; i++) {
        if (i == settings->numOfPreferences - 1) {
            printf("%d]\n", settings->preferencePercentages[i]);
        } else {
            printf("%d,", settings->preferencePercentages[i]);
        }
    }
#endif

    // Gestire prima della logica degli studenti tutta la parte della memoria e il comportamento del gestore

    // IPC CREAT e EXCL insieme creano la coda se non esiste, ma se esiste lanciano un errore.
    msgID = msgget(IPC_OWN_KEY, IPC_CREAT | IPC_EXCL | S_IWUSR | S_IRUSR);

    if (msgID == -1) {
        PRINT_ERROR("Errore durante l'apertura della coda di messaggi\n")
        PRINT_ERRNO_EXIT(-1)
    } else {
        printf("Coda di messaggi aperta correttamente con ID %d\n", msgID);
    }

    // msgctl serve anche per cancellare la coda.
    // consigliato ipcs per vedere se ci sono delle cose che non vogliamo
    // consigliato ipcrm --all=msg per rimuovere tutti i messaggi
    if (msgctl(msgID, IPC_RMID, NULL)) {
        fprintf(stderr, "%s:%d: Errore durante la chiusura della coda di messaggi %d\n", __FILE__, __LINE__, msgID);
        PRINT_ERRNO_EXIT(-1)
    } else {
        printf("Coda di messaggi %d chiusa correttamente.\n", msgID);
    }

    return 0;
}
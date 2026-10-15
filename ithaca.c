#define _GNU_SOURCE

/***********************************************
 *
 * @Proposit: Conte el servidor concurrent del proces Ithaca.
 * @Autor/s: Antonio Duran Sabates i Arnau González Hortelano
 * @Data creacio: 18/09/2026
 * @Data ultima modificacio: 15/10/2026
 *
 ************************************************/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>

#include "configuracio_itaca.h"
#include "trames.h"
#include "xarxa.h"

int finalitzar_programa = 0;
int fd_servidor = -1;

typedef struct {
    int fd_client;
    ConfiguracioItaca *configuracio;
    pthread_mutex_t *mutex_viatges;
} DadesClientIthaca;

/***********************************************
 *
 * @Finalitat: Indica la finalitzacio i tanca el servidor per sortir d'accept.
 * @Parametres: in: senyal = senyal rebut pel proces.
 * @Retorn: ----.
 *
 ************************************************/
void gestionarSigint(int senyal __attribute__((unused))) {
    finalitzar_programa = 1;
    if (fd_servidor >= 0) {
        close(fd_servidor);
        fd_servidor = -1;
    }
}

/***********************************************
 *
 * @Finalitat: Marca els viatges anteriors com a fracassats i envia els disponibles.
 * @Parametres: in: fd_client = descriptor del client.
 *              in: peticio = trama LIST VOYAGES rebuda.
 *              in: configuracio = viatges compartits d'Ithaca.
 *              in/out: mutex_viatges = mutex de disponibilitat i assignacio.
 *              in: nom_odysseus = nom identificat del client.
 *              in: viatge_actual = viatge acceptat en aquesta connexio, o 0.
 * @Retorn: Retorna 0 si envia la resposta i -1 si falla l'enviament.
 *
 ************************************************/
int gestionarListVoyages(int fd_client, unsigned char *peticio,
                         ConfiguracioItaca *configuracio,
                         pthread_mutex_t *mutex_viatges, char *nom_odysseus,
                         int viatge_actual) {
    unsigned char trama[MIDA_TRAMA];
    char dades[MIDA_DADES + 1], missatge[MIDA_DADES + 100];
    int *indexs_disponibles = NULL;
    int i = 0, total = 0, longitud = 0, resultat = 0;
    Viatge *viatge = NULL;

    if (peticio[POSICIO_FLAGS] != FLAG_PETICIO) {
        return enviarNack(fd_client, "INVALID_FLAGS");
    }
    if (peticio[POSICIO_LONGITUD] != 0) {
        return enviarNack(fd_client, "INVALID_LENGTH");
    }
    if (viatge_actual != 0) {
        crearTrama(trama, TIPUS_LIST_VOYAGES, FLAG_RESPOSTA_ERROR,
                   (unsigned char *)"Ja tens un viatge actiu", 23);
        return enviarTrama(fd_client, trama);
    }
    if (configuracio->nombre_viatges > 0) {
        indexs_disponibles = malloc(configuracio->nombre_viatges * sizeof(int));
        if (indexs_disponibles == NULL) {
            crearTrama(trama, TIPUS_LIST_VOYAGES, FLAG_RESPOSTA_ERROR,
                       (unsigned char *)"Sense memoria", 13);
            return enviarTrama(fd_client, trama);
        }
    }
    /* En tornar a llistar, fracassen els viatges anteriors del mateix navegant. */
    pthread_mutex_lock(mutex_viatges);
    for (i = 0; i < configuracio->nombre_viatges; i++) {
        viatge = &configuracio->viatges[i];
        if (viatge->disponible == 0 && viatge->fracassat == 0 &&
            viatge->identificador != viatge_actual &&
            strcmp(viatge->odysseus_assignat, nom_odysseus) == 0) {
            viatge->fracassat = 1;
        }
        if (configuracio->viatges[i].disponible == 1) {
            indexs_disponibles[total] = i;
            total++;
        }
    }
    pthread_mutex_unlock(mutex_viatges);
    longitud = snprintf(missatge, sizeof(missatge),
                         ">>> %s requested available voyages.\n", nom_odysseus);
    write(1, missatge, longitud);
    if (total == 0) {
        free(indexs_disponibles);
        crearTrama(trama, TIPUS_LIST_VOYAGES, FLAG_RESPOSTA_OK, (unsigned char *)"0", 1);
        return enviarTrama(fd_client, trama);
    }
    for (i = 0; i < total && resultat == 0; i++) {
        viatge = &configuracio->viatges[indexs_disponibles[i]];
        longitud = snprintf(dades, sizeof(dades), "%d&%d&%d&%s&%s&%d", i + 1, total,
                             viatge->identificador, viatge->nom_objecte,
                             viatge->illa_desti, viatge->recompensa);
        if (crearTrama(trama, TIPUS_LIST_VOYAGES, FLAG_RESPOSTA_OK,
                       (unsigned char *)dades, longitud) != 0) {
            crearTrama(trama, TIPUS_LIST_VOYAGES, FLAG_RESPOSTA_ERROR,
                       (unsigned char *)"Viatge massa llarg", 18);
            resultat = enviarTrama(fd_client, trama);
            break;
        }
        resultat = enviarTrama(fd_client, trama);
    }
    free(indexs_disponibles);
    return resultat;
}

/***********************************************
 *
 * @Finalitat: Assigna un viatge disponible al client que l'ha demanat.
 * @Parametres: in: fd_client = descriptor del client.
 *              in: peticio = trama ACCEPT rebuda.
 *              in/out: configuracio = viatges compartits d'Ithaca.
 *              in/out: mutex_viatges = mutex de disponibilitat i assignacio.
 *              in: nom_odysseus = nom identificat del client.
 *              out: viatge_actual = identificador del viatge acceptat.
 * @Retorn: Retorna 0 si envia la resposta i -1 si falla l'enviament.
 *
 ************************************************/
int gestionarAccept(int fd_client, unsigned char *peticio,
                     ConfiguracioItaca *configuracio,
                     pthread_mutex_t *mutex_viatges, char *nom_odysseus,
                     int *viatge_actual) {
    unsigned char trama[MIDA_TRAMA], flags = FLAG_RESPOSTA_ERROR;
    char dades[MIDA_DADES + 1], resposta[MIDA_DADES + 1];
    char missatge[MIDA_DADES + 100];
    int longitud = peticio[POSICIO_LONGITUD], identificador = 0, i = 0;
    int acceptat = 0, longitud_resposta = 0;
    Viatge *viatge = NULL;

    if (peticio[POSICIO_FLAGS] != FLAG_PETICIO) {
        return enviarNack(fd_client, "INVALID_FLAGS");
    }
    if (longitud == 0 || longitud > MIDA_DADES) {
        return enviarNack(fd_client, "INVALID_LENGTH");
    }
    memcpy(dades, peticio + POSICIO_DADES, longitud);
    dades[longitud] = '\0';
    if ((int)strlen(dades) != longitud || esNumero(dades) == 0) {
        return enviarNack(fd_client, "INVALID_DATA");
    }
    if (*viatge_actual != 0) {
        crearTrama(trama, TIPUS_ACCEPT, FLAG_RESPOSTA_ERROR,
                   (unsigned char *)"Ja tens un viatge actiu", 23);
        return enviarTrama(fd_client, trama);
    }
    strcpy(resposta, "ACCEPT invalid");
    if (peticio[POSICIO_FLAGS] == FLAG_PETICIO && longitud > 0 && longitud <= MIDA_DADES) {
        memcpy(dades, peticio + POSICIO_DADES, longitud);
        dades[longitud] = '\0';
        if (esNumero(dades) == 1 && (int)strlen(dades) == longitud) {
            /* L'identificador no pot superar el nombre de viatges carregats. */
            for (i = 0; i < longitud; i++) {
                if (identificador > configuracio->nombre_viatges / 10 ||
                    (identificador == configuracio->nombre_viatges / 10 &&
                     dades[i] - '0' > configuracio->nombre_viatges % 10)) {
                    identificador = 0;
                    break;
                }
                identificador = identificador * 10 + dades[i] - '0';
            }
        }
    }
    if (identificador > 0) {
        viatge = &configuracio->viatges[identificador - 1];
        /* Aquests camps son immutables; la resposta queda en memoria local. */
        longitud_resposta = snprintf(resposta, sizeof(resposta), "OK&%d&%s&%s&%d",
                                     viatge->identificador, viatge->nom_objecte,
                                     viatge->illa_desti, viatge->recompensa);
        pthread_mutex_lock(mutex_viatges);
        if (viatge->disponible == 1 && longitud_resposta >= 0 && longitud_resposta <= MIDA_DADES) {
            viatge->disponible = 0;
            strcpy(viatge->odysseus_assignat, nom_odysseus);
            acceptat = 1;
        }
        pthread_mutex_unlock(mutex_viatges);
        if (acceptat == 1) {
            *viatge_actual = identificador;
            flags = FLAG_RESPOSTA_OK;
            longitud = snprintf(missatge, sizeof(missatge),
                                 ">>> Voyage %d assigned to %s.\n", identificador, nom_odysseus);
            write(1, missatge, longitud);
        } else {
            strcpy(resposta, "Viatge no disponible");
        }
    }
    crearTrama(trama, TIPUS_ACCEPT, flags, (unsigned char *)resposta, strlen(resposta));
    return enviarTrama(fd_client, trama);
}

/***********************************************
 *
 * @Finalitat: Confirma la sortida d'Ithaca sense modificar el viatge assignat.
 * @Parametres: in: fd_client = descriptor del client.
 *              in: peticio = trama de desconnexio rebuda.
 *              in: nom_odysseus = nom identificat del client.
 * @Retorn: Retorna 1 si es pot tancar, 0 si es rebutja i -1 si falla l'enviament.
 *
 ************************************************/
int gestionarDesconnexio(int fd_client, unsigned char *peticio, char *nom_odysseus) {
    unsigned char trama[MIDA_TRAMA];
    char missatge[MIDA_NOM_ODYSSEUS + 100];
    int longitud = 0;

    if (peticio[POSICIO_FLAGS] != FLAG_PETICIO) {
        return enviarNack(fd_client, "INVALID_FLAGS");
    }
    if (peticio[POSICIO_LONGITUD] != 0) {
        return enviarNack(fd_client, "INVALID_LENGTH");
    }
    crearTrama(trama, TIPUS_DESCONNEXIO_ITHACA, FLAG_RESPOSTA_OK,
               (unsigned char *)"OK", 2);
    if (enviarTrama(fd_client, trama) != 0) {
        return -1;
    }
    longitud = snprintf(missatge, sizeof(missatge),
                         ">>> %s has left Ithaca. Fair winds!\n", nom_odysseus);
    write(1, missatge, longitud);
    return 1;
}

/***********************************************
 *
 * @Finalitat: Identifica un Odysseus i aten les operacions del port d'Ithaca.
 * @Parametres: in: arg = parametres reservats per a aquest client.
 * @Retorn: Retorna NULL quan acaba l'atencio del client.
 *
 ************************************************/
void *atendreOdysseus(void *arg) {
    unsigned char trama[MIDA_TRAMA];
    char nom_odysseus[MIDA_DADES + 1], *missatge = NULL;
    DadesClientIthaca *dades_client = arg;
    ConfiguracioItaca *configuracio = dades_client->configuracio;
    pthread_mutex_t *mutex_viatges = dades_client->mutex_viatges;
    int fd_client = dades_client->fd_client, longitud = 0, caracters_escrits = 0;
    int resultat = 0, viatge_actual = 0;

    free(arg);
    if (rebreTrama(fd_client, trama) != 0) {
        close(fd_client);
        return NULL;
    }
    if (trama[POSICIO_TIPUS] != TIPUS_CONNECT_ITHACA) {
        if (trama[POSICIO_TIPUS] != TIPUS_NACK) {
            enviarNack(fd_client, "INVALID_TYPE");
        }
        close(fd_client);
        return NULL;
    }
    longitud = trama[POSICIO_LONGITUD];
    if (trama[POSICIO_FLAGS] != FLAG_PETICIO ||
        longitud <= 0 || longitud > MIDA_DADES) {
        if (trama[POSICIO_FLAGS] != FLAG_PETICIO) {
            enviarNack(fd_client, "INVALID_FLAGS");
        } else {
            enviarNack(fd_client, "INVALID_LENGTH");
        }
        close(fd_client);
        return NULL;
    }
    memcpy(nom_odysseus, trama + POSICIO_DADES, longitud);
    nom_odysseus[longitud] = '\0';
    if ((int)strlen(nom_odysseus) != longitud || strchr(nom_odysseus, '&') != NULL) {
        enviarNack(fd_client, "INVALID_DATA");
        close(fd_client);
        return NULL;
    }
    if (crearTrama(trama, TIPUS_CONNECT_ITHACA, FLAG_RESPOSTA_OK,
                   (unsigned char *)"OK", 2) != 0 ||
        enviarTrama(fd_client, trama) != 0) {
        close(fd_client);
        return NULL;
    }
    caracters_escrits = asprintf(&missatge, ">>> %s connected to Ithaca.\n",
                                 nom_odysseus);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }
    while (rebreTrama(fd_client, trama) == 0) {
        if (trama[POSICIO_TIPUS] == TIPUS_NACK) {
            break;
        }
        if (trama[POSICIO_TIPUS] == TIPUS_LIST_VOYAGES) {
            resultat = gestionarListVoyages(fd_client, trama, configuracio,
                                            mutex_viatges, nom_odysseus, viatge_actual);
        } else if (trama[POSICIO_TIPUS] == TIPUS_ACCEPT) {
            resultat = gestionarAccept(fd_client, trama, configuracio,
                                       mutex_viatges, nom_odysseus, &viatge_actual);
        } else if (trama[POSICIO_TIPUS] == TIPUS_DESCONNEXIO_ITHACA) {
            resultat = gestionarDesconnexio(fd_client, trama, nom_odysseus);
        } else {
            resultat = enviarNack(fd_client, "INVALID_TYPE");
        }
        if (resultat != 0) {
            break;
        }
    }
    close(fd_client);
    return NULL;
}

/***********************************************
 *
 * @Finalitat: Carrega Ithaca i accepta connexions amb un fil per Odysseus.
 * @Parametres: in: argc = nombre d'arguments rebuts.
 *              in: argv = arguments rebuts.
 * @Retorn: Retorna 0 si els arguments son correctes i 1 altrament.
 *
 ************************************************/
int main(int argc, char *argv[]) {
    ConfiguracioItaca configuracio = {0};
    pthread_t fil;
    pthread_mutex_t mutex_viatges;
    char *missatge = NULL, *nom_programa = argv[0];
    int resultat = 0, caracters_escrits = 0, fd_client = -1, fils_creats = 0;
    DadesClientIthaca *dades_client = NULL;

    if (argc != 3) {
        caracters_escrits = asprintf(&missatge,"Usage: %s <config.dat> <voyages.dat>\n", nom_programa);
        if (caracters_escrits >= 0) {
            write(2, missatge, caracters_escrits);
            free(missatge);
        }
        return 1;
    }

    resultat = carregarConfiguracioItaca(argv[1], &configuracio);
    if (resultat == 0) {
        resultat = carregarViatges(argv[2], &configuracio);
    }
    if (resultat != 0) {
        alliberarConfiguracioItaca(&configuracio);
        return 1;
    }

    if (pthread_mutex_init(&mutex_viatges, NULL) != 0) {
        write(2, "Error: mutex de viatges.\n", strlen("Error: mutex de viatges.\n"));
        alliberarConfiguracioItaca(&configuracio);
        return 1;
    }
    signal(SIGINT, gestionarSigint);
    signal(SIGPIPE, SIG_IGN);
    fd_servidor = obrirServidor(configuracio.ip, configuracio.port);
    if (fd_servidor < 0) {
        write(2, "Error: no s'ha pogut obrir el servidor d'Ithaca.\n",
              strlen("Error: no s'ha pogut obrir el servidor d'Ithaca.\n"));
        alliberarConfiguracioItaca(&configuracio);
        pthread_mutex_destroy(&mutex_viatges);
        return 1;
    }
    caracters_escrits = asprintf(&missatge,"Ithaca initialized. %d voyages loaded.\nWaiting for Odysseus...\n",configuracio.nombre_viatges);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }

    while (finalitzar_programa == 0) {
        fd_client = accept(fd_servidor, NULL, NULL);
        if (fd_client < 0) {
            if (finalitzar_programa == 0) {
                write(2, "Error: accept d'Ithaca.\n", strlen("Error: accept d'Ithaca.\n"));
                resultat = 1;
            }
            break;
        }
        dades_client = malloc(sizeof(*dades_client));
        if (dades_client == NULL) {
            write(2, "Error: malloc del client.\n", strlen("Error: malloc del client.\n"));
            close(fd_client);
        } else {
            dades_client->fd_client = fd_client;
            dades_client->configuracio = &configuracio;
            dades_client->mutex_viatges = &mutex_viatges;
            if (pthread_create(&fil, NULL, atendreOdysseus, dades_client) != 0) {
                write(2, "Error: pthread_create.\n", strlen("Error: pthread_create.\n"));
                free(dades_client);
                close(fd_client);
            } else {
                fils_creats = 1;
            }
        }
    }

    caracters_escrits = asprintf(&missatge, "Ithaca closes the harbor.\n");
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }

    /* Si hi ha hagut fils, els recursos compartits viuen fins a sortir del proces. */
    if (fils_creats == 0) {
        alliberarConfiguracioItaca(&configuracio);
        pthread_mutex_destroy(&mutex_viatges);
    }
    if (fd_servidor >= 0) {
        close(fd_servidor);
        fd_servidor = -1;
    }
    return resultat;
}

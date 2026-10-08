#define _GNU_SOURCE

/***********************************************
 *
 * @Proposit: Conte el punt d'entrada del proces Ithaca.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 18/09/2026
 * @Data ultima modificacio: 06/10/2026
 *
 ************************************************/

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "configuracio_itaca.h"
#include "trames.h"
#include "xarxa.h"

typedef struct {
    int socket_client;
    ConfiguracioItaca *configuracio;
    pthread_mutex_t *mutex_viatges;
} ParametresOdisseu;

typedef struct {
    pthread_t fil;
    int socket_client;
} FilOdisseu;
//REVISAR
volatile sig_atomic_t finalitzar_programa = 0;
volatile sig_atomic_t socket_servidor_global = -1;

/***********************************************
 *
 * @Finalitat: Indica que Ithaca ha de finalitzar en rebre SIGINT.
 * @Parametres: in: senyal = senyal rebut pel proces.
 * @Retorn: ----.
 *
 ************************************************/
void gestionarSigint(int senyal __attribute__((unused))) {
    finalitzar_programa = 1;
    if (socket_servidor_global >= 0) {
        close(socket_servidor_global);
        socket_servidor_global = -1;
    }
}

/***********************************************
 *
 * @Finalitat: Envia una resposta d'error per una operacio coneguda.
 * @Parametres: in: socket_client = descriptor del client.
 *              in: tipus = tipus de l'operacio.
 *              in: motiu = causa de l'error.
 * @Retorn: Retorna 0 si envia la resposta i -1 altrament.
 *
 ************************************************/
int respondreErrorOperacio(int socket_client, unsigned char tipus, char *motiu) {
    unsigned char resposta[MIDA_TRAMA] = {0}; //REVISAR
    int longitud = 0, resultat = 0;

    while (motiu[longitud] != '\0') {
        longitud++;
    }
    resultat = crearTrama(resposta, tipus, FLAGS_RESPOSTA_ERROR, (unsigned char *) motiu, longitud);
    if (resultat == TRAMA_CORRECTA) {
        resultat = enviarTrama(socket_client, resposta);
    }
    return resultat;
}

/***********************************************
 *
 * @Finalitat: Envia els viatges disponibles a un Odysseus.
 * @Parametres: in: socket_client = descriptor del client.
 *              in: configuracio = viatges compartits d'Ithaca.
 *              in: mutex_viatges = proteccio de l'estat dels viatges.
 *              in: nom_odisseu = Odysseus que fa la peticio.
 *              in: peticio = trama rebuda.
 *              in/out: viatge_anterior = indica si tenia un viatge assignat.
 * @Retorn: Retorna 0 si respon correctament i -1 altrament.
 *
 ************************************************/
int gestionarLlistaViatges(int socket_client, ConfiguracioItaca *configuracio, pthread_mutex_t *mutex_viatges, char *nom_odisseu, unsigned char *peticio, int *viatge_anterior) {
    unsigned char resposta[MIDA_TRAMA] = {0}, sense_viatges[] = "0"; //REVISAR
    int *indexs_disponibles = NULL;
    char *dades = NULL, *missatge = NULL;
    int nombre_disponibles = 0, i = 0, index = 0, resultat = 0;
    int caracters_escrits = 0;

    if (peticio[POSICIO_FLAGS] != FLAGS_PETICIO_TEXTUAL) {
        return respondreErrorOperacio(socket_client, TIPUS_LLISTAR_VIATGES, MOTIU_FLAGS_INVALIDS);
    }
    if (peticio[POSICIO_LONGITUD_DADES] != 0) {
        return respondreErrorOperacio(socket_client, TIPUS_LLISTAR_VIATGES, MOTIU_DADES_INVALIDES);
    }

    if (configuracio->nombre_viatges > 0) {
        indexs_disponibles = malloc(configuracio->nombre_viatges * sizeof(*indexs_disponibles));
        if (indexs_disponibles == NULL) {
            return respondreErrorOperacio(socket_client,TIPUS_LLISTAR_VIATGES, "MEMORY_ERROR");
        }
    }

    pthread_mutex_lock(mutex_viatges);
    if (*viatge_anterior == 1) {
        for (i = 0; i < configuracio->nombre_viatges; i++) {
            if (configuracio->viatges[i].disponible == 0 &&
                configuracio->viatges[i].fracassat == 0 &&
                configuracio->viatges[i].odisseu_assignat != NULL &&
                strcmp(configuracio->viatges[i].odisseu_assignat, nom_odisseu) == 0) {
                configuracio->viatges[i].fracassat = 1;
            }
        }
        *viatge_anterior = 0;
    }
    for (i = 0; i < configuracio->nombre_viatges; i++) {
        if (configuracio->viatges[i].disponible == 1) {
            indexs_disponibles[nombre_disponibles] = i;
            nombre_disponibles++;
        }
    }
    pthread_mutex_unlock(mutex_viatges);

    caracters_escrits = asprintf(&missatge, ">>> %s requested available voyages.\n", nom_odisseu);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }

    if (nombre_disponibles == 0) {
        resultat = crearTrama(resposta, TIPUS_LLISTAR_VIATGES, FLAGS_RESPOSTA_CORRECTA, sense_viatges, 1);
        free(indexs_disponibles);
        if (resultat != TRAMA_CORRECTA) {
            return -1;
        }
        return enviarTrama(socket_client, resposta);
    }

    for (i = 0; i < nombre_disponibles && resultat == 0; i++) {
        index = indexs_disponibles[i];
        caracters_escrits = asprintf(&dades, "%d&%d&%d&%s&%s&%d",i + 1, nombre_disponibles,configuracio->viatges[index].identificador,configuracio->viatges[index].nom_objecte,configuracio->viatges[index].illa_desti,configuracio->viatges[index].recompensa);
        if (caracters_escrits < 0 || caracters_escrits > MIDA_DADES_TRAMA) {
            resultat = -1;
        } else {
            resultat = crearTrama(resposta, TIPUS_LLISTAR_VIATGES, FLAGS_RESPOSTA_CORRECTA,(unsigned char *) dades, caracters_escrits);
        }
        if (resultat == TRAMA_CORRECTA) {
            resultat = enviarTrama(socket_client, resposta);
        }
        free(dades);
        dades = NULL;
    }

    free(indexs_disponibles);
    return resultat;
}

/***********************************************
 *
 * @Finalitat: Assigna atomicament un viatge disponible a un Odysseus.
 * @Parametres: in: socket_client = descriptor del client.
 *              in/out: configuracio = viatges compartits d'Ithaca.
 *              in: mutex_viatges = proteccio de l'estat dels viatges.
 *              in: nom_odisseu = Odysseus que accepta el viatge.
 *              in: peticio = trama rebuda.
 * @Retorn: Retorna 0 si respon correctament i -1 altrament.
 *
 ************************************************/
int gestionarAcceptacio(int socket_client, ConfiguracioItaca *configuracio, pthread_mutex_t *mutex_viatges, char *nom_odisseu,unsigned char *peticio) {
    unsigned char dades_peticio[MIDA_DADES_TRAMA + 1] = {0}; //REVISAR
    unsigned char resposta[MIDA_TRAMA] = {0};
    Viatge *viatge = NULL;
    char *nom_assignat = NULL, *dades_resposta = NULL, *missatge = NULL;
    int longitud = 0, identificador = 0, i = 0, resultat = 0;
    int caracters_escrits = 0, disponible = 0;

    longitud = peticio[POSICIO_LONGITUD_DADES];
    if (peticio[POSICIO_FLAGS] != FLAGS_PETICIO_TEXTUAL) {
        return respondreErrorOperacio(socket_client, TIPUS_ACCEPTAR_VIATGE, MOTIU_FLAGS_INVALIDS);
    }
    if (longitud == 0 ||
        obtenirDades(peticio, dades_peticio) != TRAMA_CORRECTA) {
        return respondreErrorOperacio(socket_client, TIPUS_ACCEPTAR_VIATGE, MOTIU_DADES_INVALIDES);
    }
    dades_peticio[longitud] = '\0';
    for (i = 0; i < longitud; i++) {
        if (dades_peticio[i] < '0' || dades_peticio[i] > '9') {
            return respondreErrorOperacio(socket_client, TIPUS_ACCEPTAR_VIATGE, MOTIU_DADES_INVALIDES);
        }
    }
    identificador = atoi((char *) dades_peticio);
    if (identificador <= 0) {
        return respondreErrorOperacio(socket_client, TIPUS_ACCEPTAR_VIATGE, MOTIU_DADES_INVALIDES);
    }

    for (i = 0; i < configuracio->nombre_viatges && viatge == NULL; i++) {
        if (configuracio->viatges[i].identificador == identificador) {
            viatge = &configuracio->viatges[i];
        }
    }
    if (viatge == NULL) {
        return respondreErrorOperacio(socket_client, TIPUS_ACCEPTAR_VIATGE, "VOYAGE_NOT_AVAILABLE");
    }

    caracters_escrits = asprintf(&dades_resposta, "OK&%d&%s&%s&%d",viatge->identificador, viatge->nom_objecte,viatge->illa_desti, viatge->recompensa);
    if (caracters_escrits < 0 || caracters_escrits > MIDA_DADES_TRAMA ||copiarText(&nom_assignat, nom_odisseu) != 0) {
        free(dades_resposta);
        free(nom_assignat);
        return respondreErrorOperacio(socket_client, TIPUS_ACCEPTAR_VIATGE, "MEMORY_ERROR");
    }

    pthread_mutex_lock(mutex_viatges);
    if (viatge->disponible == 1) {
        viatge->disponible = 0;
        viatge->odisseu_assignat = nom_assignat;
        nom_assignat = NULL;
        disponible = 1;
    }
    pthread_mutex_unlock(mutex_viatges);

    free(nom_assignat);
    if (disponible == 0) {
        free(dades_resposta);
        return respondreErrorOperacio(socket_client, TIPUS_ACCEPTAR_VIATGE, "VOYAGE_NOT_AVAILABLE");
    }

    resultat = crearTrama(resposta, TIPUS_ACCEPTAR_VIATGE, FLAGS_RESPOSTA_CORRECTA, (unsigned char *) dades_resposta, caracters_escrits);
    free(dades_resposta);
    if (resultat == TRAMA_CORRECTA) {
        resultat = enviarTrama(socket_client, resposta);
    }

    caracters_escrits = asprintf(&missatge,">>> Voyage %d assigned to %s.\n", identificador, nom_odisseu);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }
    return resultat;
}

/***********************************************
 *
 * @Finalitat: Confirma la desconnexio controlada d'un Odysseus.
 * @Parametres: in: socket_client = descriptor del client.
 *              in: nom_odisseu = Odysseus que abandona Ithaca.
 *              in: peticio = trama de desconnexio rebuda.
 *              out: finalitzar_connexio = indica que el thread ha d'acabar.
 * @Retorn: Retorna 0 si envia la resposta i -1 altrament.
 *
 ************************************************/
int gestionarDesconnexio(int socket_client, char *nom_odisseu, unsigned char *peticio, int *finalitzar_connexio) {
    unsigned char resposta[MIDA_TRAMA] = {0}, resposta_ok[] = "OK"; //REVISAR
    char *missatge = NULL;
    int resultat = 0, caracters_escrits = 0;

    if (peticio[POSICIO_FLAGS] != FLAGS_PETICIO_TEXTUAL) {
        return respondreErrorOperacio(socket_client, TIPUS_DESCONNECTAR_ITACA, MOTIU_FLAGS_INVALIDS);
    }
    if (peticio[POSICIO_LONGITUD_DADES] != 0) {
        return respondreErrorOperacio(socket_client,TIPUS_DESCONNECTAR_ITACA, MOTIU_DADES_INVALIDES);
    }

    resultat = crearTrama(resposta, TIPUS_DESCONNECTAR_ITACA,FLAGS_RESPOSTA_CORRECTA, resposta_ok, 2);
    if (resultat == TRAMA_CORRECTA) {
        resultat = enviarTrama(socket_client, resposta);
    }
    if (resultat != TRAMA_CORRECTA) {
        return -1;
    }

    caracters_escrits = asprintf(&missatge, ">>> %s has left Ithaca. Fair winds!\n", nom_odisseu);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }
    *finalitzar_connexio = 1;
    return 0;
}

/***********************************************
 *
 * @Finalitat: Aten la connexio d'un Odysseus amb Ithaca.
 * @Parametres: in: argument = descriptor propi de la connexio.
 * @Retorn: Retorna NULL quan la connexio finalitza.
 *
 ************************************************/
void *atendreOdisseu(void *argument) {
    ParametresOdisseu *parametres = NULL;
    ConfiguracioItaca *configuracio = NULL;
    pthread_mutex_t *mutex_viatges = NULL;
    unsigned char trama[MIDA_TRAMA] = {0}, resposta[MIDA_TRAMA] = {0}; //REVISAR
    unsigned char nom_odisseu[MIDA_DADES_TRAMA + 1] = {0};
    unsigned char resposta_ok[] = "OK";
    char *motiu = NULL, *missatge = NULL;
    int socket_client = -1, resultat = 0, longitud = 0, i = 0;
    int dades_valides = 1, longitud_motiu = 0, caracters_escrits = 0;
    int finalitzar_connexio = 0, viatge_anterior = 0;

    parametres = (ParametresOdisseu *) argument;
    socket_client = parametres->socket_client;
    configuracio = parametres->configuracio;
    mutex_viatges = parametres->mutex_viatges;
    free(parametres);

    resultat = rebreTrama(socket_client, trama);
    if (resultat != TRAMA_CORRECTA) {
        close(socket_client);
        return NULL;
    }

    resultat = validarTrama(trama);
    if (resultat != TRAMA_CORRECTA) {
        if (resultat == TRAMA_ERROR_TIPUS) {
            motiu = MOTIU_TIPUS_INVALID;
        } else if (resultat == TRAMA_ERROR_FLAGS) {
            motiu = MOTIU_FLAGS_INVALIDS;
        } else if (resultat == TRAMA_ERROR_LONGITUD) {
            motiu = MOTIU_LONGITUD_INVALIDA;
        } else {
            motiu = MOTIU_DADES_INVALIDES;
        }
        if (crearNack(resposta, motiu) == TRAMA_CORRECTA) {
            enviarTrama(socket_client, resposta);
        }
        close(socket_client);
        return NULL;
    }

    longitud = trama[POSICIO_LONGITUD_DADES];
    if (longitud > 0) {
        obtenirDades(trama, nom_odisseu);
        for (i = 0; i < longitud; i++) {
            if (nom_odisseu[i] == '\0') {
                dades_valides = 0;
            }
        }
        nom_odisseu[longitud] = '\0';
    }

    if (trama[POSICIO_TIPUS] != TIPUS_CONNECTAR_ITACA) {
        motiu = MOTIU_TIPUS_INVALID;
    } else if (trama[POSICIO_FLAGS] != FLAGS_PETICIO_TEXTUAL) {
        motiu = MOTIU_FLAGS_INVALIDS;
    } else if (longitud == 0 || dades_valides == 0) {
        motiu = MOTIU_DADES_INVALIDES;
    }

    if (motiu != NULL) {
        while (motiu[longitud_motiu] != '\0') {
            longitud_motiu++;
        }
        resultat = crearTrama(resposta, TIPUS_CONNECTAR_ITACA, FLAGS_RESPOSTA_ERROR,(unsigned char *) motiu, longitud_motiu);
        if (resultat == TRAMA_CORRECTA) {
            enviarTrama(socket_client, resposta);
        }
        close(socket_client);
        return NULL;
    }

    resultat = crearTrama(resposta, TIPUS_CONNECTAR_ITACA,FLAGS_RESPOSTA_CORRECTA, resposta_ok, 2);
    if (resultat != TRAMA_CORRECTA ||
        enviarTrama(socket_client, resposta) != TRAMA_CORRECTA) {
        close(socket_client);
        return NULL;
    }

    caracters_escrits = asprintf(&missatge, ">>> %s connected to Ithaca.\n", (char *) nom_odisseu);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }

    pthread_mutex_lock(mutex_viatges);
    for (i = 0; i < configuracio->nombre_viatges &&
                viatge_anterior == 0; i++) {
        if (configuracio->viatges[i].disponible == 0 &&configuracio->viatges[i].fracassat == 0 &&configuracio->viatges[i].odisseu_assignat != NULL &&strcmp(configuracio->viatges[i].odisseu_assignat, (char *) nom_odisseu) == 0) {
            viatge_anterior = 1;
        }
    }
    pthread_mutex_unlock(mutex_viatges);

    resultat = rebreTrama(socket_client, trama);
    while (resultat == TRAMA_CORRECTA && finalitzar_connexio == 0) {
        resultat = validarTrama(trama);
        if (resultat != TRAMA_CORRECTA) {
            if (resultat == TRAMA_ERROR_TIPUS) {
                motiu = MOTIU_TIPUS_INVALID;
            } else if (resultat == TRAMA_ERROR_FLAGS) {
                motiu = MOTIU_FLAGS_INVALIDS;
            } else if (resultat == TRAMA_ERROR_LONGITUD) {
                motiu = MOTIU_LONGITUD_INVALIDA;
            } else {
                motiu = MOTIU_DADES_INVALIDES;
            }
            if (crearNack(resposta, motiu) == TRAMA_CORRECTA) {
                enviarTrama(socket_client, resposta);
            }
            resultat = TRAMA_ERROR_GENERAL;
        } else if (trama[POSICIO_TIPUS] == TIPUS_LLISTAR_VIATGES) {
            resultat = gestionarLlistaViatges(socket_client, configuracio, mutex_viatges, (char *) nom_odisseu, trama, &viatge_anterior);
        } else if (trama[POSICIO_TIPUS] == TIPUS_ACCEPTAR_VIATGE) {
            resultat = gestionarAcceptacio(socket_client, configuracio, mutex_viatges,(char *) nom_odisseu, trama);
        } else if (trama[POSICIO_TIPUS] == TIPUS_DESCONNECTAR_ITACA) {
            resultat = gestionarDesconnexio(socket_client, (char *) nom_odisseu, trama, &finalitzar_connexio);
        } else {
            resultat = crearNack(resposta, MOTIU_TIPUS_INVALID);
            if (resultat == TRAMA_CORRECTA) {
                resultat = enviarTrama(socket_client, resposta);
            }
        }
        if (resultat == TRAMA_CORRECTA && finalitzar_connexio == 0) {
            resultat = rebreTrama(socket_client, trama);
        }
    }

    close(socket_client);
    return NULL;
}

/***********************************************
 *
 * @Finalitat: Carrega la configuracio i els viatges del proces Ithaca.
 * @Parametres: in: argc = nombre d'arguments rebuts.
 *              in: argv = arguments rebuts.
 * @Retorn: Retorna 0 si els arguments son correctes i 1 altrament.
 *
 ************************************************/
int main(int argc, char *argv[]) {
    ConfiguracioItaca configuracio = {0};
    ParametresOdisseu *parametres = NULL;
    FilOdisseu *fils = NULL, *fils_ampliats = NULL;
    pthread_mutex_t mutex_viatges;
    char *missatge = NULL, *nom_programa = argv[0];
    int resultat = 0, caracters_escrits = 0, descriptor_client = -1;
    int nombre_fils = 0, i = 0;

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

    resultat = pthread_mutex_init(&mutex_viatges, NULL);
    if (resultat != 0) {
        alliberarConfiguracioItaca(&configuracio);
        return 1;
    }
    //REVISAR
    signal(SIGINT, gestionarSigint);
    signal(SIGPIPE, SIG_IGN);
    socket_servidor_global = crearServidor(configuracio.ip, configuracio.port);
    if (socket_servidor_global < 0) {
        pthread_mutex_destroy(&mutex_viatges);
        alliberarConfiguracioItaca(&configuracio);
        return 1;
    }

    caracters_escrits = asprintf(&missatge,"Ithaca initialized. %d voyages loaded.\nWaiting for Odysseus...\n",configuracio.nombre_viatges);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }

    while (finalitzar_programa == 0) {
        descriptor_client = esperarConnexio(socket_servidor_global);
        if (descriptor_client >= 0) {
            parametres = malloc(sizeof(*parametres));
            if (parametres == NULL) {
                close(descriptor_client);
            } else {
                fils_ampliats = realloc(fils, (nombre_fils + 1) * sizeof(*fils_ampliats));
                if (fils_ampliats == NULL) {
                    close(descriptor_client);
                    free(parametres);
                    parametres = NULL;
                } else {
                    fils = fils_ampliats;
                    parametres->socket_client = descriptor_client;
                    parametres->configuracio = &configuracio;
                    parametres->mutex_viatges = &mutex_viatges;
                    resultat = pthread_create(&fils[nombre_fils].fil, NULL, atendreOdisseu, parametres);
                    if (resultat == 0) {
                        fils[nombre_fils].socket_client = descriptor_client;
                        nombre_fils++;
                        parametres = NULL;
                    } else {
                        close(descriptor_client);
                        free(parametres);
                        parametres = NULL;
                    }
                }
            }
        } else if (finalitzar_programa == 0) {
            caracters_escrits = asprintf(&missatge, "Error: no s'ha pogut acceptar la connexio.\n");
            if (caracters_escrits >= 0) {
                write(2, missatge, caracters_escrits);
                free(missatge);
            }
        }
    }

    if (socket_servidor_global >= 0) {
        close(socket_servidor_global);
        socket_servidor_global = -1;
    }
    for (i = 0; i < nombre_fils; i++) {
        shutdown(fils[i].socket_client, SHUT_RDWR); //REVISAR
    }
    for (i = 0; i < nombre_fils; i++) {
        pthread_join(fils[i].fil, NULL);
    }
    free(fils);

    caracters_escrits = asprintf(&missatge, "Ithaca closes the harbor.\n");
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }

    pthread_mutex_destroy(&mutex_viatges);
    alliberarConfiguracioItaca(&configuracio);
    return 0;
}

#define _GNU_SOURCE

/***********************************************
 *
 * @Proposit: Conte el punt d'entrada del proces Island.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 18/09/2026
 * @Data ultima modificacio: 25/09/2026
 *
 ************************************************/

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "configuracio_illa.h"
#include "trames.h"
#include "xarxa.h"

typedef struct {
    int socket_client;
    char *nom;
    int atracat;
    int esperant;
    int espera_confirmada;
} ClientIlla;

typedef struct {
    int places_ocupades;
    int nombre_espera;
    ClientIlla **cua_espera;
} EstatPort;

typedef struct {
    int socket_client;
    ConfiguracioIlla *configuracio;
    EstatPort *port;
    pthread_mutex_t *mutex_port;
    pthread_cond_t *condicio_port;
} ParametresClientIlla;

int finalitzar_programa = 0;
int socket_servidor_global = -1;

/***********************************************
 *
 * @Finalitat: Indica que Island ha de finalitzar en rebre SIGINT.
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
 * @Finalitat: Envia l'estat d'una peticio d'arribada al port.
 * @Parametres: in: socket_client = descriptor del client.
 *              in: flags = resultat correcte o error.
 *              in: estat = text que s'envia a Odysseus.
 * @Retorn: Retorna 0 si envia la resposta i -1 altrament.
 *
 ************************************************/
int enviarRespostaArribada(int socket_client, unsigned char flags, char *estat) {
    unsigned char resposta[MIDA_TRAMA] = {0};
    int longitud = 0, resultat = 0;

    while (estat[longitud] != '\0') {
        longitud++;
    }
    resultat = crearTrama(resposta, TIPUS_ARRIBADA_ILLA, flags,(unsigned char *) estat, longitud);
    if (resultat == TRAMA_CORRECTA) {
        resultat = enviarTrama(socket_client, resposta);
    }
    return resultat;
}

/***********************************************
 *
 * @Finalitat: Afegeix un client al final de la cua del port.
 * @Parametres: in/out: port = estat compartit del port.
 *              in: client = client que ha d'esperar.
 * @Retorn: Retorna 0 si l'afegeix i -1 si falla la reserva.
 *
 ************************************************/
int afegirCua(EstatPort *port, ClientIlla *client) {
    ClientIlla **cua_ampliada = NULL;

    cua_ampliada = realloc(port->cua_espera,(port->nombre_espera + 1) *sizeof(*cua_ampliada));
    if (cua_ampliada == NULL) {
        return -1;
    }
    port->cua_espera = cua_ampliada;
    port->cua_espera[port->nombre_espera] = client;
    port->nombre_espera++;
    return 0;
}

/***********************************************
 *
 * @Finalitat: Elimina un client de la cua mantenint l'ordre FIFO.
 * @Parametres: in/out: port = estat compartit del port.
 *              in: client = client que s'ha d'eliminar.
 * @Retorn: ----.
 *
 ************************************************/
void eliminarClientCua(EstatPort *port, ClientIlla *client) {
    int i = 0, posicio = -1;

    for (i = 0; i < port->nombre_espera && posicio < 0; i++) {
        if (port->cua_espera[i] == client) {
            posicio = i;
        }
    }
    if (posicio >= 0) {
        for (i = posicio; i < port->nombre_espera - 1; i++) {
            port->cua_espera[i] = port->cua_espera[i + 1];
        }
        port->nombre_espera--;
    }
}

/***********************************************
 *
 * @Finalitat: Allibera la posicio d'un client i promociona el primer de la cua.
 * @Parametres: in: client = client que ha perdut la connexio.
 *              in/out: port = estat compartit del port.
 *              in: configuracio = capacitat maxima del port.
 *              in: mutex_port = proteccio de l'estat del port.
 *              in: condicio_port = espera fins que WAIT ja s'ha enviat.
 * @Retorn: ----.
 *
 ************************************************/
void alliberarPlaca(ClientIlla *client, EstatPort *port, ConfiguracioIlla *configuracio, pthread_mutex_t *mutex_port, pthread_cond_t *condicio_port) {
    ClientIlla *primer_client = NULL;
    char *nom_promogut = NULL, *missatge = NULL;
    int socket_promogut = -1, caracters_escrits = 0;

    pthread_mutex_lock(mutex_port);
    if (client->esperant == 1) {
        eliminarClientCua(port, client);
        client->esperant = 0;
        pthread_cond_broadcast(condicio_port);
    } else if (client->atracat == 1) {
        client->atracat = 0;
        if (port->places_ocupades > 0) {
            port->places_ocupades--;
        }
        while (port->nombre_espera > 0 &&port->cua_espera[0]->espera_confirmada == 0) {
            pthread_cond_wait(condicio_port, mutex_port);
        }
        if (port->nombre_espera > 0 &&port->places_ocupades < configuracio->capacitat_port) {
            primer_client = port->cua_espera[0];
            eliminarClientCua(port, primer_client);
            primer_client->esperant = 0;
            primer_client->atracat = 1;
            port->places_ocupades++;
            socket_promogut = primer_client->socket_client;
            copiarText(&nom_promogut, primer_client->nom);
        }
    }
    pthread_mutex_unlock(mutex_port);

    caracters_escrits = asprintf(&missatge, ">>> %s has left.\n", client->nom);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
        missatge = NULL;
    }

    if (socket_promogut >= 0) {
        enviarRespostaArribada(socket_promogut, FLAGS_RESPOSTA_CORRECTA, "DOCKED");
        if (nom_promogut != NULL) {
            caracters_escrits = asprintf(&missatge, ">>> DOCKED sent to %s; he may enter the port.\n", nom_promogut);
            if (caracters_escrits >= 0) {
                write(1, missatge, caracters_escrits);
                free(missatge);
            }
        }
    }
    free(nom_promogut);
}

/***********************************************
 *
 * @Finalitat: Aten l'arribada i la connexio d'un Odysseus amb una illa.
 * @Parametres: in: argument = configuracio, port, mutex i socket del client.
 * @Retorn: Retorna NULL quan la connexio finalitza.
 *
 ************************************************/
void *atendreOdisseuIlla(void *argument) {
    ParametresClientIlla *parametres = NULL;
    ConfiguracioIlla *configuracio = NULL;
    EstatPort *port = NULL;
    pthread_mutex_t *mutex_port = NULL;
    pthread_cond_t *condicio_port = NULL;
    ClientIlla *client = NULL;
    unsigned char peticio[MIDA_TRAMA] = {0}, dades[MIDA_DADES_TRAMA + 1] = {0};
    unsigned char nack[MIDA_TRAMA] = {0};
    char *motiu = NULL, *missatge = NULL;
    int socket_client = -1, resultat = 0, longitud = 0, i = 0;
    int dades_valides = 1, caracters_escrits = 0, port_ple = 0;

    parametres = (ParametresClientIlla *) argument;
    socket_client = parametres->socket_client;
    configuracio = parametres->configuracio;
    port = parametres->port;
    mutex_port = parametres->mutex_port;
    condicio_port = parametres->condicio_port;
    free(parametres);

    client = malloc(sizeof(*client));
    if (client == NULL) {
        close(socket_client);
        return NULL;
    }
    client->socket_client = socket_client;
    client->nom = NULL;
    client->atracat = 0;
    client->esperant = 0;
    client->espera_confirmada = 0;

    resultat = rebreTrama(socket_client, peticio);
    if (resultat != TRAMA_CORRECTA) {
        close(socket_client);
        free(client);
        return NULL;
    }
    resultat = validarTrama(peticio);
    if (resultat != TRAMA_CORRECTA || peticio[POSICIO_TIPUS] != TIPUS_ARRIBADA_ILLA) {
        if (resultat == TRAMA_ERROR_FLAGS) {
            motiu = MOTIU_FLAGS_INVALIDS;
        } else if (resultat == TRAMA_ERROR_LONGITUD) {
            motiu = MOTIU_LONGITUD_INVALIDA;
        } else if (resultat == TRAMA_ERROR_DADES) {
            motiu = MOTIU_DADES_INVALIDES;
        } else {
            motiu = MOTIU_TIPUS_INVALID;
        }
        if (crearNack(nack, motiu) == TRAMA_CORRECTA) {
            enviarTrama(socket_client, nack);
        }
        close(socket_client);
        free(client);
        return NULL;
    }

    longitud = peticio[POSICIO_LONGITUD_DADES];
    if (longitud > 0) {
        obtenirDades(peticio, dades);
        for (i = 0; i < longitud; i++) {
            if (dades[i] == '\0') {
                dades_valides = 0;
            }
        }
        dades[longitud] = '\0';
    }
    if (peticio[POSICIO_FLAGS] != FLAGS_PETICIO_TEXTUAL) {
        enviarRespostaArribada(socket_client, FLAGS_RESPOSTA_ERROR, MOTIU_FLAGS_INVALIDS);
        close(socket_client);
        free(client);
        return NULL;
    }
    if (longitud == 0 || dades_valides == 0) {
        enviarRespostaArribada(socket_client, FLAGS_RESPOSTA_ERROR, MOTIU_DADES_INVALIDES);
        close(socket_client);
        free(client);
        return NULL;
    }
    if (copiarText(&client->nom, (char *) dades) != 0) {
        enviarRespostaArribada(socket_client, FLAGS_RESPOSTA_ERROR, "MEMORY_ERROR");
        close(socket_client);
        free(client);
        return NULL;
    }

    caracters_escrits = asprintf(&missatge, ">>> %s approaching %s.\n", client->nom, configuracio->nom);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
        missatge = NULL;
    }

    pthread_mutex_lock(mutex_port);
    if (port->places_ocupades < configuracio->capacitat_port &&
        port->nombre_espera == 0) {
        port->places_ocupades++;
        client->atracat = 1;
    } else if (afegirCua(port, client) == 0) {
        client->esperant = 1;
        port_ple = 1;
    } else {
        resultat = -1;
    }
    pthread_mutex_unlock(mutex_port);

    if (resultat != 0) {
        enviarRespostaArribada(socket_client, FLAGS_RESPOSTA_ERROR, "MEMORY_ERROR");
    } else if (port_ple == 0) {
        resultat = enviarRespostaArribada(socket_client,FLAGS_RESPOSTA_CORRECTA, "DOCKED");
        caracters_escrits = asprintf(&missatge, ">>> %s entered the port.\n", client->nom);
    } else {
        resultat = enviarRespostaArribada(socket_client,FLAGS_RESPOSTA_CORRECTA, "WAIT");
        caracters_escrits = asprintf(&missatge, ">>> Port full. %s waiting offshore in the FIFO queue.\n", client->nom);
    }
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
        missatge = NULL;
    }

    if (port_ple == 1) {
        pthread_mutex_lock(mutex_port);
        if (resultat == TRAMA_CORRECTA) {
            client->espera_confirmada = 1;
        }
        pthread_cond_broadcast(condicio_port);
        pthread_mutex_unlock(mutex_port);
    }

    while (resultat == TRAMA_CORRECTA) {
        resultat = rebreTrama(socket_client, peticio);
    }

    alliberarPlaca(client, port, configuracio, mutex_port, condicio_port);
    close(socket_client);
    free(client->nom);
    free(client);
    return NULL;
}

/***********************************************
 *
 * @Finalitat: Carrega la configuracio i executa el servidor d'una illa.
 * @Parametres: in: argc = nombre d'arguments rebuts.
 *              in: argv = arguments rebuts.
 * @Retorn: Retorna 0 si finalitza correctament i 1 altrament.
 *
 ************************************************/
int main(int argc, char *argv[]) {
    ConfiguracioIlla configuracio = {0};
    EstatPort port = {0};
    ParametresClientIlla *parametres = NULL;
    pthread_t fil;
    pthread_mutex_t mutex_port;
    pthread_cond_t condicio_port;
    char *missatge = NULL, *nom_programa = argv[0];
    int resultat = 0, caracters_escrits = 0, descriptor_client = -1;

    if (argc != 3) {
        caracters_escrits = asprintf(&missatge,
            "Usage: %s <config.dat> <stock.db>\n", nom_programa);
        if (caracters_escrits >= 0) {
            write(2, missatge, caracters_escrits);
            free(missatge);
        }
        return 1;
    }

    resultat = carregarConfiguracioIlla(argv[1], &configuracio);
    if (resultat == 0) {
        resultat = validarRutes(&configuracio);
    }
    if (resultat == 0) {
        resultat = carregarStock(argv[2], &configuracio);
    }
    if (resultat != 0) {
        alliberarConfiguracioIlla(&configuracio);
        return 1;
    }

    resultat = pthread_mutex_init(&mutex_port, NULL);
    if (resultat != 0) {
        alliberarConfiguracioIlla(&configuracio);
        return 1;
    }
    resultat = pthread_cond_init(&condicio_port, NULL);
    if (resultat != 0) {
        pthread_mutex_destroy(&mutex_port);
        alliberarConfiguracioIlla(&configuracio);
        return 1;
    }
    signal(SIGINT, gestionarSigint);
    socket_servidor_global = crearServidor(configuracio.ip, configuracio.port);
    if (socket_servidor_global < 0) {
        pthread_cond_destroy(&condicio_port);
        pthread_mutex_destroy(&mutex_port);
        alliberarConfiguracioIlla(&configuracio);
        return 1;
    }

    if (configuracio.capacitat_port == 1) {
        caracters_escrits = asprintf(&missatge, "Island %s initialized.\nPort capacity: %d ship.\n" "%d sea routes loaded.\n%d products available.\n", configuracio.nom, configuracio.capacitat_port, configuracio.nombre_rutes, configuracio.nombre_productes);
    } else {
        caracters_escrits = asprintf(&missatge,"Island %s initialized.\nPort capacity: %d ships.\n""%d sea routes loaded.\n%d products available.\n",configuracio.nom, configuracio.capacitat_port,configuracio.nombre_rutes, configuracio.nombre_productes);
    }
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
        missatge = NULL;
    }

    while (finalitzar_programa == 0) {
        descriptor_client = esperarConnexio(socket_servidor_global);
        if (descriptor_client >= 0) {
            parametres = malloc(sizeof(*parametres));
            if (parametres == NULL) {
                close(descriptor_client);
            } else {
                parametres->socket_client = descriptor_client;
                parametres->configuracio = &configuracio;
                parametres->port = &port;
                parametres->mutex_port = &mutex_port;
                parametres->condicio_port = &condicio_port;
                resultat = pthread_create(&fil, NULL, atendreOdisseuIlla, parametres);
                if (resultat == 0) {
                    pthread_detach(fil);
                    parametres = NULL;
                } else {
                    close(descriptor_client);
                    free(parametres);
                    parametres = NULL;
                }
            }
        } else if (finalitzar_programa == 0) {
            caracters_escrits = asprintf(&missatge,
                "Error: no s'ha pogut acceptar la connexio.\n");
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
    caracters_escrits = asprintf(&missatge, "%s closes its port.\n", configuracio.nom);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }

    free(port.cua_espera);
    pthread_cond_destroy(&condicio_port);
    pthread_mutex_destroy(&mutex_port);
    alliberarConfiguracioIlla(&configuracio);
    return 0;
}

#define _GNU_SOURCE

/***********************************************
 *
 * @Proposit: Conte el punt d'entrada del proces Island.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 18/09/2026
 * @Data ultima modificacio: 07/10/2026
 *
 ************************************************/

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
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
    pthread_mutex_t *mutex_stock;
    pthread_cond_t *condicio_port;
    char *ruta_stock;
} ParametresClientIlla;

typedef struct {
    pthread_t fil;
    int socket_client;
} FilClientIlla;
//REVISAR
volatile sig_atomic_t finalitzar_programa = 0;
volatile sig_atomic_t socket_servidor_global = -1;

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
int enviarRespostaArribada(int socket_client, unsigned char flags, char *estat) { //REVISAR
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
 * @Finalitat: Envia una resposta de LIST MARKET, BUY o SELL.
 * @Parametres: in: socket_client = descriptor del client.
 *              in: tipus = operacio de mercat que es respon.
 *              in: flags = resultat correcte o error.
 *              in: dades = contingut textual de la resposta.
 * @Retorn: Retorna 0 si envia la resposta i -1 altrament.
 *
 ************************************************/
int enviarRespostaMercat(int socket_client, unsigned char tipus,unsigned char flags, char *dades) { //REVISAR
    unsigned char resposta[MIDA_TRAMA] = {0};
    int longitud = 0, resultat = 0;

    while (dades[longitud] != '\0') {
        longitud++;
    }
    resultat = crearTrama(resposta, tipus, flags, (unsigned char *) dades, longitud);
    if (resultat == TRAMA_CORRECTA) {
        resultat = enviarTrama(socket_client, resposta);
    }
    return resultat;
}

/***********************************************
 *
 * @Finalitat: Extreu el producte i la quantitat d'una compra o venda.
 * @Parametres: in: peticio = trama rebuda del client.
 *              in: tipus = operacio de mercat esperada.
 *              out: dades = buffer que conserva els camps extrets.
 *              out: nom_producte = nom del producte demanat.
 *              out: quantitat = quilograms demanats.
 * @Retorn: Retorna 0 si la peticio es valida i -1 altrament.
 *
 ************************************************/
int obtenirOperacioMercat(unsigned char *peticio, unsigned char tipus, unsigned char *dades, char **nom_producte, int *quantitat) { //REVISAR
    char *quantitat_text = NULL, *camp_extra = NULL;
    int longitud = 0, i = 0;

    if (peticio[POSICIO_TIPUS] != tipus ||peticio[POSICIO_FLAGS] != FLAGS_PETICIO_TEXTUAL) {
        return -1;
    }
    longitud = peticio[POSICIO_LONGITUD_DADES];
    if (longitud == 0 || obtenirDades(peticio, dades) != TRAMA_CORRECTA) {
        return -1;
    }
    for (i = 0; i < longitud; i++) {
        if (dades[i] == '\0') {
            return -1;
        }
    }
    dades[longitud] = '\0';

    *nom_producte = strtok((char *) dades, "&");
    quantitat_text = strtok(NULL, "&");
    camp_extra = strtok(NULL, "&");
    if (*nom_producte == NULL || (*nom_producte)[0] == '\0' ||quantitat_text == NULL || camp_extra != NULL ||quantitat_text[0] == '\0') {
        return -1;
    }
    for (i = 0; quantitat_text[i] != '\0'; i++) {
        if (quantitat_text[i] < '0' || quantitat_text[i] > '9') {
            return -1;
        }
    }
    *quantitat = atoi(quantitat_text);
    if (*quantitat <= 0) {
        return -1;
    }
    return 0;
}

/***********************************************
 *
 * @Finalitat: Envia una copia coherent del mercat i l'entrada del mapa.
 * @Parametres: in: socket_client = descriptor del client.
 *              in: configuracio = productes actuals de l'illa.
 *              in: mutex_stock = proteccio del stock compartit.
 *              in: atracat = indica si el client ocupa una placa.
 *              in: peticio = trama LIST MARKET rebuda.
 * @Retorn: Retorna 0 si respon correctament i -1 altrament.
 *
 ************************************************/
int gestionarLlistaMercat(int socket_client, ConfiguracioIlla *configuracio, pthread_mutex_t *mutex_stock, int atracat, unsigned char *peticio) { //REVISAR
    Producte *copia_productes = NULL;
    char *dades = NULL;
    int nombre_productes = 0, nombre_total = 0, resultat = 0;
    int i = 0, longitud = 0;

    if (atracat == 0) {
        return enviarRespostaMercat(socket_client, TIPUS_LLISTAR_MERCAT, FLAGS_RESPOSTA_ERROR, "NOT_DOCKED");
    }
    if (peticio[POSICIO_FLAGS] != FLAGS_PETICIO_TEXTUAL ||
        peticio[POSICIO_LONGITUD_DADES] != 0) {
        return enviarRespostaMercat(socket_client, TIPUS_LLISTAR_MERCAT, FLAGS_RESPOSTA_ERROR, "INVALID_DATA");
    }

    pthread_mutex_lock(mutex_stock);
    nombre_productes = configuracio->nombre_productes;
    if (nombre_productes > 0) {
        copia_productes = malloc(nombre_productes * sizeof(*copia_productes));
        if (copia_productes != NULL) {
            for (i = 0; i < nombre_productes; i++) {
                copia_productes[i] = configuracio->productes[i];
            }
        }
    }
    pthread_mutex_unlock(mutex_stock);

    if (nombre_productes > 0 && copia_productes == NULL) {
        return enviarRespostaMercat(socket_client, TIPUS_LLISTAR_MERCAT, FLAGS_RESPOSTA_ERROR, "MEMORY_ERROR");
    }

    nombre_total = nombre_productes + 1;
    for (i = 0; i < nombre_productes && resultat == TRAMA_CORRECTA; i++) {
        longitud = asprintf(&dades, "%d&%d&%s&%d&%d", i + 1,
            nombre_total, copia_productes[i].nom,
            copia_productes[i].quantitat, copia_productes[i].preu);
        if (longitud < 0 || longitud > MIDA_DADES_TRAMA) {
            resultat = -1;
        } else {
            resultat = enviarRespostaMercat(socket_client,TIPUS_LLISTAR_MERCAT, FLAGS_RESPOSTA_CORRECTA, dades);
        }
        free(dades);
        dades = NULL;
    }
    if (resultat == TRAMA_CORRECTA) {
        longitud = asprintf(&dades, "%d&%d&MAP&1&50", nombre_total, nombre_total);
        if (longitud < 0 || longitud > MIDA_DADES_TRAMA) {
            resultat = -1;
        } else {
            resultat = enviarRespostaMercat(socket_client, TIPUS_LLISTAR_MERCAT, FLAGS_RESPOSTA_CORRECTA, dades);
        }
        free(dades);
    }
    free(copia_productes);
    return resultat;
}

/***********************************************
 *
 * @Finalitat: Processa una compra i actualitza memoria i stock.db.
 * @Parametres: in: socket_client = descriptor del client.
 *              in/out: configuracio = stock actual de l'illa.
 *              in: mutex_stock = proteccio del stock compartit.
 *              in: ruta_stock = ruta del fitxer binari.
 *              in: atracat = indica si el client ocupa una placa.
 *              in: peticio = trama BUY rebuda.
 * @Retorn: Retorna 0 si respon correctament i -1 altrament.
 *
 ************************************************/
int gestionarCompra(int socket_client, ConfiguracioIlla *configuracio,pthread_mutex_t *mutex_stock, char *ruta_stock, int atracat, unsigned char *peticio) {
    unsigned char dades_peticio[MIDA_DADES_TRAMA + 1] = {0}; //REVISAR
    char *nom_producte = NULL, *dades_resposta = NULL, *motiu = NULL;
    int quantitat = 0, index_producte = -1, quantitat_anterior = 0;
    int cost = 0, resultat = 0, longitud = 0, i = 0;

    if (atracat == 0) {
        return enviarRespostaMercat(socket_client, TIPUS_COMPRAR, FLAGS_RESPOSTA_ERROR, "NOT_DOCKED");
    }
    resultat = obtenirOperacioMercat(peticio, TIPUS_COMPRAR, dades_peticio, &nom_producte, &quantitat);
    if (resultat != 0 || strcasecmp(nom_producte, "MAP") == 0) {
        return enviarRespostaMercat(socket_client, TIPUS_COMPRAR, FLAGS_RESPOSTA_ERROR, "INVALID_DATA");
    }

    pthread_mutex_lock(mutex_stock);
    for (i = 0; i < configuracio->nombre_productes &&index_producte < 0; i++) {
        if (strcasecmp(configuracio->productes[i].nom,nom_producte) == 0) {
            index_producte = i;
        }
    }
    if (index_producte < 0) {
        motiu = "UNKNOWN_PRODUCT";
    } else if (configuracio->productes[index_producte].quantitat <
               quantitat) {
        motiu = "INSUFFICIENT_STOCK";
    } else {
        quantitat_anterior = configuracio->productes[index_producte].quantitat;
        cost = quantitat * configuracio->productes[index_producte].preu;
        configuracio->productes[index_producte].quantitat = configuracio->productes[index_producte].quantitat - quantitat;
        if (guardarStock(ruta_stock, configuracio) != 0) {
            configuracio->productes[index_producte].quantitat = quantitat_anterior;
            guardarStock(ruta_stock, configuracio);
            motiu = "STOCK_FILE_ERROR";
        }
    }
    pthread_mutex_unlock(mutex_stock);

    if (motiu != NULL) {
        return enviarRespostaMercat(socket_client, TIPUS_COMPRAR, FLAGS_RESPOSTA_ERROR, motiu);
    }
    longitud = asprintf(&dades_resposta, "OK&%s&%d&%d", configuracio->productes[index_producte].nom, quantitat, cost);
    if (longitud < 0 || longitud > MIDA_DADES_TRAMA) {
        free(dades_resposta);
        return -1;
    }
    resultat = enviarRespostaMercat(socket_client, TIPUS_COMPRAR,FLAGS_RESPOSTA_CORRECTA,dades_resposta);
    free(dades_resposta);
    return resultat;
}

/***********************************************
 *
 * @Finalitat: Processa una venda i actualitza memoria i stock.db.
 * @Parametres: in: socket_client = descriptor del client.
 *              in/out: configuracio = stock actual de l'illa.
 *              in: mutex_stock = proteccio del stock compartit.
 *              in: ruta_stock = ruta del fitxer binari.
 *              in: atracat = indica si el client ocupa una placa.
 *              in: peticio = trama SELL rebuda.
 * @Retorn: Retorna 0 si respon correctament i -1 altrament.
 *
 ************************************************/
int gestionarVenda(int socket_client, ConfiguracioIlla *configuracio, pthread_mutex_t *mutex_stock, char *ruta_stock, int atracat, unsigned char *peticio) {
    unsigned char dades_peticio[MIDA_DADES_TRAMA + 1] = {0}; //REVISAR
    char *nom_producte = NULL, *dades_resposta = NULL, *motiu = NULL;
    int quantitat = 0, index_producte = -1, quantitat_anterior = 0;
    int ingres = 0, resultat = 0, longitud = 0, i = 0;

    if (atracat == 0) {
        return enviarRespostaMercat(socket_client, TIPUS_VENDRE, FLAGS_RESPOSTA_ERROR, "NOT_DOCKED");
    }
    resultat = obtenirOperacioMercat(peticio, TIPUS_VENDRE, dades_peticio, &nom_producte, &quantitat);
    if (resultat != 0 || strcasecmp(nom_producte, "MAP") == 0) {
        return enviarRespostaMercat(socket_client, TIPUS_VENDRE,FLAGS_RESPOSTA_ERROR, "INVALID_DATA");
    }

    pthread_mutex_lock(mutex_stock);
    for (i = 0; i < configuracio->nombre_productes && index_producte < 0; i++) {
        if (strcasecmp(configuracio->productes[i].nom,nom_producte) == 0) {
            index_producte = i;
        }
    }
    if (index_producte < 0) {
        motiu = "UNKNOWN_PRODUCT";
    } else {
        quantitat_anterior = configuracio->productes[index_producte].quantitat;
        ingres = quantitat * configuracio->productes[index_producte].preu;
        configuracio->productes[index_producte].quantitat =configuracio->productes[index_producte].quantitat + quantitat;
        if (guardarStock(ruta_stock, configuracio) != 0) {
            configuracio->productes[index_producte].quantitat =quantitat_anterior;
            guardarStock(ruta_stock, configuracio);
            motiu = "STOCK_FILE_ERROR";
        }
    }
    pthread_mutex_unlock(mutex_stock);

    if (motiu != NULL) {
        return enviarRespostaMercat(socket_client, TIPUS_VENDRE, FLAGS_RESPOSTA_ERROR, motiu);
    }
    longitud = asprintf(&dades_resposta, "OK&%s&%d&%d", configuracio->productes[index_producte].nom, quantitat, ingres);
    if (longitud < 0 || longitud > MIDA_DADES_TRAMA) {
        free(dades_resposta);
        return -1;
    }
    resultat = enviarRespostaMercat(socket_client, TIPUS_VENDRE, FLAGS_RESPOSTA_CORRECTA,dades_resposta);
    free(dades_resposta);
    return resultat;
}

/***********************************************
 *
 * @Finalitat: Envia les rutes valides del mapa local en una compra 0x14.
 * @Parametres: in: socket_client = descriptor del client.
 *              in: configuracio = rutes filtrades de l'illa.
 *              in: atracat = indica si el client ocupa una placa.
 *              in: nom_odisseu = nom utilitzat al log de l'illa.
 *              in: peticio = trama BUY MAP rebuda.
 * @Retorn: Retorna 0 si respon correctament i -1 altrament.
 *
 ************************************************/
int gestionarCompraMapa(int socket_client, ConfiguracioIlla *configuracio,int atracat, char *nom_odisseu, unsigned char *peticio) {
    unsigned char dades_peticio[MIDA_DADES_TRAMA + 1] = {0}; // REVISAR
    char *dades_resposta = NULL, *missatge = NULL;
    int longitud = 0, resultat = 0, i = 0, caracters_escrits = 0;

    if (atracat == 0) {
        return enviarRespostaMercat(socket_client, TIPUS_COMPRAR_MAPA, FLAGS_RESPOSTA_ERROR, "NOT_DOCKED");
    }
    longitud = peticio[POSICIO_LONGITUD_DADES];
    if (peticio[POSICIO_FLAGS] != FLAGS_PETICIO_TEXTUAL || longitud != 1 ||obtenirDades(peticio, dades_peticio) != TRAMA_CORRECTA ||dades_peticio[0] != '1') {
        return enviarRespostaMercat(socket_client, TIPUS_COMPRAR_MAPA, FLAGS_RESPOSTA_ERROR, "INVALID_DATA");
    }

    if (configuracio->nombre_rutes == 0) {
        resultat = enviarRespostaMercat(socket_client, TIPUS_COMPRAR_MAPA, FLAGS_RESPOSTA_CORRECTA, "OK&50&0");
    }
    for (i = 0; i < configuracio->nombre_rutes &&resultat == TRAMA_CORRECTA; i++) {
        longitud = asprintf(&dades_resposta, "OK&50&%d&%d&%s&%s&%d",i + 1, configuracio->nombre_rutes,configuracio->rutes[i].nom_illa, configuracio->rutes[i].ip,configuracio->rutes[i].port);
        if (longitud < 0 || longitud > MIDA_DADES_TRAMA) {
            resultat = -1;
        } else {
            resultat = enviarRespostaMercat(socket_client,TIPUS_COMPRAR_MAPA, FLAGS_RESPOSTA_CORRECTA,dades_resposta);
        }
        free(dades_resposta);
        dades_resposta = NULL;
    }
    if (resultat == TRAMA_CORRECTA) {
        caracters_escrits = asprintf(&missatge, ">>> %s bought the local map.\n", nom_odisseu);
        if (caracters_escrits >= 0) {
            write(1, missatge, caracters_escrits);
            free(missatge);
        }
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
 *              in: confirmar_sortida = indica si s'ha de respondre 0x16.
 * @Retorn: Retorna 0 si allibera i notifica correctament i -1 altrament.
 *
 ************************************************/
int alliberarPlaca(ClientIlla *client, EstatPort *port, ConfiguracioIlla *configuracio, pthread_mutex_t *mutex_port,pthread_cond_t *condicio_port, int confirmar_sortida) {
    ClientIlla *primer_client = NULL;
    char *nom_promogut = NULL, *missatge = NULL;
    unsigned char resposta[MIDA_TRAMA] = {0}; //REVISAR
    int socket_promogut = -1, caracters_escrits = 0, resultat = 0;

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

    if (confirmar_sortida == 1) {
        resultat = crearTrama(resposta, TIPUS_SORTIDA_ILLA, FLAGS_RESPOSTA_CORRECTA, (unsigned char *) "OK", 2);
        if (resultat == TRAMA_CORRECTA) {
            resultat = enviarTrama(client->socket_client, resposta);
        }
    }

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
    return resultat;
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
    pthread_mutex_t *mutex_port = NULL, *mutex_stock = NULL;
    pthread_cond_t *condicio_port = NULL;
    ClientIlla *client = NULL;
    unsigned char peticio[MIDA_TRAMA] = {0}, dades[MIDA_DADES_TRAMA + 1] = {0}; //REVISAR
    unsigned char nack[MIDA_TRAMA] = {0};
    char *motiu = NULL, *missatge = NULL, *ruta_stock = NULL;
    int socket_client = -1, resultat = 0, longitud = 0, i = 0;
    int dades_valides = 1, caracters_escrits = 0, port_ple = 0;
    int client_atracat = 0, finalitzar_connexio = 0;
    int sortida_controlada = 0;

    parametres = (ParametresClientIlla *) argument;
    socket_client = parametres->socket_client;
    configuracio = parametres->configuracio;
    port = parametres->port;
    mutex_port = parametres->mutex_port;
    mutex_stock = parametres->mutex_stock;
    condicio_port = parametres->condicio_port;
    ruta_stock = parametres->ruta_stock;
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

    while (resultat == TRAMA_CORRECTA && finalitzar_connexio == 0) {
        resultat = rebreTrama(socket_client, peticio);
        if (resultat == TRAMA_CORRECTA) {
            resultat = validarTrama(peticio);
        }
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
            if (crearNack(nack, motiu) == TRAMA_CORRECTA) {
                enviarTrama(socket_client, nack);
            }
        } else {
            pthread_mutex_lock(mutex_port);
            client_atracat = client->atracat;
            pthread_mutex_unlock(mutex_port);

            if (peticio[POSICIO_TIPUS] == TIPUS_LLISTAR_MERCAT) {
                resultat = gestionarLlistaMercat(socket_client, configuracio, mutex_stock, client_atracat, peticio);
            } else if (peticio[POSICIO_TIPUS] == TIPUS_COMPRAR) {
                resultat = gestionarCompra(socket_client, configuracio, mutex_stock, ruta_stock, client_atracat, peticio);
            } else if (peticio[POSICIO_TIPUS] == TIPUS_VENDRE) {
                resultat = gestionarVenda(socket_client, configuracio,mutex_stock, ruta_stock, client_atracat, peticio);
            } else if (peticio[POSICIO_TIPUS] == TIPUS_COMPRAR_MAPA) {
                resultat = gestionarCompraMapa(socket_client, configuracio, client_atracat, client->nom, peticio);
            } else if (peticio[POSICIO_TIPUS] == TIPUS_SORTIDA_ILLA) {
                if (client_atracat == 0) {
                    resultat = enviarRespostaMercat(socket_client,TIPUS_SORTIDA_ILLA, FLAGS_RESPOSTA_ERROR,"NOT_DOCKED");
                } else if (peticio[POSICIO_FLAGS] != FLAGS_PETICIO_TEXTUAL || peticio[POSICIO_LONGITUD_DADES] != 0) {
                    resultat = enviarRespostaMercat(socket_client,TIPUS_SORTIDA_ILLA, FLAGS_RESPOSTA_ERROR,"INVALID_DATA");
                } else {
                    sortida_controlada = 1;
                    finalitzar_connexio = 1;
                    resultat = alliberarPlaca(client, port, configuracio, mutex_port, condicio_port, 1);
                }
            } else {
                resultat = crearNack(nack, MOTIU_TIPUS_INVALID);
                if (resultat == TRAMA_CORRECTA) {
                    resultat = enviarTrama(socket_client, nack);
                }
            }
        }
    }

    if (sortida_controlada == 0) {
        alliberarPlaca(client, port, configuracio, mutex_port,condicio_port, 0);
    }
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
    FilClientIlla *fils = NULL, *fils_ampliats = NULL;
    pthread_mutex_t mutex_port, mutex_stock;
    pthread_cond_t condicio_port;
    char *missatge = NULL, *nom_programa = argv[0];
    int resultat = 0, caracters_escrits = 0, descriptor_client = -1;
    int nombre_fils = 0, i = 0;

    if (argc != 3) {
        caracters_escrits = asprintf(&missatge,"Usage: %s <config.dat> <stock.db>\n", nom_programa);
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
    resultat = pthread_mutex_init(&mutex_stock, NULL);
    if (resultat != 0) {
        pthread_mutex_destroy(&mutex_port);
        alliberarConfiguracioIlla(&configuracio);
        return 1;
    }
    resultat = pthread_cond_init(&condicio_port, NULL);
    if (resultat != 0) {
        pthread_mutex_destroy(&mutex_stock);
        pthread_mutex_destroy(&mutex_port);
        alliberarConfiguracioIlla(&configuracio);
        return 1;
    }
    signal(SIGINT, gestionarSigint);
    signal(SIGPIPE, SIG_IGN); //REVISAR
    socket_servidor_global = crearServidor(configuracio.ip, configuracio.port);
    if (socket_servidor_global < 0) {
        pthread_cond_destroy(&condicio_port);
        pthread_mutex_destroy(&mutex_stock);
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
                fils_ampliats = realloc(fils, (nombre_fils + 1) * sizeof(*fils_ampliats));
                if (fils_ampliats == NULL) {
                    close(descriptor_client);
                    free(parametres);
                    parametres = NULL;
                } else {
                    fils = fils_ampliats;
                    parametres->socket_client = descriptor_client;
                    parametres->configuracio = &configuracio;
                    parametres->port = &port;
                    parametres->mutex_port = &mutex_port;
                    parametres->mutex_stock = &mutex_stock;
                    parametres->condicio_port = &condicio_port;
                    parametres->ruta_stock = argv[2];
                    resultat = pthread_create(&fils[nombre_fils].fil, NULL,atendreOdisseuIlla, parametres);
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
    caracters_escrits = asprintf(&missatge, "%s closes its port.\n", configuracio.nom);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }

    free(port.cua_espera);
    pthread_cond_destroy(&condicio_port);
    pthread_mutex_destroy(&mutex_stock);
    pthread_mutex_destroy(&mutex_port);
    alliberarConfiguracioIlla(&configuracio);
    return 0;
}

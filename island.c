#define _GNU_SOURCE

/***********************************************
 *
 * @Proposit: Conte el servidor concurrent i el port del proces Island.
 * @Autor/s: Antonio Duran Sabates i Arnau González Hortelano
 * @Data creacio: 18/09/2026
 * @Data ultima modificacio: 15/10/2026
 *
 ************************************************/

#include <signal.h>
#include <limits.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>

#include "configuracio_illa.h"
#include "trames.h"
#include "xarxa.h"

int finalitzar_programa = 0;
int fd_servidor = -1;

typedef struct {
    int fd_client;
    char nom[MIDA_NOM_ODYSSEUS];
    int espera_notificada;
} VaixellPort;

typedef struct {
    int places_ocupades;
    VaixellPort *atracats;
    int num_atracats;
    VaixellPort *fifo;
    int num_fifo;
} EstatPort;

typedef struct {
    int fd_client;
    ConfiguracioIlla *configuracio;
    EstatPort *port;
    pthread_mutex_t *mutex_port;
    pthread_mutex_t *mutex_stock;
    char *ruta_stock;
} DadesClientIsland;

/***********************************************
 * @Finalitat: Envia les rutes ja filtrades com una unica compra del mapa local.
 * @Parametres: in: fd_client = socket; peticio = BUY MAP; nom = comprador.
 *              in: configuracio = rutes valides de l'illa.
 * @Retorn: Retorna 0 si envia la resposta i -1 si falla la connexio.
 ************************************************/
int gestionarBuyMap(int fd_client, unsigned char *peticio, char *nom,
                     ConfiguracioIlla *configuracio) {
    unsigned char trama[MIDA_TRAMA];
    char dades[MIDA_DADES + 1], missatge[MIDA_NOM_ODYSSEUS + 60], *error = NULL;
    int i = 0, longitud = 0, resultat = 0, total = configuracio->nombre_rutes;

    if (peticio[POSICIO_LONGITUD] == 0 || peticio[POSICIO_LONGITUD] > MIDA_DADES) {
        return enviarNack(fd_client, "INVALID_LENGTH");
    }
    memcpy(dades, peticio + POSICIO_DADES, peticio[POSICIO_LONGITUD]);
    dades[peticio[POSICIO_LONGITUD]] = '\0';
    if ((int)strlen(dades) != peticio[POSICIO_LONGITUD] || esNumero(dades) == 0) {
        return enviarNack(fd_client, "INVALID_DATA");
    }
    if (peticio[POSICIO_FLAGS] != FLAG_PETICIO || peticio[POSICIO_LONGITUD] != 1 ||
        peticio[POSICIO_DADES] != '1') {
        error = "Quantitat de mapa invalida";
    }
    for (i = 0; i < total && error == NULL; i++) {
        longitud = snprintf(dades, sizeof(dades), "OK&50&%d&%d&%s&%s&%d", i + 1, total,
                             configuracio->rutes[i].nom_illa, configuracio->rutes[i].ip,
                             configuracio->rutes[i].port);
        if (longitud > MIDA_DADES) {
            error = "Ruta massa llarga";
        }
    }
    if (error != NULL) {
        crearTrama(trama, TIPUS_BUY_MAP, FLAG_RESPOSTA_ERROR,
                   (unsigned char *)error, strlen(error));
        return enviarTrama(fd_client, trama);
    }
    if (total == 0) {
        crearTrama(trama, TIPUS_BUY_MAP, FLAG_RESPOSTA_OK, (unsigned char *)"OK&50&0", 7);
        resultat = enviarTrama(fd_client, trama);
    }
    for (i = 0; i < total && resultat == 0; i++) {
        longitud = snprintf(dades, sizeof(dades), "OK&50&%d&%d&%s&%s&%d", i + 1, total,
                             configuracio->rutes[i].nom_illa, configuracio->rutes[i].ip,
                             configuracio->rutes[i].port);
        crearTrama(trama, TIPUS_BUY_MAP, FLAG_RESPOSTA_OK, (unsigned char *)dades, longitud);
        resultat = enviarTrama(fd_client, trama);
    }
    if (resultat == 0) {
        longitud = snprintf(missatge, sizeof(missatge), ">>> %s bought the local map.\n", nom);
        write(1, missatge, longitud);
    }
    return resultat;
}

/***********************************************
 * @Finalitat: Envia una copia coherent dels aliments i l'entrada virtual MAP.
 * @Parametres: in: fd_client = socket; peticio = trama LIST MARKET.
 *              in: configuracio = productes de l'illa.
 *              in/out: mutex_stock = mutex compartit del stock.
 * @Retorn: Retorna 0 si envia la resposta i -1 si falla la connexio.
 ************************************************/
int gestionarListMarket(int fd_client, unsigned char *peticio,
                        ConfiguracioIlla *configuracio, pthread_mutex_t *mutex_stock) {
    Producte *copia_productes = NULL;
    unsigned char trama[MIDA_TRAMA];
    char dades[MIDA_DADES + 1], *error = NULL;
    int i = 0, total = configuracio->nombre_productes + 1, resultat = 0, longitud = 0;

    if (peticio[POSICIO_LONGITUD] != 0) {
        return enviarNack(fd_client, "INVALID_LENGTH");
    }
    if (total > 1) {
        copia_productes = malloc((total - 1) * sizeof(*copia_productes));
        if (copia_productes == NULL) {
            error = "Sense memoria";
        } else {
            pthread_mutex_lock(mutex_stock);
            memcpy(copia_productes, configuracio->productes, (total - 1) * sizeof(*copia_productes));
            pthread_mutex_unlock(mutex_stock);
        }
    }
    if (error != NULL) {
        crearTrama(trama, TIPUS_LIST_MARKET, FLAG_RESPOSTA_ERROR,
                   (unsigned char *)error, strlen(error));
        return enviarTrama(fd_client, trama);
    }
    for (i = 0; i < total && resultat == 0; i++) {
        if (i == total - 1) {
            longitud = snprintf(dades, sizeof(dades), "%d&%d&MAP&1&50", i + 1, total);
        } else {
            longitud = snprintf(dades, sizeof(dades), "%d&%d&%s&%d&%d", i + 1, total,
                                 copia_productes[i].nom, copia_productes[i].quantitat,
                                 copia_productes[i].preu);
        }
        resultat = crearTrama(trama, TIPUS_LIST_MARKET, FLAG_RESPOSTA_OK,
                               (unsigned char *)dades, longitud);
        if (resultat == 0) {
            resultat = enviarTrama(fd_client, trama);
        }
    }
    free(copia_productes);
    return resultat;
}

/***********************************************
 * @Finalitat: Compra o ven aliments mantenint junts la comprovacio i el canvi.
 * @Parametres: in: fd_client = socket; peticio = BUY o SELL; nom = comprador.
 *              in/out: configuracio = stock; mutex_stock = mutex del stock.
 *              in: ruta_stock = fitxer binari que conserva les quantitats.
 * @Retorn: Retorna 0 si envia la resposta i -1 si falla la connexio.
 ************************************************/
int gestionarCompraVenda(int fd_client, unsigned char *peticio, char *nom,
                         ConfiguracioIlla *configuracio, pthread_mutex_t *mutex_stock,
                         char *ruta_stock) {
    unsigned char trama[MIDA_TRAMA], tipus = peticio[POSICIO_TIPUS];
    char dades[MIDA_DADES + 1], *quantitat_text = NULL, *error = "Peticio invalida";
    char missatge[MIDA_NOM_ODYSSEUS + MIDA_NOM_PRODUCTE + 100], *accio = "bought";
    int longitud = peticio[POSICIO_LONGITUD], quantitat = 0, i = 0, index = -1;
    int nova_quantitat = 0, import = 0, valida = 1, format_valid = 0;

    if (longitud == 0 || longitud > MIDA_DADES) {
        return enviarNack(fd_client, "INVALID_LENGTH");
    }
    if (peticio[POSICIO_FLAGS] == FLAG_PETICIO && longitud > 0 && longitud <= MIDA_DADES) {
        memcpy(dades, peticio + POSICIO_DADES, longitud);
        dades[longitud] = '\0';
        quantitat_text = strchr(dades, '&');
        if ((int)strlen(dades) == longitud && quantitat_text != NULL) {
            *quantitat_text = '\0';
            quantitat_text++;
            valida = esNumero(quantitat_text);
            if (valida == 1 && dades[0] != '\0') {
                format_valid = 1;
            }
            for (i = 0; valida == 1 && quantitat_text[i] != '\0'; i++) {
                if (quantitat > (INT_MAX - (quantitat_text[i] - '0')) / 10) {
                    valida = 0;
                } else {
                    quantitat = quantitat * 10 + quantitat_text[i] - '0';
                }
            }
            if (valida == 1 && quantitat > 0 && dades[0] != '\0' &&
                strlen(dades) < MIDA_NOM_PRODUCTE && strcasecmp(dades, "MAP") != 0) {
                error = "Producte desconegut";
                pthread_mutex_lock(mutex_stock);
                for (i = 0; i < configuracio->nombre_productes; i++) {
                    if (strcmp(configuracio->productes[i].nom, dades) == 0) {
                        index = i;
                    }
                }
                if (index >= 0) {
                    error = "Quantitat fora de rang";
                    if (configuracio->productes[index].preu >= 0 &&
                        (configuracio->productes[index].preu == 0 ||
                         quantitat <= INT_MAX / configuracio->productes[index].preu)) {
                        import = quantitat * configuracio->productes[index].preu;
                        nova_quantitat = configuracio->productes[index].quantitat;
                        if (tipus == TIPUS_BUY && nova_quantitat >= quantitat) {
                            nova_quantitat -= quantitat;
                            error = NULL;
                        } else if (tipus == TIPUS_BUY) {
                            error = "Stock insuficient";
                        } else if (nova_quantitat <= INT_MAX - quantitat) {
                            nova_quantitat += quantitat;
                            error = NULL;
                        }
                        if (error == NULL) {
                            if (actualitzarQuantitatStock(ruta_stock, index, nova_quantitat) == 0) {
                                configuracio->productes[index].quantitat = nova_quantitat;
                            } else {
                                error = "Error escrivint stock";
                            }
                        }
                    }
                }
                pthread_mutex_unlock(mutex_stock);
            }
        }
    }
    if (format_valid == 0) {
        return enviarNack(fd_client, "INVALID_DATA");
    }
    if (error != NULL) {
        crearTrama(trama, tipus, FLAG_RESPOSTA_ERROR, (unsigned char *)error, strlen(error));
    } else {
        if (tipus == TIPUS_SELL) {
            accio = "sold";
        }
        longitud = snprintf(missatge, sizeof(missatge), ">>> %s %s %d kg of %s.\n",
                             nom, accio, quantitat, dades);
        write(1, missatge, longitud);
        longitud = snprintf(dades, sizeof(dades), "OK&%s&%d&%d",
                             configuracio->productes[index].nom, quantitat, import);
        crearTrama(trama, tipus, FLAG_RESPOSTA_OK, (unsigned char *)dades, longitud);
    }
    return enviarTrama(fd_client, trama);
}

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
 * @Finalitat: Reserva una placa o afegeix el vaixell al final de la FIFO.
 * @Parametres: in/out: port = estat compartit del port.
 *              in: capacitat = nombre de places configurades.
 *              in: vaixell = vaixell que arriba.
 *              in/out: mutex_port = mutex que protegeix tot el port.
 * @Retorn: Retorna 1 si atraca, 2 si espera i -1 si falla la reserva.
 *
 ************************************************/
int registrarArribada(EstatPort *port, int capacitat, VaixellPort *vaixell,
                      pthread_mutex_t *mutex_port) {
    VaixellPort *temporal = NULL;
    int resultat = -1;

    pthread_mutex_lock(mutex_port);
    if (port->places_ocupades < capacitat && port->num_fifo == 0) {
        temporal = realloc(port->atracats, (port->num_atracats + 1) * sizeof(*temporal));
        if (temporal != NULL) {
            port->atracats = temporal;
            port->atracats[port->num_atracats] = *vaixell;
            port->num_atracats++;
            port->places_ocupades++;
            resultat = 1;
        }
    } else {
        temporal = realloc(port->fifo, (port->num_fifo + 1) * sizeof(*temporal));
        if (temporal != NULL) {
            port->fifo = temporal;
            port->fifo[port->num_fifo] = *vaixell;
            port->num_fifo++;
            resultat = 2;
        }
    }
    pthread_mutex_unlock(mutex_port);
    return resultat;
}

/***********************************************
 *
 * @Finalitat: Notifica al vaixell promogut que ja pot entrar al port.
 * @Parametres: in: vaixell = copia local del vaixell que ha obtingut placa.
 * @Retorn: Retorna 0 si envia DOCKED i -1 si falla l'enviament.
 *
 ************************************************/
int notificarEntradaPort(VaixellPort *vaixell) {
    unsigned char trama[MIDA_TRAMA];
    char missatge[MIDA_NOM_ODYSSEUS + 100];
    int longitud = 0;

    crearTrama(trama, TIPUS_ARRIBADA_ILLA, FLAG_RESPOSTA_OK, (unsigned char *)"DOCKED", 6);
    if (enviarTrama(vaixell->fd_client, trama) != 0) {
        return -1;
    }
    longitud = snprintf(missatge, sizeof(missatge),
                         ">>> DOCKED sent to %s; he may enter the port.\n", vaixell->nom);
    write(1, missatge, longitud);
    return 0;
}

/***********************************************
 *
 * @Finalitat: Registra que WAIT ja s'ha enviat i comprova si ja ha estat promogut.
 * @Parametres: in/out: port = estat compartit del port.
 *              in: fd_client = descriptor del vaixell que ha rebut WAIT.
 *              in/out: mutex_port = mutex que protegeix tot el port.
 * @Retorn: Retorna 1 si aquest fil ha d'enviar DOCKED i 0 altrament.
 *
 ************************************************/
int confirmarEspera(EstatPort *port, int fd_client, pthread_mutex_t *mutex_port) {
    int i = 0, promogut = 0;

    /* La promocio no pot enviar DOCKED abans que aquest fil acabi d'enviar WAIT. */
    pthread_mutex_lock(mutex_port);
    for (i = 0; i < port->num_fifo; i++) {
        if (port->fifo[i].fd_client == fd_client) {
            port->fifo[i].espera_notificada = 1;
        }
    }
    for (i = 0; i < port->num_atracats; i++) {
        if (port->atracats[i].fd_client == fd_client &&
            port->atracats[i].espera_notificada == 0) {
            port->atracats[i].espera_notificada = 1;
            promogut = 1;
        }
    }
    pthread_mutex_unlock(mutex_port);
    return promogut;
}

/***********************************************
 *
 * @Finalitat: Elimina un vaixell desconnectat i reserva la placa al primer FIFO.
 * @Parametres: in/out: port = estat compartit del port.
 *              in: fd_client = descriptor del vaixell que marxa.
 *              in/out: mutex_port = mutex que protegeix tot el port.
 * @Retorn: ----.
 *
 ************************************************/
void alliberarPlacaPort(EstatPort *port, int fd_client, pthread_mutex_t *mutex_port) {
    VaixellPort promogut = {0};
    int i = 0, j = 0;

    promogut.fd_client = -1;
    pthread_mutex_lock(mutex_port);
    for (i = 0; i < port->num_fifo; i++) {
        if (port->fifo[i].fd_client == fd_client) {
            for (j = i; j < port->num_fifo - 1; j++) {
                port->fifo[j] = port->fifo[j + 1];
            }
            port->num_fifo--;
            break;
        }
    }
    for (i = 0; i < port->num_atracats; i++) {
        if (port->atracats[i].fd_client == fd_client) {
            for (j = i; j < port->num_atracats - 1; j++) {
                port->atracats[j] = port->atracats[j + 1];
            }
            port->num_atracats--;
            port->places_ocupades--;
            if (port->num_fifo > 0) {
                promogut = port->fifo[0];
                /* Reutilitzem la posicio alliberada a l'array d'atracats. */
                port->atracats[port->num_atracats] = promogut;
                port->num_atracats++;
                port->places_ocupades++;
                for (j = 0; j < port->num_fifo - 1; j++) {
                    port->fifo[j] = port->fifo[j + 1];
                }
                port->num_fifo--;
            }
            break;
        }
    }
    pthread_mutex_unlock(mutex_port);
    if (promogut.fd_client >= 0 && promogut.espera_notificada == 1) {
        if (notificarEntradaPort(&promogut) != 0) {
            write(2, "Error: no s'ha pogut enviar DOCKED.\n",
                  strlen("Error: no s'ha pogut enviar DOCKED.\n"));
        }
    }
}

/***********************************************
 *
 * @Finalitat: Rep l'arribada d'un Odysseus i el mante al port fins que desconnecta.
 * @Parametres: in: arg = parametres reservats per a aquest client.
 * @Retorn: Retorna NULL quan acaba l'atencio del client.
 *
 ************************************************/
void *atendreOdysseusIsland(void *arg) {
    DadesClientIsland *dades_client = arg;
    ConfiguracioIlla *configuracio = dades_client->configuracio;
    EstatPort *port = dades_client->port;
    pthread_mutex_t *mutex_port = dades_client->mutex_port;
    pthread_mutex_t *mutex_stock = dades_client->mutex_stock;
    char *ruta_stock = dades_client->ruta_stock;
    VaixellPort vaixell = {0};
    unsigned char trama[MIDA_TRAMA], flags = FLAG_RESPOSTA_OK;
    char missatge[MIDA_NOM_ODYSSEUS + 100], *resposta = NULL;
    int fd_client = dades_client->fd_client, longitud = 0, resultat_port = -1, resultat = 0;
    int i = 0, atracat = 0, tipus = 0;

    free(arg);
    if (rebreTrama(fd_client, trama) != 0) {
        close(fd_client);
        return NULL;
    }
    if (trama[POSICIO_TIPUS] != TIPUS_ARRIBADA_ILLA) {
        if (trama[POSICIO_TIPUS] != TIPUS_NACK) {
            enviarNack(fd_client, "INVALID_TYPE");
        }
        close(fd_client);
        return NULL;
    }
    longitud = trama[POSICIO_LONGITUD];
    if (trama[POSICIO_FLAGS] != FLAG_PETICIO || longitud <= 0 || longitud > MIDA_DADES) {
        if (trama[POSICIO_FLAGS] != FLAG_PETICIO) {
            enviarNack(fd_client, "INVALID_FLAGS");
        } else {
            enviarNack(fd_client, "INVALID_LENGTH");
        }
        close(fd_client);
        return NULL;
    }
    vaixell.fd_client = fd_client;
    memcpy(vaixell.nom, trama + POSICIO_DADES, longitud);
    vaixell.nom[longitud] = '\0';
    if ((int)strlen(vaixell.nom) != longitud || strchr(vaixell.nom, '&') != NULL) {
        enviarNack(fd_client, "INVALID_DATA");
        close(fd_client);
        return NULL;
    }
    longitud = snprintf(missatge, sizeof(missatge), ">>> %s approaching %s.\n",
                         vaixell.nom, configuracio->nom);
    write(1, missatge, longitud);
    resultat_port = registrarArribada(port, configuracio->capacitat_port, &vaixell, mutex_port);
    if (resultat_port == 1) {
        resposta = "DOCKED";
        longitud = snprintf(missatge, sizeof(missatge), ">>> %s entered the port.\n", vaixell.nom);
    } else if (resultat_port == 2) {
        resposta = "WAIT";
        longitud = snprintf(missatge, sizeof(missatge),
                             ">>> Port full. %s waiting offshore in the FIFO queue.\n", vaixell.nom);
    } else {
        resposta = "Sense memoria";
        flags = FLAG_RESPOSTA_ERROR;
    }
    crearTrama(trama, TIPUS_ARRIBADA_ILLA, flags, (unsigned char *)resposta, strlen(resposta));
    resultat = enviarTrama(fd_client, trama);
    if (resultat == 0 && resultat_port > 0) {
        write(1, missatge, longitud);
        if (resultat_port == 2 && confirmarEspera(port, fd_client, mutex_port) == 1) {
            resultat = notificarEntradaPort(&vaixell);
        }
        while (resultat == 0 && rebreTrama(fd_client, trama) == 0) {
            tipus = trama[POSICIO_TIPUS];
            if (tipus == TIPUS_NACK) {
                break;
            }
            if (tipus == TIPUS_LIST_MARKET || tipus == TIPUS_BUY || tipus == TIPUS_SELL ||
                tipus == TIPUS_BUY_MAP || tipus == TIPUS_SORTIDA_ILLA) {
                if (trama[POSICIO_FLAGS] != FLAG_PETICIO) {
                    resultat = enviarNack(fd_client, "INVALID_FLAGS");
                    continue;
                }
                if (trama[POSICIO_LONGITUD] > MIDA_DADES) {
                    resultat = enviarNack(fd_client, "INVALID_LENGTH");
                    continue;
                }
                atracat = 0;
                pthread_mutex_lock(mutex_port);
                for (i = 0; i < port->num_atracats; i++) {
                    if (port->atracats[i].fd_client == fd_client) {
                        atracat = 1;
                    }
                }
                pthread_mutex_unlock(mutex_port);
                if (atracat == 0) {
                    crearTrama(trama, tipus, FLAG_RESPOSTA_ERROR,
                               (unsigned char *)"Cal estar atracat", 17);
                    resultat = enviarTrama(fd_client, trama);
                } else if (tipus == TIPUS_LIST_MARKET) {
                    resultat = gestionarListMarket(fd_client, trama, configuracio, mutex_stock);
                } else if (tipus == TIPUS_BUY_MAP) {
                    resultat = gestionarBuyMap(fd_client, trama, vaixell.nom, configuracio);
                } else if (tipus == TIPUS_SORTIDA_ILLA) {
                    if (trama[POSICIO_LONGITUD] != 0) {
                        resultat = enviarNack(fd_client, "INVALID_LENGTH");
                        continue;
                    } else {
                        alliberarPlacaPort(port, fd_client, mutex_port);
                        longitud = snprintf(missatge, sizeof(missatge), ">>> %s has left.\n", vaixell.nom);
                        write(1, missatge, longitud);
                        crearTrama(trama, tipus, FLAG_RESPOSTA_OK, (unsigned char *)"OK", 2);
                    }
                    resultat = enviarTrama(fd_client, trama);
                } else {
                    resultat = gestionarCompraVenda(fd_client, trama, vaixell.nom,
                                                     configuracio, mutex_stock, ruta_stock);
                }
            } else {
                resultat = enviarNack(fd_client, "INVALID_TYPE");
            }
        }
    }
    alliberarPlacaPort(port, fd_client, mutex_port);
    close(fd_client);
    return NULL;
}

/***********************************************
 *
 * @Finalitat: Carrega l'illa i executa el servidor concurrent del port.
 * @Parametres: in: argc = nombre d'arguments rebuts.
 *              in: argv = arguments rebuts.
 * @Retorn: Retorna 0 si el servidor finalitza correctament i 1 si hi ha un error.
 *
 ************************************************/
int main(int argc, char *argv[]) {
    ConfiguracioIlla configuracio = {0};
    EstatPort port = {0};
    pthread_mutex_t mutex_port;
    pthread_mutex_t mutex_stock;
    pthread_t fil;
    DadesClientIsland *dades_client = NULL;
    char *missatge = NULL, *nom_programa = argv[0];
    int resultat = 0, caracters_escrits = 0, fd_client = -1, fils_creats = 0;

    if (argc != 3) {
        caracters_escrits = asprintf(&missatge, "Usage: %s <config.dat> <stock.db>\n", nom_programa);
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

    if (pthread_mutex_init(&mutex_port, NULL) != 0) {
        write(2, "Error: mutex del port.\n", strlen("Error: mutex del port.\n"));
        alliberarConfiguracioIlla(&configuracio);
        return 1;
    }
    if (pthread_mutex_init(&mutex_stock, NULL) != 0) {
        write(2, "Error: mutex del stock.\n", strlen("Error: mutex del stock.\n"));
        pthread_mutex_destroy(&mutex_port);
        alliberarConfiguracioIlla(&configuracio);
        return 1;
    }
    signal(SIGINT, gestionarSigint);
    signal(SIGPIPE, SIG_IGN);
    fd_servidor = obrirServidor(configuracio.ip, configuracio.port);
    if (fd_servidor < 0) {
        write(2, "Error: no s'ha pogut obrir el servidor d'Island.\n",
              strlen("Error: no s'ha pogut obrir el servidor d'Island.\n"));
        pthread_mutex_destroy(&mutex_port);
        pthread_mutex_destroy(&mutex_stock);
        alliberarConfiguracioIlla(&configuracio);
        return 1;
    }
    if (configuracio.capacitat_port == 1) {
        caracters_escrits = asprintf(&missatge,"Island %s initialized.\nPort capacity: %d ship.\n""%d sea routes loaded.\n%d products available.\n",configuracio.nom, configuracio.capacitat_port,configuracio.nombre_rutes, configuracio.nombre_productes);
    } else {
        caracters_escrits = asprintf(&missatge,"Island %s initialized.\nPort capacity: %d ships.\n""%d sea routes loaded.\n%d products available.\n",configuracio.nom, configuracio.capacitat_port, configuracio.nombre_rutes, configuracio.nombre_productes);
    }
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }

    while (finalitzar_programa == 0) {
        fd_client = accept(fd_servidor, NULL, NULL);
        if (fd_client < 0) {
            if (finalitzar_programa == 0) {
                write(2, "Error: accept d'Island.\n", strlen("Error: accept d'Island.\n"));
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
            dades_client->port = &port;
            dades_client->mutex_port = &mutex_port;
            dades_client->mutex_stock = &mutex_stock;
            dades_client->ruta_stock = argv[2];
            if (pthread_create(&fil, NULL, atendreOdysseusIsland, dades_client) != 0) {
                write(2, "Error: pthread_create.\n", strlen("Error: pthread_create.\n"));
                free(dades_client);
                close(fd_client);
            } else {
                fils_creats = 1;
            }
        }
    }

    caracters_escrits = asprintf(&missatge, "%s closes its port.\n", configuracio.nom);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }

    /* Els fils poden usar la configuracio i el port fins a sortir del proces. */
    if (fils_creats == 0) {
        free(port.atracats);
        free(port.fifo);
        pthread_mutex_destroy(&mutex_stock);
        pthread_mutex_destroy(&mutex_port);
        alliberarConfiguracioIlla(&configuracio);
    }
    if (fd_servidor >= 0) {
        close(fd_servidor);
        fd_servidor = -1;
    }
    return resultat;
}

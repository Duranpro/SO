#define _GNU_SOURCE

/***********************************************
 *
 * @Proposit: Conte el servidor concurrent i el port del proces Island.
 * @Autor/s: Antonio Duran Sabates i Arnau González Hortelano
 * @Data creacio: 18/09/2026
 * @Data ultima modificacio: 19/10/2026
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
#include "gestio_navegants.h"

volatile sig_atomic_t finalitzar_programa = 0;
int fd_servidor = -1;

typedef struct {
    int fd_navegant;
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
    int fd_navegant;
    int fd_finalitzacio;
    FilNavegant *control;
    ConfiguracioIlla *configuracio;
    EstatPort *port;
    pthread_mutex_t *mutex_port;
    pthread_mutex_t *mutex_stock;
    char *ruta_stock;
} DadesNavegantIsland;

/***********************************************
 * @Finalitat: Envia les rutes ja filtrades com una unica compra del mapa local.
 * @Parametres: in: fd_navegant = socket; peticio = BUY MAP; nom = comprador.
 *              in: configuracio = rutes valides de l'illa.
 * @Retorn: Retorna 0 si envia la resposta i -1 si falla la connexio.
 ************************************************/
int gestionarBuyMap(int fd_navegant, unsigned char *peticio, char *nom,
                     ConfiguracioIlla *configuracio) {
    unsigned char trama[MIDA_TRAMA];
    char dades[MIDA_DADES + 1], missatge[MIDA_NOM_ODYSSEUS + 60], *error = NULL;
    int i = 0, longitud = 0, resultat = 0, total = configuracio->nombre_rutes;

    if (peticio[POSICIO_LONGITUD] == 0 || peticio[POSICIO_LONGITUD] > MIDA_DADES) {
        return enviarNack(fd_navegant, "INVALID_LENGTH");
    }
    memcpy(dades, peticio + POSICIO_DADES, peticio[POSICIO_LONGITUD]);
    dades[peticio[POSICIO_LONGITUD]] = '\0';
    if ((int)strlen(dades) != peticio[POSICIO_LONGITUD] || esNumero(dades) == 0) {
        return enviarNack(fd_navegant, "INVALID_DATA");
    }
    error = malloc((MIDA_DADES + 1) * sizeof(*error));
    if (error == NULL) {
        return -1;
    }
    error[0] = '\0';
    if (peticio[POSICIO_FLAGS] != FLAG_PETICIO || peticio[POSICIO_LONGITUD] != 1 ||
        peticio[POSICIO_DADES] != '1') {
        strcpy(error, "Quantitat de mapa invalida");
    }
    for (i = 0; i < total && error[0] == '\0'; i++) {
        longitud = snprintf(dades, sizeof(dades), "OK&50&%d&%d&%s&%s&%d", i + 1, total,
                             configuracio->rutes[i].nom_illa, configuracio->rutes[i].ip,
                             configuracio->rutes[i].port);
        if (longitud > MIDA_DADES) {
            strcpy(error, "Ruta massa llarga");
        }
    }
    if (error[0] != '\0') {
        crearTrama(trama, TIPUS_BUY_MAP, FLAG_RESPOSTA_ERROR,
                   (unsigned char *)error, strlen(error));
        free(error);
        return enviarTrama(fd_navegant, trama);
    }
    free(error);
    if (total == 0) {
        crearTrama(trama, TIPUS_BUY_MAP, FLAG_RESPOSTA_OK, (unsigned char *)"OK&50&0", 7);
        resultat = enviarTrama(fd_navegant, trama);
    }
    for (i = 0; i < total && resultat == 0; i++) {
        longitud = snprintf(dades, sizeof(dades), "OK&50&%d&%d&%s&%s&%d", i + 1, total,
                             configuracio->rutes[i].nom_illa, configuracio->rutes[i].ip,
                             configuracio->rutes[i].port);
        crearTrama(trama, TIPUS_BUY_MAP, FLAG_RESPOSTA_OK, (unsigned char *)dades, longitud);
        resultat = enviarTrama(fd_navegant, trama);
    }
    if (resultat == 0) {
        longitud = snprintf(missatge, sizeof(missatge), ">>> %s bought the local map.\n", nom);
        write(1, missatge, longitud);
    }
    return resultat;
}

/***********************************************
 * @Finalitat: Envia una copia coherent dels aliments i l'entrada virtual MAP.
 * @Parametres: in: fd_navegant = socket; peticio = trama LIST MARKET.
 *              in: configuracio = productes de l'illa.
 *              in/out: mutex_stock = mutex compartit del stock.
 * @Retorn: Retorna 0 si envia la resposta i -1 si falla la connexio.
 ************************************************/
int gestionarListMarket(int fd_navegant, unsigned char *peticio,
                        ConfiguracioIlla *configuracio, pthread_mutex_t *mutex_stock) {
    Producte *copia_productes = NULL;
    unsigned char trama[MIDA_TRAMA];
    char dades[MIDA_DADES + 1];
    int i = 0, total = configuracio->nombre_productes + 1, resultat = 0, longitud = 0;

    if (peticio[POSICIO_LONGITUD] != 0) {
        return enviarNack(fd_navegant, "INVALID_LENGTH");
    }
    if (total > 1) {
        copia_productes = malloc((total - 1) * sizeof(*copia_productes));
        if (copia_productes == NULL) {
            crearTrama(trama, TIPUS_LIST_MARKET, FLAG_RESPOSTA_ERROR,
                       (unsigned char *)"Sense memoria", 13);
            return enviarTrama(fd_navegant, trama);
        } else {
            pthread_mutex_lock(mutex_stock);
            memcpy(copia_productes, configuracio->productes, (total - 1) * sizeof(*copia_productes));
            pthread_mutex_unlock(mutex_stock);
        }
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
            resultat = enviarTrama(fd_navegant, trama);
        }
    }
    free(copia_productes);
    return resultat;
}

/***********************************************
 * @Finalitat: Compra o ven aliments mantenint junts la comprovacio i el canvi.
 * @Parametres: in: fd_navegant = socket; peticio = BUY o SELL; nom = comprador.
 *              in/out: configuracio = stock; mutex_stock = mutex del stock.
 *              in: ruta_stock = fitxer binari que conserva les quantitats.
 * @Retorn: Retorna 0 si envia la resposta i -1 si falla la connexio.
 ************************************************/
int gestionarCompraVenda(int fd_navegant, unsigned char *peticio, char *nom,
                         ConfiguracioIlla *configuracio, pthread_mutex_t *mutex_stock,
                         char *ruta_stock) {
    unsigned char trama[MIDA_TRAMA], tipus = peticio[POSICIO_TIPUS];
    char dades[MIDA_DADES + 1], *quantitat_text = NULL, *error = NULL;
    char missatge[MIDA_NOM_ODYSSEUS + MIDA_NOM_PRODUCTE + 100], accio[] = "bought";
    int longitud = peticio[POSICIO_LONGITUD], quantitat = 0, i = 0, index = -1;
    int nova_quantitat = 0, import = 0, valida = 1, format_valid = 0;

    if (longitud == 0 || longitud > MIDA_DADES) {
        return enviarNack(fd_navegant, "INVALID_LENGTH");
    }
    error = malloc((MIDA_DADES + 1) * sizeof(*error));
    if (error == NULL) {
        return -1;
    }
    strcpy(error, "Peticio invalida");
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
                strcpy(error, "Producte desconegut");
                pthread_mutex_lock(mutex_stock);
                for (i = 0; i < configuracio->nombre_productes; i++) {
                    if (strcmp(configuracio->productes[i].nom, dades) == 0) {
                        index = i;
                    }
                }
                if (index >= 0) {
                    strcpy(error, "Quantitat fora de rang");
                    if (configuracio->productes[index].preu >= 0 &&
                        (configuracio->productes[index].preu == 0 ||
                         quantitat <= INT_MAX / configuracio->productes[index].preu)) {
                        import = quantitat * configuracio->productes[index].preu;
                        nova_quantitat = configuracio->productes[index].quantitat;
                        if (tipus == TIPUS_BUY && nova_quantitat >= quantitat) {
                            nova_quantitat -= quantitat;
                            error[0] = '\0';
                        } else if (tipus == TIPUS_BUY) {
                            strcpy(error, "Stock insuficient");
                        } else if (nova_quantitat <= INT_MAX - quantitat) {
                            nova_quantitat += quantitat;
                            error[0] = '\0';
                        }
                        if (error[0] == '\0') {
                            if (actualitzarQuantitatStock(ruta_stock, index, nova_quantitat) == 0) {
                                configuracio->productes[index].quantitat = nova_quantitat;
                            } else {
                                strcpy(error, "Error escrivint stock");
                            }
                        }
                    }
                }
                pthread_mutex_unlock(mutex_stock);
            }
        }
    }
    if (format_valid == 0) {
        free(error);
        return enviarNack(fd_navegant, "INVALID_DATA");
    }
    if (error[0] != '\0') {
        crearTrama(trama, tipus, FLAG_RESPOSTA_ERROR, (unsigned char *)error, strlen(error));
    } else {
        if (tipus == TIPUS_SELL) {
            strcpy(accio, "sold");
        }
        longitud = snprintf(missatge, sizeof(missatge), ">>> %s %s %d kg of %s.\n",
                             nom, accio, quantitat, dades);
        write(1, missatge, longitud);
        longitud = snprintf(dades, sizeof(dades), "OK&%s&%d&%d",
                             configuracio->productes[index].nom, quantitat, import);
        crearTrama(trama, tipus, FLAG_RESPOSTA_OK, (unsigned char *)dades, longitud);
    }
    free(error);
    return enviarTrama(fd_navegant, trama);
}

/***********************************************
 *
 * @Finalitat: Indica la finalitzacio que comprova el bucle del servidor.
 * @Parametres: in: senyal = senyal rebut pel proces.
 * @Retorn: ----.
 *
 ************************************************/
void gestionarSigint(int senyal __attribute__((unused))) {
    finalitzar_programa = 1;
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
    if (enviarTrama(vaixell->fd_navegant, trama) != 0) {
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
 *              in: fd_navegant = descriptor del vaixell que ha rebut WAIT.
 *              in/out: mutex_port = mutex que protegeix tot el port.
 * @Retorn: Retorna 1 si aquest fil ha d'enviar DOCKED i 0 altrament.
 *
 ************************************************/
int confirmarEspera(EstatPort *port, int fd_navegant, pthread_mutex_t *mutex_port) {
    int i = 0, promogut = 0;

    /* La promocio no pot enviar DOCKED abans que aquest fil acabi d'enviar WAIT. */
    pthread_mutex_lock(mutex_port);
    for (i = 0; i < port->num_fifo; i++) {
        if (port->fifo[i].fd_navegant == fd_navegant) {
            port->fifo[i].espera_notificada = 1;
        }
    }
    for (i = 0; i < port->num_atracats; i++) {
        if (port->atracats[i].fd_navegant == fd_navegant &&
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
 *              in: fd_navegant = descriptor del vaixell que marxa.
 *              in/out: mutex_port = mutex que protegeix tot el port.
 * @Retorn: ----.
 *
 ************************************************/
void alliberarPlacaPort(EstatPort *port, int fd_navegant, pthread_mutex_t *mutex_port) {
    VaixellPort promogut = {0};
    int i = 0, j = 0;

    promogut.fd_navegant = -1;
    pthread_mutex_lock(mutex_port);
    for (i = 0; i < port->num_fifo; i++) {
        if (port->fifo[i].fd_navegant == fd_navegant) {
            for (j = i; j < port->num_fifo - 1; j++) {
                port->fifo[j] = port->fifo[j + 1];
            }
            port->num_fifo--;
            break;
        }
    }
    for (i = 0; i < port->num_atracats; i++) {
        if (port->atracats[i].fd_navegant == fd_navegant) {
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
    if (promogut.fd_navegant >= 0 && promogut.espera_notificada == 1) {
        if (notificarEntradaPort(&promogut) != 0) {
            write(2, "Error: no s'ha pogut enviar DOCKED.\n",
                  strlen("Error: no s'ha pogut enviar DOCKED.\n"));
        }
    }
}

/***********************************************
 *
 * @Finalitat: Rep l'arribada d'un Odysseus i el mante al port fins que desconnecta.
 * @Parametres: in: arg = parametres reservats per a aquest navegant.
 * @Retorn: Retorna NULL quan acaba l'atencio del navegant.
 *
 ************************************************/
void *atendreOdysseusIsland(void *arg) {
    DadesNavegantIsland *dades_navegant = arg;
    FilNavegant *control = dades_navegant->control;
    ConfiguracioIlla *configuracio = dades_navegant->configuracio;
    EstatPort *port = dades_navegant->port;
    pthread_mutex_t *mutex_port = dades_navegant->mutex_port;
    pthread_mutex_t *mutex_stock = dades_navegant->mutex_stock;
    char *ruta_stock = dades_navegant->ruta_stock;
    VaixellPort vaixell = {0};
    unsigned char trama[MIDA_TRAMA], flags = FLAG_RESPOSTA_OK;
    char missatge[MIDA_NOM_ODYSSEUS + 100], *resposta = NULL;
    int fd_navegant = dades_navegant->fd_navegant, longitud = 0, resultat_port = -1, resultat = 0;
    int fd_finalitzacio = dades_navegant->fd_finalitzacio;
    int i = 0, atracat = 0, tipus = 0;

    free(arg);
    if (rebreTramaNavegant(fd_navegant, fd_finalitzacio, trama) != 0) {
        close(fd_navegant);
        finalitzarNavegant(control);
        return NULL;
    }
    if (trama[POSICIO_TIPUS] != TIPUS_ARRIBADA_ILLA) {
        if (trama[POSICIO_TIPUS] != TIPUS_NACK) {
            enviarNack(fd_navegant, "INVALID_TYPE");
        }
        close(fd_navegant);
        finalitzarNavegant(control);
        return NULL;
    }
    longitud = trama[POSICIO_LONGITUD];
    if (trama[POSICIO_FLAGS] != FLAG_PETICIO || longitud <= 0 || longitud > MIDA_DADES) {
        if (trama[POSICIO_FLAGS] != FLAG_PETICIO) {
            enviarNack(fd_navegant, "INVALID_FLAGS");
        } else {
            enviarNack(fd_navegant, "INVALID_LENGTH");
        }
        close(fd_navegant);
        finalitzarNavegant(control);
        return NULL;
    }
    vaixell.fd_navegant = fd_navegant;
    memcpy(vaixell.nom, trama + POSICIO_DADES, longitud);
    vaixell.nom[longitud] = '\0';
    if ((int)strlen(vaixell.nom) != longitud || strchr(vaixell.nom, '&') != NULL) {
        enviarNack(fd_navegant, "INVALID_DATA");
        close(fd_navegant);
        finalitzarNavegant(control);
        return NULL;
    }
    resposta = malloc(sizeof("Sense memoria"));
    if (resposta == NULL) {
        close(fd_navegant);
        finalitzarNavegant(control);
        return NULL;
    }
    longitud = snprintf(missatge, sizeof(missatge), ">>> %s approaching %s.\n",
                         vaixell.nom, configuracio->nom);
    write(1, missatge, longitud);
    resultat_port = registrarArribada(port, configuracio->capacitat_port, &vaixell, mutex_port);
    if (resultat_port == 1) {
        strcpy(resposta, "DOCKED");
        longitud = snprintf(missatge, sizeof(missatge), ">>> %s entered the port.\n", vaixell.nom);
    } else if (resultat_port == 2) {
        strcpy(resposta, "WAIT");
        longitud = snprintf(missatge, sizeof(missatge),
                             ">>> Port full. %s waiting offshore in the FIFO queue.\n", vaixell.nom);
    } else {
        strcpy(resposta, "Sense memoria");
        flags = FLAG_RESPOSTA_ERROR;
    }
    crearTrama(trama, TIPUS_ARRIBADA_ILLA, flags, (unsigned char *)resposta, strlen(resposta));
    free(resposta);
    resultat = enviarTrama(fd_navegant, trama);
    if (resultat == 0 && resultat_port > 0) {
        write(1, missatge, longitud);
        if (resultat_port == 2 && confirmarEspera(port, fd_navegant, mutex_port) == 1) {
            resultat = notificarEntradaPort(&vaixell);
        }
        while (resultat == 0 && rebreTramaNavegant(fd_navegant, fd_finalitzacio, trama) == 0) {
            tipus = trama[POSICIO_TIPUS];
            if (tipus == TIPUS_NACK) {
                break;
            }
            if (tipus == TIPUS_LIST_MARKET || tipus == TIPUS_BUY || tipus == TIPUS_SELL ||
                tipus == TIPUS_BUY_MAP || tipus == TIPUS_SORTIDA_ILLA) {
                if (trama[POSICIO_FLAGS] != FLAG_PETICIO) {
                    resultat = enviarNack(fd_navegant, "INVALID_FLAGS");
                    continue;
                }
                if (trama[POSICIO_LONGITUD] > MIDA_DADES) {
                    resultat = enviarNack(fd_navegant, "INVALID_LENGTH");
                    continue;
                }
                atracat = 0;
                pthread_mutex_lock(mutex_port);
                for (i = 0; i < port->num_atracats; i++) {
                    if (port->atracats[i].fd_navegant == fd_navegant) {
                        atracat = 1;
                    }
                }
                pthread_mutex_unlock(mutex_port);
                if (atracat == 0) {
                    crearTrama(trama, tipus, FLAG_RESPOSTA_ERROR,
                               (unsigned char *)"Cal estar atracat", 17);
                    resultat = enviarTrama(fd_navegant, trama);
                } else if (tipus == TIPUS_LIST_MARKET) {
                    resultat = gestionarListMarket(fd_navegant, trama, configuracio, mutex_stock);
                } else if (tipus == TIPUS_BUY_MAP) {
                    resultat = gestionarBuyMap(fd_navegant, trama, vaixell.nom, configuracio);
                } else if (tipus == TIPUS_SORTIDA_ILLA) {
                    if (trama[POSICIO_LONGITUD] != 0) {
                        resultat = enviarNack(fd_navegant, "INVALID_LENGTH");
                        continue;
                    } else {
                        alliberarPlacaPort(port, fd_navegant, mutex_port);
                        longitud = snprintf(missatge, sizeof(missatge), ">>> %s has left.\n", vaixell.nom);
                        write(1, missatge, longitud);
                        crearTrama(trama, tipus, FLAG_RESPOSTA_OK, (unsigned char *)"OK", 2);
                    }
                    resultat = enviarTrama(fd_navegant, trama);
                } else {
                    resultat = gestionarCompraVenda(fd_navegant, trama, vaixell.nom,
                                                     configuracio, mutex_stock, ruta_stock);
                }
            } else {
                resultat = enviarNack(fd_navegant, "INVALID_TYPE");
            }
        }
    }
    alliberarPlacaPort(port, fd_navegant, mutex_port);
    close(fd_navegant);
    finalitzarNavegant(control);
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
    RegistreNavegants navegants;
    DadesNavegantIsland *dades_navegant = NULL;
    FilNavegant *navegant = NULL;
    char *missatge = NULL, *nom_programa = argv[0];
    int resultat = 0, caracters_escrits = 0, fd_navegant = -1, preparat = 0;

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
    if (iniciarNavegants(&navegants) != 0) {
        write(2, "Error: registre de navegants.\n", strlen("Error: registre de navegants.\n"));
        pthread_mutex_destroy(&mutex_stock);
        pthread_mutex_destroy(&mutex_port);
        alliberarConfiguracioIlla(&configuracio);
        return 1;
    }
    signal(SIGINT, gestionarSigint);
    signal(SIGPIPE, SIG_IGN);
    fd_servidor = obrirServidor(configuracio.ip, configuracio.port);
    if (fd_servidor < 0) {
        recollirNavegants(&navegants, 1);
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
        recollirNavegants(&navegants, 0);
        preparat = esperarNavegant(fd_servidor);
        if (finalitzar_programa != 0) {
            break;
        }
        if (preparat == 0) {
            continue;
        }
        if (preparat < 0) {
            write(2, "Error: select d'Island.\n", strlen("Error: select d'Island.\n"));
            resultat = 1;
            break;
        }
        fd_navegant = accept(fd_servidor, NULL, NULL);
        if (fd_navegant < 0) {
            if (finalitzar_programa == 0) {
                write(2, "Error: accept d'Island.\n", strlen("Error: accept d'Island.\n"));
                resultat = 1;
            }
            break;
        }
        dades_navegant = malloc(sizeof(*dades_navegant));
        if (dades_navegant == NULL) {
            write(2, "Error: malloc del navegant.\n", strlen("Error: malloc del navegant.\n"));
            close(fd_navegant);
        } else {
            dades_navegant->fd_navegant = fd_navegant;
            dades_navegant->fd_finalitzacio = navegants.finalitzacio[0];
            dades_navegant->configuracio = &configuracio;
            dades_navegant->port = &port;
            dades_navegant->mutex_port = &mutex_port;
            dades_navegant->mutex_stock = &mutex_stock;
            dades_navegant->ruta_stock = argv[2];
            navegant = reservarNavegant(&navegants);
            dades_navegant->control = navegant;
            if (navegant == NULL || pthread_create(&navegant->fil, NULL, atendreOdysseusIsland, dades_navegant) != 0) {
                write(2, "Error: pthread_create.\n", strlen("Error: pthread_create.\n"));
                free(navegant);
                free(dades_navegant);
                close(fd_navegant);
            } else {
                registrarNavegant(navegant);
            }
        }
    }

    caracters_escrits = asprintf(&missatge, "%s closes its port.\n", configuracio.nom);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }

    recollirNavegants(&navegants, 1);
    free(port.atracats);
    free(port.fifo);
    pthread_mutex_destroy(&mutex_stock);
    pthread_mutex_destroy(&mutex_port);
    alliberarConfiguracioIlla(&configuracio);
    if (fd_servidor >= 0) {
        close(fd_servidor);
        fd_servidor = -1;
    }
    return resultat;
}

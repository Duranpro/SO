#define _GNU_SOURCE

/***********************************************
 *
 * @Proposit: Implementa el terminal i el parser de comandes d'Odysseus.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 23/09/2026
 * @Data ultima modificacio: 07/10/2026
 *
 ************************************************/

#include "comandes.h"

#include <ctype.h>

#include "sphragis.h"
#include "trames.h"
#include "xarxa.h"

/***********************************************
 *
 * @Finalitat: Escriu un missatge per la sortida estandard.
 * @Parametres: in: text = text que es vol escriure.
 * @Retorn: Retorna 0 si pot escriure i -1 si falla la reserva.
 *
 ************************************************/
int escriureMissatge(char *text) {
    int caracters_escrits = 0;
    char *missatge = NULL;

    caracters_escrits = asprintf(&missatge, "%s", text);
    if (caracters_escrits < 0) {
        return -1;
    }
    write(1, missatge, caracters_escrits);
    free(missatge);
    return 0;
}

/***********************************************
 *
 * @Finalitat: Connecta Odysseus amb Ithaca i envia la seva identificacio.
 * @Parametres: in: configuracio = dades de connexio i nom d'Odysseus.
 *              in/out: socket_actual = connexio activa amb Ithaca.
 * @Retorn: Retorna 0 si es connecta correctament i -1 altrament.
 *
 ************************************************/
int connectarItaca(ConfiguracioOdisseu *configuracio, int *socket_actual) {
    unsigned char trama_peticio[MIDA_TRAMA] = {0}, trama_resposta[MIDA_TRAMA] = {0}; //REVISAR
    unsigned char dades_resposta[MIDA_DADES_TRAMA + 1] = {0};
    int socket_itaca = -1, longitud_nom = 0, resultat = 0;

    if (*socket_actual >= 0) {
        escriureMissatge("Already connected to Ithaca.\n");
        return -1;
    }

    while (configuracio->nom[longitud_nom] != '\0') {
        longitud_nom++;
    }
    if (longitud_nom == 0 || longitud_nom > MIDA_DADES_TRAMA) {
        escriureMissatge("Error: invalid Odysseus name.\n");
        return -1;
    }

    socket_itaca = connectarServidor(configuracio->ip_itaca, configuracio->port_itaca);
    if (socket_itaca < 0) {
        return -1;
    }
    *socket_actual = socket_itaca;

    resultat = crearTrama(trama_peticio, TIPUS_CONNECTAR_ITACA, FLAGS_PETICIO_TEXTUAL,(unsigned char *) configuracio->nom, longitud_nom);
    if (resultat == TRAMA_CORRECTA) {
        resultat = enviarTrama(socket_itaca, trama_peticio);
    }
    if (resultat == TRAMA_CORRECTA) {
        resultat = rebreTrama(socket_itaca, trama_resposta);
    }
    if (resultat == TRAMA_CORRECTA) {
        resultat = validarTrama(trama_resposta);
    }
    if (resultat == TRAMA_CORRECTA && trama_resposta[POSICIO_TIPUS] == TIPUS_CONNECTAR_ITACA && trama_resposta[POSICIO_FLAGS] == FLAGS_RESPOSTA_CORRECTA &&trama_resposta[POSICIO_LONGITUD_DADES] == 2) {resultat = obtenirDades(trama_resposta, dades_resposta);
    } else {
        resultat = TRAMA_ERROR_GENERAL;
    }

    if (resultat != TRAMA_CORRECTA ||dades_resposta[0] != 'O' || dades_resposta[1] != 'K') {
        escriureMissatge("Error: Ithaca has rejected the connection.\n");
        close(socket_itaca);
        *socket_actual = -1;
        return -1;
    }

    configuracio->tipus_connexio = CONNEXIO_ITACA;
    escriureMissatge("Connected to Ithaca.\n");
    return 0;
}

/***********************************************
 *
 * @Finalitat: Valida i mostra una resposta individual de LIST VOYAGES.
 * @Parametres: in: trama = resposta rebuda d'Ithaca.
 *              in: index_esperat = posicio esperada dins les respostes.
 *              in/out: nombre_total = total de respostes de la llista.
 * @Retorn: Retorna 0 si la resposta es valida i -1 altrament.
 *
 ************************************************/
int mostrarViatgeDisponible(unsigned char *trama, int index_esperat, int *nombre_total) {
    unsigned char dades[MIDA_DADES_TRAMA + 1] = {0}; //REVISAR
    char *index_text = NULL, *total_text = NULL, *identificador_text = NULL;
    char *objecte = NULL, *desti = NULL, *recompensa_text = NULL;
    char *camp_extra = NULL, *missatge = NULL;
    int longitud = 0, index = 0, total = 0, caracters_escrits = 0;

    if (validarTrama(trama) != TRAMA_CORRECTA || trama[POSICIO_TIPUS] != TIPUS_LLISTAR_VIATGES || trama[POSICIO_FLAGS] != FLAGS_RESPOSTA_CORRECTA) {
        return -1;
    }
    longitud = trama[POSICIO_LONGITUD_DADES];
    if (longitud == 0 || obtenirDades(trama, dades) != TRAMA_CORRECTA) {
        return -1;
    }
    dades[longitud] = '\0';

    index_text = strtok((char *) dades, "&");
    total_text = strtok(NULL, "&");
    identificador_text = strtok(NULL, "&");
    objecte = strtok(NULL, "&");
    desti = strtok(NULL, "&");
    recompensa_text = strtok(NULL, "&");
    camp_extra = strtok(NULL, "&");
    if (index_text == NULL || total_text == NULL || identificador_text == NULL || objecte == NULL || desti == NULL || recompensa_text == NULL || camp_extra != NULL || esNumero(index_text) == 0 || esNumero(total_text) == 0 ||esNumero(identificador_text) == 0 ||esNumero(recompensa_text) == 0) {
        return -1;
    }

    index = atoi(index_text);
    total = atoi(total_text);
    if (index != index_esperat || total <= 0 ||
        (*nombre_total != 0 && *nombre_total != total)) {
        return -1;
    }
    *nombre_total = total;

    caracters_escrits = asprintf(&missatge, "%s. %s -> %s [%s]\n", identificador_text, objecte, desti, recompensa_text);
    if (caracters_escrits < 0) {
        return -1;
    }
    write(1, missatge, caracters_escrits);
    free(missatge);
    return 0;
}

/***********************************************
 *
 * @Finalitat: Demana i mostra els viatges disponibles d'Ithaca.
 * @Parametres: in: socket_actual = connexio activa amb Ithaca.
 * @Retorn: Retorna 0 si rep la llista i -1 altrament.
 *
 ************************************************/
int llistarViatges(ConfiguracioOdisseu *configuracio, int socket_actual) {
    unsigned char peticio[MIDA_TRAMA] = {0}, resposta[MIDA_TRAMA] = {0}; //REVISAR
    unsigned char dades[MIDA_DADES_TRAMA + 1] = {0};
    int resultat = 0, nombre_total = 0, i = 0, longitud = 0;

    if (socket_actual < 0 ||
        configuracio->tipus_connexio != CONNEXIO_ITACA) {
        escriureMissatge("Not connected to Ithaca.\n");
        return -1;
    }
    if (configuracio->identificador_viatge != 0) {
        escriureMissatge("An active voyage already exists.\n");
        return -1;
    }

    resultat = crearTrama(peticio, TIPUS_LLISTAR_VIATGES, FLAGS_PETICIO_TEXTUAL, NULL, 0);
    if (resultat == TRAMA_CORRECTA) {
        resultat = enviarTrama(socket_actual, peticio);
    }
    if (resultat == TRAMA_CORRECTA) {
        resultat = rebreTrama(socket_actual, resposta);
    }
    if (resultat != TRAMA_CORRECTA || validarTrama(resposta) != TRAMA_CORRECTA || resposta[POSICIO_TIPUS] != TIPUS_LLISTAR_VIATGES) {
        escriureMissatge("Error: invalid response from Ithaca.\n");
        return -1;
    }
    if (resposta[POSICIO_FLAGS] == FLAGS_RESPOSTA_ERROR) {
        escriureMissatge("Error: Ithaca could not list voyages.\n");
        return -1;
    }

    longitud = resposta[POSICIO_LONGITUD_DADES];
    if (longitud == 1 && obtenirDades(resposta, dades) == TRAMA_CORRECTA &&dades[0] == '0') {
        escriureMissatge("--- AVAILABLE VOYAGES ---\n");
        return 0;
    }

    escriureMissatge("--- AVAILABLE VOYAGES ---\n");
    resultat = mostrarViatgeDisponible(resposta, 1, &nombre_total);
    for (i = 2; i <= nombre_total && resultat == 0; i++) {
        resultat = rebreTrama(socket_actual, resposta);
        if (resultat == TRAMA_CORRECTA) {
            resultat = mostrarViatgeDisponible(resposta, i, &nombre_total);
        }
    }
    if (resultat != 0) {
        escriureMissatge("Error: invalid response from Ithaca.\n");
        return -1;
    }
    return 0;
}

/***********************************************
 *
 * @Finalitat: Demana a Ithaca l'assignacio d'un viatge disponible.
 * @Parametres: in/out: configuracio = estat d'Odysseus.
 *              in: socket_actual = connexio activa amb Ithaca.
 *              in: identificador = viatge que es vol acceptar.
 * @Retorn: Retorna 0 si accepta el viatge i -1 altrament.
 *
 ************************************************/
int acceptarViatge(ConfiguracioOdisseu *configuracio, int socket_actual, int identificador) {
    unsigned char peticio[MIDA_TRAMA] = {0}, resposta[MIDA_TRAMA] = {0}; //REVISAR
    unsigned char dades[MIDA_DADES_TRAMA + 1] = {0};
    char *identificador_text = NULL, *estat = NULL, *camp_id = NULL;
    char *objecte = NULL, *desti = NULL, *recompensa_text = NULL;
    char *camp_extra = NULL, *missatge = NULL;
    int longitud = 0, resultat = 0, caracters_escrits = 0;

    if (socket_actual < 0 ||
        configuracio->tipus_connexio != CONNEXIO_ITACA) {
        escriureMissatge("Not connected to Ithaca.\n");
        return -1;
    }
    if (configuracio->identificador_viatge != 0) {
        escriureMissatge("An active voyage already exists.\n");
        return -1;
    }

    longitud = asprintf(&identificador_text, "%d", identificador);
    if (longitud < 0) {
        return -1;
    }
    resultat = crearTrama(peticio, TIPUS_ACCEPTAR_VIATGE, FLAGS_PETICIO_TEXTUAL,(unsigned char *) identificador_text, longitud);
    free(identificador_text);
    if (resultat == TRAMA_CORRECTA) {
        resultat = enviarTrama(socket_actual, peticio);
    }
    if (resultat == TRAMA_CORRECTA) {
        resultat = rebreTrama(socket_actual, resposta);
    }
    if (resultat != TRAMA_CORRECTA ||validarTrama(resposta) != TRAMA_CORRECTA ||resposta[POSICIO_TIPUS] != TIPUS_ACCEPTAR_VIATGE) {
        escriureMissatge("Error: invalid response from Ithaca.\n");
        return -1;
    }
    if (resposta[POSICIO_FLAGS] == FLAGS_RESPOSTA_ERROR) {
        caracters_escrits = asprintf(&missatge,"Voyage %d is not available.\n", identificador);
        if (caracters_escrits >= 0) {
            write(1, missatge, caracters_escrits);
            free(missatge);
        }
        return -1;
    }
    if (resposta[POSICIO_FLAGS] != FLAGS_RESPOSTA_CORRECTA) {
        escriureMissatge("Error: invalid response from Ithaca.\n");
        return -1;
    }

    longitud = resposta[POSICIO_LONGITUD_DADES];
    if (longitud == 0 || obtenirDades(resposta, dades) != TRAMA_CORRECTA) {
        return -1;
    }
    dades[longitud] = '\0';
    estat = strtok((char *) dades, "&");
    camp_id = strtok(NULL, "&");
    objecte = strtok(NULL, "&");
    desti = strtok(NULL, "&");
    recompensa_text = strtok(NULL, "&");
    camp_extra = strtok(NULL, "&");
    if (estat == NULL || camp_id == NULL || objecte == NULL ||desti == NULL || recompensa_text == NULL || camp_extra != NULL ||strcmp(estat, "OK") != 0 || esNumero(camp_id) == 0 ||atoi(camp_id) != identificador || esNumero(recompensa_text) == 0) {
        escriureMissatge("Error: invalid response from Ithaca.\n");
        return -1;
    }

    resultat = copiarText(&configuracio->objecte_viatge, objecte);
    if (resultat == 0) {
        resultat = copiarText(&configuracio->illa_desti_viatge, desti);
    }
    if (resultat != 0) {
        free(configuracio->objecte_viatge);
        configuracio->objecte_viatge = NULL;
        return -1;
    }
    configuracio->identificador_viatge = identificador;
    configuracio->recompensa_viatge = atoi(recompensa_text);
    configuracio->desti_assolit = 0;

    caracters_escrits = asprintf(&missatge,"Voyage %d accepted.\nDestination: %s.\n",identificador, configuracio->illa_desti_viatge);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }
    return 0;
}

/***********************************************
 *
 * @Finalitat: Tanca de manera controlada la connexio amb Ithaca.
 * @Parametres: in/out: configuracio = estat de connexio d'Odysseus.
 *              in/out: socket_actual = connexio activa amb Ithaca.
 * @Retorn: Retorna 0 si Ithaca confirma la desconnexio i -1 altrament.
 *
 ************************************************/
int desconnectarItaca(ConfiguracioOdisseu *configuracio, int *socket_actual) {
    unsigned char peticio[MIDA_TRAMA] = {0}, resposta[MIDA_TRAMA] = {0}; //REVISAR
    unsigned char dades[MIDA_DADES_TRAMA + 1] = {0};
    int resultat = 0;

    resultat = crearTrama(peticio, TIPUS_DESCONNECTAR_ITACA,FLAGS_PETICIO_TEXTUAL, NULL, 0);
    if (resultat == TRAMA_CORRECTA) {
        resultat = enviarTrama(*socket_actual, peticio);
    }
    if (resultat == TRAMA_CORRECTA) {
        resultat = rebreTrama(*socket_actual, resposta);
    }
    if (resultat != TRAMA_CORRECTA) {
        close(*socket_actual);
        *socket_actual = -1;
        configuracio->tipus_connexio = SENSE_CONNEXIO;
        escriureMissatge("Error: connection with Ithaca was lost.\n");
        return -1;
    }

    resultat = validarTrama(resposta);
    if (resultat != TRAMA_CORRECTA || resposta[POSICIO_TIPUS] != TIPUS_DESCONNECTAR_ITACA || resposta[POSICIO_FLAGS] != FLAGS_RESPOSTA_CORRECTA ||resposta[POSICIO_LONGITUD_DADES] != 2 || obtenirDades(resposta, dades) != TRAMA_CORRECTA || dades[0] != 'O' || dades[1] != 'K') {
        escriureMissatge("Error: Ithaca has rejected the disconnection.\n");
        return -1;
    }

    close(*socket_actual);
    *socket_actual = -1;
    configuracio->tipus_connexio = SENSE_CONNEXIO;
    return 0;
}

/***********************************************
 *
 * @Finalitat: Rep i valida una resposta WAIT o DOCKED d'una illa.
 * @Parametres: in: socket_illa = connexio activa amb l'illa.
 * @Retorn: Retorna PORT_WAIT, PORT_DOCKED o -1 si la resposta no es valida.
 *
 ************************************************/
int rebreEstatPort(int socket_illa) {
    unsigned char resposta[MIDA_TRAMA] = {0}; //REVISAR
    unsigned char dades[MIDA_DADES_TRAMA + 1] = {0};
    int longitud = 0, resultat = 0;

    resultat = rebreTrama(socket_illa, resposta);
    if (resultat == TRAMA_CORRECTA) {
        resultat = validarTrama(resposta);
    }
    longitud = resposta[POSICIO_LONGITUD_DADES];
    if (resultat != TRAMA_CORRECTA || resposta[POSICIO_TIPUS] != TIPUS_ARRIBADA_ILLA || resposta[POSICIO_FLAGS] != FLAGS_RESPOSTA_CORRECTA || longitud == 0 || obtenirDades(resposta, dades) != TRAMA_CORRECTA) {
        return -1;
    }
    dades[longitud] = '\0';
    if (longitud == 6 && strcmp((char *) dades, "DOCKED") == 0) {
        return PORT_DOCKED;
    }
    if (longitud == 4 && strcmp((char *) dades, "WAIT") == 0) {
        return PORT_WAIT;
    }
    return -1;
}

/***********************************************
 *
 * @Finalitat: Calcula els quilos totals d'aliments d'Odysseus.
 * @Parametres: in: configuracio = carrega actual d'Odysseus.
 * @Retorn: Retorna la suma de les quantitats de tots els aliments.
 *
 ************************************************/
int calcularTotalProvisions(ConfiguracioOdisseu *configuracio) {
    int total_provisions = 0, i = 0;

    for (i = 0; i < configuracio->nombre_aliments; i++) {
        total_provisions = total_provisions +configuracio->aliments[i].quantitat;
    }
    return total_provisions;
}

/***********************************************
 *
 * @Finalitat: Consumeix aliments en l'ordre en que estan guardats.
 * @Parametres: in/out: configuracio = carrega actual d'Odysseus.
 *              in: quantitat = quilos que es volen consumir.
 * @Retorn: Retorna els quilos que s'han pogut consumir.
 *
 ************************************************/
int consumirProvisions(ConfiguracioOdisseu *configuracio, int quantitat) {
    int quantitat_pendent = quantitat, consumit = 0, i = 0;

    for (i = 0; i < configuracio->nombre_aliments &&quantitat_pendent > 0; i++) {
        if (configuracio->aliments[i].quantitat >= quantitat_pendent) {
            configuracio->aliments[i].quantitat = configuracio->aliments[i].quantitat - quantitat_pendent;
            consumit = consumit + quantitat_pendent;
            quantitat_pendent = 0;
        } else {
            consumit = consumit + configuracio->aliments[i].quantitat;
            quantitat_pendent = quantitat_pendent -configuracio->aliments[i].quantitat;
            configuracio->aliments[i].quantitat = 0;
        }
    }
    return consumit;
}

/***********************************************
 *
 * @Finalitat: Espera DOCKED i consumeix una racio cada cinc segons complets.
 * @Parametres: in/out: configuracio = carrega i estat d'Odysseus.
 *              in/out: socket_actual = connexio activa amb l'illa.
 * @Retorn: Retorna PORT_DOCKED, ODISSEU_MORT o -1 si falla la connexio.
 *
 ************************************************/
int esperarEntradaPort(ConfiguracioOdisseu *configuracio,int *socket_actual) {
    fd_set descriptors;
    struct timeval temps; //REVISAR
    char *missatge = NULL;
    int resultat = 0, estat_port = 0, esperant = 1;
    int total_provisions = 0, caracters_escrits = 0;

    while (esperant == 1) {
        if (*socket_actual < 0) {
            return -1;
        }
        FD_ZERO(&descriptors);
        FD_SET(*socket_actual, &descriptors);
        temps.tv_sec = SEGONS_RACIO;
        temps.tv_usec = 0;
        resultat = select(*socket_actual + 1, &descriptors, NULL, NULL, &temps);

        if (resultat > 0 && FD_ISSET(*socket_actual, &descriptors)) {
            estat_port = rebreEstatPort(*socket_actual);
            if (estat_port == PORT_DOCKED) {
                esperant = 0;
            } else {
                resultat = -1;
                esperant = 0;
            }
        } else if (resultat == 0) {
            consumirProvisions(configuracio, QUILOS_RACIO);
            total_provisions = calcularTotalProvisions(configuracio);
            caracters_escrits = asprintf(&missatge, ">>> The crew consumes another ration.\n""Provisions remaining: %d kg\n", total_provisions);
            if (caracters_escrits >= 0) {
                write(1, missatge, caracters_escrits);
                free(missatge);
                missatge = NULL;
            }
            if (total_provisions <= 0) {
                escriureMissatge("The crew has run out of provisions.\n");
                caracters_escrits = asprintf(&missatge,"Odysseus %s has been lost at sea.\n",configuracio->nom);
                if (caracters_escrits >= 0) {
                    write(1, missatge, caracters_escrits);
                    free(missatge);
                }
                close(*socket_actual);
                *socket_actual = -1;
                configuracio->tipus_connexio = SENSE_CONNEXIO;
                return ODISSEU_MORT;
            }
        } else {
            resultat = -1;
            esperant = 0;
        }
    }

    if (resultat < 0) {
        if (*socket_actual >= 0) {
            close(*socket_actual);
            *socket_actual = -1;
        }
        configuracio->tipus_connexio = SENSE_CONNEXIO;
        return -1;
    }
    return PORT_DOCKED;
}

/***********************************************
 *
 * @Finalitat: Connecta amb una illa i espera fins que permet atracar.
 * @Parametres: in/out: configuracio = dades i estat d'Odysseus.
 *              in/out: socket_actual = connexio activa amb l'illa.
 *              in: nom_illa = illa a la qual arriba Odysseus.
 * @Retorn: Retorna 0 si Odysseus atraca i -1 altrament.
 *
 ************************************************/
int esperarPort(ConfiguracioOdisseu *configuracio, int *socket_actual,char *nom_illa, char *ip, int port) {
    unsigned char peticio[MIDA_TRAMA] = {0}; //REVISAR
    char *missatge = NULL, *nova_ubicacio = NULL;
    int socket_illa = -1, longitud_nom = 0, resultat = 0, estat_port = 0;
    int caracters_escrits = 0, ha_esperat = 0;

    socket_illa = connectarServidor(ip, port);
    if (socket_illa < 0) {
        return -1;
    }
    *socket_actual = socket_illa;
    configuracio->tipus_connexio = CONNEXIO_ILLA;

    while (configuracio->nom[longitud_nom] != '\0') {
        longitud_nom++;
    }
    resultat = crearTrama(peticio, TIPUS_ARRIBADA_ILLA,FLAGS_PETICIO_TEXTUAL,(unsigned char *) configuracio->nom, longitud_nom);
    if (resultat == TRAMA_CORRECTA) {
        resultat = enviarTrama(socket_illa, peticio);
    }
    if (resultat == TRAMA_CORRECTA) {
        estat_port = rebreEstatPort(socket_illa);
    }

    if (resultat == TRAMA_CORRECTA && estat_port != -1) {
        caracters_escrits = asprintf(&missatge, "Connected to %s.\n", nom_illa);
        if (caracters_escrits >= 0) {
            write(1, missatge, caracters_escrits);
            free(missatge);
            missatge = NULL;
        }
    }

    if (estat_port == PORT_WAIT) {
        escriureMissatge("The harbor is full. We drop anchor and wait.\n");
        ha_esperat = 1;
        estat_port = esperarEntradaPort(configuracio, socket_actual);
        if (estat_port == ODISSEU_MORT) {
            return ODISSEU_MORT;
        }
        if (estat_port != PORT_DOCKED) {
            return -1;
        }
    }
    if (resultat != TRAMA_CORRECTA || estat_port != PORT_DOCKED) {
        escriureMissatge("Error: invalid response from Island.\n");
        close(socket_illa);
        *socket_actual = -1;
        configuracio->tipus_connexio = SENSE_CONNEXIO;
        return -1;
    }
    if (copiarText(&nova_ubicacio, nom_illa) != 0) {
        close(socket_illa);
        *socket_actual = -1;
        configuracio->tipus_connexio = SENSE_CONNEXIO;
        return -1;
    }
    free(configuracio->ubicacio_actual);
    configuracio->ubicacio_actual = nova_ubicacio;

    free(configuracio->productes_mercat);
    configuracio->productes_mercat = NULL;
    configuracio->nombre_productes_mercat = 0;
    configuracio->desti_assolit = 0;
    if (configuracio->identificador_viatge != 0 &&
        configuracio->illa_desti_viatge != NULL &&
        strcasecmp(configuracio->ubicacio_actual,configuracio->illa_desti_viatge) == 0) {
        configuracio->desti_assolit = 1;
    }

    if (ha_esperat == 1) {
        caracters_escrits = asprintf(&missatge, ">>> %s has granted access to the port.\n", nom_illa);
    } else {
        caracters_escrits = asprintf(&missatge,"%s has granted access to the port.\n", nom_illa);
    }
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }
    return 0;
}

/***********************************************
 *
 * @Finalitat: Inicia la navegacio des d'Ithaca fins a Aeaea.
 * @Parametres: in/out: configuracio = dades i estat d'Odysseus.
 *              in/out: socket_actual = connexio activa.
 *              in: nom_illa = destinacio demanada.
 * @Retorn: Retorna 0 si arriba al port d'Aeaea i -1 altrament.
 *
 ************************************************/
int navegarAeaea(ConfiguracioOdisseu *configuracio, int *socket_actual, char *nom_illa) {
    char *missatge = NULL;
    int caracters_escrits = 0;

    if (*socket_actual < 0 ||configuracio->tipus_connexio != CONNEXIO_ITACA) {
        escriureMissatge("Not connected to Ithaca.\n");
        return -1;
    }
    if (configuracio->identificador_viatge == 0) {
        escriureMissatge("No active voyage.\n");
        return -1;
    }
    if (strcasecmp(nom_illa, NOM_ILLA_INICIAL) != 0) {
        caracters_escrits = asprintf(&missatge, "We can't sail to %s from here...\n", nom_illa);
        if (caracters_escrits >= 0) {
            write(1, missatge, caracters_escrits);
            free(missatge);
        }
        return -1;
    }
    if (desconnectarItaca(configuracio, socket_actual) != 0) {
        return -1;
    }

    escriureMissatge("Leaving Ithaca...\n");
    caracters_escrits = asprintf(&missatge, "Sailing to %s...\n", nom_illa);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }
    return esperarPort(configuracio, socket_actual, nom_illa,configuracio->ip_illa_inicial, configuracio->port_illa_inicial);
}

/***********************************************
 *
 * @Finalitat: Busca una illa dins el mapa conegut per Odysseus.
 * @Parametres: in: configuracio = mapa conegut.
 *              in: nom_illa = nom que es vol buscar.
 * @Retorn: Retorna l'index de l'illa o -1 si no existeix.
 *
 ************************************************/
int buscarIllaConeguda(ConfiguracioOdisseu *configuracio, char *nom_illa) {
    int index_illa = -1, i = 0;

    for (i = 0; i < configuracio->nombre_illes_conegudes && index_illa < 0; i++) {
        if (strcasecmp(configuracio->illes_conegudes[i].nom, nom_illa) == 0) {
            index_illa = i;
        }
    }
    return index_illa;
}

/***********************************************
 *
 * @Finalitat: Afegeix una illa sense duplicar-la al mapa conegut.
 * @Parametres: in/out: configuracio = mapa conegut.
 *              in: nom_illa = nom de l'illa.
 *              in: ip = adreca de connexio de l'illa.
 *              in: port = port de connexio de l'illa.
 * @Retorn: Retorna l'index de l'illa o -1 si falla la reserva.
 *
 ************************************************/
int afegirIllaConeguda(ConfiguracioOdisseu *configuracio, char *nom_illa, char *ip, int port) {
    IllaConeguda *illes_ampliades = NULL, *illa = NULL;
    int index_illa = -1;

    index_illa = buscarIllaConeguda(configuracio, nom_illa);
    if (index_illa >= 0) {
        illa = &configuracio->illes_conegudes[index_illa];
        if (illa->ip == NULL && ip != NULL && copiarText(&illa->ip, ip) != 0) {
            return -1;
        }
        if (port > 0) {
            illa->port = port;
        }
        return index_illa;
    }
    if (configuracio->nombre_illes_conegudes >= MAX_ILLES_CONEGUDES) {
        return -1;
    }

    illes_ampliades = realloc(configuracio->illes_conegudes,(configuracio->nombre_illes_conegudes + 1) * sizeof(*illes_ampliades));
    if (illes_ampliades == NULL) {
        return -1;
    }
    configuracio->illes_conegudes = illes_ampliades;
    index_illa = configuracio->nombre_illes_conegudes;
    illa = &configuracio->illes_conegudes[index_illa];
    illa->nom = NULL;
    illa->ip = NULL;
    illa->port = port;
    illa->nombre_connexions = 0;
    illa->connexions = NULL;
    if (copiarText(&illa->nom, nom_illa) != 0) {
        return -1;
    }
    if (ip != NULL && copiarText(&illa->ip, ip) != 0) {
        free(illa->nom);
        illa->nom = NULL;
        return -1;
    }
    configuracio->nombre_illes_conegudes++;
    return index_illa;
}

/***********************************************
 *
 * @Finalitat: Afegeix una connexio directa sense duplicar-la.
 * @Parametres: in/out: illa = illa que coneix la connexio.
 *              in: nom_desti = nom de la destinacio directa.
 * @Retorn: Retorna 0 si la connexio existeix o s'afegeix i -1 si falla.
 *
 ************************************************/
int afegirConnexioConeguda(IllaConeguda *illa, char *nom_desti) {
    char **connexions_ampliades = NULL;
    int i = 0;

    for (i = 0; i < illa->nombre_connexions; i++) {
        if (strcasecmp(illa->connexions[i], nom_desti) == 0) {
            return 0;
        }
    }
    connexions_ampliades = realloc(illa->connexions, (illa->nombre_connexions + 1) * sizeof(*connexions_ampliades));
    if (connexions_ampliades == NULL) {
        return -1;
    }
    illa->connexions = connexions_ampliades;
    illa->connexions[illa->nombre_connexions] = NULL;
    if (copiarText(&illa->connexions[illa->nombre_connexions], nom_desti) != 0) {
        return -1;
    }
    illa->nombre_connexions++;
    return 0;
}

/***********************************************
 *
 * @Finalitat: Incorpora una ruta bidireccional al mapa conegut.
 * @Parametres: in/out: configuracio = mapa conegut.
 *              in: nom_origen = illa on s'ha comprat el mapa.
 *              in: nom_desti = illa comunicada pel mapa.
 *              in: ip_desti = adreca de la destinacio.
 *              in: port_desti = port de la destinacio.
 * @Retorn: Retorna 0 si incorpora la ruta i -1 altrament.
 *
 ************************************************/
int afegirRutaConeguda(ConfiguracioOdisseu *configuracio, char *nom_origen,char *nom_desti, char *ip_desti, int port_desti) {
    int index_origen = -1, index_desti = -1, resultat = 0;

    index_origen = buscarIllaConeguda(configuracio, nom_origen);
    if (index_origen < 0) {
        return -1;
    }
    index_desti = afegirIllaConeguda(configuracio, nom_desti, ip_desti,port_desti);
    if (index_desti < 0) {
        return -1;
    }
    resultat = afegirConnexioConeguda(&configuracio->illes_conegudes[index_origen],configuracio->illes_conegudes[index_desti].nom);
    if (resultat == 0) {
        resultat = afegirConnexioConeguda(&configuracio->illes_conegudes[index_desti],configuracio->illes_conegudes[index_origen].nom);
    }
    return resultat;
}

/***********************************************
 *
 * @Finalitat: Comprova una ruta directa i retorna la destinacio coneguda.
 * @Parametres: in: configuracio = mapa conegut.
 *              in: nom_origen = ubicacio actual.
 *              in: nom_desti = illa a la qual es vol navegar.
 * @Retorn: Retorna l'index de la destinacio directa o -1 si no ho es.
 *
 ************************************************/
int buscarDestiDirecte(ConfiguracioOdisseu *configuracio, char *nom_origen,char *nom_desti) {
    IllaConeguda *origen = NULL;
    int index_origen = -1, index_desti = -1, connexio_directa = 0, i = 0;

    index_origen = buscarIllaConeguda(configuracio, nom_origen);
    if (index_origen < 0) {
        return -1;
    }
    origen = &configuracio->illes_conegudes[index_origen];
    for (i = 0; i < origen->nombre_connexions && connexio_directa == 0; i++) {
        if (strcasecmp(origen->connexions[i], nom_desti) == 0) {
            connexio_directa = 1;
        }
    }
    if (connexio_directa == 1) {
        index_desti = buscarIllaConeguda(configuracio, nom_desti);
        if (index_desti >= 0 &&
            (configuracio->illes_conegudes[index_desti].ip == NULL ||
             configuracio->illes_conegudes[index_desti].port <= 0)) {
            index_desti = -1;
        }
    }
    return index_desti;
}

/***********************************************
 *
 * @Finalitat: Valida una ruta rebuda en comprar el mapa local.
 * @Parametres: in: trama = resposta 0x14 de l'illa.
 *              in: index_esperat = posicio esperada de la ruta.
 *              in/out: nombre_total = total de rutes de la compra.
 *              out: ruta = nom, IP i port rebuts.
 *              out: sense_rutes = indica una resposta OK&50&0.
 * @Retorn: Retorna 0 si la resposta es valida i -1 altrament.
 *
 ************************************************/
int interpretarRutaMapa(unsigned char *trama, int index_esperat,int *nombre_total, Ruta *ruta, int *sense_rutes) {
    unsigned char dades[MIDA_DADES_TRAMA + 1] = {0}; //REVISAR
    char *estat = NULL, *cost_text = NULL, *index_text = NULL;
    char *total_text = NULL, *nom_illa = NULL, *ip = NULL;
    char *port_text = NULL, *camp_extra = NULL;
    int longitud = 0, index = 0, total = 0, port = 0, i = 0;

    if (validarTrama(trama) != TRAMA_CORRECTA ||trama[POSICIO_TIPUS] != TIPUS_COMPRAR_MAPA ||trama[POSICIO_FLAGS] != FLAGS_RESPOSTA_CORRECTA) {
        return -1;
    }
    longitud = trama[POSICIO_LONGITUD_DADES];
    if (longitud == 0 || obtenirDades(trama, dades) != TRAMA_CORRECTA) {
        return -1;
    }
    for (i = 0; i < longitud; i++) {
        if (dades[i] == '\0') {
            return -1;
        }
    }
    dades[longitud] = '\0';
    estat = strtok((char *) dades, "&");
    cost_text = strtok(NULL, "&");
    index_text = strtok(NULL, "&");
    if (estat == NULL || cost_text == NULL || index_text == NULL ||strcmp(estat, "OK") != 0 || esNumero(cost_text) == 0 ||atoi(cost_text) != 50 || esNumero(index_text) == 0) {
        return -1;
    }

    index = atoi(index_text);
    *sense_rutes = 0;
    if (index == 0) {
        camp_extra = strtok(NULL, "&");
        if (index_esperat != 1 || camp_extra != NULL) {
            return -1;
        }
        *nombre_total = 0;
        *sense_rutes = 1;
        return 0;
    }

    total_text = strtok(NULL, "&");
    nom_illa = strtok(NULL, "&");
    ip = strtok(NULL, "&");
    port_text = strtok(NULL, "&");
    camp_extra = strtok(NULL, "&");
    if (total_text == NULL || nom_illa == NULL || ip == NULL ||port_text == NULL || camp_extra != NULL ||esNumero(total_text) == 0 || esNumero(port_text) == 0) {
        return -1;
    }
    total = atoi(total_text);
    port = atoi(port_text);
    if (index != index_esperat || total <= 0 || index > total || port <= 0 ||(*nombre_total != 0 && *nombre_total != total)) {
        return -1;
    }
    *nombre_total = total;
    if (copiarText(&ruta->nom_illa, nom_illa) != 0 ||copiarText(&ruta->ip, ip) != 0) {
        free(ruta->nom_illa);
        free(ruta->ip);
        ruta->nom_illa = NULL;
        ruta->ip = NULL;
        return -1;
    }
    ruta->port = port;
    return 0;
}

/***********************************************
 *
 * @Finalitat: Compra el mapa local i incorpora les rutes rebudes.
 * @Parametres: in/out: configuracio = diners i mapa conegut d'Odysseus.
 *              in: socket_actual = connexio activa amb l'illa.
 *              in: quantitat = quantitat indicada a BUY MAP.
 * @Retorn: Retorna 0 si completa la compra i -1 altrament.
 *
 ************************************************/
int comprarMapa(ConfiguracioOdisseu *configuracio, int socket_actual,int quantitat) {
    Ruta ruta = {0}, *rutes_rebudes = NULL, *rutes_ampliades = NULL;
    unsigned char peticio[MIDA_TRAMA] = {0}, resposta[MIDA_TRAMA] = {0}; //REVISAR
    unsigned char dades_error[MIDA_DADES_TRAMA + 1] = {0};
    char *missatge = NULL;
    int resultat = 0, nombre_total = 0, nombre_rutes = 0;
    int index = 1, sense_rutes = 0, i = 0, longitud = 0;
    int caracters_escrits = 0, mapa_complet = 0;

    if (socket_actual < 0 ||configuracio->tipus_connexio != CONNEXIO_ILLA ||configuracio->ubicacio_actual == NULL) {
        escriureMissatge("Not docked at an Island.\n");
        return -1;
    }
    if (quantitat != 1) {
        escriureMissatge("Only one map can be bought at a time.\n");
        return -1;
    }
    if (configuracio->diners < 50) {
        escriureMissatge("Not enough gold.\n");
        return -1;
    }

    resultat = crearTrama(peticio, TIPUS_COMPRAR_MAPA,FLAGS_PETICIO_TEXTUAL,(unsigned char *) "1", 1);
    if (resultat == TRAMA_CORRECTA) {
        resultat = enviarTrama(socket_actual, peticio);
    }
    if (resultat == TRAMA_CORRECTA) {
        resultat = rebreTrama(socket_actual, resposta);
    }
    if (resultat == TRAMA_CORRECTA &&validarTrama(resposta) == TRAMA_CORRECTA &&resposta[POSICIO_TIPUS] == TIPUS_COMPRAR_MAPA &&resposta[POSICIO_FLAGS] == FLAGS_RESPOSTA_ERROR) {
        longitud = resposta[POSICIO_LONGITUD_DADES];
        obtenirDades(resposta, dades_error);
        dades_error[longitud] = '\0';
        caracters_escrits = asprintf(&missatge, "Map purchase failed: %s.\n", (char *) dades_error);
        if (caracters_escrits >= 0) {
            write(1, missatge, caracters_escrits);
            free(missatge);
        }
        return -1;
    }

    while (resultat == TRAMA_CORRECTA && mapa_complet == 0) {
        ruta.nom_illa = NULL;
        ruta.ip = NULL;
        ruta.port = 0;
        resultat = interpretarRutaMapa(resposta, index, &nombre_total,&ruta, &sense_rutes);
        if (resultat == TRAMA_CORRECTA && sense_rutes == 0) {
            rutes_ampliades = realloc(rutes_rebudes,(nombre_rutes + 1) * sizeof(*rutes_ampliades));
            if (rutes_ampliades == NULL) {
                free(ruta.nom_illa);
                free(ruta.ip);
                resultat = -1;
            } else {
                rutes_rebudes = rutes_ampliades;
                rutes_rebudes[nombre_rutes] = ruta;
                nombre_rutes++;
            }
        }
        if (resultat == TRAMA_CORRECTA) {
            if (sense_rutes == 1 || index >= nombre_total) {
                mapa_complet = 1;
            } else {
                index++;
                resultat = rebreTrama(socket_actual, resposta);
            }
        }
    }

    if (resultat == TRAMA_CORRECTA && sense_rutes == 0 &&nombre_rutes != nombre_total) {
        resultat = -1;
    }
    for (i = 0; i < nombre_rutes && resultat == TRAMA_CORRECTA; i++) {
        resultat = afegirRutaConeguda(configuracio,configuracio->ubicacio_actual, rutes_rebudes[i].nom_illa,rutes_rebudes[i].ip, rutes_rebudes[i].port);
    }
    for (i = 0; i < nombre_rutes; i++) {
        free(rutes_rebudes[i].nom_illa);
        free(rutes_rebudes[i].ip);
    }
    free(rutes_rebudes);
    if (resultat != TRAMA_CORRECTA) {
        escriureMissatge("Error: invalid map response.\n");
        return -1;
    }

    configuracio->diners = configuracio->diners - 50;
    caracters_escrits = asprintf(&missatge, "Bought the map of %s for 50 gold.\n""New routes have been charted.\n",configuracio->ubicacio_actual);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }
    return 0;
}

/***********************************************
 *
 * @Finalitat: Mostra localment el mapa acumulat mitjancant SPHRAGIS.
 * @Parametres: in: configuracio = illes i connexions conegudes.
 * @Retorn: Retorna 0 si SPHRAGIS mostra el mapa i -1 altrament.
 *
 ************************************************/
int mostrarMapa(ConfiguracioOdisseu *configuracio) {
    SPHRAGIS_Island *mapa = NULL;
    int resultat = 0, i = 0;

    if (configuracio->nombre_illes_conegudes < 0 ||configuracio->nombre_illes_conegudes > MAX_ILLES_CONEGUDES ||(configuracio->nombre_illes_conegudes > 0 && strcasecmp(configuracio->illes_conegudes[0].nom,NOM_ILLA_INICIAL) != 0)) {
        return -1;
    }
    if (configuracio->nombre_illes_conegudes > 0) {
        mapa = malloc(configuracio->nombre_illes_conegudes * sizeof(*mapa));
        if (mapa == NULL) {
            return -1;
        }
    }
    for (i = 0; i < configuracio->nombre_illes_conegudes; i++) {
        mapa[i].name = configuracio->illes_conegudes[i].nom;
        mapa[i].known_islands = configuracio->illes_conegudes[i].connexions;
        mapa[i].known_island_count =configuracio->illes_conegudes[i].nombre_connexions;
    }
    resultat = SPHRAGIS_display_map(mapa,configuracio->nombre_illes_conegudes);
    free(mapa);
    if (resultat != SPHRAGIS_OK) {
        escriureMissatge("Error: SPHRAGIS could not display the map.\n");
        return -1;
    }
    return 0;
}

/***********************************************
 *
 * @Finalitat: Demana la sortida controlada de l'illa actual.
 * @Parametres: in/out: configuracio = estat de connexio d'Odysseus.
 *              in/out: socket_actual = connexio activa amb l'illa.
 * @Retorn: Retorna 0 si l'illa allibera la placa i -1 altrament.
 *
 ************************************************/
int sortirIlla(ConfiguracioOdisseu *configuracio, int *socket_actual) {
    unsigned char peticio[MIDA_TRAMA] = {0}, resposta[MIDA_TRAMA] = {0}; //REVISAR
    unsigned char dades[MIDA_DADES_TRAMA + 1] = {0};
    int resultat = 0, longitud = 0;

    resultat = crearTrama(peticio, TIPUS_SORTIDA_ILLA,FLAGS_PETICIO_TEXTUAL, NULL, 0);
    if (resultat == TRAMA_CORRECTA) {
        resultat = enviarTrama(*socket_actual, peticio);
    }
    if (resultat == TRAMA_CORRECTA) {
        resultat = rebreTrama(*socket_actual, resposta);
    }
    if (resultat != TRAMA_CORRECTA) {
        close(*socket_actual);
        *socket_actual = -1;
        configuracio->tipus_connexio = SENSE_CONNEXIO;
        escriureMissatge("Error: connection with Island was lost.\n");
        return -1;
    }
    if (validarTrama(resposta) != TRAMA_CORRECTA ||resposta[POSICIO_TIPUS] != TIPUS_SORTIDA_ILLA) {
        close(*socket_actual);
        *socket_actual = -1;
        configuracio->tipus_connexio = SENSE_CONNEXIO;
        escriureMissatge("Error: invalid departure response.\n");
        return -1;
    }
    longitud = resposta[POSICIO_LONGITUD_DADES];
    if (longitud > 0) {
        obtenirDades(resposta, dades);
        dades[longitud] = '\0';
    }
    if (resposta[POSICIO_FLAGS] == FLAGS_RESPOSTA_ERROR) {
        escriureMissatge("Island rejected the departure.\n");
        return -1;
    }
    if (resposta[POSICIO_FLAGS] != FLAGS_RESPOSTA_CORRECTA ||longitud != 2 || strcmp((char *) dades, "OK") != 0) {
        close(*socket_actual);
        *socket_actual = -1;
        configuracio->tipus_connexio = SENSE_CONNEXIO;
        escriureMissatge("Error: invalid departure response.\n");
        return -1;
    }

    close(*socket_actual);
    *socket_actual = -1;
    configuracio->tipus_connexio = SENSE_CONNEXIO;
    return 0;
}

/***********************************************
 *
 * @Finalitat: Navega a una connexio directa de l'illa actual.
 * @Parametres: in/out: configuracio = mapa, ubicacio i connexio actuals.
 *              in/out: socket_actual = connexio amb l'illa.
 *              in: nom_illa = destinacio demanada.
 * @Retorn: Retorna 0 si atraca, ODISSEU_MORT si mor o -1 si falla.
 *
 ************************************************/
int navegarEntreIlles(ConfiguracioOdisseu *configuracio, int *socket_actual, char *nom_illa) {
    IllaConeguda *desti = NULL;
    char *missatge = NULL;
    int index_desti = -1, caracters_escrits = 0;

    if (*socket_actual < 0 ||configuracio->tipus_connexio != CONNEXIO_ILLA ||configuracio->ubicacio_actual == NULL) {
        escriureMissatge("Not docked at an Island.\n");
        return -1;
    }
    index_desti = buscarDestiDirecte(configuracio,configuracio->ubicacio_actual, nom_illa);
    if (index_desti < 0) {
        caracters_escrits = asprintf(&missatge, "We can't sail to %s from here...\n", nom_illa);
        if (caracters_escrits >= 0) {
            write(1, missatge, caracters_escrits);
            free(missatge);
        }
        return -1;
    }
    desti = &configuracio->illes_conegudes[index_desti];
    if (sortirIlla(configuracio, socket_actual) != 0) {
        return -1;
    }

    caracters_escrits = asprintf(&missatge, "Leaving %s...\n",configuracio->ubicacio_actual);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
        missatge = NULL;
    }
    caracters_escrits = asprintf(&missatge, "Sailing to %s...\n", desti->nom);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }
    return esperarPort(configuracio, socket_actual, desti->nom, desti->ip,desti->port);
}

/***********************************************
 *
 * @Finalitat: Valida una entrada rebuda amb LIST MARKET.
 * @Parametres: in: trama = resposta enviada per l'illa.
 *              in: index_esperat = posicio esperada de la resposta.
 *              in/out: nombre_total = total de respostes del mercat.
 *              out: producte = producte extret de la resposta.
 *              out: es_mapa = indica si l'entrada correspon al mapa.
 * @Retorn: Retorna 0 si l'entrada es valida i -1 altrament.
 *
 ************************************************/
int interpretarEntradaMercat(unsigned char *trama, int index_esperat,int *nombre_total, Producte *producte,int *es_mapa) {
    unsigned char dades[MIDA_DADES_TRAMA + 1] = {0}; //REVISAR
    char *index_text = NULL, *total_text = NULL, *nom_producte = NULL;
    char *quantitat_text = NULL, *preu_text = NULL, *camp_extra = NULL;
    int longitud = 0, index = 0, total = 0, llargada_nom = 0;

    if (validarTrama(trama) != TRAMA_CORRECTA ||trama[POSICIO_TIPUS] != TIPUS_LLISTAR_MERCAT ||trama[POSICIO_FLAGS] != FLAGS_RESPOSTA_CORRECTA) {
        return -1;
    }
    longitud = trama[POSICIO_LONGITUD_DADES];
    if (longitud == 0 || obtenirDades(trama, dades) != TRAMA_CORRECTA) {
        return -1;
    }
    dades[longitud] = '\0';

    index_text = strtok((char *) dades, "&");
    total_text = strtok(NULL, "&");
    nom_producte = strtok(NULL, "&");
    quantitat_text = strtok(NULL, "&");
    preu_text = strtok(NULL, "&");
    camp_extra = strtok(NULL, "&");
    if (index_text == NULL || total_text == NULL || nom_producte == NULL ||quantitat_text == NULL || preu_text == NULL || camp_extra != NULL ||esNumero(index_text) == 0 || esNumero(total_text) == 0 ||esNumero(quantitat_text) == 0 || esNumero(preu_text) == 0) {
        return -1;
    }

    index = atoi(index_text);
    total = atoi(total_text);
    if (index != index_esperat || total <= 0 || index > total ||(*nombre_total != 0 && *nombre_total != total)) {
        return -1;
    }
    *nombre_total = total;
    *es_mapa = 0;

    if (strcasecmp(nom_producte, "MAP") == 0) {
        if (index != total || atoi(quantitat_text) != 1 ||atoi(preu_text) != 50) {
            return -1;
        }
        *es_mapa = 1;
        return 0;
    }
    if (index == total) {
        return -1;
    }

    while (nom_producte[llargada_nom] != '\0') {
        llargada_nom++;
    }
    if (llargada_nom == 0 || llargada_nom >= MIDA_NOM_PRODUCTE) {
        return -1;
    }
    snprintf(producte->nom, MIDA_NOM_PRODUCTE, "%s", nom_producte); //REVISAR
    producte->quantitat = atoi(quantitat_text);
    producte->preu = atoi(preu_text);
    return 0;
}

/***********************************************
 *
 * @Finalitat: Demana i mostra el mercat actual de l'illa.
 * @Parametres: in/out: configuracio = estat i mercat d'Odysseus.
 *              in: socket_actual = connexio activa amb l'illa.
 * @Retorn: Retorna 0 si rep el mercat complet i -1 altrament.
 *
 ************************************************/
int llistarMercat(ConfiguracioOdisseu *configuracio, int socket_actual) {
    Producte producte = {0}, *productes = NULL, *productes_ampliats = NULL;
    unsigned char peticio[MIDA_TRAMA] = {0}, resposta[MIDA_TRAMA] = {0}; //REVISAR
    char *nom_majuscules = NULL, *missatge = NULL;
    int resultat = 0, nombre_total = 0, nombre_productes = 0;
    int index = 1, es_mapa = 0, mapa_rebut = 0, longitud_nom = 0;
    int i = 0, caracters_escrits = 0;

    if (socket_actual < 0 ||configuracio->tipus_connexio != CONNEXIO_ILLA ||configuracio->ubicacio_actual == NULL) {
        escriureMissatge("Not docked at an Island.\n");
        return -1;
    }

    free(configuracio->productes_mercat);
    configuracio->productes_mercat = NULL;
    configuracio->nombre_productes_mercat = 0;

    resultat = crearTrama(peticio, TIPUS_LLISTAR_MERCAT,FLAGS_PETICIO_TEXTUAL, NULL, 0);
    if (resultat == TRAMA_CORRECTA) {
        resultat = enviarTrama(socket_actual, peticio);
    }

    while (configuracio->ubicacio_actual[longitud_nom] != '\0') {
        longitud_nom++;
    }
    nom_majuscules = malloc((longitud_nom + 1) * sizeof(*nom_majuscules));
    if (nom_majuscules == NULL) {
        return -1;
    }
    for (i = 0; i < longitud_nom; i++) {
        nom_majuscules[i] = toupper((unsigned char) configuracio->ubicacio_actual[i]);
    }
    nom_majuscules[longitud_nom] = '\0';
    caracters_escrits = asprintf(&missatge, "--- %s MARKET ---\n",nom_majuscules);
    free(nom_majuscules);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
        missatge = NULL;
    }

    while (resultat == TRAMA_CORRECTA &&(nombre_total == 0 || index <= nombre_total)) {
        resultat = rebreTrama(socket_actual, resposta);
        if (resultat == TRAMA_CORRECTA) {
            resultat = interpretarEntradaMercat(resposta, index, &nombre_total, &producte, &es_mapa);
        }
        if (resultat == TRAMA_CORRECTA && es_mapa == 1) {
            mapa_rebut = 1;
            escriureMissatge("Map 50 gold\n");
        } else if (resultat == TRAMA_CORRECTA) {
            productes_ampliats = realloc(productes,(nombre_productes + 1) * sizeof(*productes_ampliats));
            if (productes_ampliats == NULL) {
                resultat = -1;
            } else {
                productes = productes_ampliats;
                productes[nombre_productes] = producte;
                nombre_productes++;
                caracters_escrits = asprintf(&missatge,"%s %d kg %d gold/kg\n", producte.nom,producte.quantitat, producte.preu);
                if (caracters_escrits >= 0) {
                    write(1, missatge, caracters_escrits);
                    free(missatge);
                    missatge = NULL;
                }
            }
        }
        index++;
    }

    if (resultat != TRAMA_CORRECTA || mapa_rebut == 0 ||nombre_productes != nombre_total - 1) {
        free(productes);
        escriureMissatge("Error: invalid market response.\n");
        return -1;
    }
    configuracio->productes_mercat = productes;
    configuracio->nombre_productes_mercat = nombre_productes;
    return 0;
}

/***********************************************
 *
 * @Finalitat: Valida una resposta de compra o venda.
 * @Parametres: in: resposta = trama rebuda de l'illa.
 *              in: tipus = operacio de compra o venda esperada.
 *              in: producte_esperat = producte enviat a l'illa.
 *              in: quantitat_esperada = quantitat enviada.
 *              out: valor = cost de la compra o ingres de la venda.
 * @Retorn: Retorna 0 si l'operacio s'ha completat i -1 altrament.
 *
 ************************************************/
int rebreRespostaComerc(unsigned char *resposta, int tipus,char *producte_esperat, int quantitat_esperada,int *valor) {
    unsigned char dades[MIDA_DADES_TRAMA + 1] = {0}; //REVISAR
    char *estat = NULL, *producte = NULL, *quantitat_text = NULL;
    char *valor_text = NULL, *camp_extra = NULL, *missatge = NULL;
    int longitud = 0, caracters_escrits = 0;

    if (validarTrama(resposta) != TRAMA_CORRECTA ||resposta[POSICIO_TIPUS] != tipus) {
        return -1;
    }
    longitud = resposta[POSICIO_LONGITUD_DADES];
    if (longitud == 0 || obtenirDades(resposta, dades) != TRAMA_CORRECTA) {
        return -1;
    }
    dades[longitud] = '\0';
    if (resposta[POSICIO_FLAGS] == FLAGS_RESPOSTA_ERROR) {
        caracters_escrits = asprintf(&missatge,"Market operation failed: %s.\n", (char *) dades);
        if (caracters_escrits >= 0) {
            write(1, missatge, caracters_escrits);
            free(missatge);
        }
        return -1;
    }
    if (resposta[POSICIO_FLAGS] != FLAGS_RESPOSTA_CORRECTA) {
        return -1;
    }

    estat = strtok((char *) dades, "&");
    producte = strtok(NULL, "&");
    quantitat_text = strtok(NULL, "&");
    valor_text = strtok(NULL, "&");
    camp_extra = strtok(NULL, "&");
    if (estat == NULL || producte == NULL || quantitat_text == NULL ||valor_text == NULL || camp_extra != NULL || strcmp(estat, "OK") != 0 ||strcasecmp(producte, producte_esperat) != 0 ||esNumero(quantitat_text) == 0 || esNumero(valor_text) == 0 ||atoi(quantitat_text) != quantitat_esperada) {
        return -1;
    }
    *valor = atoi(valor_text);
    return 0;
}

/***********************************************
 *
 * @Finalitat: Compra aliments al mercat de l'illa actual.
 * @Parametres: in/out: configuracio = diners, carrega i mercat d'Odysseus.
 *              in: socket_actual = connexio activa amb l'illa.
 *              in: nom_producte = aliment que es vol comprar.
 *              in: quantitat = quilograms que es volen comprar.
 * @Retorn: Retorna 0 si completa la compra i -1 altrament.
 *
 ************************************************/
int comprarAliment(ConfiguracioOdisseu *configuracio, int socket_actual, char *nom_producte, int quantitat) {
    Aliment *aliments_ampliats = NULL;
    Producte *producte_mercat = NULL;
    unsigned char peticio[MIDA_TRAMA] = {0}, resposta[MIDA_TRAMA] = {0}; //REVISAR
    char *dades = NULL, *missatge = NULL;
    int index_mercat = -1, index_aliment = -1, aliment_preparat = 0;
    int cost_esperat = 0, cost = 0, longitud = 0, resultat = 0;
    int i = 0, caracters_escrits = 0;

    if (socket_actual < 0 || configuracio->tipus_connexio != CONNEXIO_ILLA) {
        escriureMissatge("Not docked at an Island.\n");
        return -1;
    }
    if (strcasecmp(nom_producte, "MAP") == 0) {
        escriureMissatge("Map purchases are not available yet.\n");
        return -1;
    }
    for (i = 0; i < configuracio->nombre_productes_mercat && index_mercat < 0; i++) {
        if (strcasecmp(configuracio->productes_mercat[i].nom, nom_producte) == 0) {
            index_mercat = i;
        }
    }
    if (index_mercat < 0) {
        escriureMissatge("List the market before buying this product.\n");
        return -1;
    }
    producte_mercat = &configuracio->productes_mercat[index_mercat];
    cost_esperat = quantitat * producte_mercat->preu;
    if (configuracio->diners < cost_esperat) {
        escriureMissatge("Not enough gold.\n");
        return -1;
    }

    for (i = 0; i < configuracio->nombre_aliments && index_aliment < 0; i++) {
        if (strcasecmp(configuracio->aliments[i].nom, producte_mercat->nom) == 0) {
            index_aliment = i;
        }
    }
    if (index_aliment < 0) {
        aliments_ampliats = realloc(configuracio->aliments, (configuracio->nombre_aliments + 1) * sizeof(*aliments_ampliats));
        if (aliments_ampliats == NULL) {
            return -1;
        }
        configuracio->aliments = aliments_ampliats;
        index_aliment = configuracio->nombre_aliments;
        configuracio->aliments[index_aliment].nom = NULL;
        configuracio->aliments[index_aliment].quantitat = 0;
        if (copiarText(&configuracio->aliments[index_aliment].nom,producte_mercat->nom) != 0) {
            return -1;
        }
        aliment_preparat = 1;
    }

    longitud = asprintf(&dades, "%s&%d", producte_mercat->nom, quantitat);
    if (longitud < 0 || longitud > MIDA_DADES_TRAMA) {
        resultat = -1;
    } else {
        resultat = crearTrama(peticio, TIPUS_COMPRAR, FLAGS_PETICIO_TEXTUAL,(unsigned char *) dades, longitud);
    }
    free(dades);
    if (resultat == TRAMA_CORRECTA) {
        resultat = enviarTrama(socket_actual, peticio);
    }
    if (resultat == TRAMA_CORRECTA) {
        resultat = rebreTrama(socket_actual, resposta);
    }
    if (resultat == TRAMA_CORRECTA) {
        resultat = rebreRespostaComerc(resposta, TIPUS_COMPRAR,producte_mercat->nom, quantitat, &cost);
    }
    if (resultat != TRAMA_CORRECTA || cost != cost_esperat) {
        if (aliment_preparat == 1) {
            free(configuracio->aliments[index_aliment].nom);
            configuracio->aliments[index_aliment].nom = NULL;
        }
        if (resultat == TRAMA_CORRECTA) {
            escriureMissatge("Error: invalid BUY response.\n");
        }
        return -1;
    }

    configuracio->diners = configuracio->diners - cost;
    configuracio->aliments[index_aliment].quantitat =configuracio->aliments[index_aliment].quantitat + quantitat;
    if (aliment_preparat == 1) {
        configuracio->nombre_aliments++;
    }
    if (producte_mercat->quantitat >= quantitat) {
        producte_mercat->quantitat = producte_mercat->quantitat - quantitat;
    } else {
        producte_mercat->quantitat = 0;
    }

    caracters_escrits = asprintf(&missatge,"Bought %d kg of %s for %d gold.\n", quantitat,producte_mercat->nom, cost);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }
    return 0;
}

/***********************************************
 *
 * @Finalitat: Ven aliments de la carrega al mercat de l'illa actual.
 * @Parametres: in/out: configuracio = diners, carrega i mercat d'Odysseus.
 *              in: socket_actual = connexio activa amb l'illa.
 *              in: nom_producte = aliment que es vol vendre.
 *              in: quantitat = quilograms que es volen vendre.
 * @Retorn: Retorna 0 si completa la venda i -1 altrament.
 *
 ************************************************/
int vendreAliment(ConfiguracioOdisseu *configuracio, int socket_actual,char *nom_producte, int quantitat) {
    unsigned char peticio[MIDA_TRAMA] = {0}, resposta[MIDA_TRAMA] = {0}; //REVISAR
    char *dades = NULL, *missatge = NULL;
    int index_aliment = -1, index_mercat = -1, ingres = 0;
    int longitud = 0, resultat = 0, i = 0, caracters_escrits = 0;

    if (socket_actual < 0 ||configuracio->tipus_connexio != CONNEXIO_ILLA) {
        escriureMissatge("Not docked at an Island.\n");
        return -1;
    }
    if (strcasecmp(nom_producte, "MAP") == 0) {
        escriureMissatge("Maps cannot be sold.\n");
        return -1;
    }
    for (i = 0; i < configuracio->nombre_aliments && index_aliment < 0; i++) {
        if (strcasecmp(configuracio->aliments[i].nom, nom_producte) == 0) {
            index_aliment = i;
        }
    }
    if (index_aliment < 0 ||configuracio->aliments[index_aliment].quantitat < quantitat) {
        escriureMissatge("Not enough cargo to sell.\n");
        return -1;
    }

    longitud = asprintf(&dades, "%s&%d", configuracio->aliments[index_aliment].nom,quantitat);
    if (longitud < 0 || longitud > MIDA_DADES_TRAMA) {
        resultat = -1;
    } else {
        resultat = crearTrama(peticio, TIPUS_VENDRE, FLAGS_PETICIO_TEXTUAL,(unsigned char *) dades, longitud);
    }
    free(dades);
    if (resultat == TRAMA_CORRECTA) {
        resultat = enviarTrama(socket_actual, peticio);
    }
    if (resultat == TRAMA_CORRECTA) {
        resultat = rebreTrama(socket_actual, resposta);
    }
    if (resultat == TRAMA_CORRECTA) {
        resultat = rebreRespostaComerc(resposta, TIPUS_VENDRE,configuracio->aliments[index_aliment].nom, quantitat, &ingres);
    }
    if (resultat != TRAMA_CORRECTA) {
        return -1;
    }

    configuracio->aliments[index_aliment].quantitat =configuracio->aliments[index_aliment].quantitat - quantitat;
    configuracio->diners = configuracio->diners + ingres;
    for (i = 0; i < configuracio->nombre_productes_mercat &&index_mercat < 0; i++) {
        if (strcasecmp(configuracio->productes_mercat[i].nom,configuracio->aliments[index_aliment].nom) == 0) {
            index_mercat = i;
        }
    }
    if (index_mercat >= 0) {
        configuracio->productes_mercat[index_mercat].quantitat =configuracio->productes_mercat[index_mercat].quantitat +quantitat;
    }

    caracters_escrits = asprintf(&missatge,"Sold %d kg of %s for %d gold.\n", quantitat,configuracio->aliments[index_aliment].nom, ingres);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }
    return 0;
}

/***********************************************
 *
 * @Finalitat: Mostra localment l'estat actual d'Odysseus.
 * @Parametres: in: configuracio = dades actuals d'Odysseus.
 * @Retorn: Retorna 0 si mostra l'estat i -1 si falla la reserva.
 *
 ************************************************/
int mostrarEstat(ConfiguracioOdisseu *configuracio) {
    char *nom_majuscules = NULL, *missatge = NULL, *ubicacio = NULL;
    int longitud_nom = 0, i = 0, caracters_escrits = 0;
    int total_provisions = 0;

    while (configuracio->nom[longitud_nom] != '\0') {
        longitud_nom++;
    }
    nom_majuscules = malloc((longitud_nom + 1) * sizeof(*nom_majuscules));
    if (nom_majuscules == NULL) {
        return -1;
    }
    for (i = 0; i < longitud_nom; i++) {
        nom_majuscules[i] = toupper((unsigned char) configuracio->nom[i]);
    }
    nom_majuscules[longitud_nom] = '\0';
    caracters_escrits = asprintf(&missatge, "--- %s ---\nGold: %d\n\nCargo:\n", nom_majuscules, configuracio->diners);
    free(nom_majuscules);
    if (caracters_escrits < 0) {
        return -1;
    }
    write(1, missatge, caracters_escrits);
    free(missatge);
    missatge = NULL;

    for (i = 0; i < configuracio->nombre_aliments; i++) {
        if (configuracio->aliments[i].quantitat > 0) {
            caracters_escrits = asprintf(&missatge, "%s: %d kg\n",configuracio->aliments[i].nom,configuracio->aliments[i].quantitat);
            if (caracters_escrits < 0) {
                return -1;
            }
            write(1, missatge, caracters_escrits);
            free(missatge);
            missatge = NULL;
        }
    }
    total_provisions = calcularTotalProvisions(configuracio);
    caracters_escrits = asprintf(&missatge,"Total provisions: %d kg\n\n", total_provisions);
    if (caracters_escrits < 0) {
        return -1;
    }
    write(1, missatge, caracters_escrits);
    free(missatge);
    missatge = NULL;

    if (configuracio->identificador_viatge == 0) {
        escriureMissatge("Current voyage: None\n");
    } else {
        caracters_escrits = asprintf(&missatge, "Current voyage: %s -> %s\nReward: %d\n", configuracio->objecte_viatge, configuracio->illa_desti_viatge, configuracio->recompensa_viatge);
        if (caracters_escrits < 0) {
            return -1;
        }
        write(1, missatge, caracters_escrits);
        free(missatge);
        missatge = NULL;
    }

    ubicacio = configuracio->ubicacio_actual;
    if (ubicacio == NULL) {
        ubicacio = "Ithaca";
    }
    caracters_escrits = asprintf(&missatge, "\nCurrent location: %s\n",ubicacio);
    if (caracters_escrits < 0) {
        return -1;
    }
    write(1, missatge, caracters_escrits);
    free(missatge);
    return 0;
}

/***********************************************
 *
 * @Finalitat: Comprova si un text conte nomes digits.
 * @Parametres: in: text = text que es vol comprovar.
 * @Retorn: Retorna 1 si es un numero i 0 altrament.
 *
 ************************************************/
int esNumero(char *text) {
    int i = 0;

    if (text == NULL || text[0] == '\0') {
        return 0;
    }
    while (text[i] != '\0') {
        if (text[i] < '0' || text[i] > '9') {
            return 0;
        }
        i++;
    }
    return 1;
}

/***********************************************
 *
 * @Finalitat: Analitza la comanda CONNECT.
 * @Parametres: in: paraules = paraules de la comanda.
 *              in: nombre_paraules = nombre de paraules llegides.
 *              out: comanda = comanda reconeguda.
 * @Retorn: Retorna el resultat de l'analisi.
 *
 ************************************************/
int analitzarConnexio(char *paraules[], int nombre_paraules, Comanda *comanda) {
    comanda->tipus = COMANDA_CONNECT;
    if (nombre_paraules == 2 &&
        strcasecmp(paraules[1], "ITHACA") == 0) {
        return 1;
    }
    return 0;
}

/***********************************************
 *
 * @Finalitat: Analitza les comandes LIST VOYAGES i LIST MARKET.
 * @Parametres: in: paraules = paraules de la comanda.
 *              in: nombre_paraules = nombre de paraules llegides.
 *              out: comanda = comanda reconeguda.
 * @Retorn: Retorna el resultat de l'analisi.
 *
 ************************************************/
int analitzarLlista(char *paraules[], int nombre_paraules, Comanda *comanda) {
    if (nombre_paraules < 2) {
        return -1;
    }
    if (strcasecmp(paraules[1], "VOYAGES") == 0) {
        comanda->tipus = COMANDA_LIST_VOYAGES;
    } else if (strcasecmp(paraules[1], "MARKET") == 0) {
        comanda->tipus = COMANDA_LIST_MARKET;
    } else {
        return -1;
    }

    if (nombre_paraules != 2) {
        return 0;
    }
    return 1;
}

/***********************************************
 *
 * @Finalitat: Analitza la comanda ACCEPT i extreu l'identificador.
 * @Parametres: in: paraules = paraules de la comanda.
 *              in: nombre_paraules = nombre de paraules llegides.
 *              out: comanda = comanda i identificador extret.
 * @Retorn: Retorna el resultat de l'analisi.
 *
 ************************************************/
int analitzarAcceptacio(char *paraules[], int nombre_paraules, Comanda *comanda) {
    int identificador = 0;

    comanda->tipus = COMANDA_ACCEPT;
    if (nombre_paraules == 2 && esNumero(paraules[1]) == 1) {
        identificador = atoi(paraules[1]);
        if (identificador > 0) {
            comanda->valor = identificador;
            return 1;
        }
    }
    return 0;
}

/***********************************************
 *
 * @Finalitat: Analitza la comanda SAIL i extreu el nom de l'illa.
 * @Parametres: in: paraules = paraules de la comanda.
 *              in: nombre_paraules = nombre de paraules llegides.
 *              out: comanda = comanda i illa extreta.
 * @Retorn: Retorna el resultat de l'analisi.
 *
 ************************************************/
int analitzarNavegacio(char *paraules[], int nombre_paraules, Comanda *comanda) {
    comanda->tipus = COMANDA_SAIL;
    if (nombre_paraules == 2) {
        comanda->argument = paraules[1];
        return 1;
    }
    return 0;
}

/***********************************************
 *
 * @Finalitat: Analitza una comanda BUY o SELL.
 * @Parametres: in: paraules = paraules de la comanda.
 *              in: nombre_paraules = nombre de paraules llegides.
 *              in: tipus = tipus de compra o venda.
 *              out: comanda = comanda i arguments extrets.
 * @Retorn: Retorna el resultat de l'analisi.
 *
 ************************************************/
int analitzarCompraVenda(char *paraules[], int nombre_paraules, int tipus, Comanda *comanda) {
    int quantitat = 0;

    comanda->tipus = tipus;
    if (nombre_paraules == 3 && esNumero(paraules[2]) == 1) {
        quantitat = atoi(paraules[2]);
        if (quantitat > 0) {
            comanda->argument = paraules[1];
            comanda->valor = quantitat;
            return 1;
        }
    }
    return 0;
}

/***********************************************
 *
 * @Finalitat: Analitza una comanda que no admet arguments.
 * @Parametres: in: nombre_paraules = nombre de paraules llegides.
 *              in: tipus = tipus de comanda reconeguda.
 *              out: comanda = comanda reconeguda.
 * @Retorn: Retorna el resultat de l'analisi.
 *
 ************************************************/
int analitzarSenseArguments(int nombre_paraules, int tipus, Comanda *comanda) {
    comanda->tipus = tipus;
    if (nombre_paraules == 1) {
        return 1;
    }
    return 0;
}

/***********************************************
 *
 * @Finalitat: Separa i despatxa una comanda al seu analitzador.
 * @Parametres: in/out: linia = linia que conte la comanda.
 *              out: comanda = tipus i arguments extrets.
 * @Retorn: Retorna si la comanda es correcta, incorrecta o desconeguda.
 *
 ************************************************/
int analitzarComanda(char *linia, Comanda *comanda) {
    char *paraules[MAX_PARAULES] = {NULL}, *paraula = NULL;
    int nombre_paraules = 0;

    comanda->tipus = COMANDA_DESCONEGUDA;
    comanda->argument = NULL;
    comanda->valor = 0;

    paraula = strtok(linia, " ");
    while (paraula != NULL && nombre_paraules < MAX_PARAULES) {
        paraules[nombre_paraules] = paraula;
        nombre_paraules++;
        paraula = strtok(NULL, " ");
    }

    if (nombre_paraules == 0) {
        return -1;
    }
    if (strcasecmp(paraules[0], "CONNECT") == 0) {
        return analitzarConnexio(paraules, nombre_paraules, comanda);
    }
    if (strcasecmp(paraules[0], "LIST") == 0) {
        return analitzarLlista(paraules, nombre_paraules, comanda);
    }
    if (strcasecmp(paraules[0], "ACCEPT") == 0) {
        return analitzarAcceptacio(paraules, nombre_paraules, comanda);
    }
    if (strcasecmp(paraules[0], "SAIL") == 0) {
        return analitzarNavegacio(paraules, nombre_paraules, comanda);
    }
    if (strcasecmp(paraules[0], "BUY") == 0) {
        return analitzarCompraVenda(paraules, nombre_paraules, COMANDA_BUY, comanda);
    }
    if (strcasecmp(paraules[0], "SELL") == 0) {
        return analitzarCompraVenda(paraules, nombre_paraules, COMANDA_SELL, comanda);
    }
    if (strcasecmp(paraules[0], "MAP") == 0) {
        return analitzarSenseArguments(nombre_paraules, COMANDA_MAP, comanda);
    }
    if (strcasecmp(paraules[0], "STATUS") == 0) {
        return analitzarSenseArguments(nombre_paraules, COMANDA_STATUS,comanda);
    }
    if (strcasecmp(paraules[0], "DELIVER") == 0) {
        return analitzarSenseArguments(nombre_paraules, COMANDA_DELIVER, comanda);
    }
    if (strcasecmp(paraules[0], "CLAIM") == 0) {
        return analitzarSenseArguments(nombre_paraules, COMANDA_CLAIM, comanda);
    }
    return -1;
}

/***********************************************
 *
 * @Finalitat: Mostra la sintaxi esperada per una comanda coneguda.
 * @Parametres: in: tipus = tipus de comanda amb sintaxi incorrecta.
 * @Retorn: Retorna 0 si pot escriure i -1 si es produeix un error.
 *
 ************************************************/
int mostrarUsComanda(int tipus) {
    char *us = NULL;

    switch (tipus) {
    case COMANDA_CONNECT:
        us = "Usage: CONNECT ITHACA\n";
        break;
    case COMANDA_LIST_VOYAGES:
        us = "Usage: LIST VOYAGES\n";
        break;
    case COMANDA_ACCEPT:
        us = "Usage: ACCEPT <voyage_id>\n";
        break;
    case COMANDA_SAIL:
        us = "Usage: SAIL <island>\n";
        break;
    case COMANDA_MAP:
        us = "Usage: MAP\n";
        break;
    case COMANDA_LIST_MARKET:
        us = "Usage: LIST MARKET\n";
        break;
    case COMANDA_BUY:
        us = "Usage: BUY <product> <amount>\n";
        break;
    case COMANDA_SELL:
        us = "Usage: SELL <product> <amount>\n";
        break;
    case COMANDA_STATUS:
        us = "Usage: STATUS\n";
        break;
    case COMANDA_DELIVER:
        us = "Usage: DELIVER\n";
        break;
    case COMANDA_CLAIM:
        us = "Usage: CLAIM\n";
        break;
    }

    if (us == NULL) {
        return -1;
    }
    return escriureMissatge(us);
}

/***********************************************
 *
 * @Finalitat: Mostra el resultat obtingut en analitzar una comanda.
 * @Parametres: in: resultat = resultat de l'analisi.
 *              in: tipus = tipus de comanda reconeguda.
 * @Retorn: Retorna 0 si pot escriure i -1 si es produeix un error.
 *
 ************************************************/
int mostrarResultatComanda(int resultat, int tipus) {
    if (resultat == 1) {
        return escriureMissatge("Command OK\n");
    }
    if (resultat == 0) {
        return mostrarUsComanda(tipus);
    }
    return escriureMissatge("Unknown command\n");
}

/***********************************************
 *
 * @Finalitat: Executa el terminal interactiu fins que arriba EOF.
 * @Parametres: in: finalitzar_programa = indica si s'ha rebut SIGINT.
 * @Retorn: Retorna 0 en arribar a EOF i -1 si es produeix un error.
 *
 ************************************************/
int executarTerminal(volatile sig_atomic_t *finalitzar_programa, ConfiguracioOdisseu *configuracio, int *socket_actual) {
    Comanda comanda = {0};
    int resultat_analisi = 0, resultat_execucio = 0, finalitzat = 0;
    char *linia = NULL;

    while (finalitzat == 0 && *finalitzar_programa == 0) {
        if (escriureMissatge("$ ") != 0) {
            return -1;
        }
        linia = llegirLinia(0);

        if (*finalitzar_programa != 0 || linia == NULL) {
            finalitzat = 1;
        } else {
            resultat_analisi = analitzarComanda(linia, &comanda);
            if (resultat_analisi == 1 &&comanda.tipus == COMANDA_CONNECT) {
                connectarItaca(configuracio, socket_actual);
            } else if (resultat_analisi == 1 &&comanda.tipus == COMANDA_LIST_VOYAGES) {
                llistarViatges(configuracio, *socket_actual);
            } else if (resultat_analisi == 1 &&comanda.tipus == COMANDA_ACCEPT) {
                acceptarViatge(configuracio, *socket_actual, comanda.valor);
            } else if (resultat_analisi == 1 &&comanda.tipus == COMANDA_SAIL) {
                if (configuracio->tipus_connexio == CONNEXIO_ILLA) {
                    resultat_execucio = navegarEntreIlles(configuracio,
                        socket_actual, comanda.argument);
                } else {
                    resultat_execucio = navegarAeaea(configuracio,
                        socket_actual, comanda.argument);
                }
                if (resultat_execucio == ODISSEU_MORT) {
                    *finalitzar_programa = 1;
                }
            } else if (resultat_analisi == 1 &&comanda.tipus == COMANDA_LIST_MARKET) {
                llistarMercat(configuracio, *socket_actual);
            } else if (resultat_analisi == 1 &&comanda.tipus == COMANDA_BUY) {
                if (strcasecmp(comanda.argument, "MAP") == 0) {
                    comprarMapa(configuracio, *socket_actual,comanda.valor);
                } else {
                    comprarAliment(configuracio, *socket_actual,comanda.argument, comanda.valor);
                }
            } else if (resultat_analisi == 1 &&comanda.tipus == COMANDA_SELL) {
                vendreAliment(configuracio, *socket_actual,comanda.argument, comanda.valor);
            } else if (resultat_analisi == 1 &&comanda.tipus == COMANDA_STATUS) {
                mostrarEstat(configuracio);
            } else if (resultat_analisi == 1 &&comanda.tipus == COMANDA_MAP) {
                mostrarMapa(configuracio);
            } else if (mostrarResultatComanda(resultat_analisi, comanda.tipus) != 0) {
                free(linia);
                return -1;
            }
        }
        free(linia);
        linia = NULL;
        comanda.argument = NULL;
    }
    return 0;
}

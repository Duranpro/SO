#define _GNU_SOURCE

/***********************************************
 *
 * @Proposit: Implementa el terminal i el parser de comandes d'Odysseus.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 23/09/2026
 * @Data ultima modificacio: 25/09/2026
 *
 ************************************************/

#include "comandes.h"

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
    unsigned char trama_peticio[MIDA_TRAMA] = {0}, trama_resposta[MIDA_TRAMA] = {0};
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
    unsigned char dades[MIDA_DADES_TRAMA + 1] = {0};
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
    if (index_text == NULL || total_text == NULL ||
        identificador_text == NULL || objecte == NULL || desti == NULL ||
        recompensa_text == NULL || camp_extra != NULL ||
        esNumero(index_text) == 0 || esNumero(total_text) == 0 ||
        esNumero(identificador_text) == 0 ||
        esNumero(recompensa_text) == 0) {
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
    unsigned char peticio[MIDA_TRAMA] = {0}, resposta[MIDA_TRAMA] = {0};
    unsigned char dades[MIDA_DADES_TRAMA + 1] = {0};
    int resultat = 0, nombre_total = 0, i = 0, longitud = 0;

    if (socket_actual < 0 ||
        configuracio->tipus_connexio != CONNEXIO_ITACA) {
        escriureMissatge("Not connected to Ithaca.\n");
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
    unsigned char peticio[MIDA_TRAMA] = {0}, resposta[MIDA_TRAMA] = {0};
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

    caracters_escrits = asprintf(&missatge,
        "Voyage %d accepted.\nDestination: %s.\n",
        identificador, configuracio->illa_desti_viatge);
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
    unsigned char peticio[MIDA_TRAMA] = {0}, resposta[MIDA_TRAMA] = {0};
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
    unsigned char resposta[MIDA_TRAMA] = {0};
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
 * @Finalitat: Connecta amb una illa i espera fins que permet atracar.
 * @Parametres: in/out: configuracio = dades i estat d'Odysseus.
 *              in/out: socket_actual = connexio activa amb l'illa.
 *              in: nom_illa = illa a la qual arriba Odysseus.
 * @Retorn: Retorna 0 si Odysseus atraca i -1 altrament.
 *
 ************************************************/
int esperarPort(ConfiguracioOdisseu *configuracio, int *socket_actual, char *nom_illa) {
    unsigned char peticio[MIDA_TRAMA] = {0};
    char *missatge = NULL;
    int socket_illa = -1, longitud_nom = 0, resultat = 0, estat_port = 0;
    int caracters_escrits = 0;

    socket_illa = connectarServidor(configuracio->ip_illa_inicial, configuracio->port_illa_inicial);
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
        estat_port = rebreEstatPort(socket_illa);
    }
    if (resultat != TRAMA_CORRECTA || estat_port != PORT_DOCKED ||
        copiarText(&configuracio->ubicacio_actual, nom_illa) != 0) {
        escriureMissatge("Error: invalid response from Island.\n");
        close(socket_illa);
        *socket_actual = -1;
        configuracio->tipus_connexio = SENSE_CONNEXIO;
        return -1;
    }

    caracters_escrits = asprintf(&missatge, "%s has granted access to the port.\n", nom_illa);
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
    return esperarPort(configuracio, socket_actual, nom_illa);
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
        return RESULTAT_CORRECTE;
    }
    return RESULTAT_SINTAXI_INCORRECTA;
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
        return RESULTAT_DESCONEGUDA;
    }
    if (strcasecmp(paraules[1], "VOYAGES") == 0) {
        comanda->tipus = COMANDA_LIST_VOYAGES;
    } else if (strcasecmp(paraules[1], "MARKET") == 0) {
        comanda->tipus = COMANDA_LIST_MARKET;
    } else {
        return RESULTAT_DESCONEGUDA;
    }

    if (nombre_paraules != 2) {
        return RESULTAT_SINTAXI_INCORRECTA;
    }
    return RESULTAT_CORRECTE;
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
            return RESULTAT_CORRECTE;
        }
    }
    return RESULTAT_SINTAXI_INCORRECTA;
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
        return RESULTAT_CORRECTE;
    }
    return RESULTAT_SINTAXI_INCORRECTA;
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
            return RESULTAT_CORRECTE;
        }
    }
    return RESULTAT_SINTAXI_INCORRECTA;
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
        return RESULTAT_CORRECTE;
    }
    return RESULTAT_SINTAXI_INCORRECTA;
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
        return RESULTAT_DESCONEGUDA;
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
    return RESULTAT_DESCONEGUDA;
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
    if (resultat == RESULTAT_CORRECTE) {
        return escriureMissatge("Command OK\n");
    }
    if (resultat == RESULTAT_SINTAXI_INCORRECTA) {
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
int executarTerminal(int *finalitzar_programa,
                     ConfiguracioOdisseu *configuracio, int *socket_actual) {
    Comanda comanda = {0};
    int resultat_analisi = 0, finalitzat = 0;
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
            if (resultat_analisi == RESULTAT_CORRECTE &&
                comanda.tipus == COMANDA_CONNECT) {
                connectarItaca(configuracio, socket_actual);
            } else if (resultat_analisi == RESULTAT_CORRECTE &&
                       comanda.tipus == COMANDA_LIST_VOYAGES) {
                llistarViatges(configuracio, *socket_actual);
            } else if (resultat_analisi == RESULTAT_CORRECTE &&
                       comanda.tipus == COMANDA_ACCEPT) {
                acceptarViatge(configuracio, *socket_actual, comanda.valor);
            } else if (resultat_analisi == RESULTAT_CORRECTE &&
                       comanda.tipus == COMANDA_SAIL) {
                navegarAeaea(configuracio, socket_actual, comanda.argument);
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

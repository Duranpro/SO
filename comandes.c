#define _GNU_SOURCE

/***********************************************
 *
 * @Proposit: Implementa el terminal i el parser de comandes d'Odysseus.
 * @Autor/s: Antonio Duran Sabates i Arnau González Hortelano
 * @Data creacio: 23/09/2026
 * @Data ultima modificacio: 15/10/2026
 *
 ************************************************/

#include "comandes.h"

/***********************************************
 * @Finalitat: Cerca una illa coneguda sense distingir majuscules.
 * @Parametres: in: configuracio = mapa conegut; nom = illa cercada.
 * @Retorn: Retorna la posicio de l'illa o -1 si no es coneix.
 ************************************************/
int buscarIllaConeguda(ConfiguracioOdisseu *configuracio, char *nom) {
    int i = 0;

    for (i = 0; i < configuracio->num_illes_conegudes; i++) {
        if (strcasecmp(configuracio->illes_conegudes[i].nom, nom) == 0) {
            return i;
        }
    }
    return -1;
}

/***********************************************
 * @Finalitat: Guarda l'adreca d'una illa valida sense duplicar-la ni reordenar.
 * @Parametres: in/out: configuracio = mapa conegut.
 *              in: nom, ip, port = dades rebudes del mapa local.
 * @Retorn: Retorna la posicio de l'illa o -1 si les dades no son valides.
 ************************************************/
int afegirIllaConeguda(ConfiguracioOdisseu *configuracio, char *nom, char *ip, int port) {
    char *noms_valids[MAX_ILLES] = {"Aeaea", "Ogygia", "Scheria", "Thrinacia", "Aeolia", "Ismarus"};
    int i = 0, index = -1, nom_valid = -1;

    for (i = 0; i < MAX_ILLES; i++) {
        if (strcasecmp(nom, noms_valids[i]) == 0) {
            nom_valid = i;
        }
    }
    if (nom_valid < 0 || ip[0] == '\0' || strlen(ip) >= MIDA_IP || port < 1 || port > 65535) {
        return -1;
    }
    index = buscarIllaConeguda(configuracio, nom);
    if (index < 0) {
        if (configuracio->num_illes_conegudes >= MAX_ILLES) {
            return -1;
        }
        index = configuracio->num_illes_conegudes++;
        strcpy(configuracio->illes_conegudes[index].nom, noms_valids[nom_valid]);
        configuracio->illes_conegudes[index].num_connexions = 0;
    }
    strcpy(configuracio->illes_conegudes[index].ip, ip);
    configuracio->illes_conegudes[index].port = port;
    return index;
}

/***********************************************
 * @Finalitat: Afegeix una connexio directa si encara no es coneix.
 * @Parametres: in/out: origen = illa on s'afegeix la connexio.
 *              in: desti = illa que ja te una entrada al mapa.
 * @Retorn: ----.
 ************************************************/
void afegirConnexio(IllaConeguda *origen, IllaConeguda *desti) {
    int i = 0;

    for (i = 0; i < origen->num_connexions; i++) {
        if (strcasecmp(origen->connexions[i], desti->nom) == 0) {
            return;
        }
    }
    if (origen != desti && origen->num_connexions < MAX_ILLES) {
        strcpy(origen->connexions[origen->num_connexions], desti->nom);
        origen->num_connexions++;
    }
}

/***********************************************
 * @Finalitat: Mostra el coneixement acumulat amb la llibreria SPHRAGIS.
 * @Parametres: in: configuracio = illes i connexions descobertes.
 * @Retorn: Retorna 0 si mostra el mapa o el missatge d'error i -1 si falla la sortida.
 ************************************************/
int mostrarMapa(ConfiguracioOdisseu *configuracio) {
    SPHRAGIS_Island mapa_sphragis[MAX_ILLES] = {0};
    char *connexions[MAX_ILLES][MAX_ILLES];
    int i = 0, j = 0;

    for (i = 0; i < configuracio->num_illes_conegudes; i++) {
        mapa_sphragis[i].name = configuracio->illes_conegudes[i].nom;
        mapa_sphragis[i].known_island_count = configuracio->illes_conegudes[i].num_connexions;
        mapa_sphragis[i].known_islands = connexions[i];
        for (j = 0; j < mapa_sphragis[i].known_island_count; j++) {
            connexions[i][j] = configuracio->illes_conegudes[i].connexions[j];
        }
    }
    if (SPHRAGIS_display_map(mapa_sphragis, configuracio->num_illes_conegudes) != SPHRAGIS_OK) {
        return escriureMissatge("Error: SPHRAGIS no ha pogut mostrar el mapa; cal un terminal de 80 columnes.\n");
    }
    return 0;
}

/***********************************************
 * @Finalitat: Mostra diners, carrega, viatge i ubicacio sense utilitzar la xarxa.
 * @Parametres: in: configuracio = estat actual d'Odysseus.
 * @Retorn: Retorna 0 si mostra l'estat i -1 si falla la reserva del missatge.
 ************************************************/
int mostrarStatus(ConfiguracioOdisseu *configuracio) {
    char nom[MIDA_NOM_ODYSSEUS], *missatge = NULL;
    int i = 0, longitud = 0;

    for (i = 0; configuracio->nom[i] != '\0' && i < MIDA_NOM_ODYSSEUS - 1; i++) {
        nom[i] = toupper((unsigned char)configuracio->nom[i]);
    }
    nom[i] = '\0';
    longitud = asprintf(&missatge, "--- %s ---\nGold: %d\nCargo:\n", nom, configuracio->diners);
    if (longitud < 0) {
        return -1;
    }
    write(1, missatge, longitud);
    free(missatge);
    for (i = 0; i < configuracio->nombre_aliments; i++) {
        longitud = asprintf(&missatge, "%s: %d kg\n", configuracio->aliments[i].nom,
                             configuracio->aliments[i].quantitat);
        if (longitud < 0) {
            return -1;
        }
        write(1, missatge, longitud);
        free(missatge);
    }
    longitud = asprintf(&missatge, "Total provisions: %d kg\n", consumirProvisions(configuracio, 0));
    if (longitud < 0) {
        return -1;
    }
    write(1, missatge, longitud);
    free(missatge);
    if (configuracio->te_viatge == 1) {
        longitud = asprintf(&missatge, "Current voyage: %s -> %s\nReward: %d\nCurrent location: %s\n",
                             configuracio->viatge_actiu.nom_objecte, configuracio->viatge_actiu.illa_desti,
                             configuracio->viatge_actiu.recompensa, configuracio->ubicacio_actual);
    } else {
        longitud = asprintf(&missatge, "Current voyage: None\nCurrent location: %s\n",
                             configuracio->ubicacio_actual);
    }
    if (longitud < 0) {
        return -1;
    }
    write(1, missatge, longitud);
    free(missatge);
    return 0;
}

/***********************************************
 * @Finalitat: Compra el mapa local i incorpora les rutes en ambdos sentits.
 * @Parametres: in/out: configuracio = diners i mapa; fd_illa = socket actual.
 *              in: quantitat = nombre de mapes demanats, que ha de ser 1.
 * @Retorn: Retorna 0 si mostra el resultat i -1 si falla la sortida.
 ************************************************/
int executarBuyMap(ConfiguracioOdisseu *configuracio, int *fd_illa, int quantitat) {
    unsigned char trama[MIDA_TRAMA];
    char dades[MIDA_DADES + 1], missatge[MIDA_DADES + 100];
    char *camps[7], *camp = NULL;
    int origen = buscarIllaConeguda(configuracio, configuracio->ubicacio_actual);
    int index = 1, total = 1, desti = -1, nombre_camps = 0, longitud = 0, resultat = 0;

    if (*fd_illa < 0 || origen < 0 || quantitat != 1) {
        return escriureMissatge("Error: cal estar atracat i executar BUY MAP 1.\n");
    }
    if (configuracio->diners < 50) {
        return escriureMissatge("Error: diners insuficients.\n");
    }
    crearTrama(trama, TIPUS_BUY_MAP, FLAG_PETICIO, (unsigned char *)"1", 1);
    resultat = enviarTrama(*fd_illa, trama);
    for (index = 1; index <= total && resultat == 0; index++) {
        resultat = rebreTrama(*fd_illa, trama);
        if (resultat == 0 && trama[POSICIO_TIPUS] != TIPUS_BUY_MAP) {
            if (trama[POSICIO_TIPUS] != TIPUS_NACK) {
                enviarNack(*fd_illa, "INVALID_TYPE");
            }
            resultat = -1;
        }
        if (resultat == 0 && trama[POSICIO_FLAGS] != FLAG_RESPOSTA_OK &&
            trama[POSICIO_FLAGS] != FLAG_RESPOSTA_ERROR) {
            enviarNack(*fd_illa, "INVALID_FLAGS");
            resultat = -1;
        }
        if (resultat == 0 && (trama[POSICIO_LONGITUD] == 0 || trama[POSICIO_LONGITUD] > MIDA_DADES)) {
            enviarNack(*fd_illa, "INVALID_LENGTH");
            resultat = -1;
        }
        if (resultat != 0) {
            resultat = -1;
            break;
        }
        longitud = trama[POSICIO_LONGITUD];
        memcpy(dades, trama + POSICIO_DADES, longitud);
        dades[longitud] = '\0';
        if ((int)strlen(dades) != longitud) {
            enviarNack(*fd_illa, "INVALID_DATA");
            resultat = -1;
            break;
        }
        if (trama[POSICIO_FLAGS] == FLAG_RESPOSTA_ERROR && index == 1) {
            snprintf(missatge, sizeof(missatge), "Error: %s.\n", dades);
            return escriureMissatge(missatge);
        }
        if (trama[POSICIO_FLAGS] != FLAG_RESPOSTA_OK || (int)strlen(dades) != longitud) {
            resultat = -1;
            break;
        }
        if (index == 1 && strcmp(dades, "OK&50&0") == 0) {
            configuracio->diners -= 50;
            break;
        }
        if (dades[0] == '&' || dades[longitud - 1] == '&' || strstr(dades, "&&") != NULL) {
            enviarNack(*fd_illa, "INVALID_DATA");
            resultat = -1;
            break;
        }
        nombre_camps = 0;
        camp = strtok(dades, "&");
        while (camp != NULL && nombre_camps < 7) {
            camps[nombre_camps++] = camp;
            camp = strtok(NULL, "&");
        }
        if (nombre_camps != 7 || camp != NULL || strcmp(camps[0], "OK") != 0 ||
            strcmp(camps[1], "50") != 0 || !esNumero(camps[2]) || strlen(camps[2]) != 1 ||
            atoi(camps[2]) != index || !esNumero(camps[3]) || strlen(camps[3]) != 1 ||
            atoi(camps[3]) < index || atoi(camps[3]) > MAX_ILLES || !esNumero(camps[6]) ||
            strlen(camps[6]) > 5) {
            enviarNack(*fd_illa, "INVALID_DATA");
            resultat = -1;
            break;
        }
        if (index == 1) {
            total = atoi(camps[3]);
        }
        if (atoi(camps[3]) != total) {
            enviarNack(*fd_illa, "INVALID_DATA");
            resultat = -1;
            break;
        }
        desti = afegirIllaConeguda(configuracio, camps[4], camps[5], atoi(camps[6]));
        if (desti < 0) {
            enviarNack(*fd_illa, "INVALID_DATA");
            resultat = -1;
            break;
        }
        if (index == 1) {
            configuracio->diners -= 50;
        }
        afegirConnexio(&configuracio->illes_conegudes[origen], &configuracio->illes_conegudes[desti]);
        afegirConnexio(&configuracio->illes_conegudes[desti], &configuracio->illes_conegudes[origen]);
    }
    if (resultat == 0) {
        snprintf(missatge, sizeof(missatge), "Bought the map of %s for 50 gold.\nNew routes have been charted.\n",
                  configuracio->ubicacio_actual);
        return escriureMissatge(missatge);
    }
    close(*fd_illa);
    *fd_illa = -1;
    free(configuracio->mercat_actual);
    configuracio->mercat_actual = NULL;
    configuracio->num_productes_mercat = 0;
    return escriureMissatge("Error: connexio o resposta BUY MAP invalida.\n");
}

/***********************************************
 * @Finalitat: Comprova nomes les connexions directes de la ubicacio actual.
 * @Parametres: in: configuracio = mapa i ubicacio; desti = illa demanada.
 * @Retorn: Retorna la posicio del desti o -1 si no es una connexio directa.
 ************************************************/
int esConnexioDirecta(ConfiguracioOdisseu *configuracio, char *desti) {
    int origen = buscarIllaConeguda(configuracio, configuracio->ubicacio_actual), i = 0;

    if (origen >= 0) {
        for (i = 0; i < configuracio->illes_conegudes[origen].num_connexions; i++) {
            if (strcasecmp(configuracio->illes_conegudes[origen].connexions[i], desti) == 0) {
                return buscarIllaConeguda(configuracio, desti);
            }
        }
    }
    return -1;
}

/***********************************************
 * @Finalitat: Demana la sortida i tanca el socket nomes despres de rebre OK.
 * @Parametres: in/out: configuracio = mercat a invalidar; fd_illa = socket del port.
 * @Retorn: Retorna 0 amb sortida confirmada i -1 si falla l'operacio.
 ************************************************/
int sortirIlla(ConfiguracioOdisseu *configuracio, int *fd_illa) {
    unsigned char trama[MIDA_TRAMA];
    char dades[MIDA_DADES + 1];
    int longitud = 0, resultat = -1;

    crearTrama(trama, TIPUS_SORTIDA_ILLA, FLAG_PETICIO, NULL, 0);
    if (enviarTrama(*fd_illa, trama) == 0) {
        resultat = rebreTrama(*fd_illa, trama);
    }
    if (resultat == 0 && trama[POSICIO_TIPUS] != TIPUS_SORTIDA_ILLA) {
        if (trama[POSICIO_TIPUS] != TIPUS_NACK) {
            enviarNack(*fd_illa, "INVALID_TYPE");
        }
        resultat = -1;
    }
    if (resultat == 0 && trama[POSICIO_FLAGS] != FLAG_RESPOSTA_OK &&
        trama[POSICIO_FLAGS] != FLAG_RESPOSTA_ERROR) {
        enviarNack(*fd_illa, "INVALID_FLAGS");
        resultat = -1;
    }
    if (resultat == 0 && (trama[POSICIO_LONGITUD] == 0 || trama[POSICIO_LONGITUD] > MIDA_DADES)) {
        enviarNack(*fd_illa, "INVALID_LENGTH");
        resultat = -1;
    }
    if (resultat == 0) {
        longitud = trama[POSICIO_LONGITUD];
        memcpy(dades, trama + POSICIO_DADES, longitud);
        dades[longitud] = '\0';
        if (trama[POSICIO_FLAGS] == FLAG_RESPOSTA_ERROR) {
            escriureMissatge("Error: Island ha rebutjat la sortida.\n");
            return -1;
        }
        if (trama[POSICIO_FLAGS] == FLAG_RESPOSTA_OK && longitud == 2 && strcmp(dades, "OK") == 0) {
            resultat = 0;
        }
    }
    if (resultat == 0 && (trama[POSICIO_LONGITUD] != 2 || strcmp(dades, "OK") != 0)) {
        if (trama[POSICIO_LONGITUD] != 2) {
            enviarNack(*fd_illa, "INVALID_LENGTH");
        } else {
            enviarNack(*fd_illa, "INVALID_DATA");
        }
        resultat = -1;
    }
    close(*fd_illa);
    *fd_illa = -1;
    free(configuracio->mercat_actual);
    configuracio->mercat_actual = NULL;
    configuracio->num_productes_mercat = 0;
    return resultat;
}

/***********************************************
 * @Finalitat: Consulta el mercat pel socket del port i conserva els seus preus.
 * @Parametres: in/out: configuracio = ultim mercat; fd_illa = socket actual.
 *              in: nom_illa = nom de l'illa on s'ha atracat.
 * @Retorn: Retorna 0 si mostra el resultat i -1 si falla la sortida.
 ************************************************/
int executarListMarket(ConfiguracioOdisseu *configuracio, int *fd_illa, char *nom_illa) {
    unsigned char trama[MIDA_TRAMA];
    Producte *mercat = NULL;
    char dades[MIDA_DADES + 1], missatge[MIDA_DADES + 100], nom[MIDA_DADES + 1];
    char *camps[5], *camp = NULL;
    int index = 1, total = 1, longitud = 0, i = 0, nombre_camps = 0;
    int quantitat = 0, preu = 0, resultat = 0;

    if (*fd_illa < 0) {
        return escriureMissatge("Error: cal estar atracat en una Island.\n");
    }
    free(configuracio->mercat_actual);
    configuracio->mercat_actual = NULL;
    configuracio->num_productes_mercat = 0;
    crearTrama(trama, TIPUS_LIST_MARKET, FLAG_PETICIO, NULL, 0);
    resultat = enviarTrama(*fd_illa, trama);
    for (index = 1; index <= total && resultat == 0; index++) {
        resultat = rebreTrama(*fd_illa, trama);
        if (resultat == 0 && trama[POSICIO_TIPUS] != TIPUS_LIST_MARKET) {
            if (trama[POSICIO_TIPUS] != TIPUS_NACK) {
                enviarNack(*fd_illa, "INVALID_TYPE");
            }
            resultat = -1;
        }
        if (resultat == 0 && trama[POSICIO_FLAGS] != FLAG_RESPOSTA_OK &&
            trama[POSICIO_FLAGS] != FLAG_RESPOSTA_ERROR) {
            enviarNack(*fd_illa, "INVALID_FLAGS");
            resultat = -1;
        }
        if (resultat == 0 && (trama[POSICIO_LONGITUD] == 0 || trama[POSICIO_LONGITUD] > MIDA_DADES)) {
            enviarNack(*fd_illa, "INVALID_LENGTH");
            resultat = -1;
        }
        if (resultat != 0) {
            resultat = -1;
            break;
        }
        longitud = trama[POSICIO_LONGITUD];
        memcpy(dades, trama + POSICIO_DADES, longitud);
        dades[longitud] = '\0';
        if ((int)strlen(dades) != longitud) {
            enviarNack(*fd_illa, "INVALID_DATA");
            resultat = -1;
            break;
        }
        if (trama[POSICIO_FLAGS] == FLAG_RESPOSTA_ERROR) {
            free(mercat);
            snprintf(missatge, sizeof(missatge), "Error: %s.\n", dades);
            return escriureMissatge(missatge);
        }
        if (dades[0] == '&' || dades[longitud - 1] == '&' || strstr(dades, "&&") != NULL) {
            enviarNack(*fd_illa, "INVALID_DATA");
            resultat = -1;
            break;
        }
        nombre_camps = 0;
        camp = strtok(dades, "&");
        while (camp != NULL && nombre_camps < 5) {
            camps[nombre_camps++] = camp;
            camp = strtok(NULL, "&");
        }
        if (trama[POSICIO_FLAGS] != FLAG_RESPOSTA_OK || nombre_camps != 5 || camp != NULL ||
            !esNumero(camps[0]) || !esNumero(camps[1]) || !esNumero(camps[3]) ||
            !esNumero(camps[4]) || atoi(camps[0]) != index || atoi(camps[1]) < index ||
            strlen(camps[2]) >= MIDA_NOM_PRODUCTE) {
            enviarNack(*fd_illa, "INVALID_DATA");
            resultat = -1;
            break;
        }
        if (index == 1) {
            total = atoi(camps[1]);
            if (total > 1) {
                mercat = malloc((total - 1) * sizeof(*mercat));
                if (mercat == NULL) {
                    resultat = -1;
                    break;
                }
            }
            for (i = 0; nom_illa[i] != '\0' && i < MIDA_DADES; i++) {
                nom[i] = toupper((unsigned char)nom_illa[i]);
            }
            nom[i] = '\0';
            snprintf(missatge, sizeof(missatge), "--- %s MARKET ---\n", nom);
            escriureMissatge(missatge);
        }
        quantitat = atoi(camps[3]);
        preu = atoi(camps[4]);
        if (atoi(camps[1]) != total || quantitat < 0 || preu < 0 ||
            (index == total && (strcmp(camps[2], "MAP") != 0 || quantitat != 1 || preu != 50)) ||
            (index < total && strcasecmp(camps[2], "MAP") == 0)) {
            enviarNack(*fd_illa, "INVALID_DATA");
            resultat = -1;
            break;
        }
        if (index == total) {
            snprintf(missatge, sizeof(missatge), "Map 50 gold\n");
        } else {
            strcpy(mercat[index - 1].nom, camps[2]);
            mercat[index - 1].quantitat = quantitat;
            mercat[index - 1].preu = preu;
            snprintf(missatge, sizeof(missatge), "%s %d kg %d gold/kg\n", camps[2], quantitat, preu);
        }
        escriureMissatge(missatge);
    }
    if (resultat == 0) {
        configuracio->mercat_actual = mercat;
        configuracio->num_productes_mercat = total - 1;
        return 0;
    }
    free(mercat);
    close(*fd_illa);
    *fd_illa = -1;
    return escriureMissatge("Error: connexio o resposta LIST MARKET invalida.\n");
}

/***********************************************
 * @Finalitat: Intercanvia una peticio BUY o SELL i comprova el seu rebut.
 * @Parametres: in/out: fd_illa = socket del port.
 *              in: tipus = BUY o SELL; producte = aliment; quantitat = kg.
 *              out: import = cost o ingres confirmat per Island.
 * @Retorn: Retorna 1 amb OK, 0 amb error d'operacio i -1 amb error de connexio.
 ************************************************/
int intercanviarAliments(int *fd_illa, int tipus, char *producte, int quantitat, int *import) {
    unsigned char trama[MIDA_TRAMA];
    char dades[MIDA_DADES + 1], missatge[MIDA_DADES + 20];
    char *estat = NULL, *nom = NULL, *kg = NULL, *or = NULL;
    int longitud = 0, resultat = -1;

    longitud = snprintf(dades, sizeof(dades), "%s&%d", producte, quantitat);
    if (crearTrama(trama, tipus, FLAG_PETICIO, (unsigned char *)dades, longitud) == 0 &&
        enviarTrama(*fd_illa, trama) == 0) {
        resultat = rebreTrama(*fd_illa, trama);
    }
    if (resultat == 0 && trama[POSICIO_TIPUS] != tipus) {
        if (trama[POSICIO_TIPUS] != TIPUS_NACK) {
            enviarNack(*fd_illa, "INVALID_TYPE");
        }
        resultat = -1;
    }
    if (resultat == 0 && trama[POSICIO_FLAGS] != FLAG_RESPOSTA_OK &&
        trama[POSICIO_FLAGS] != FLAG_RESPOSTA_ERROR) {
        enviarNack(*fd_illa, "INVALID_FLAGS");
        resultat = -1;
    }
    if (resultat == 0 && (trama[POSICIO_LONGITUD] == 0 || trama[POSICIO_LONGITUD] > MIDA_DADES)) {
        enviarNack(*fd_illa, "INVALID_LENGTH");
        resultat = -1;
    }
    if (resultat == 0) {
        longitud = trama[POSICIO_LONGITUD];
        memcpy(dades, trama + POSICIO_DADES, longitud);
        dades[longitud] = '\0';
        if ((int)strlen(dades) != longitud ||
            (trama[POSICIO_FLAGS] == FLAG_RESPOSTA_OK &&
             (dades[0] == '&' || dades[longitud - 1] == '&' || strstr(dades, "&&") != NULL))) {
            enviarNack(*fd_illa, "INVALID_DATA");
            resultat = -1;
        }
        if (resultat == 0 && trama[POSICIO_FLAGS] == FLAG_RESPOSTA_ERROR) {
            snprintf(missatge, sizeof(missatge), "Error: %s.\n", dades);
            escriureMissatge(missatge);
            return 0;
        }
        estat = strtok(dades, "&");
        nom = strtok(NULL, "&");
        kg = strtok(NULL, "&");
        or = strtok(NULL, "&");
        if (resultat == 0 && trama[POSICIO_FLAGS] == FLAG_RESPOSTA_OK && estat != NULL && nom != NULL &&
            kg != NULL && or != NULL && strtok(NULL, "&") == NULL && strcmp(estat, "OK") == 0 &&
            strcmp(nom, producte) == 0 && esNumero(kg) && atoi(kg) == quantitat && esNumero(or)) {
            *import = atoi(or);
            if (*import >= 0) {
                return 1;
            }
        }
    }
    if (resultat == 0) {
        enviarNack(*fd_illa, "INVALID_DATA");
    }
    close(*fd_illa);
    *fd_illa = -1;
    escriureMissatge("Error: connexio o resposta de compra/venda invalida.\n");
    return -1;
}

/***********************************************
 * @Finalitat: Compra un aliment amb preu conegut i afegeix els kg a la carrega.
 * @Parametres: in/out: configuracio = diners, carrega i mercat; fd_illa = socket.
 *              in: producte = aliment; quantitat = kg demanats.
 * @Retorn: Retorna 0 si mostra el resultat i -1 si falla la sortida.
 ************************************************/
int executarBuy(ConfiguracioOdisseu *configuracio, int *fd_illa, char *producte, int quantitat) {
    Aliment *aliments = NULL;
    char *nom_nou = NULL, missatge[MIDA_DADES + 100];
    int i = 0, index_mercat = -1, index_aliment = -1, cost = 0, import = 0, resultat = 0;

    if (strcasecmp(producte, "MAP") == 0) {
        return executarBuyMap(configuracio, fd_illa, quantitat);
    }
    if (*fd_illa < 0 || quantitat <= 0) {
        return escriureMissatge("Error: cal estar atracat i comprar un aliment.\n");
    }
    for (i = 0; i < configuracio->num_productes_mercat; i++) {
        if (strcmp(configuracio->mercat_actual[i].nom, producte) == 0) {
            index_mercat = i;
        }
    }
    if (index_mercat < 0) {
        return escriureMissatge("Error: consulta LIST MARKET per coneixer el preu de l'aliment.\n");
    }
    if (configuracio->mercat_actual[index_mercat].preu > 0 &&
        quantitat > configuracio->diners / configuracio->mercat_actual[index_mercat].preu) {
        return escriureMissatge("Error: diners insuficients.\n");
    }
    cost = quantitat * configuracio->mercat_actual[index_mercat].preu;
    if (quantitat > INT_MAX - consumirProvisions(configuracio, 0)) {
        return escriureMissatge("Error: massa provisions.\n");
    }
    for (i = 0; i < configuracio->nombre_aliments; i++) {
        if (strcmp(configuracio->aliments[i].nom, producte) == 0) {
            index_aliment = i;
        }
    }
    /* Reservem l'espai abans d'enviar, sense canviar diners ni quantitats. */
    if (index_aliment < 0) {
        if (copiarText(&nom_nou, producte) != 0) {
            return escriureMissatge("Error: sense memoria per a l'aliment.\n");
        }
        aliments = realloc(configuracio->aliments,
                            (configuracio->nombre_aliments + 1) * sizeof(*aliments));
        if (aliments == NULL) {
            free(nom_nou);
            return escriureMissatge("Error: sense memoria per a l'aliment.\n");
        }
        configuracio->aliments = aliments;
    }
    resultat = intercanviarAliments(fd_illa, TIPUS_BUY, producte, quantitat, &import);
    if (resultat == 1 && import == cost) {
        if (index_aliment < 0) {
            index_aliment = configuracio->nombre_aliments;
            configuracio->aliments[index_aliment].nom = nom_nou;
            configuracio->aliments[index_aliment].quantitat = 0;
            configuracio->nombre_aliments++;
            nom_nou = NULL;
        }
        configuracio->diners -= import;
        configuracio->aliments[index_aliment].quantitat += quantitat;
        snprintf(missatge, sizeof(missatge), "Bought %d kg of %s for %d gold.\n", quantitat, producte, import);
        escriureMissatge(missatge);
    } else if (resultat == 1) {
        close(*fd_illa);
        *fd_illa = -1;
        escriureMissatge("Error: cost de compra inesperat.\n");
    }
    free(nom_nou);
    if (*fd_illa < 0) {
        free(configuracio->mercat_actual);
        configuracio->mercat_actual = NULL;
        configuracio->num_productes_mercat = 0;
    }
    return 0;
}

/***********************************************
 * @Finalitat: Ven aliments transportats i aplica l'ingres nomes despres de l'OK.
 * @Parametres: in/out: configuracio = diners i carrega; fd_illa = socket del port.
 *              in: producte = aliment; quantitat = kg que es volen vendre.
 * @Retorn: Retorna 0 si mostra el resultat i -1 si falla la sortida.
 ************************************************/
int executarSell(ConfiguracioOdisseu *configuracio, int *fd_illa, char *producte, int quantitat) {
    char missatge[MIDA_DADES + 100];
    int i = 0, index = -1, import = 0, resultat = 0;

    if (*fd_illa < 0 || quantitat <= 0 || strcasecmp(producte, "MAP") == 0) {
        return escriureMissatge("Error: cal estar atracat i vendre un aliment.\n");
    }
    for (i = 0; i < configuracio->nombre_aliments; i++) {
        if (strcmp(configuracio->aliments[i].nom, producte) == 0) {
            index = i;
        }
    }
    if (index < 0 || configuracio->aliments[index].quantitat < quantitat) {
        return escriureMissatge("Error: carrega insuficient.\n");
    }
    resultat = intercanviarAliments(fd_illa, TIPUS_SELL, producte, quantitat, &import);
    if (resultat == 1 && import <= INT_MAX - configuracio->diners) {
        configuracio->aliments[index].quantitat -= quantitat;
        configuracio->diners += import;
        snprintf(missatge, sizeof(missatge), "Sold %d kg of %s for %d gold.\n", quantitat, producte, import);
        escriureMissatge(missatge);
    } else if (resultat == 1) {
        close(*fd_illa);
        *fd_illa = -1;
        escriureMissatge("Error: ingres fora de rang.\n");
    }
    if (*fd_illa < 0) {
        free(configuracio->mercat_actual);
        configuracio->mercat_actual = NULL;
        configuracio->num_productes_mercat = 0;
    }
    return 0;
}

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
 * @Finalitat: Connecta amb Ithaca i intercanvia la identificacio d'Odysseus.
 * @Parametres: in: configuracio = nom i adreca d'Ithaca ja carregats.
 *              in/out: fd_ithaca = descriptor de la connexio actual.
 * @Retorn: Retorna 0 si pot mostrar el resultat i -1 si falla la sortida.
 *
 ************************************************/
int connectarIthaca(ConfiguracioOdisseu *configuracio, int *fd_ithaca) {
    unsigned char trama[MIDA_TRAMA];
    char resposta[3];
    int resultat = -1, longitud = 0;

    if (strcasecmp(configuracio->ubicacio_actual, "Ithaca") != 0) {
        return escriureMissatge("Error: Odysseus no es a Ithaca.\n");
    }
    if (*fd_ithaca >= 0) {
        return 0;
    }
    longitud = strlen(configuracio->nom);
    if (longitud > 0 && crearTrama(trama, TIPUS_CONNECT_ITHACA, FLAG_PETICIO,
                                  (unsigned char *)configuracio->nom, longitud) == 0) {
        *fd_ithaca = connectarServidor(configuracio->ip_itaca,
                                       configuracio->port_itaca);
        if (*fd_ithaca >= 0 && enviarTrama(*fd_ithaca, trama) == 0) {
            resultat = rebreTrama(*fd_ithaca, trama);
        }
    }
    if (resultat == 0 && trama[POSICIO_TIPUS] != TIPUS_CONNECT_ITHACA) {
        if (trama[POSICIO_TIPUS] != TIPUS_NACK) {
            enviarNack(*fd_ithaca, "INVALID_TYPE");
        }
        resultat = -1;
    }
    if (resultat == 0 && trama[POSICIO_FLAGS] != FLAG_RESPOSTA_OK &&
        trama[POSICIO_FLAGS] != FLAG_RESPOSTA_ERROR) {
        enviarNack(*fd_ithaca, "INVALID_FLAGS");
        resultat = -1;
    }
    if (resultat == 0 && (trama[POSICIO_LONGITUD] == 0 || trama[POSICIO_LONGITUD] > MIDA_DADES)) {
        enviarNack(*fd_ithaca, "INVALID_LENGTH");
        resultat = -1;
    }
    if (resultat == 0 && trama[POSICIO_TIPUS] == TIPUS_CONNECT_ITHACA &&
        trama[POSICIO_FLAGS] == FLAG_RESPOSTA_OK &&
        trama[POSICIO_LONGITUD] == 2) {
        memcpy(resposta, trama + POSICIO_DADES, 2);
        resposta[2] = '\0';
        if (strcmp(resposta, "OK") == 0) {
            return escriureMissatge("Connected to Ithaca.\n");
        }
    }
    if (resultat == 0 && trama[POSICIO_FLAGS] == FLAG_RESPOSTA_OK) {
        if (trama[POSICIO_LONGITUD] != 2) {
            enviarNack(*fd_ithaca, "INVALID_LENGTH");
        } else {
            enviarNack(*fd_ithaca, "INVALID_DATA");
        }
    }
    if (*fd_ithaca >= 0) {
        close(*fd_ithaca);
        *fd_ithaca = -1;
    }
    return escriureMissatge("Error: no s'ha pogut connectar amb Ithaca.\n");
}

/***********************************************
 *
 * @Finalitat: Demana i mostra els viatges disponibles sense guardar-ne la llista.
 * @Parametres: in: configuracio = estat del viatge actiu.
 *              in/out: fd_ithaca = descriptor de la connexio actual.
 * @Retorn: Retorna 0 si pot mostrar el resultat i -1 si falla la sortida.
 *
 ************************************************/
int executarListVoyages(ConfiguracioOdisseu *configuracio, int *fd_ithaca) {
    unsigned char trama[MIDA_TRAMA];
    char dades[MIDA_DADES + 1], missatge[MIDA_DADES + 100];
    char *camps[6], *camp = NULL;
    int index = 1, total = 1, i = 0, longitud = 0, resultat = -1;

    if (configuracio->te_viatge == 1) {
        return escriureMissatge("Error: ja tens un viatge actiu.\n");
    }
    if (*fd_ithaca < 0) {
        return escriureMissatge("Error: no estas connectat a Ithaca.\n");
    }
    crearTrama(trama, TIPUS_LIST_VOYAGES, FLAG_PETICIO, NULL, 0);
    if (enviarTrama(*fd_ithaca, trama) == 0) {
        for (index = 1; index <= total; index++) {
            resultat = rebreTrama(*fd_ithaca, trama);
            if (resultat == 0 && trama[POSICIO_TIPUS] != TIPUS_LIST_VOYAGES) {
                if (trama[POSICIO_TIPUS] != TIPUS_NACK) {
                    enviarNack(*fd_ithaca, "INVALID_TYPE");
                }
                resultat = -1;
            }
            if (resultat == 0 && trama[POSICIO_FLAGS] != FLAG_RESPOSTA_OK &&
                trama[POSICIO_FLAGS] != FLAG_RESPOSTA_ERROR) {
                enviarNack(*fd_ithaca, "INVALID_FLAGS");
                resultat = -1;
            }
            if (resultat == 0 && (trama[POSICIO_LONGITUD] == 0 || trama[POSICIO_LONGITUD] > MIDA_DADES)) {
                enviarNack(*fd_ithaca, "INVALID_LENGTH");
                resultat = -1;
            }
            if (resultat != 0) {
                break;
            }
            longitud = trama[POSICIO_LONGITUD];
            memcpy(dades, trama + POSICIO_DADES, longitud);
            dades[longitud] = '\0';
            if (trama[POSICIO_FLAGS] == FLAG_RESPOSTA_ERROR) {
                snprintf(missatge, sizeof(missatge), "Error: %s\n", dades);
                return escriureMissatge(missatge);
            }
            if (trama[POSICIO_FLAGS] != FLAG_RESPOSTA_OK || (int)strlen(dades) != longitud ||
                dades[0] == '&' || dades[longitud - 1] == '&' || strstr(dades, "&&") != NULL) {
                enviarNack(*fd_ithaca, "INVALID_DATA");
                break;
            }
            if (index == 1 && strcmp(dades, "0") == 0) {
                return escriureMissatge("--- AVAILABLE VOYAGES ---\n");
            }
            camp = strtok(dades, "&");
            for (i = 0; i < 6 && camp != NULL; i++) {
                camps[i] = camp;
                camp = strtok(NULL, "&");
            }
            if (i != 6 || camp != NULL || esNumero(camps[0]) == 0 ||
                esNumero(camps[1]) == 0 || esNumero(camps[2]) == 0 ||
                esNumero(camps[5]) == 0 || atoi(camps[0]) != index ||
                atoi(camps[1]) <= 0 || atoi(camps[2]) <= 0) {
                enviarNack(*fd_ithaca, "INVALID_DATA");
                break;
            }
            if (index == 1) {
                total = atoi(camps[1]);
                if (escriureMissatge("--- AVAILABLE VOYAGES ---\n") != 0) {
                    return -1;
                }
            }
            if (atoi(camps[1]) != total) {
                enviarNack(*fd_ithaca, "INVALID_DATA");
                break;
            }
            snprintf(missatge, sizeof(missatge), "%s. %s -> %s [%s]\n",
                      camps[2], camps[3], camps[4], camps[5]);
            if (escriureMissatge(missatge) != 0) {
                return -1;
            }
        }
        if (index > total) {
            return 0;
        }
    }
    if (*fd_ithaca >= 0) {
        close(*fd_ithaca);
        *fd_ithaca = -1;
    }
    return escriureMissatge("Error: no s'han pogut llistar els viatges.\n");
}

/***********************************************
 *
 * @Finalitat: Demana un viatge i conserva les dades del viatge acceptat.
 * @Parametres: in/out: configuracio = dades i viatge actiu d'Odysseus.
 *              in/out: fd_ithaca = descriptor de la connexio actual.
 *              in: identificador = VoyageID validat pel parser.
 * @Retorn: Retorna 0 si pot mostrar el resultat i -1 si falla la sortida.
 *
 ************************************************/
int executarAccept(ConfiguracioOdisseu *configuracio, int *fd_ithaca, int identificador) {
    unsigned char trama[MIDA_TRAMA];
    char dades[MIDA_DADES + 1], missatge[MIDA_DADES + 100];
    char *camps[5], *camp = NULL;
    Viatge viatge = {0};
    int longitud = 0, resultat = -1, i = 0;

    if (*fd_ithaca < 0) {
        return escriureMissatge("Error: no estas connectat a Ithaca.\n");
    }
    if (configuracio->te_viatge == 1) {
        return escriureMissatge("Error: ja tens un viatge actiu.\n");
    }
    longitud = snprintf(dades, sizeof(dades), "%d", identificador);
    if (crearTrama(trama, TIPUS_ACCEPT, FLAG_PETICIO, (unsigned char *)dades, longitud) == 0 &&
        enviarTrama(*fd_ithaca, trama) == 0) {
        resultat = rebreTrama(*fd_ithaca, trama);
    }
    if (resultat == 0 && trama[POSICIO_TIPUS] != TIPUS_ACCEPT) {
        if (trama[POSICIO_TIPUS] != TIPUS_NACK) {
            enviarNack(*fd_ithaca, "INVALID_TYPE");
        }
        resultat = -1;
    }
    if (resultat == 0 && trama[POSICIO_FLAGS] != FLAG_RESPOSTA_OK &&
        trama[POSICIO_FLAGS] != FLAG_RESPOSTA_ERROR) {
        enviarNack(*fd_ithaca, "INVALID_FLAGS");
        resultat = -1;
    }
    if (resultat == 0 && (trama[POSICIO_LONGITUD] == 0 || trama[POSICIO_LONGITUD] > MIDA_DADES)) {
        enviarNack(*fd_ithaca, "INVALID_LENGTH");
        resultat = -1;
    }
    if (resultat == 0 && trama[POSICIO_TIPUS] == TIPUS_ACCEPT &&
        trama[POSICIO_LONGITUD] <= MIDA_DADES) {
        longitud = trama[POSICIO_LONGITUD];
        memcpy(dades, trama + POSICIO_DADES, longitud);
        dades[longitud] = '\0';
        if (trama[POSICIO_FLAGS] == FLAG_RESPOSTA_ERROR) {
            snprintf(missatge, sizeof(missatge), "Error: %s\n", dades);
            return escriureMissatge(missatge);
        }
        if (trama[POSICIO_FLAGS] == FLAG_RESPOSTA_OK && (int)strlen(dades) == longitud &&
            dades[0] != '&' && dades[longitud - 1] != '&' && strstr(dades, "&&") == NULL) {
            camp = strtok(dades, "&");
            for (i = 0; i < 5 && camp != NULL; i++) {
                camps[i] = camp;
                camp = strtok(NULL, "&");
            }
            if (i == 5 && camp == NULL && strcmp(camps[0], "OK") == 0 &&
                esNumero(camps[1]) == 1 && atoi(camps[1]) == identificador &&
                esNumero(camps[4]) == 1) {
                viatge.identificador = identificador;
                viatge.recompensa = atoi(camps[4]);
                if (copiarText(&viatge.nom_objecte, camps[2]) == 0 &&
                    copiarText(&viatge.illa_desti, camps[3]) == 0) {
                    configuracio->viatge_actiu = viatge;
                    configuracio->te_viatge = 1;
                    snprintf(missatge, sizeof(missatge),
                              "Voyage %d accepted.\nDestination: %s.\n",
                              identificador, viatge.illa_desti);
                    return escriureMissatge(missatge);
                }
                free(viatge.nom_objecte);
                free(viatge.illa_desti);
                resultat = -1;
            }
        }
    }
    if (resultat == 0) {
        enviarNack(*fd_ithaca, "INVALID_DATA");
    }
    if (*fd_ithaca >= 0) {
        close(*fd_ithaca);
        *fd_ithaca = -1;
    }
    return escriureMissatge("Error: no s'ha pogut acceptar el viatge.\n");
}

/***********************************************
 *
 * @Finalitat: Confirma la desconnexio d'Ithaca abans de tancar el socket.
 * @Parametres: in/out: fd_ithaca = descriptor de la connexio actual.
 * @Retorn: Retorna 0 si Ithaca confirma la desconnexio i -1 altrament.
 *
 ************************************************/
int desconnectarIthaca(int *fd_ithaca) {
    unsigned char trama[MIDA_TRAMA];
    char resposta[3];
    int resultat = -1;

    crearTrama(trama, TIPUS_DESCONNEXIO_ITHACA, FLAG_PETICIO, NULL, 0);
    if (enviarTrama(*fd_ithaca, trama) == 0) {
        resultat = rebreTrama(*fd_ithaca, trama);
    }
    if (resultat == 0 && trama[POSICIO_TIPUS] != TIPUS_DESCONNEXIO_ITHACA) {
        if (trama[POSICIO_TIPUS] != TIPUS_NACK) {
            enviarNack(*fd_ithaca, "INVALID_TYPE");
        }
        resultat = -1;
    }
    if (resultat == 0 && trama[POSICIO_FLAGS] != FLAG_RESPOSTA_OK &&
        trama[POSICIO_FLAGS] != FLAG_RESPOSTA_ERROR) {
        enviarNack(*fd_ithaca, "INVALID_FLAGS");
        resultat = -1;
    }
    if (resultat == 0 && (trama[POSICIO_LONGITUD] == 0 || trama[POSICIO_LONGITUD] > MIDA_DADES)) {
        enviarNack(*fd_ithaca, "INVALID_LENGTH");
        resultat = -1;
    }
    if (resultat == 0) {
        if (trama[POSICIO_FLAGS] == FLAG_RESPOSTA_ERROR) {
            return -1;
        }
        if (trama[POSICIO_FLAGS] == FLAG_RESPOSTA_OK && trama[POSICIO_LONGITUD] == 2) {
            memcpy(resposta, trama + POSICIO_DADES, 2);
            resposta[2] = '\0';
            if (strcmp(resposta, "OK") == 0) {
                close(*fd_ithaca);
                *fd_ithaca = -1;
                return 0;
            }
        }
    }
    if (resultat == 0) {
        if (trama[POSICIO_LONGITUD] != 2) {
            enviarNack(*fd_ithaca, "INVALID_LENGTH");
        } else {
            enviarNack(*fd_ithaca, "INVALID_DATA");
        }
    }
    if (*fd_ithaca >= 0) {
        close(*fd_ithaca);
        *fd_ithaca = -1;
    }
    return -1;
}

/***********************************************
 *
 * @Finalitat: Consumeix aliments en ordre i retorna la suma real de kg restants.
 * @Parametres: in/out: configuracio = carrega d'aliments d'Odysseus.
 *              in: quantitat = kg que es volen consumir.
 * @Retorn: Retorna els kg totals que queden a la carrega.
 *
 ************************************************/
int consumirProvisions(ConfiguracioOdisseu *configuracio, int quantitat) {
    int i = 0, provisions = 0;

    for (i = 0; i < configuracio->nombre_aliments; i++) {
        if (quantitat > 0) {
            if (configuracio->aliments[i].quantitat <= quantitat) {
                quantitat -= configuracio->aliments[i].quantitat;
                configuracio->aliments[i].quantitat = 0;
            } else {
                configuracio->aliments[i].quantitat -= quantitat;
                quantitat = 0;
            }
        }
        provisions += configuracio->aliments[i].quantitat;
    }
    return provisions;
}

/***********************************************
 *
 * @Finalitat: Espera DOCKED i consumeix una racio per cada cinc segons complets.
 * @Parametres: in/out: configuracio = nom i carrega real d'Odysseus.
 *              in/out: fd_illa = descriptor de la connexio amb la Island.
 *              in: nom_illa = nom de la Island on s'espera.
 *              in/out: finalitzar_programa = finalitzacio per SIGINT o mort.
 * @Retorn: Retorna 0 si pot mostrar el resultat i -1 si falla la sortida.
 *
 ************************************************/
int esperarPort(ConfiguracioOdisseu *configuracio, int *fd_illa, char *nom_illa,
                int *finalitzar_programa) {
    fd_set descriptors;
    struct timeval temps;
    unsigned char trama[MIDA_TRAMA];
    char missatge[MIDA_NOM_ODYSSEUS + 100];
    int fd_espera = *fd_illa, resultat = 0, provisions = 0;

    while (*finalitzar_programa == 0 && fd_espera >= 0) {
        FD_ZERO(&descriptors);
        FD_SET(fd_espera, &descriptors);
        temps.tv_sec = 5;
        temps.tv_usec = 0;
        resultat = select(fd_espera + 1, &descriptors, NULL, NULL, &temps);
        if (*finalitzar_programa != 0) {
            break;
        }
        if (resultat > 0 && FD_ISSET(fd_espera, &descriptors)) {
            resultat = rebreTrama(fd_espera, trama);
            if (resultat == 0 && trama[POSICIO_TIPUS] != TIPUS_ARRIBADA_ILLA) {
                if (trama[POSICIO_TIPUS] != TIPUS_NACK) {
                    enviarNack(fd_espera, "INVALID_TYPE");
                }
                resultat = -1;
            }
            if (resultat == 0 && trama[POSICIO_FLAGS] != FLAG_RESPOSTA_OK &&
                trama[POSICIO_FLAGS] != FLAG_RESPOSTA_ERROR) {
                enviarNack(fd_espera, "INVALID_FLAGS");
                resultat = -1;
            }
            if (resultat == 0 && (trama[POSICIO_LONGITUD] == 0 || trama[POSICIO_LONGITUD] > MIDA_DADES)) {
                enviarNack(fd_espera, "INVALID_LENGTH");
                resultat = -1;
            }
            if (resultat != 0 ||
                trama[POSICIO_TIPUS] != TIPUS_ARRIBADA_ILLA ||
                trama[POSICIO_FLAGS] != FLAG_RESPOSTA_OK ||
                trama[POSICIO_LONGITUD] != 6 ||
                memcmp(trama + POSICIO_DADES, "DOCKED", 6) != 0) {
                if (resultat == 0 && trama[POSICIO_FLAGS] == FLAG_RESPOSTA_OK) {
                    if (trama[POSICIO_LONGITUD] != 6) {
                        enviarNack(fd_espera, "INVALID_LENGTH");
                    } else {
                        enviarNack(fd_espera, "INVALID_DATA");
                    }
                }
                break;
            }
            snprintf(missatge, sizeof(missatge), ">>> %s has granted access to the port.\n", nom_illa);
            strcpy(configuracio->ubicacio_actual, nom_illa);
            return escriureMissatge(missatge);
        } else if (resultat == 0) {
            provisions = consumirProvisions(configuracio, 7);
            snprintf(missatge, sizeof(missatge),
                      ">>> The crew consumes another ration.\nProvisions remaining: %d kg\n", provisions);
            if (escriureMissatge(missatge) != 0) {
                return -1;
            }
            if (provisions == 0) {
                close(*fd_illa);
                *fd_illa = -1;
                *finalitzar_programa = 1;
                snprintf(missatge, sizeof(missatge),
                          "The crew has run out of provisions.\nOdysseus %s has been lost at sea.\n",
                          configuracio->nom);
                return escriureMissatge(missatge);
            }
        } else {
            break;
        }
    }
    if (*fd_illa >= 0) {
        close(*fd_illa);
        *fd_illa = -1;
    }
    if (*finalitzar_programa != 0) {
        return 0;
    }
    return escriureMissatge("Error: s'ha interromput l'espera davant del port.\n");
}

/***********************************************
 *
 * @Finalitat: Identifica Odysseus a la nova illa i espera permis d'entrada al port.
 * @Parametres: in/out: configuracio = nom, carrega i ubicacio actual.
 *              in/out: fd_illa = descriptor de la connexio amb l'illa.
 *              in: desti = nom, IP i port coneguts de l'illa desti.
 *              in/out: finalitzar_programa = finalitzacio per SIGINT o mort.
 * @Retorn: Retorna 1 si atraca al desti del viatge, 0 altrament i -1 si falla la sortida.
 *
 ************************************************/
int arribarIlla(ConfiguracioOdisseu *configuracio, int *fd_illa, IllaConeguda *desti,
                int *finalitzar_programa) {
    unsigned char trama[MIDA_TRAMA];
    char dades[MIDA_DADES + 1];
    char missatge[MIDA_NOM_ILLA + 100];
    int longitud = 0, resultat = -1;

    free(configuracio->mercat_actual);
    configuracio->mercat_actual = NULL;
    configuracio->num_productes_mercat = 0;
    longitud = strlen(configuracio->nom);
    if (crearTrama(trama, TIPUS_ARRIBADA_ILLA, FLAG_PETICIO,
                   (unsigned char *)configuracio->nom, longitud) != 0) {
        return escriureMissatge("Error: nom d'Odysseus invalid.\n");
    }
    *fd_illa = connectarServidor(desti->ip, desti->port);
    if (*fd_illa >= 0 && enviarTrama(*fd_illa, trama) == 0) {
        resultat = rebreTrama(*fd_illa, trama);
        if (resultat == 0 && trama[POSICIO_TIPUS] != TIPUS_ARRIBADA_ILLA) {
            if (trama[POSICIO_TIPUS] != TIPUS_NACK) {
                enviarNack(*fd_illa, "INVALID_TYPE");
            }
            resultat = -1;
        }
        if (resultat == 0 && trama[POSICIO_FLAGS] != FLAG_RESPOSTA_OK &&
            trama[POSICIO_FLAGS] != FLAG_RESPOSTA_ERROR) {
            enviarNack(*fd_illa, "INVALID_FLAGS");
            resultat = -1;
        }
        if (resultat == 0 && (trama[POSICIO_LONGITUD] == 0 || trama[POSICIO_LONGITUD] > MIDA_DADES)) {
            enviarNack(*fd_illa, "INVALID_LENGTH");
            resultat = -1;
        }
        if (resultat == 0) {
            longitud = trama[POSICIO_LONGITUD];
            if (trama[POSICIO_TIPUS] == TIPUS_ARRIBADA_ILLA &&
                trama[POSICIO_FLAGS] == FLAG_RESPOSTA_OK && longitud <= MIDA_DADES) {
                memcpy(dades, trama + POSICIO_DADES, longitud);
                dades[longitud] = '\0';
                if ((int)strlen(dades) == longitud &&
                    (strcmp(dades, "DOCKED") == 0 || strcmp(dades, "WAIT") == 0)) {
                    snprintf(missatge, sizeof(missatge), "Connected to %s.\n", desti->nom);
                    if (escriureMissatge(missatge) != 0) {
                        return -1;
                    }
                    if (strcmp(dades, "DOCKED") == 0) {
                        strcpy(configuracio->ubicacio_actual, desti->nom);
                        snprintf(missatge, sizeof(missatge), "%s has granted access to the port.\n", desti->nom);
                        resultat = escriureMissatge(missatge);
                    } else {
                        if (escriureMissatge("The harbor is full. We drop anchor and wait.\n") != 0) {
                            return -1;
                        }
                        resultat = esperarPort(configuracio, fd_illa, desti->nom, finalitzar_programa);
                    }
                    if (resultat != 0) {
                        return resultat;
                    }
                    if (*fd_illa >= 0 && *finalitzar_programa == 0 && configuracio->te_viatge == 1 &&
                        strcasecmp(configuracio->ubicacio_actual, configuracio->viatge_actiu.illa_desti) == 0) {
                        /* Arribar no lliura l'objecte ni canvia el viatge o els diners. */
                        return 1;
                    }
                    return 0;
                }
            }
        }
    }
    if (resultat == 0 && trama[POSICIO_FLAGS] == FLAG_RESPOSTA_OK) {
        enviarNack(*fd_illa, "INVALID_DATA");
    }
    if (*fd_illa >= 0) {
        close(*fd_illa);
        *fd_illa = -1;
    }
    snprintf(missatge, sizeof(missatge), "Error: no s'ha pogut entrar al port de %s.\n", desti->nom);
    return escriureMissatge(missatge);
}

/***********************************************
 *
 * @Finalitat: Navega a una connexio directa coneguda, amb viatge actiu o sense.
 * @Parametres: in/out: configuracio = ubicacio, mapa i carrega d'Odysseus.
 *              in/out: fd_ithaca = descriptor de la connexio amb Ithaca.
 *              in/out: fd_illa = descriptor de la connexio amb la Island actual.
 *              in: desti = illa indicada a la comanda SAIL.
 *              in/out: finalitzar_programa = finalitzacio per SIGINT o mort.
 * @Retorn: Retorna 0 si pot mostrar el resultat i -1 si falla la sortida.
 *
 ************************************************/
int executarSail(ConfiguracioOdisseu *configuracio, int *fd_ithaca, int *fd_illa,
                 char *desti, int *finalitzar_programa) {
    char *missatge = NULL;
    int index_desti = -1, longitud = 0;

    if (strcasecmp(configuracio->ubicacio_actual, "Ithaca") == 0) {
        if (strcasecmp(desti, "Aeaea") == 0) {
            index_desti = 0;
        }
    } else {
        index_desti = esConnexioDirecta(configuracio, desti);
    }
    if (index_desti < 0) {
        longitud = asprintf(&missatge, "We can't sail to %s from here...\n", desti);
        if (longitud < 0) {
            return -1;
        }
        write(1, missatge, longitud);
        free(missatge);
        return 0;
    }
    if (strcasecmp(configuracio->ubicacio_actual, "Ithaca") == 0) {
        if (*fd_ithaca < 0 || *fd_illa >= 0) {
            return escriureMissatge("Error: cal connectar amb Ithaca abans de sortir.\n");
        }
        escriureMissatge("Leaving Ithaca...\n");
        if (desconnectarIthaca(fd_ithaca) != 0) {
            return escriureMissatge("Error: Ithaca no ha confirmat la desconnexio.\n");
        }
    } else if (*fd_illa < 0 || sortirIlla(configuracio, fd_illa) != 0) {
        return escriureMissatge("Error: no s'ha pogut abandonar el port actual.\n");
    }
    strcpy(configuracio->ubicacio_actual, "At sea");
    longitud = asprintf(&missatge, "Sailing to %s...\n", configuracio->illes_conegudes[index_desti].nom);
    if (longitud < 0) {
        return -1;
    }
    write(1, missatge, longitud);
    free(missatge);
    if (arribarIlla(configuracio, fd_illa, &configuracio->illes_conegudes[index_desti], finalitzar_programa) < 0) {
        return -1;
    }
    return 0;
}

/***********************************************
 *
 * @Finalitat: Executa el terminal interactiu fins que arriba EOF.
 * @Parametres: in: finalitzar_programa = indica si s'ha rebut SIGINT.
 *              in: configuracio = configuracio carregada d'Odysseus.
 *              in/out: fd_ithaca = descriptor de la connexio amb Ithaca.
 *              in/out: fd_illa = descriptor de la connexio amb la Island actual.
 * @Retorn: Retorna 0 en arribar a EOF i -1 si es produeix un error.
 *
 ************************************************/
int executarTerminal(int *finalitzar_programa, ConfiguracioOdisseu *configuracio,
                     int *fd_ithaca, int *fd_illa) {
    Comanda comanda = {0};
    int resultat_analisi = 0, finalitzat = 0, resultat = 0;
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
            if (resultat_analisi == 1 && comanda.tipus == COMANDA_CONNECT) {
                resultat = connectarIthaca(configuracio, fd_ithaca);
            } else if (resultat_analisi == 1 && comanda.tipus == COMANDA_LIST_VOYAGES) {
                resultat = executarListVoyages(configuracio, fd_ithaca);
            } else if (resultat_analisi == 1 && comanda.tipus == COMANDA_ACCEPT) {
                resultat = executarAccept(configuracio, fd_ithaca, comanda.valor);
            } else if (resultat_analisi == 1 && comanda.tipus == COMANDA_SAIL) {
                resultat = executarSail(configuracio, fd_ithaca, fd_illa,
                                        comanda.argument, finalitzar_programa);
            } else if (resultat_analisi == 1 && comanda.tipus == COMANDA_LIST_MARKET) {
                resultat = executarListMarket(configuracio, fd_illa, configuracio->ubicacio_actual);
            } else if (resultat_analisi == 1 && comanda.tipus == COMANDA_BUY) {
                resultat = executarBuy(configuracio, fd_illa, comanda.argument, comanda.valor);
            } else if (resultat_analisi == 1 && comanda.tipus == COMANDA_SELL) {
                resultat = executarSell(configuracio, fd_illa, comanda.argument, comanda.valor);
            } else if (resultat_analisi == 1 && comanda.tipus == COMANDA_MAP) {
                resultat = mostrarMapa(configuracio);
            } else if (resultat_analisi == 1 && comanda.tipus == COMANDA_STATUS) {
                resultat = mostrarStatus(configuracio);
            } else {
                resultat = mostrarResultatComanda(resultat_analisi, comanda.tipus);
            }
            if (resultat != 0) {
                free(linia);
                return -1;
            }
        }
        free(linia);
        linia = NULL;
        comanda.argument = NULL;
    }
    if (*finalitzar_programa == 0 && *fd_illa >= 0) {
        sortirIlla(configuracio, fd_illa);
    }
    if (*finalitzar_programa == 0 && *fd_ithaca >= 0) {
        desconnectarIthaca(fd_ithaca);
    }
    return 0;
}

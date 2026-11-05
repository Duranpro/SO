#define _GNU_SOURCE

/***********************************************
 *
 * @Proposit: Carrega i allibera la configuracio d'Odysseus.
 * @Autor/s: Antonio Duran Sabates i Arnau González Hortelano
 * @Data creacio: 18/09/2026
 * @Data ultima modificacio: 19/10/2026
 *
 ************************************************/

#include "configuracio_odisseu.h"

/***********************************************
 *
 * @Finalitat: Carrega la identitat i les localitzacions d'Odysseus.
 * @Parametres: in: fd = descriptor del fitxer amb format de configuracio correcte.
 *              out: configuracio = dades que es carreguen.
 * @Retorn: Retorna 0 si es carreguen les dades i -1 si falla la lectura,
 *          la reserva de memoria o una dada no cap al buffer de destinacio.
 *
 ************************************************/
int carregarLocalitzacio(int fd, ConfiguracioOdisseu *configuracio) {
    char *linia = NULL, *camp = NULL;

    linia = llegirLinia(fd);
    if (linia == NULL) {
        return -1;
    }
    if (copiarText(&configuracio->ruta_carpeta, linia) != 0) {
        free(linia);
        return -1;
    }
    free(linia);
    linia = NULL;

    linia = llegirLinia(fd);
    if (linia == NULL) {
        return -1;
    }
    camp = strtok(linia, " ");
    if (copiarText(&configuracio->nom, camp) != 0) {
        free(linia);
        return -1;
    }
    camp = strtok(NULL, " ");
    if (copiarText(&configuracio->ip_itaca, camp) != 0) {
        free(linia);
        return -1;
    }
    camp = strtok(NULL, " ");
    configuracio->port_itaca = atoi(camp);
    free(linia);
    linia = NULL;

    linia = llegirLinia(fd);
    if (linia == NULL) {
        return -1;
    }
    camp = strtok(linia, " ");
    if (copiarText(&configuracio->ip_illa_inicial, camp) != 0) {
        free(linia);
        return -1;
    }
    camp = strtok(NULL, " ");
    configuracio->port_illa_inicial = atoi(camp);
    free(linia);

    if (strlen(configuracio->ip_illa_inicial) >= MIDA_IP) {
        return -1;
    }
    strcpy(configuracio->ubicacio_actual, "Ithaca");
    strcpy(configuracio->illes_conegudes[0].nom, "Aeaea");
    strcpy(configuracio->illes_conegudes[0].ip, configuracio->ip_illa_inicial);
    configuracio->illes_conegudes[0].port = configuracio->port_illa_inicial;
    configuracio->num_illes_conegudes = 1;

    return 0;
}

/***********************************************
 *
 * @Finalitat: Carrega els diners i els aliments inicials d'Odysseus.
 * @Parametres: in: fd = descriptor del fitxer amb format i dades correctes.
 *              out: configuracio = dades que es carreguen.
 * @Retorn: Retorna 0 si es carreguen les dades i -1 si es produeix un error.
 *
 ************************************************/
int carregarAliments(int fd, ConfiguracioOdisseu *configuracio) {
    int i = 0;
    char *linia = NULL, *camp = NULL;

    linia = llegirLinia(fd);
    if (linia == NULL) {
        return -1;
    }
    configuracio->diners = atoi(linia);
    free(linia);
    linia = NULL;

    linia = llegirLinia(fd);
    if (linia == NULL) {
        return -1;
    }
    configuracio->nombre_aliments = atoi(linia);
    free(linia);
    linia = NULL;
    if (configuracio->nombre_aliments == 0) {
        return 0;
    }

    configuracio->aliments = malloc(configuracio->nombre_aliments * sizeof(*configuracio->aliments));
    if (configuracio->aliments == NULL) {
        return -1;
    }

    for (i = 0; i < configuracio->nombre_aliments; i++) {
        configuracio->aliments[i].nom = NULL;
        configuracio->aliments[i].quantitat = 0;
    }

    for (i = 0; i < configuracio->nombre_aliments; i++) {
        linia = llegirLinia(fd);
        if (linia == NULL) {
            return -1;
        }
        camp = strtok(linia, " ");
        if (copiarText(&configuracio->aliments[i].nom, camp) != 0) {
            free(linia);
            return -1;
        }
        camp = strtok(NULL, " ");
        configuracio->aliments[i].quantitat = atoi(camp);
        free(linia);
        linia = NULL;
    }

    return 0;
}

/***********************************************
 *
 * @Finalitat: Allibera la memoria de la configuracio d'Odysseus.
 * @Parametres: in/out: configuracio = configuracio que es vol alliberar.
 * @Retorn: ----.
 *
 ************************************************/
void alliberarConfiguracioOdisseu(ConfiguracioOdisseu *configuracio) {
    int i = 0;

    free(configuracio->ruta_carpeta);
    free(configuracio->nom);
    free(configuracio->ip_itaca);
    free(configuracio->ip_illa_inicial);

    if (configuracio->aliments != NULL) {
        for (i = 0; i < configuracio->nombre_aliments; i++) {
            free(configuracio->aliments[i].nom);
        }
    }
    free(configuracio->aliments);
    free(configuracio->mercat_actual);
    free(configuracio->viatges_consultats);
    free(configuracio->viatge_actiu.nom_objecte);
    free(configuracio->viatge_actiu.illa_desti);

    configuracio->ruta_carpeta = NULL;
    configuracio->nom = NULL;
    configuracio->ip_itaca = NULL;
    configuracio->ip_illa_inicial = NULL;
    configuracio->aliments = NULL;
    configuracio->mercat_actual = NULL;
    configuracio->num_productes_mercat = 0;
    configuracio->viatges_consultats = NULL;
    configuracio->nombre_viatges_consultats = 0;
    configuracio->nombre_aliments = 0;
    configuracio->viatge_actiu.nom_objecte = NULL;
    configuracio->viatge_actiu.illa_desti = NULL;
    configuracio->te_viatge = 0;
    configuracio->entrega_completada = 0;
    configuracio->fitxer_objecte[0] = '\0';
}

/***********************************************
 *
 * @Finalitat: Obre i carrega el fitxer de configuracio d'Odysseus.
 * @Parametres: in: nom_fitxer = ruta del fitxer de configuracio.
 *              out: configuracio = configuracio carregada.
 * @Retorn: Retorna 0 si es carrega correctament i -1 si es produeix un error.
 *
 ************************************************/
int carregarConfiguracioOdisseu(char *nom_fitxer, ConfiguracioOdisseu *configuracio) {
    int fd = -1, resultat = 0, caracters_escrits = 0;
    char *missatge = NULL;

    fd = open(nom_fitxer, O_RDONLY);
    if (fd < 0) {
        caracters_escrits = asprintf(&missatge, "Error: no s'ha pogut obrir %s.\n",  nom_fitxer);
        if (caracters_escrits >= 0) {
            write(2, missatge, caracters_escrits);
            free(missatge);
        }
        return -1;
    }

    resultat = carregarLocalitzacio(fd, configuracio);
    if (resultat == 0) {
        resultat = carregarAliments(fd, configuracio);
    }
    close(fd);

    if (resultat != 0) {
        alliberarConfiguracioOdisseu(configuracio);
        caracters_escrits = asprintf(&missatge, "Error: no s'ha pogut carregar %s.\n",nom_fitxer);
        if (caracters_escrits >= 0) {
            write(2, missatge, caracters_escrits);
            free(missatge);
        }
        return -1;
    }

    return 0;
}

/***********************************************
 * @Finalitat: Desa el mapa conegut, camp per camp, sense recursos del SO.
 * @Parametres: in: fd = fitxer d'estat; configuracio = mapa actual.
 * @Retorn: 0 si es desa tot; -1 amb error.
 ************************************************/
static int guardarMapaEstat(int fd, ConfiguracioOdisseu *configuracio) {
    int i = 0, j = 0;
    IllaConeguda *illa = NULL;

    if (write(fd, &configuracio->num_illes_conegudes, sizeof(int)) != sizeof(int)) {
        return -1;
    }
    for (i = 0; i < configuracio->num_illes_conegudes; i++) {
        illa = &configuracio->illes_conegudes[i];
        if (write(fd, illa->nom, MIDA_NOM_ILLA) != MIDA_NOM_ILLA ||
            write(fd, illa->ip, MIDA_IP) != MIDA_IP ||
            write(fd, &illa->port, sizeof(int)) != sizeof(int) ||
            write(fd, &illa->num_connexions, sizeof(int)) != sizeof(int)) {
            return -1;
        }
        for (j = 0; j < illa->num_connexions; j++) {
            if (write(fd, illa->connexions[j], MIDA_NOM_ILLA) != MIDA_NOM_ILLA) {
                return -1;
            }
        }
    }
    return 0;
}

/***********************************************
 * @Finalitat: Desa ubicacio, mapa i missio per poder reprendre el viatge.
 * @Parametres: in: fd = fitxer d'estat; configuracio = estat logic actual.
 * @Retorn: 0 si es desa tot; -1 amb error.
 ************************************************/
static int guardarEstatLogic(int fd, ConfiguracioOdisseu *configuracio) {
    char objecte[MIDA_NOM_FITXER] = {0}, desti[MIDA_NOM_ILLA] = {0};

    if (write(fd, &configuracio->te_viatge, sizeof(int)) != sizeof(int) ||
        write(fd, &configuracio->entrega_completada, sizeof(int)) != sizeof(int) ||
        write(fd, configuracio->ubicacio_actual, MIDA_NOM_ILLA) != MIDA_NOM_ILLA ||
        guardarMapaEstat(fd, configuracio) != 0) {
        return -1;
    }
    if (configuracio->te_viatge == 0) {
        return 0;
    }
    if (snprintf(objecte, sizeof(objecte), "%s", configuracio->viatge_actiu.nom_objecte) >= MIDA_NOM_FITXER ||
        snprintf(desti, sizeof(desti), "%s", configuracio->viatge_actiu.illa_desti) >= MIDA_NOM_ILLA) {
        return -1;
    }
    if (write(fd, &configuracio->viatge_actiu.identificador, sizeof(int)) != sizeof(int) ||
        write(fd, &configuracio->viatge_actiu.recompensa, sizeof(int)) != sizeof(int) ||
        write(fd, objecte, sizeof(objecte)) != sizeof(objecte) ||
        write(fd, desti, sizeof(desti)) != sizeof(desti) ||
        write(fd, configuracio->fitxer_objecte, MIDA_NOM_FITXER) != MIDA_NOM_FITXER) {
        return -1;
    }
    return 0;
}

/***********************************************
 * @Finalitat: Recupera el mapa en buffers locals amb comptadors limitats.
 * @Parametres: in: fd = fitxer d'estat; out: estat = dades encara no aplicades.
 * @Retorn: 0 amb mapa valid; -1 amb lectura incompleta o camps invalids.
 ************************************************/
static int carregarMapaEstat(int fd, ConfiguracioOdisseu *estat) {
    int i = 0, j = 0;
    IllaConeguda *illa = NULL;

    if (read(fd, &estat->num_illes_conegudes, sizeof(int)) != sizeof(int) ||
        estat->num_illes_conegudes < 1 || estat->num_illes_conegudes > MAX_ILLES) {
        return -1;
    }
    for (i = 0; i < estat->num_illes_conegudes; i++) {
        illa = &estat->illes_conegudes[i];
        if (read(fd, illa->nom, MIDA_NOM_ILLA) != MIDA_NOM_ILLA ||
            read(fd, illa->ip, MIDA_IP) != MIDA_IP ||
            read(fd, &illa->port, sizeof(int)) != sizeof(int) ||
            read(fd, &illa->num_connexions, sizeof(int)) != sizeof(int) ||
            illa->nom[0] == '\0' || illa->nom[MIDA_NOM_ILLA - 1] != '\0' ||
            illa->ip[0] == '\0' || illa->ip[MIDA_IP - 1] != '\0' ||
            illa->port < 1 || illa->port > 65535 ||
            illa->num_connexions < 0 || illa->num_connexions >= MAX_ILLES) {
            return -1;
        }
        for (j = 0; j < illa->num_connexions; j++) {
            if (read(fd, illa->connexions[j], MIDA_NOM_ILLA) != MIDA_NOM_ILLA ||
                illa->connexions[j][0] == '\0' || illa->connexions[j][MIDA_NOM_ILLA - 1] != '\0') {
                return -1;
            }
        }
    }
    return strcmp(estat->illes_conegudes[0].nom, "Aeaea") == 0 ? 0 : -1;
}

/***********************************************
 * @Finalitat: Recupera la missio i reserva nomes els dos textos que utilitza.
 * @Parametres: in: fd = fitxer d'estat; out: estat = missio temporal.
 * @Retorn: 0 amb missio valida; -1 amb error.
 ************************************************/
static int carregarViatgeEstat(int fd, ConfiguracioOdisseu *estat) {
    char objecte[MIDA_NOM_FITXER], desti[MIDA_NOM_ILLA];

    if (read(fd, &estat->viatge_actiu.identificador, sizeof(int)) != sizeof(int) ||
        read(fd, &estat->viatge_actiu.recompensa, sizeof(int)) != sizeof(int) ||
        read(fd, objecte, sizeof(objecte)) != sizeof(objecte) ||
        read(fd, desti, sizeof(desti)) != sizeof(desti) ||
        read(fd, estat->fitxer_objecte, MIDA_NOM_FITXER) != MIDA_NOM_FITXER ||
        estat->viatge_actiu.identificador < 1 || estat->viatge_actiu.recompensa < 0 ||
        objecte[0] == '\0' || objecte[MIDA_NOM_FITXER - 1] != '\0' ||
        desti[0] == '\0' || desti[MIDA_NOM_ILLA - 1] != '\0' ||
        estat->fitxer_objecte[MIDA_NOM_FITXER - 1] != '\0' ||
        (estat->entrega_completada && estat->fitxer_objecte[0] == '\0')) {
        return -1;
    }
    if (copiarText(&estat->viatge_actiu.nom_objecte, objecte) != 0 ||
        copiarText(&estat->viatge_actiu.illa_desti, desti) != 0) {
        return -1;
    }
    return 0;
}

/***********************************************
 * @Finalitat: Recupera el sufix logic, admetent l'antic format de Gold i carrega.
 * @Parametres: in: fd = fitxer; in/out: estat = valors inicials i dades carregades.
 * @Retorn: 0 amb estat valid o format antic; -1 amb error.
 ************************************************/
static int carregarEstatLogic(int fd, ConfiguracioOdisseu *estat) {
    int bytes_llegits = 0;

    bytes_llegits = read(fd, &estat->te_viatge, sizeof(int));
    if (bytes_llegits == 0) {
        return 0;
    }
    if (bytes_llegits != sizeof(int) || estat->te_viatge < 0 || estat->te_viatge > 1 ||
        read(fd, &estat->entrega_completada, sizeof(int)) != sizeof(int) ||
        estat->entrega_completada < 0 || estat->entrega_completada > estat->te_viatge ||
        read(fd, estat->ubicacio_actual, MIDA_NOM_ILLA) != MIDA_NOM_ILLA ||
        estat->ubicacio_actual[0] == '\0' || estat->ubicacio_actual[MIDA_NOM_ILLA - 1] != '\0' ||
        carregarMapaEstat(fd, estat) != 0) {
        return -1;
    }
    if (estat->te_viatge) {
        return carregarViatgeEstat(fd, estat);
    }
    return 0;
}

/***********************************************
 * @Finalitat: Desa diners, carrega i estat logic al state.dat propi.
 * @Parametres: in: configuracio = carpeta i dades vigents d'Odysseus.
 * @Retorn: 0 si es desa tot; -1 amb error, que tambe s'informa per stderr.
 ************************************************/
int guardarEstatOdysseus(ConfiguracioOdisseu *configuracio) {
    char ruta[MIDA_RUTA_FITXER], nom[MIDA_NOM_PRODUCTE];
    int fd = -1, resultat = 0, i = 0, longitud = 0;

    longitud = snprintf(ruta, sizeof(ruta), "%s/state.dat", configuracio->ruta_carpeta);
    if (longitud < 1 || longitud >= MIDA_RUTA_FITXER || configuracio->diners < 0 ||
        configuracio->nombre_aliments < 0 || configuracio->nombre_aliments > MAX_ALIMENTS_ESTAT) {
        resultat = -1;
    } else {
        fd = open(ruta, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            resultat = -1;
        }
    }
    if (resultat == 0 && (write(fd, &configuracio->diners, sizeof(int)) != sizeof(int) ||
        write(fd, &configuracio->nombre_aliments, sizeof(int)) != sizeof(int))) {
        resultat = -1;
    }
    for (i = 0; i < configuracio->nombre_aliments && resultat == 0; i++) {
        memset(nom, 0, sizeof(nom));
        longitud = snprintf(nom, sizeof(nom), "%s", configuracio->aliments[i].nom);
        if (longitud < 1 || longitud >= MIDA_NOM_PRODUCTE || configuracio->aliments[i].quantitat < 0 ||
            write(fd, nom, sizeof(nom)) != sizeof(nom) ||
            write(fd, &configuracio->aliments[i].quantitat, sizeof(int)) != sizeof(int)) {
            resultat = -1;
        }
    }
    if (resultat == 0) {
        resultat = guardarEstatLogic(fd, configuracio);
    }
    if (fd >= 0 && close(fd) != 0) {
        resultat = -1;
    }
    if (resultat != 0) {
        write(2, "Error: no s'ha pogut guardar l'estat d'Odysseus.\n",
              strlen("Error: no s'ha pogut guardar l'estat d'Odysseus.\n"));
    }
    return resultat;
}

/***********************************************
 * @Finalitat: Recupera l estat logic nomes si tot el state.dat es valid.
 * @Parametres: in/out: configuracio = dades inicials, intactes si falla la carrega.
 * @Retorn: 0 amb estat carregat; -1 si falta, es invalid o no es pot llegir.
 ************************************************/
int carregarEstatOdysseus(ConfiguracioOdisseu *configuracio) {
    char ruta[MIDA_RUTA_FITXER], nom[MIDA_NOM_PRODUCTE], sobrant = 0;
    Aliment *aliments = NULL;
    ConfiguracioOdisseu estat = {0};
    int fd = -1, resultat = 0, diners = 0, nombre = 0, i = 0, total = 0, longitud = 0;

    longitud = snprintf(ruta, sizeof(ruta), "%s/state.dat", configuracio->ruta_carpeta);
    if (longitud < 1 || longitud >= MIDA_RUTA_FITXER) {
        return -1;
    }
    fd = open(ruta, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    if (read(fd, &diners, sizeof(int)) != sizeof(int) || read(fd, &nombre, sizeof(int)) != sizeof(int) ||
        diners < 0 || nombre < 0 || nombre > MAX_ALIMENTS_ESTAT) {
        resultat = -1;
    }
    if (resultat == 0 && nombre > 0) {
        aliments = malloc(nombre * sizeof(*aliments));
        if (aliments == NULL) {
            resultat = -1;
        } else {
            for (i = 0; i < nombre; i++) {
                aliments[i].nom = NULL;
                aliments[i].quantitat = 0;
            }
        }
    }
    for (i = 0; i < nombre && resultat == 0; i++) {
        if (read(fd, nom, sizeof(nom)) != sizeof(nom) ||
            read(fd, &aliments[i].quantitat, sizeof(int)) != sizeof(int) ||
            nom[0] == '\0' || nom[MIDA_NOM_PRODUCTE - 1] != '\0' ||
            aliments[i].quantitat < 0 || aliments[i].quantitat > INT_MAX - total ||
            copiarText(&aliments[i].nom, nom) != 0) {
            resultat = -1;
        } else {
            total += aliments[i].quantitat;
        }
    }
    estat.num_illes_conegudes = configuracio->num_illes_conegudes;
    memcpy(estat.illes_conegudes, configuracio->illes_conegudes, sizeof(estat.illes_conegudes));
    strcpy(estat.ubicacio_actual, configuracio->ubicacio_actual);
    if (resultat == 0) {
        resultat = carregarEstatLogic(fd, &estat);
    }
    if (resultat == 0 && read(fd, &sobrant, 1) != 0) {
        resultat = -1;
    }
    if (close(fd) != 0) {
        resultat = -1;
    }
    if (resultat == 0) {
        for (i = 0; i < configuracio->nombre_aliments; i++) {
            free(configuracio->aliments[i].nom);
        }
        free(configuracio->aliments);
        configuracio->aliments = aliments;
        configuracio->nombre_aliments = nombre;
        configuracio->diners = diners;
        free(configuracio->viatge_actiu.nom_objecte);
        free(configuracio->viatge_actiu.illa_desti);
        configuracio->viatge_actiu = estat.viatge_actiu;
        configuracio->te_viatge = estat.te_viatge;
        configuracio->entrega_completada = estat.entrega_completada;
        strcpy(configuracio->fitxer_objecte, estat.fitxer_objecte);
        strcpy(configuracio->ubicacio_actual, estat.ubicacio_actual);
        configuracio->num_illes_conegudes = estat.num_illes_conegudes;
        memcpy(configuracio->illes_conegudes, estat.illes_conegudes, sizeof(estat.illes_conegudes));
    } else {
        free(estat.viatge_actiu.nom_objecte);
        free(estat.viatge_actiu.illa_desti);
    }
    if (resultat != 0 && aliments != NULL) {
        for (i = 0; i < nombre; i++) {
            free(aliments[i].nom);
        }
        free(aliments);
    }
    return resultat;
}

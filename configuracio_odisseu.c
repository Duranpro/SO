#define _GNU_SOURCE

/***********************************************
 *
 * @Proposit: Carrega i allibera la configuracio d'Odysseus.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 18/09/2026
 * @Data ultima modificacio: 21/09/2026
 *
 ************************************************/

#include "configuracio_odisseu.h"

/***********************************************
 *
 * @Finalitat: Carrega la identitat i les localitzacions d'Odysseus.
 * @Parametres: in: fd = descriptor del fitxer de configuracio.
 *              out: configuracio = dades que es carreguen.
 * @Retorn: Retorna 0 si es carreguen les dades i -1 si falta algun camp.
 *
 ************************************************/
int carregarLocalitzacio(int fd, ConfiguracioOdisseu *configuracio) {
    char linia[MIDA_LINIA], *camp = NULL;

    if (llegirLinia(fd, linia, MIDA_LINIA) != 1 ||copiarText(&configuracio->ruta_carpeta, linia) != 0) {
        return -1;
    }

    if (llegirLinia(fd, linia, MIDA_LINIA) != 1) {
        return -1;
    }
    camp = strtok(linia, " ");
    if (camp == NULL || copiarText(&configuracio->nom, camp) != 0) {
        return -1;
    }
    camp = strtok(NULL, " ");
    if (camp == NULL || copiarText(&configuracio->ip_itaca, camp) != 0) {
        return -1;
    }
    camp = strtok(NULL, " ");
    if (camp == NULL) {
        return -1;
    }
    configuracio->port_itaca = atoi(camp);

    if (llegirLinia(fd, linia, MIDA_LINIA) != 1) {
        return -1;
    }
    camp = strtok(linia, " ");
    if (camp == NULL || copiarText(&configuracio->ip_illa_inicial, camp) != 0) {
        return -1;
    }
    camp = strtok(NULL, " ");
    if (camp == NULL) {
        return -1;
    }
    configuracio->port_illa_inicial = atoi(camp);

    return 0;
}

/***********************************************
 *
 * @Finalitat: Carrega els diners i els aliments inicials d'Odysseus.
 * @Parametres: in: fd = descriptor del fitxer de configuracio.
 *              out: configuracio = dades que es carreguen.
 * @Retorn: Retorna 0 si es carreguen les dades i -1 si es produeix un error.
 *
 ************************************************/
int carregarAliments(int fd, ConfiguracioOdisseu *configuracio) {
    int i = 0;
    char linia[MIDA_LINIA], *camp = NULL;

    if (llegirLinia(fd, linia, MIDA_LINIA) != 1) {
        return -1;
    }
    configuracio->diners = atoi(linia);

    if (llegirLinia(fd, linia, MIDA_LINIA) != 1) {
        return -1;
    }
    configuracio->nombre_aliments = atoi(linia);
    if (configuracio->nombre_aliments < 0) {
        return -1;
    }
    if (configuracio->nombre_aliments == 0) {
        return 0;
    }

    configuracio->aliments = malloc(configuracio->nombre_aliments *sizeof(Aliment));
    if (configuracio->aliments == NULL) {
        return -1;
    }

    for (i = 0; i < configuracio->nombre_aliments; i++) {
        configuracio->aliments[i].nom = NULL;
        configuracio->aliments[i].quantitat = 0;
    }

    for (i = 0; i < configuracio->nombre_aliments; i++) {
        if (llegirLinia(fd, linia, MIDA_LINIA) != 1) {
            return -1;
        }
        camp = strtok(linia, " ");
        if (camp == NULL ||
            copiarText(&configuracio->aliments[i].nom, camp) != 0) {
            return -1;
        }
        camp = strtok(NULL, " ");
        if (camp == NULL) {
            return -1;
        }
        configuracio->aliments[i].quantitat = atoi(camp);
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

    configuracio->ruta_carpeta = NULL;
    configuracio->nom = NULL;
    configuracio->ip_itaca = NULL;
    configuracio->ip_illa_inicial = NULL;
    configuracio->aliments = NULL;
    configuracio->nombre_aliments = 0;
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

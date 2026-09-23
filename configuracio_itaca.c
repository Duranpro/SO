#define _GNU_SOURCE

/***********************************************
 *
 * @Proposit: Carrega i allibera la configuracio i els viatges d'Ithaca.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 18/09/2026
 * @Data ultima modificacio: 18/09/2026
 *
 ************************************************/

#include "configuracio_itaca.h"

/***********************************************
 *
 * @Finalitat: Carrega el fitxer de configuracio d'Ithaca.
 * @Parametres: in: nom_fitxer = ruta del fitxer de configuracio.
 *              out: configuracio = configuracio carregada.
 * @Retorn: Retorna 0 si es carrega correctament i -1 si es produeix un error.
 *
 ************************************************/
int carregarConfiguracioItaca(char *nom_fitxer, ConfiguracioItaca *configuracio) {
    int fd = -1, resultat = 0, caracters_escrits = 0;
    char linia[MIDA_LINIA], *camp = NULL, *missatge = NULL;

    fd = open(nom_fitxer, O_RDONLY);
    if (fd < 0) {
        caracters_escrits = asprintf(&missatge, "Error: no s'ha pogut obrir %s.\n", nom_fitxer);
        if (caracters_escrits >= 0) {
            write(2, missatge, caracters_escrits);
            free(missatge);
        }
        return -1;
    }

    if (llegirLinia(fd, linia, MIDA_LINIA) != 1 ||copiarText(&configuracio->nom, linia) != 0) {
        resultat = -1;
    }
    if (resultat == 0 &&
        (llegirLinia(fd, linia, MIDA_LINIA) != 1 || copiarText(&configuracio->ruta_carpeta, linia) != 0)) {
        resultat = -1;
    }
    if (resultat == 0 && llegirLinia(fd, linia, MIDA_LINIA) == 1) {
        camp = strtok(linia, " ");
        if (camp == NULL || copiarText(&configuracio->ip, camp) != 0) {
            resultat = -1;
        } else {
            camp = strtok(NULL, " ");
            if (camp == NULL) {
                resultat = -1;
            } else {
                configuracio->port = atoi(camp);
            }
        }
    } else if (resultat == 0) {
        resultat = -1;
    }
    close(fd);

    if (resultat != 0) {
        caracters_escrits = asprintf(&missatge,"Error: no s'ha pogut carregar %s.\n", nom_fitxer);
        if (caracters_escrits >= 0) {
            write(2, missatge, caracters_escrits);
            free(missatge);
        }
        return -1;
    }

    return 0;
}

/***********************************************
 *
 * @Finalitat: Afegeix a memoria un viatge llegit del fitxer.
 * @Parametres: in/out: linia = linia que conte el viatge.
 *              in/out: configuracio = dades d'Ithaca i els seus viatges.
 * @Retorn: Retorna 0 si s'afegeix el viatge i -1 si es produeix un error.
 *
 ************************************************/
int afegirViatge(char *linia, ConfiguracioItaca *configuracio) {
    Viatge *viatges_ampliats = NULL, *viatge = NULL;
    char *camp = NULL;

    viatges_ampliats = realloc(configuracio->viatges, (configuracio->nombre_viatges + 1) * sizeof(Viatge));
    if (viatges_ampliats == NULL) {
        return -1;
    }
    configuracio->viatges = viatges_ampliats;
    viatge = &configuracio->viatges[configuracio->nombre_viatges];
    viatge->identificador = configuracio->nombre_viatges + 1;
    viatge->nom_objecte = NULL;
    viatge->ruta_fitxer = NULL;
    viatge->illa_desti = NULL;
    viatge->recompensa = 0;
    configuracio->nombre_viatges++;

    camp = strtok(linia, " ");
    if (camp == NULL || copiarText(&viatge->nom_objecte, camp) != 0) {
        return -1;
    }
    camp = strtok(NULL, " ");
    if (camp == NULL || copiarText(&viatge->ruta_fitxer, camp) != 0) {
        return -1;
    }
    camp = strtok(NULL, " ");
    if (camp == NULL || copiarText(&viatge->illa_desti, camp) != 0) {
        return -1;
    }
    camp = strtok(NULL, " ");
    if (camp == NULL) {
        return -1;
    }
    viatge->recompensa = atoi(camp);

    return 0;
}

/***********************************************
 *
 * @Finalitat: Llegeix tots els viatges i els carrega a memoria.
 * @Parametres: in: nom_fitxer = ruta del fitxer de viatges.
 *              in/out: configuracio = dades d'Ithaca i els seus viatges.
 * @Retorn: Retorna 0 si es carreguen els viatges i -1 si hi ha un error.
 *
 ************************************************/
int carregarViatges(char *nom_fitxer, ConfiguracioItaca *configuracio) {
    int fd = -1, resultat = 0, caracters_escrits = 0;
    char linia[MIDA_LINIA], *missatge = NULL;

    fd = open(nom_fitxer, O_RDONLY);
    if (fd < 0) {
        resultat = -1;
    } else {
        resultat = llegirLinia(fd, linia, MIDA_LINIA);
        while (resultat == 1) {
            resultat = afegirViatge(linia, configuracio);
            if (resultat == 0) {
                resultat = llegirLinia(fd, linia, MIDA_LINIA);
            }
        }
        close(fd);
        if (resultat == 0) {
            return 0;
        }
    }

    caracters_escrits = asprintf(&missatge, "Error: no s'ha pogut carregar %s.\n", nom_fitxer);
    if (caracters_escrits >= 0) {
        write(2, missatge, caracters_escrits);
        free(missatge);
    }

    return -1;
}

/***********************************************
 *
 * @Finalitat: Allibera la configuracio i els viatges d'Ithaca.
 * @Parametres: in/out: configuracio = dades que es volen alliberar.
 * @Retorn: ----.
 *
 ************************************************/
void alliberarConfiguracioItaca(ConfiguracioItaca *configuracio) {
    int i = 0;

    free(configuracio->nom);
    free(configuracio->ruta_carpeta);
    free(configuracio->ip);

    for (i = 0; i < configuracio->nombre_viatges; i++) {
        free(configuracio->viatges[i].nom_objecte);
        free(configuracio->viatges[i].ruta_fitxer);
        free(configuracio->viatges[i].illa_desti);
    }
    free(configuracio->viatges);

    configuracio->nom = NULL;
    configuracio->ruta_carpeta = NULL;
    configuracio->ip = NULL;
    configuracio->viatges = NULL;
    configuracio->nombre_viatges = 0;
}

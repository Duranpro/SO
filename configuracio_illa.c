#define _GNU_SOURCE

/***********************************************
 *
 * @Proposit: Carrega, valida i allibera les dades d'una illa.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 21/09/2026
 * @Data ultima modificacio: 21/09/2026
 *
 ************************************************/

#include "configuracio_illa.h"

/***********************************************
 *
 * @Finalitat: Afegeix a memoria una ruta llegida del fitxer.
 * @Parametres: in/out: linia = linia que conte la ruta.
 *              in/out: configuracio = configuracio de l'illa.
 * @Retorn: Retorna 0 si s'afegeix la ruta i -1 si es produeix un error.
 *
 ************************************************/
int afegirRuta(char *linia, ConfiguracioIlla *configuracio) {
    Ruta *rutes_ampliades = NULL, *ruta = NULL;
    char *camp = NULL;

    rutes_ampliades = realloc(configuracio->rutes, (configuracio->nombre_rutes + 1) * sizeof(Ruta));
    if (rutes_ampliades == NULL) {
        return -1;
    }
    configuracio->rutes = rutes_ampliades;
    ruta = &configuracio->rutes[configuracio->nombre_rutes];
    ruta->nom_illa = NULL;
    ruta->ip = NULL;
    ruta->port = 0;
    configuracio->nombre_rutes++;

    camp = strtok(linia, " ");
    if (camp == NULL || copiarText(&ruta->nom_illa, camp) != 0) {
        return -1;
    }
    camp = strtok(NULL, " ");
    if (camp == NULL || copiarText(&ruta->ip, camp) != 0) {
        return -1;
    }
    camp = strtok(NULL, " ");
    if (camp == NULL) {
        return -1;
    }
    ruta->port = atoi(camp);

    return 0;
}

/***********************************************
 *
 * @Finalitat: Llegeix les rutes fins arribar al final del fitxer.
 * @Parametres: in: fd = descriptor del fitxer de configuracio.
 *              in/out: configuracio = configuracio de l'illa.
 * @Retorn: Retorna 0 si es carreguen les rutes i -1 si hi ha un error.
 *
 ************************************************/
int carregarRutes(int fd, ConfiguracioIlla *configuracio) {
    int resultat = 0;
    char linia[MIDA_LINIA];

    resultat = llegirLinia(fd, linia, MIDA_LINIA);
    while (resultat == 1) {
        resultat = afegirRuta(linia, configuracio);
        if (resultat == 0) {
            resultat = llegirLinia(fd, linia, MIDA_LINIA);
        }
    }

    if (resultat == 0) {
        return 0;
    }
    return -1;
}

/***********************************************
 *
 * @Finalitat: Carrega el fitxer de configuracio d'una illa.
 * @Parametres: in: nom_fitxer = ruta del fitxer de configuracio.
 *              out: configuracio = configuracio carregada.
 * @Retorn: Retorna 0 si es carrega correctament i -1 si es produeix un error.
 *
 ************************************************/
int carregarConfiguracioIlla(char *nom_fitxer, ConfiguracioIlla *configuracio) {
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
    if (resultat == 0 && llegirLinia(fd, linia, MIDA_LINIA) == 1) {
        configuracio->capacitat_port = atoi(linia);
    } else if (resultat == 0) {
        resultat = -1;
    }
    if (resultat == 0 && llegirLinia(fd, linia, MIDA_LINIA) == 1) {
        if (strcmp(linia, "--- ROUTES ---") != 0) {
            resultat = -1;
        }
    } else if (resultat == 0) {
        resultat = -1;
    }
    if (resultat == 0) {
        resultat = carregarRutes(fd, configuracio);
    }
    close(fd);

    if (resultat != 0) {
        caracters_escrits = asprintf(
            &missatge, "Error: no s'ha pogut carregar %s.\n", nom_fitxer
        );
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
 * @Finalitat: Allibera els noms temporals utilitzats per SPHRAGIS.
 * @Parametres: in/out: illa = configuracio temporal de SPHRAGIS.
 *              in: nombre_rutes = mida original de l'array de noms.
 * @Retorn: ----.
 *
 ************************************************/
void alliberarConfiguracioSphragis(SPHRAGIS_Island *illa, int nombre_rutes) {
    int i = 0;

    if (illa->known_islands != NULL) {
        for (i = 0; i < nombre_rutes; i++) {
            if (illa->known_islands[i] != NULL) {
                free(illa->known_islands[i]);
            }
        }
    }
    free(illa->known_islands);
    illa->known_islands = NULL;
    illa->known_island_count = 0;
}

/***********************************************
 *
 * @Finalitat: Conserva les rutes acceptades amb la seva IP i el seu port.
 * @Parametres: in/out: configuracio = rutes originals de l'illa.
 *              in: illa_sphragis = noms de les rutes acceptades.
 * @Retorn: ----.
 *
 ************************************************/
void conservarRutesValides(ConfiguracioIlla *configuracio, SPHRAGIS_Island *illa_sphragis) {
    int nombre_original = configuracio->nombre_rutes;
    int i = 0, j = 0, posicio = 0, ruta_valida = 0;

    for (i = 0; i < nombre_original; i++) {
        ruta_valida = 0;
        for (j = 0; j < illa_sphragis->known_island_count; j++) {
            if (illa_sphragis->known_islands[j] != NULL &&
                strcmp(configuracio->rutes[i].nom_illa, illa_sphragis->known_islands[j]) == 0) {
                ruta_valida = 1;
            }
        }
        if (ruta_valida == 1) {
            if (posicio != i) {
                configuracio->rutes[posicio] = configuracio->rutes[i];
            }
            posicio++;
        } else {
            free(configuracio->rutes[i].nom_illa);
            free(configuracio->rutes[i].ip);
        }
    }
    configuracio->nombre_rutes = posicio;
}

/***********************************************
 *
 * @Finalitat: Filtra les rutes amb la llibreria oficial SPHRAGIS.
 * @Parametres: in/out: configuracio = rutes que es volen validar.
 * @Retorn: Retorna 0 si es validen les rutes i -1 si es produeix un error.
 *
 ************************************************/
int validarRutes(ConfiguracioIlla *configuracio) {
    SPHRAGIS_Island illa_sphragis = {0};
    int nombre_original = configuracio->nombre_rutes, resultat = 0;
    int i = 0, caracters_escrits = 0;
    char *missatge = NULL;

    illa_sphragis.name = configuracio->nom;
    illa_sphragis.known_island_count = nombre_original;
    if (nombre_original > 0) {
        illa_sphragis.known_islands = malloc(nombre_original * sizeof(char *));
        if (illa_sphragis.known_islands == NULL) {
            resultat = -1;
        }
    }
    for (i = 0; i < nombre_original && resultat == 0; i++) {
        illa_sphragis.known_islands[i] = NULL;
    }
    for (i = 0; i < nombre_original && resultat == 0; i++) {
        resultat = copiarText(&illa_sphragis.known_islands[i], configuracio->rutes[i].nom_illa);
    }
    if (resultat == 0) {
        resultat = SPHRAGIS_filter_island_configuration(&illa_sphragis);
    }

    if (resultat >= 0) {
        conservarRutesValides(configuracio, &illa_sphragis);
    } else {
        caracters_escrits = asprintf(&missatge, "Error: SPHRAGIS no ha pogut validar les rutes.\n");
        if (caracters_escrits >= 0) {
            write(2, missatge, caracters_escrits);
            free(missatge);
        }
    }

    alliberarConfiguracioSphragis(&illa_sphragis, nombre_original);
    if (resultat < 0) {
        return -1;
    }
    return 0;
}

/***********************************************
 *
 * @Finalitat: Carrega els productes d'un fitxer de stock binari.
 * @Parametres: in: nom_fitxer = ruta del fitxer de stock.
 *              in/out: configuracio = configuracio i productes de l'illa.
 * @Retorn: Retorna 0 si es carrega el stock i -1 si es produeix un error.
 *
 ************************************************/
int carregarStock(char *nom_fitxer, ConfiguracioIlla *configuracio) {
    Producte producte = {0}, *productes_ampliats = NULL;
    int fd = -1, bytes_llegits = 0, mida_producte = sizeof(Producte);
    int resultat = 0, caracters_escrits = 0;
    char *missatge = NULL;

    fd = open(nom_fitxer, O_RDONLY);
    if (fd < 0) {
        caracters_escrits = asprintf(&missatge, "Error: no s'ha pogut obrir %s.\n", nom_fitxer);
        if (caracters_escrits >= 0) {
            write(2, missatge, caracters_escrits);
            free(missatge);
        }
        return -1;
    }

    bytes_llegits = read(fd, &producte, mida_producte);
    while (bytes_llegits == mida_producte && resultat == 0) {
        productes_ampliats = realloc(configuracio->productes, (configuracio->nombre_productes + 1) * sizeof(Producte));
        if (productes_ampliats == NULL) {
            resultat = -1;
        } else {
            configuracio->productes = productes_ampliats;
            configuracio->productes[configuracio->nombre_productes] = producte;
            configuracio->nombre_productes++;
            bytes_llegits = read(fd, &producte, mida_producte);
        }
    }
    close(fd);

    if (resultat != 0 || bytes_llegits != 0) {
        caracters_escrits = asprintf(&missatge, "Error: no s'ha pogut carregar %s.\n", nom_fitxer);
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
 * @Finalitat: Allibera la configuracio, les rutes i el stock d'una illa.
 * @Parametres: in/out: configuracio = dades que es volen alliberar.
 * @Retorn: ----.
 *
 ************************************************/
void alliberarConfiguracioIlla(ConfiguracioIlla *configuracio) {
    int i = 0;

    free(configuracio->nom);
    free(configuracio->ruta_carpeta);
    free(configuracio->ip);
    for (i = 0; i < configuracio->nombre_rutes; i++) {
        free(configuracio->rutes[i].nom_illa);
        free(configuracio->rutes[i].ip);
    }
    free(configuracio->rutes);
    free(configuracio->productes);

    configuracio->nom = NULL;
    configuracio->ruta_carpeta = NULL;
    configuracio->ip = NULL;
    configuracio->rutes = NULL;
    configuracio->nombre_rutes = 0;
    configuracio->productes = NULL;
    configuracio->nombre_productes = 0;
}

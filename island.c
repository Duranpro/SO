#define _GNU_SOURCE

/***********************************************
 *
 * @Proposit: Conte el punt d'entrada del proces Island.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 18/09/2026
 * @Data ultima modificacio: 23/09/2026
 *
 ************************************************/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "configuracio_illa.h"

int finalitzar_programa = 0;

/***********************************************
 *
 * @Finalitat: Indica que Island ha de finalitzar en rebre SIGINT.
 * @Parametres: in: senyal = senyal rebut pel proces.
 * @Retorn: ----.
 *
 ************************************************/
void gestionarSigint(int senyal __attribute__((unused))) {
    finalitzar_programa = 1;
}

/***********************************************
 *
 * @Finalitat: Carrega la configuracio, les rutes i el stock d'una illa.
 * @Parametres: in: argc = nombre d'arguments rebuts.
 *              in: argv = arguments rebuts.
 * @Retorn: Retorna 0 si els arguments son correctes i 1 altrament.
 *
 ************************************************/
int main(int argc, char *argv[]) {
    ConfiguracioIlla configuracio = {0};
    char *missatge = NULL, *nom_programa = argv[0];
    int resultat = 0, caracters_escrits = 0;

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

    signal(SIGINT, gestionarSigint);
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
        pause();
    }

    caracters_escrits = asprintf(&missatge, "%s closes its port.\n", configuracio.nom);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }

    alliberarConfiguracioIlla(&configuracio);
    return 0;
}

#define _GNU_SOURCE

/***********************************************
 *
 * @Proposit: Conte el punt d'entrada del proces Ithaca.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 18/09/2026
 * @Data ultima modificacio: 23/09/2026
 *
 ************************************************/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "configuracio_itaca.h"

int finalitzar_programa = 0;

/***********************************************
 *
 * @Finalitat: Indica que Ithaca ha de finalitzar en rebre SIGINT.
 * @Parametres: in: senyal = senyal rebut pel proces.
 * @Retorn: ----.
 *
 ************************************************/
void gestionarSigint(int senyal __attribute__((unused))) {
    finalitzar_programa = 1;
}

/***********************************************
 *
 * @Finalitat: Carrega la configuracio i els viatges del proces Ithaca.
 * @Parametres: in: argc = nombre d'arguments rebuts.
 *              in: argv = arguments rebuts.
 * @Retorn: Retorna 0 si els arguments son correctes i 1 altrament.
 *
 ************************************************/
int main(int argc, char *argv[]) {
    ConfiguracioItaca configuracio = {0};
    char *missatge = NULL, *nom_programa = argv[0];
    int resultat = 0, caracters_escrits = 0;

    if (argc != 3) {
        caracters_escrits = asprintf(&missatge,"Usage: %s <config.dat> <voyages.dat>\n", nom_programa);
        if (caracters_escrits >= 0) {
            write(2, missatge, caracters_escrits);
            free(missatge);
        }
        return 1;
    }

    resultat = carregarConfiguracioItaca(argv[1], &configuracio);
    if (resultat == 0) {
        resultat = carregarViatges(argv[2], &configuracio);
    }
    if (resultat != 0) {
        alliberarConfiguracioItaca(&configuracio);
        return 1;
    }

    signal(SIGINT, gestionarSigint);
    caracters_escrits = asprintf(&missatge,"Ithaca initialized. %d voyages loaded.\nWaiting for Odysseus...\n",configuracio.nombre_viatges);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }

    while (finalitzar_programa == 0) {
        pause();
    }

    caracters_escrits = asprintf(&missatge, "Ithaca closes the harbor.\n");
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }

    alliberarConfiguracioItaca(&configuracio);
    return 0;
}

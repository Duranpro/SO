#define _GNU_SOURCE

/***********************************************
 *
 * @Proposit: Conte el punt d'entrada del proces Odysseus.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 18/09/2026
 * @Data ultima modificacio: 23/09/2026
 *
 ************************************************/

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "comandes.h"
#include "configuracio_odisseu.h"

int finalitzar_programa = 0;

/***********************************************
 *
 * @Finalitat: Indica que Odysseus ha de finalitzar en rebre SIGINT.
 * @Parametres: in: senyal = senyal rebut pel proces.
 * @Retorn: ----.
 *
 ************************************************/
void gestionarSigint(int senyal __attribute__((unused))) {
    finalitzar_programa = 1;
}

/***********************************************
 *
 * @Finalitat: Carrega la configuracio i executa el terminal d'Odysseus.
 * @Parametres: in: argc = nombre d'arguments rebuts.
 *              in: argv = arguments rebuts.
 * @Retorn: Retorna 0 si els arguments son correctes i 1 altrament.
 *
 ************************************************/
int main(int argc, char *argv[]) {
    ConfiguracioOdisseu configuracio = {0};
    char *missatge = NULL, *nom_programa = argv[0];
    int resultat = 0, caracters_escrits = 0;

    if (argc != 2) {
        caracters_escrits = asprintf(&missatge, "Usage: %s <config.dat>\n",nom_programa);
        if (caracters_escrits >= 0) {
            write(2, missatge, caracters_escrits);
            free(missatge);
        }
        return 1;
    }

    resultat = carregarConfiguracioOdisseu(argv[1], &configuracio);
    if (resultat != 0) {
        return 1;
    }

    signal(SIGINT, gestionarSigint);
    caracters_escrits = asprintf(&missatge,"Odysseus %s is ready to sail.\n", configuracio.nom);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }

    resultat = executarTerminal(&finalitzar_programa);
    alliberarConfiguracioOdisseu(&configuracio);
    if (resultat != 0) {
        return 1;
    }
    return 0;
}

#define _GNU_SOURCE

/***********************************************
 *
 * @Proposit: Conte el punt d'entrada del proces Odysseus.
 * @Autor/s: Antonio Duran Sabates i Arnau González Hortelano
 * @Data creacio: 18/09/2026
 * @Data ultima modificacio: 15/10/2026
 *
 ************************************************/

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "comandes.h"
#include "configuracio_odisseu.h"

int finalitzar_programa = 0;
int fd_ithaca = -1;
int fd_illa = -1;

/***********************************************
 *
 * @Finalitat: Indica la finalitzacio i tanca els descriptors que poden esperar.
 * @Parametres: in: senyal = senyal rebut pel proces.
 * @Retorn: ----.
 *
 ************************************************/
void gestionarSigint(int senyal __attribute__((unused))) {
    finalitzar_programa = 1;
    close(0);
    if (fd_ithaca >= 0) {
        close(fd_ithaca);
        fd_ithaca = -1;
    }
    if (fd_illa >= 0) {
        close(fd_illa);
        fd_illa = -1;
    }
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
    signal(SIGPIPE, SIG_IGN);
    caracters_escrits = asprintf(&missatge,"Odysseus %s is ready to sail.\n", configuracio.nom);
    if (caracters_escrits >= 0) {
        write(1, missatge, caracters_escrits);
        free(missatge);
    }

    resultat = executarTerminal(&finalitzar_programa, &configuracio, &fd_ithaca, &fd_illa);
    if (fd_ithaca >= 0) {
        close(fd_ithaca);
        fd_ithaca = -1;
    }
    if (fd_illa >= 0) {
        close(fd_illa);
        fd_illa = -1;
    }
    alliberarConfiguracioOdisseu(&configuracio);
    if (resultat != 0) {
        return 1;
    }
    return 0;
}

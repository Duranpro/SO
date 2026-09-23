/***********************************************
 *
 * @Proposit: Declara el terminal i el parser de comandes d'Odysseus.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 23/09/2026
 * @Data ultima modificacio: 23/09/2026
 *
 ************************************************/

#ifndef COMANDES_H
#define COMANDES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "comun.h"

#define MAX_PARAULES 4

#define RESULTAT_DESCONEGUDA -1
#define RESULTAT_SINTAXI_INCORRECTA 0
#define RESULTAT_CORRECTE 1

#define COMANDA_DESCONEGUDA 0
#define COMANDA_CONNECT 1
#define COMANDA_LIST_VOYAGES 2
#define COMANDA_ACCEPT 3
#define COMANDA_SAIL 4
#define COMANDA_MAP 5
#define COMANDA_LIST_MARKET 6
#define COMANDA_BUY 7
#define COMANDA_SELL 8
#define COMANDA_STATUS 9
#define COMANDA_DELIVER 10
#define COMANDA_CLAIM 11

typedef struct {
    int tipus;
    char argument[MIDA_LINIA];
    int valor;
} Comanda;

int escriureMissatge(char *text);
int llegirComanda(char *buffer, int mida);
int esNumero(char *text);
int separarParaules(char *linia, char *paraules[], int maxim);
int mostrarUsComanda(int tipus);
int analitzarConnexio(char *paraules[], int nombre_paraules,
                      Comanda *comanda);
int analitzarLlista(char *paraules[], int nombre_paraules, Comanda *comanda);
int analitzarAcceptacio(char *paraules[], int nombre_paraules,
                        Comanda *comanda);
int analitzarNavegacio(char *paraules[], int nombre_paraules,
                       Comanda *comanda);
int analitzarCompraVenda(char *paraules[], int nombre_paraules, int tipus,
                         Comanda *comanda);
int analitzarSenseArguments(int nombre_paraules, int tipus,
                            Comanda *comanda);
int analitzarComanda(char *linia, Comanda *comanda);
int mostrarResultatComanda(int resultat, int tipus);
int executarTerminal(int *finalitzar_programa);

#endif

/***********************************************
 *
 * @Proposit: Declara el terminal i el parser de comandes d'Odysseus.
 * @Autor/s: Antonio Duran Sabates i Arnau González Hortelano
 * @Data creacio: 23/09/2026
 * @Data ultima modificacio: 15/10/2026
 *
 ************************************************/

#ifndef COMANDES_H
#define COMANDES_H

#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/select.h>

#include "comun.h"
#include "tipus.h"
#include "trames.h"
#include "xarxa.h"
#include "sphragis.h"

#define MAX_PARAULES 4

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
    char *argument;
    int valor;
} Comanda;

int escriureMissatge(char *text);
int mostrarUsComanda(int tipus);
int analitzarConnexio(char *paraules[], int nombre_paraules,Comanda *comanda);
int analitzarLlista(char *paraules[], int nombre_paraules, Comanda *comanda);
int analitzarAcceptacio(char *paraules[], int nombre_paraules,Comanda *comanda);
int analitzarNavegacio(char *paraules[], int nombre_paraules,Comanda *comanda);
int analitzarCompraVenda(char *paraules[], int nombre_paraules, int tipus,Comanda *comanda);
int analitzarSenseArguments(int nombre_paraules, int tipus,Comanda *comanda);
int analitzarComanda(char *linia, Comanda *comanda);
int mostrarResultatComanda(int resultat, int tipus);
int connectarIthaca(ConfiguracioOdisseu *configuracio, int *fd_ithaca);
int executarListVoyages(ConfiguracioOdisseu *configuracio, int *fd_ithaca);
int executarAccept(ConfiguracioOdisseu *configuracio, int *fd_ithaca, int identificador);
int desconnectarIthaca(int *fd_ithaca);
int consumirProvisions(ConfiguracioOdisseu *configuracio, int quantitat);
int esperarPort(ConfiguracioOdisseu *configuracio, int *fd_illa, char *nom_illa,
                int *finalitzar_programa);
int arribarIlla(ConfiguracioOdisseu *configuracio, int *fd_illa, IllaConeguda *desti,
                int *finalitzar_programa);
int executarSail(ConfiguracioOdisseu *configuracio, int *fd_ithaca, int *fd_illa,
                 char *desti, int *finalitzar_programa);
int executarTerminal(int *finalitzar_programa, ConfiguracioOdisseu *configuracio,
                     int *fd_ithaca, int *fd_illa);
int executarListMarket(ConfiguracioOdisseu *configuracio, int *fd_illa, char *nom_illa);
int intercanviarAliments(int *fd_illa, int tipus, char *producte, int quantitat, int *import);
int executarBuy(ConfiguracioOdisseu *configuracio, int *fd_illa, char *producte, int quantitat);
int executarSell(ConfiguracioOdisseu *configuracio, int *fd_illa, char *producte, int quantitat);
int buscarIllaConeguda(ConfiguracioOdisseu *configuracio, char *nom);
int afegirIllaConeguda(ConfiguracioOdisseu *configuracio, char *nom, char *ip, int port);
void afegirConnexio(IllaConeguda *origen, IllaConeguda *desti);
int executarBuyMap(ConfiguracioOdisseu *configuracio, int *fd_illa, int quantitat);
int mostrarMapa(ConfiguracioOdisseu *configuracio);
int mostrarStatus(ConfiguracioOdisseu *configuracio);
int esConnexioDirecta(ConfiguracioOdisseu *configuracio, char *desti);
int sortirIlla(ConfiguracioOdisseu *configuracio, int *fd_illa);

#endif

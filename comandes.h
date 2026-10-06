/***********************************************
 *
 * @Proposit: Declara el terminal i el parser de comandes d'Odysseus.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 23/09/2026
 * @Data ultima modificacio: 25/09/2026
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
#include "tipus.h"

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

#define SENSE_CONNEXIO 0
#define CONNEXIO_ITACA 1
#define CONNEXIO_ILLA 2

#define NOM_ILLA_INICIAL "Aeaea"

#define PORT_DOCKED 1
#define PORT_WAIT 2

typedef struct {
    int tipus;
    char *argument;
    int valor;
} Comanda;

int escriureMissatge(char *text);
int esNumero(char *text);
int mostrarUsComanda(int tipus);
int analitzarConnexio(char *paraules[], int nombre_paraules,Comanda *comanda);
int analitzarLlista(char *paraules[], int nombre_paraules, Comanda *comanda);
int analitzarAcceptacio(char *paraules[], int nombre_paraules, Comanda *comanda);
int analitzarNavegacio(char *paraules[], int nombre_paraules,Comanda *comanda);
int analitzarCompraVenda(char *paraules[], int nombre_paraules, int tipus, Comanda *comanda);
int analitzarSenseArguments(int nombre_paraules, int tipus, Comanda *comanda);
int analitzarComanda(char *linia, Comanda *comanda);
int mostrarResultatComanda(int resultat, int tipus);
int connectarItaca(ConfiguracioOdisseu *configuracio, int *socket_actual);
int mostrarViatgeDisponible(unsigned char *trama, int index_esperat, int *nombre_total);
int llistarViatges(ConfiguracioOdisseu *configuracio, int socket_actual);
int acceptarViatge(ConfiguracioOdisseu *configuracio, int socket_actual,int identificador);
int desconnectarItaca(ConfiguracioOdisseu *configuracio, int *socket_actual);
int rebreEstatPort(int socket_illa);
int esperarPort(ConfiguracioOdisseu *configuracio, int *socket_actual,char *nom_illa);
int navegarAeaea(ConfiguracioOdisseu *configuracio, int *socket_actual, char *nom_illa);
int executarTerminal(int *finalitzar_programa, ConfiguracioOdisseu *configuracio, int *socket_actual);

#endif

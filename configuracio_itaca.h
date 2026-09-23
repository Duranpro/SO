/***********************************************
 *
 * @Proposit: Declara la carrega de la configuracio i els viatges d'Ithaca.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 18/09/2026
 * @Data ultima modificacio: 18/09/2026
 *
 ************************************************/

#ifndef CONFIGURACIO_ITACA_H
#define CONFIGURACIO_ITACA_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "comun.h"
#include "tipus.h"

int carregarConfiguracioItaca(char *nom_fitxer, ConfiguracioItaca *configuracio);
int afegirViatge(char *linia, ConfiguracioItaca *configuracio);
int carregarViatges(char *nom_fitxer, ConfiguracioItaca *configuracio);
void alliberarConfiguracioItaca(ConfiguracioItaca *configuracio);

#endif

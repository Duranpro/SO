/***********************************************
 *
 * @Proposit: Declara la carrega de la configuracio d'Odysseus.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 18/09/2026
 * @Data ultima modificacio: 18/09/2026
 *
 ************************************************/

#ifndef CONFIGURACIO_ODISSEU_H
#define CONFIGURACIO_ODISSEU_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "comun.h"
#include "tipus.h"

int carregarLocalitzacio(int fd, ConfiguracioOdisseu *configuracio);
int carregarAliments(int fd, ConfiguracioOdisseu *configuracio);
int carregarConfiguracioOdisseu(char *nom_fitxer,ConfiguracioOdisseu *configuracio);
void alliberarConfiguracioOdisseu(ConfiguracioOdisseu *configuracio);

#endif

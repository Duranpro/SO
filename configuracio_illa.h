/***********************************************
 *
 * @Proposit: Declara la carrega de la configuracio i el stock d'una illa.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 21/09/2026
 * @Data ultima modificacio: 21/09/2026
 *
 ************************************************/

#ifndef CONFIGURACIO_ILLA_H
#define CONFIGURACIO_ILLA_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "comun.h"
#include "sphragis.h"
#include "tipus.h"

int afegirRuta(char *linia, ConfiguracioIlla *configuracio);
int carregarConfiguracioIlla(char *nom_fitxer, ConfiguracioIlla *configuracio);
void alliberarConfiguracioSphragis(SPHRAGIS_Island *illa, int nombre_rutes);
void conservarRutesValides(ConfiguracioIlla *configuracio, SPHRAGIS_Island *illa_sphragis);
int validarRutes(ConfiguracioIlla *configuracio);
int carregarStock(char *nom_fitxer, ConfiguracioIlla *configuracio);
void alliberarConfiguracioIlla(ConfiguracioIlla *configuracio);

#endif

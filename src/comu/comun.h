/***********************************************
 *
 * @Proposit: Declara les operacions comunes dels tres processos.
 * @Autor/s: Antonio Duran Sabates i Arnau González Hortelano
 * @Data creacio: 21/09/2026
 * @Data ultima modificacio: 04/11/2026
 *
 ************************************************/

#ifndef COMUN_H
#define COMUN_H

#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "trames.h"

#define MIDA_RUTA_FITXER 1024

char *llegirLinia(int fd);
int esNumero(char *text);
int copiarText(char **desti, char *origen);
int calcularMd5(char *ruta_fitxer, char *md5);
int obtenirMidaFitxer(char *ruta_fitxer);
int enviarFitxer(int fd_socket, char *ruta_fitxer, char *tipus_transferencia,
                 unsigned char flags_capcalera, unsigned char flags_dades, int fd_finalitzacio);
int rebreFitxer(int fd_socket, char *carpeta_desti, char *tipus_esperat,
                unsigned char flags_capcalera, unsigned char flags_dades,
                char *nom_fitxer_rebut, int fd_finalitzacio);

int rebreFitxerDesDeCapcalera(int fd_socket, unsigned char *trama_capcalera,
                              char *carpeta_desti, char *tipus_esperat,
                              unsigned char flags_capcalera, unsigned char flags_dades,
                              char *nom_fitxer_rebut, int fd_finalitzacio);

#endif

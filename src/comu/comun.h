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
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

char *llegirLinia(int fd);
int esNumero(char *text);
int copiarText(char **desti, char *origen);
int calcularMd5(char *ruta_fitxer, char *md5);

#endif

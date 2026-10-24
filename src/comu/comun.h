/***********************************************
 *
 * @Proposit: Declara les operacions comunes dels tres processos.
 * @Autor/s: Antonio Duran Sabates i Arnau González Hortelano
 * @Data creacio: 21/09/2026
 * @Data ultima modificacio: 15/10/2026
 *
 ************************************************/

#ifndef COMUN_H
#define COMUN_H

#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

char *llegirLinia(int fd);
int esNumero(char *text);
int copiarText(char **desti, char *origen);

#endif

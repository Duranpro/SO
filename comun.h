/***********************************************
 *
 * @Proposit: Declara les operacions comunes dels tres processos.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 21/09/2026
 * @Data ultima modificacio: 21/09/2026
 *
 ************************************************/

#ifndef COMUN_H
#define COMUN_H

#include <stdlib.h>
#include <unistd.h>

char *llegirLinia(int fd);
int copiarText(char **desti, char *origen);

#endif

/***********************************************
 * @Proposit: Declara el registre compartit dels fils d'Ithaca i Island.
 * @Autor/s: Antonio Duran Sabates i Arnau Gonzalez Hortelano
 * @Data creacio: 19/10/2026
 * @Data ultima modificacio: 19/10/2026
 ************************************************/
#ifndef _GESTIO_NAVEGANTS_H
#define _GESTIO_NAVEGANTS_H

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>

typedef struct FilNavegant FilNavegant;

typedef struct {
    FilNavegant *primers;
    pthread_mutex_t mutex;
    int finalitzacio[2];
} RegistreNavegants;

struct FilNavegant {
    pthread_t fil;
    int acabat;
    RegistreNavegants *registre;
    FilNavegant *seguent;
};

int iniciarNavegants(RegistreNavegants *registre);
FilNavegant *reservarNavegant(RegistreNavegants *registre);
void registrarNavegant(FilNavegant *navegant);
void finalitzarNavegant(FilNavegant *navegant);
void recollirNavegants(RegistreNavegants *registre, int finalitzar);
int esperarNavegant(int fd_servidor);
int esperarPeticioNavegant(int fd_navegant, int fd_finalitzacio);

#endif

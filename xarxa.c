#include "xarxa.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_CONNEXIONS_PENDENTS 3

/***********************************************
 *
 * @Proposit: Implementa les operacions basiques de connexio TCP.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 24/09/2026
 * @Data ultima modificacio: 24/09/2026
 *
 ************************************************/

/***********************************************
 *
 * @Finalitat: Crea un socket TCP servidor i el deixa en escolta.
 * @Parametres: in: ip = adreca IP que utilitzara el servidor.
 *              in: port = port que utilitzara el servidor.
 * @Retorn: Retorna el descriptor del socket servidor o -1 si falla.
 *
 ************************************************/
int crearServidor(char *ip, int port) {
    struct sockaddr_in adreca_servidor = {0};
    char *missatge = NULL;
    int socket_servidor = -1, resultat = 0, caracters_escrits = 0, reutilitzar_adreca = 1;

    socket_servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_servidor < 0) {
        caracters_escrits = asprintf(&missatge, "Error: no s'ha pogut crear el socket servidor.\n");
        if (caracters_escrits >= 0) {
            write(2, missatge, caracters_escrits);
            free(missatge);
        }
        return -1;
    }

    adreca_servidor.sin_family = AF_INET;
    adreca_servidor.sin_port = htons(port);
    resultat = inet_pton(AF_INET, ip, &adreca_servidor.sin_addr);
    if (resultat != 1) {
        caracters_escrits = asprintf(&missatge, "Error: l'adreca IP del servidor no es valida.\n");
        if (caracters_escrits >= 0) {
            write(2, missatge, caracters_escrits);
            free(missatge);
        }
        close(socket_servidor);
        return -1;
    }

    resultat = bind(socket_servidor, (struct sockaddr *) &adreca_servidor, sizeof(adreca_servidor));
    if (resultat < 0) {
        caracters_escrits = asprintf(&missatge, "Error: no s'ha pogut associar el socket servidor.\n");
        if (caracters_escrits >= 0) {
            write(2, missatge, caracters_escrits);
            free(missatge);
        }
        close(socket_servidor);
        return -1;
    }

    resultat = listen(socket_servidor, MAX_CONNEXIONS_PENDENTS);
    if (resultat < 0) {
        caracters_escrits = asprintf(&missatge, "Error: no s'ha pogut posar el socket en escolta.\n");
        if (caracters_escrits >= 0) {
            write(2, missatge, caracters_escrits);
            free(missatge);
        }
        close(socket_servidor);
        return -1;
    }

    return socket_servidor;
}

/***********************************************
 *
 * @Finalitat: Espera i accepta una connexio TCP en un socket servidor.
 * @Parametres: in: socket_servidor = descriptor del socket servidor.
 * @Retorn: Retorna el descriptor de la connexio o -1 si falla.
 *
 ************************************************/
int esperarConnexio(int socket_servidor) {
    int socket_client = -1;

    socket_client = accept(socket_servidor, NULL, NULL);
    return socket_client;
}

/***********************************************
 *
 * @Finalitat: Crea un socket TCP client i el connecta a un servidor.
 * @Parametres: in: ip = adreca IP del servidor.
 *              in: port = port del servidor.
 * @Retorn: Retorna el descriptor de la connexio o -1 si falla.
 *
 ************************************************/
int connectarServidor(char *ip, int port) {
    struct sockaddr_in adreca_servidor = {0};
    char *missatge = NULL;
    int socket_servidor = -1, resultat = 0, caracters_escrits = 0;

    socket_servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_servidor < 0) {
        caracters_escrits = asprintf(&missatge, "Error: no s'ha pogut crear el socket client.\n");
        if (caracters_escrits >= 0) {
            write(2, missatge, caracters_escrits);
            free(missatge);
        }
        return -1;
    }

    adreca_servidor.sin_family = AF_INET;
    adreca_servidor.sin_port = htons(port);
    resultat = inet_pton(AF_INET, ip, &adreca_servidor.sin_addr);
    if (resultat != 1) {
        caracters_escrits = asprintf(&missatge, "Error: l'adreca IP del servidor no es valida.\n");
        if (caracters_escrits >= 0) {
            write(2, missatge, caracters_escrits);
            free(missatge);
        }
        close(socket_servidor);
        return -1;
    }

    resultat = connect(socket_servidor, (struct sockaddr *) &adreca_servidor, sizeof(adreca_servidor));
    if (resultat < 0) {
        caracters_escrits = asprintf(&missatge, "Error: no s'ha pogut connectar amb el servidor.\n");
        if (caracters_escrits >= 0) {
            write(2, missatge, caracters_escrits);
            free(missatge);
        }
        close(socket_servidor);
        return -1;
    }

    return socket_servidor;
}

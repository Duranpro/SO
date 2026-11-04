#include "xarxa.h"

#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/***********************************************
 *
 * @Proposit: Implementa l'obertura de servidors i connexions TCP IPv4.
 * @Autor/s: Antonio Duran Sabates i Arnau Gonzalez Hortelano
 * @Data creacio: 15/10/2026
 * @Data ultima modificacio: 15/10/2026
 *
 ************************************************/

#define MAX_CONNEXIONS 5

/***********************************************
 *
 * @Finalitat: Obre un socket TCP IPv4 i el posa en escolta.
 * @Parametres: in: ip = adreca IPv4 del servidor.
 *              in: port = port del servidor.
 * @Retorn: Retorna el descriptor del servidor o -1 si es produeix un error.
 *
 ************************************************/
int obrirServidor(char *ip, int port) {
    struct sockaddr_in adreca;
    int fd_socket = -1;

    fd_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_socket < 0) {
        return -1;
    }

    memset(&adreca, 0, sizeof(adreca));
    adreca.sin_family = AF_INET;
    adreca.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &adreca.sin_addr) != 1) {
        close(fd_socket);
        return -1;
    }
    if (bind(fd_socket, (struct sockaddr *)&adreca, sizeof(adreca)) < 0) {
        close(fd_socket);
        return -1;
    }
    if (listen(fd_socket, MAX_CONNEXIONS) < 0) {
        close(fd_socket);
        return -1;
    }
    return fd_socket;
}

/***********************************************
 *
 * @Finalitat: Obre un socket TCP IPv4 i el connecta al servidor.
 * @Parametres: in: ip = adreca IPv4 del servidor.
 *              in: port = port del servidor.
 * @Retorn: Retorna el descriptor connectat o -1 si es produeix un error.
 *
 ************************************************/
int connectarServidor(char *ip, int port) {
    struct sockaddr_in adreca;
    int fd_socket = -1;

    fd_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_socket < 0) {
        return -1;
    }

    memset(&adreca, 0, sizeof(adreca));
    adreca.sin_family = AF_INET;
    adreca.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &adreca.sin_addr) != 1) {
        close(fd_socket);
        return -1;
    }
    if (connect(fd_socket, (struct sockaddr *)&adreca, sizeof(adreca)) < 0) {
        close(fd_socket);
        return -1;
    }
    return fd_socket;
}

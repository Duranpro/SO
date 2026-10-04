/***********************************************
 *
 * @Proposit: Declara les operacions basiques de connexio TCP.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 24/09/2026
 * @Data ultima modificacio: 24/09/2026
 *
 ************************************************/

#ifndef XARXA_H
#define XARXA_H

int crearServidor(char *ip, int port);
int esperarConnexio(int socket_servidor);
int connectarServidor(char *ip, int port);

#endif

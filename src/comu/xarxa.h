/***********************************************
 *
 * @Proposit: Declara l'obertura de servidors i connexions TCP IPv4.
 * @Autor/s: Antonio Duran Sabates i Arnau Gonzalez Hortelano
 * @Data creacio: 15/10/2026
 * @Data ultima modificacio: 15/10/2026
 *
 ************************************************/

#ifndef XARXA_H
#define XARXA_H

int obrirServidor(char *ip, int port);
int connectarServidor(char *ip, int port);

#endif

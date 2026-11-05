/***********************************************
 *
 * @Proposit: Declara les operacions basiques de les trames Nostos.
 * @Autor/s: Antonio Duran Sabates i Arnau Gonzalez Hortelano
 * @Data creacio: 15/10/2026
 * @Data ultima modificacio: 04/11/2026
 *
 ************************************************/

#ifndef TRAMES_H
#define TRAMES_H

#include <sys/select.h>

#define MIDA_TRAMA 256
#define MIDA_DADES 253

#define POSICIO_TIPUS 0
#define POSICIO_FLAGS 1
#define POSICIO_LONGITUD 2
#define POSICIO_DADES 3

#define TIPUS_CONNECT_ITHACA 0x01
#define TIPUS_LIST_VOYAGES 0x02
#define TIPUS_ACCEPT 0x03
#define TIPUS_CLAIM 0x04
#define TIPUS_DESCONNEXIO_ITHACA 0x05
#define TIPUS_ARRIBADA_ILLA 0x10
#define TIPUS_LIST_MARKET 0x11
#define TIPUS_BUY 0x12
#define TIPUS_SELL 0x13
#define TIPUS_BUY_MAP 0x14
#define TIPUS_DELIVER 0x15
#define TIPUS_SORTIDA_ILLA 0x16
#define TIPUS_CAPCALERA_FITXER 0x20
#define TIPUS_DADES_FITXER 0x21
#define TIPUS_READY_FITXER 0x22
#define TIPUS_ACK_FITXER 0x23
#define TIPUS_NACK 0x7F
#define FLAG_PETICIO 0x00
#define FLAG_RESPOSTA_OK 0x80
#define FLAG_RESPOSTA_ERROR 0xC0
#define FLAG_DADES_PETICIO 0x20
#define FLAG_DADES_RESPOSTA 0xA0


int crearTrama(unsigned char *trama, unsigned char tipus, unsigned char flags, unsigned char *dades, int longitud);
int enviarTrama(int fd, unsigned char *trama);
int rebreTrama(int fd, unsigned char *trama);
int enviarNack(int fd, char *motiu);

int enviarTramaAmbFinalitzacio(int fd, int fd_finalitzacio, unsigned char *trama);
int rebreTramaAmbFinalitzacio(int fd, int fd_finalitzacio, unsigned char *trama);

int enviarNackAmbFinalitzacio(int fd, int fd_finalitzacio, char *motiu);

#endif

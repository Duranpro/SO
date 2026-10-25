/***********************************************
 *
 * @Proposit: Declara les operacions basiques de les trames Nostos.
 * @Autor/s: Antonio Duran Sabates i Arnau Gonzalez Hortelano
 * @Data creacio: 15/10/2026
 * @Data ultima modificacio: 15/10/2026
 *
 ************************************************/

#ifndef TRAMES_H
#define TRAMES_H

#define MIDA_TRAMA 256
#define MIDA_DADES 253

#define POSICIO_TIPUS 0
#define POSICIO_FLAGS 1
#define POSICIO_LONGITUD 2
#define POSICIO_DADES 3

#define TIPUS_CONNECT_ITHACA 0x01
#define TIPUS_LIST_VOYAGES 0x02
#define TIPUS_ACCEPT 0x03
#define TIPUS_DESCONNEXIO_ITHACA 0x05
#define TIPUS_ARRIBADA_ILLA 0x10
#define TIPUS_LIST_MARKET 0x11
#define TIPUS_BUY 0x12
#define TIPUS_SELL 0x13
#define TIPUS_BUY_MAP 0x14
#define TIPUS_SORTIDA_ILLA 0x16
#define TIPUS_NACK 0x7F
#define FLAG_PETICIO 0x00
#define FLAG_RESPOSTA_OK 0x80
#define FLAG_RESPOSTA_ERROR 0xC0

int crearTrama(unsigned char *trama, unsigned char tipus, unsigned char flags, unsigned char *dades, int longitud);
int enviarTrama(int fd, unsigned char *trama);
int rebreTrama(int fd, unsigned char *trama);
int enviarNack(int fd, char *motiu);

#endif

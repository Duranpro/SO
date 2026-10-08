/***********************************************
 *
 * @Proposit: Declara el format i les operacions comunes de les trames.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 24/09/2026
 * @Data ultima modificacio: 24/09/2026
 *
 ************************************************/

#ifndef TRAMES_H
#define TRAMES_H

#define MIDA_TRAMA 256
#define MIDA_DADES_TRAMA 253
// REVISAR
#define POSICIO_TIPUS 0
#define POSICIO_FLAGS 1
#define POSICIO_LONGITUD_DADES 2
#define POSICIO_DADES 3

#define FLAGS_PETICIO_TEXTUAL 0x00
#define FLAGS_RESPOSTA_CORRECTA 0x80
#define FLAGS_RESPOSTA_ERROR 0xC0
#define FLAGS_DADES_BINARIES 0x20
#define FLAGS_RESPOSTA_BINARIA 0xA0

#define TIPUS_CONNECTAR_ITACA 0x01
#define TIPUS_LLISTAR_VIATGES 0x02
#define TIPUS_ACCEPTAR_VIATGE 0x03
#define TIPUS_RECLAMAR_RECOMPENSA 0x04
#define TIPUS_DESCONNECTAR_ITACA 0x05

#define TIPUS_ARRIBADA_ILLA 0x10
#define TIPUS_LLISTAR_MERCAT 0x11
#define TIPUS_COMPRAR 0x12
#define TIPUS_VENDRE 0x13
#define TIPUS_COMPRAR_MAPA 0x14
#define TIPUS_ENTREGAR 0x15
#define TIPUS_SORTIDA_ILLA 0x16

#define TIPUS_CAPCALERA_FITXER 0x20
#define TIPUS_DADES_FITXER 0x21
#define TIPUS_PREPARAT 0x22
#define TIPUS_CONFIRMAR_FITXER 0x23
#define TIPUS_NACK 0x7F

#define MOTIU_TIPUS_INVALID "INVALID_TYPE"
#define MOTIU_FLAGS_INVALIDS "INVALID_FLAGS"
#define MOTIU_LONGITUD_INVALIDA "INVALID_LENGTH"
#define MOTIU_DADES_INVALIDES "INVALID_DATA"
// REVISAR
#define TRAMA_CORRECTA 0
#define TRAMA_ERROR_GENERAL -1
#define TRAMA_ERROR_TIPUS -2
#define TRAMA_ERROR_FLAGS -3
#define TRAMA_ERROR_LONGITUD -4
#define TRAMA_ERROR_DADES -5

int crearTrama(unsigned char *trama, unsigned char tipus,unsigned char flags, unsigned char *dades, int longitud);
int obtenirDades(unsigned char *trama, unsigned char *dades);
int validarTrama(unsigned char *trama);
int crearNack(unsigned char *trama, char *motiu);
int enviarTrama(int fd, unsigned char *trama);
int rebreTrama(int fd, unsigned char *trama);

#endif

#include "trames.h"

#include <string.h>
#include <unistd.h>



/***********************************************
 *
 * @Proposit: Implementa les operacions basiques de les trames Nostos.
 * @Autor/s: Antonio Duran Sabates i Arnau Gonzalez Hortelano
 * @Data creacio: 15/10/2026
 * @Data ultima modificacio: 15/10/2026
 *
 ************************************************/

/***********************************************
 *
 * @Finalitat: Construeix una trama serialitzada amb padding a zero.
 * @Parametres: out: trama = buffer de MIDA_TRAMA bytes.
 *              in: tipus = tipus de la trama.
 *              in: flags = valor del camp FLAGS.
 *              in: dades = bytes que es volen copiar.
 *              in: longitud = nombre de bytes utils de dades.
 * @Retorn: Retorna 0 si es crea la trama i -1 si els parametres no son valids.
 *
 ************************************************/
int crearTrama(unsigned char *trama, unsigned char tipus, unsigned char flags,
               unsigned char *dades, int longitud) {
    if (trama == NULL || longitud < 0 || longitud > MIDA_DADES) {
        return -1;
    }
    if (longitud > 0 && dades == NULL) {
        return -1;
    }

    memset(trama, 0, MIDA_TRAMA);
    trama[POSICIO_TIPUS] = tipus;
    trama[POSICIO_FLAGS] = flags;
    trama[POSICIO_LONGITUD] = longitud;
    if (longitud > 0) {
        memcpy(trama + POSICIO_DADES, dades, longitud);
    }
    return 0;
}

/***********************************************
 *
 * @Finalitat: Escriu una trama sencera amb una unica crida a write.
 * @Parametres: in: fd = descriptor de sortida.
 *              in: trama = buffer de MIDA_TRAMA bytes.
 * @Retorn: Retorna 0 si escriu MIDA_TRAMA bytes i -1 altrament.
 *
 ************************************************/
int enviarTrama(int fd, unsigned char *trama) {
    int bytes_escrits = 0;

    bytes_escrits = write(fd, trama, MIDA_TRAMA);
    if (bytes_escrits == MIDA_TRAMA) {
        return 0;
    }
    return -1;
}

/***********************************************
 *
 * @Finalitat: Rep una trama sencera amb una unica crida a read.
 * @Parametres: in: fd = descriptor d'entrada.
 *              out: trama = buffer de MIDA_TRAMA bytes.
 * @Retorn: Retorna 0 si rep la trama, 1 si arriba EOF i -1 altrament.
 *
 ************************************************/
int rebreTrama(int fd, unsigned char *trama) {
    int bytes_llegits = 0;

    bytes_llegits = read(fd, trama, MIDA_TRAMA);
    if (bytes_llegits == MIDA_TRAMA) {
        return 0;
    }
    if (bytes_llegits == 0) {
        return 1;
    }
    return -1;
}

/***********************************************
 * @Finalitat: Respon a una trama que no correspon a un format valid.
 * @Parametres: in: fd = socket de l'emissor; motiu = error de format detectat.
 * @Retorn: Retorna 0 si envia el NACK i -1 si falla la creacio o l'enviament.
 ************************************************/
int enviarNack(int fd, char *motiu) {
    unsigned char trama[MIDA_TRAMA];

    if (crearTrama(trama, TIPUS_NACK, FLAG_RESPOSTA_ERROR,
                   (unsigned char *)motiu, strlen(motiu)) != 0) {
        return -1;
    }
    return enviarTrama(fd, trama);
}

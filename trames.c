/***********************************************
 *
 * @Proposit: Implementa la construccio i validacio de les trames.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 24/09/2026
 * @Data ultima modificacio: 24/09/2026
 *
 ************************************************/

#include "trames.h"

#include <string.h>
#include <unistd.h>

/***********************************************
 *
 * @Finalitat: Crea una trama serialitzada i completa el padding amb zeros.
 * @Parametres: out: trama = buffer de 256 bytes que es construeix.
 *              in: tipus = tipus de missatge.
 *              in: flags = propietats de la trama.
 *              in: dades = bytes que es copiaran al camp DATA.
 *              in: longitud = nombre de bytes valids de DATA.
 * @Retorn: Retorna 0 si crea la trama i -1 si els parametres no son valids.
 *
 ************************************************/
int crearTrama(unsigned char *trama, unsigned char tipus, unsigned char flags, unsigned char *dades, int longitud) {
    if (trama == NULL || longitud < 0 || longitud > MIDA_DADES_TRAMA) {
        return TRAMA_ERROR_GENERAL;
    }
    if (longitud > 0 && dades == NULL) {
        return TRAMA_ERROR_GENERAL;
    }

    memset(trama, 0, MIDA_TRAMA);
    trama[POSICIO_TIPUS] = tipus;
    trama[POSICIO_FLAGS] = flags;
    trama[POSICIO_LONGITUD_DADES] = longitud;
    if (longitud > 0) {
        memcpy(&trama[POSICIO_DADES], dades, longitud);
    }

    return TRAMA_CORRECTA;
}

/***********************************************
 *
 * @Finalitat: Copia els bytes valids de DATA al buffer del cridador.
 * @Parametres: in: trama = trama que es consulta.
 *              out: dades = buffer on es copien les dades.
 * @Retorn: Retorna 0 si copia les dades i -1 si els parametres no son valids.
 *
 ************************************************/
int obtenirDades(unsigned char *trama, unsigned char *dades) {
    int longitud = 0;

    if (trama == NULL) {
        return TRAMA_ERROR_GENERAL;
    }
    longitud = trama[POSICIO_LONGITUD_DADES];
    if (longitud > MIDA_DADES_TRAMA || (longitud > 0 && dades == NULL)) {
        return TRAMA_ERROR_GENERAL;
    }
    if (longitud > 0) {
        memcpy(dades, &trama[POSICIO_DADES], longitud);
    }
    return TRAMA_CORRECTA;
}

/***********************************************
 *
 * @Finalitat: Valida l'estructura general d'una trama.
 * @Parametres: in: trama = trama que es vol validar.
 * @Retorn: Retorna 0 si es valida o el codi de l'error detectat.
 *
 ************************************************/
int validarTrama(unsigned char *trama) {
    int longitud = 0, i = 0;

    if (trama == NULL) {
        return TRAMA_ERROR_DADES;
    }

    switch (trama[POSICIO_TIPUS]) {
    case TIPUS_CONNECTAR_ITACA:
    case TIPUS_LLISTAR_VIATGES:
    case TIPUS_ACCEPTAR_VIATGE:
    case TIPUS_RECLAMAR_RECOMPENSA:
    case TIPUS_DESCONNECTAR_ITACA:
    case TIPUS_ARRIBADA_ILLA:
    case TIPUS_LLISTAR_MERCAT:
    case TIPUS_COMPRAR:
    case TIPUS_VENDRE:
    case TIPUS_COMPRAR_MAPA:
    case TIPUS_ENTREGAR:
    case TIPUS_SORTIDA_ILLA:
    case TIPUS_CAPCALERA_FITXER:
    case TIPUS_DADES_FITXER:
    case TIPUS_PREPARAT:
    case TIPUS_CONFIRMAR_FITXER:
    case TIPUS_NACK:
        break;
    default:
        return TRAMA_ERROR_TIPUS;
    }

    if (trama[POSICIO_FLAGS] != FLAGS_PETICIO_TEXTUAL &&
        trama[POSICIO_FLAGS] != FLAGS_RESPOSTA_CORRECTA &&
        trama[POSICIO_FLAGS] != FLAGS_RESPOSTA_ERROR &&
        trama[POSICIO_FLAGS] != FLAGS_DADES_BINARIES &&
        trama[POSICIO_FLAGS] != FLAGS_RESPOSTA_BINARIA) {
        return TRAMA_ERROR_FLAGS;
    }

    longitud = trama[POSICIO_LONGITUD_DADES];
    if (longitud > MIDA_DADES_TRAMA) {
        return TRAMA_ERROR_LONGITUD;
    }

    for (i = POSICIO_DADES + longitud; i < MIDA_TRAMA; i++) {
        if (trama[i] != 0x00) {
            return TRAMA_ERROR_DADES;
        }
    }

    return TRAMA_CORRECTA;
}

/***********************************************
 *
 * @Finalitat: Crea una trama NACK amb el motiu de l'error.
 * @Parametres: out: trama = buffer de 256 bytes que es construeix.
 *              in: motiu = text que descriu l'error.
 * @Retorn: Retorna 0 si crea la trama i -1 si no la pot crear.
 *
 ************************************************/
int crearNack(unsigned char *trama, char *motiu) {
    int longitud = 0;

    if (motiu == NULL) {
        return TRAMA_ERROR_GENERAL;
    }
    while (motiu[longitud] != '\0') {
        longitud++;
    }

    return crearTrama(trama, TIPUS_NACK, FLAGS_RESPOSTA_ERROR,(unsigned char *) motiu, longitud);
}

/***********************************************
 *
 * @Finalitat: Envia una trama completa amb una unica escriptura.
 * @Parametres: in: fd = descriptor on s'envia la trama.
 *              in: trama = trama que es vol enviar.
 * @Retorn: Retorna 0 si envia 256 bytes i -1 altrament.
 *
 ************************************************/
int enviarTrama(int fd, unsigned char *trama) {
    int bytes_escrits = 0;

    if (trama == NULL) {
        return TRAMA_ERROR_GENERAL;
    }
    bytes_escrits = write(fd, trama, MIDA_TRAMA);
    if (bytes_escrits != MIDA_TRAMA) {
        return TRAMA_ERROR_GENERAL;
    }
    return TRAMA_CORRECTA;
}

/***********************************************
 *
 * @Finalitat: Rep una trama completa amb una unica lectura.
 * @Parametres: in: fd = descriptor d'on es rep la trama.
 *              out: trama = buffer on es guarda la trama.
 * @Retorn: Retorna 0 si rep 256 bytes i -1 altrament.
 *
 ************************************************/
int rebreTrama(int fd, unsigned char *trama) {
    int bytes_llegits = 0;

    if (trama == NULL) {
        return TRAMA_ERROR_GENERAL;
    }
    bytes_llegits = read(fd, trama, MIDA_TRAMA);
    if (bytes_llegits != MIDA_TRAMA) {
        return TRAMA_ERROR_GENERAL;
    }
    return TRAMA_CORRECTA;
}

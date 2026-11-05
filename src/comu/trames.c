#include "trames.h"

#include <string.h>
#include <unistd.h>



/***********************************************
 *
 * @Proposit: Implementa les operacions basiques de les trames Nostos.
 * @Autor/s: Antonio Duran Sabates i Arnau Gonzalez Hortelano
 * @Data creacio: 15/10/2026
 * @Data ultima modificacio: 19/10/2026
 *
 ************************************************/

/***********************************************
 *
 * @Finalitat: Construeix una trama serialitzada amb padding a zero.
 * @Parametres: out: trama = buffer valid de MIDA_TRAMA bytes.
 *              in: tipus = tipus de la trama.
 *              in: flags = valor del camp FLAGS.
 *              in: dades = buffer valid de longitud bytes si longitud > 0;
 *                          pot ser NULL si longitud es 0.
 *              in: longitud = nombre de bytes utils de dades.
 * @Retorn: Retorna 0 si es crea la trama i -1 si la longitud no es valida.
 *
 ************************************************/
int crearTrama(unsigned char *trama, unsigned char tipus, unsigned char flags, unsigned char *dades, int longitud) {
    if (longitud < 0 || longitud > MIDA_DADES) {
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
 * @Finalitat: Escriu una trama amb un unic write de 256 bytes (Annex II).
 * @Parametres: in: fd = descriptor de sortida.
 *              in: trama = buffer de MIDA_TRAMA bytes.
 * @Retorn: Retorna 0 si escriu MIDA_TRAMA bytes i -1 altrament.
 *
 ************************************************/
int enviarTrama(int fd, unsigned char *trama) {
    return write(fd, trama, MIDA_TRAMA) == MIDA_TRAMA ? 0 : -1;
}

/***********************************************
 *
 * @Finalitat: Rep una trama amb un unic read de 256 bytes (Annex II).
 * @Parametres: in: fd = descriptor d'entrada.
 *              out: trama = buffer de MIDA_TRAMA bytes.
 * @Retorn: Retorna 0 si rep la trama, 1 si arriba EOF abans de llegir dades
 *          i -1 si hi ha un error o la connexio es tanca amb la trama incompleta.
 *
 ************************************************/
static int llegirTrama(int fd, unsigned char *trama) {
    int bytes_llegits = 0, i = 0;

    bytes_llegits = read(fd, trama, MIDA_TRAMA);
    if (bytes_llegits == 0) {
        return 1;
    }
    if (bytes_llegits != MIDA_TRAMA) {
        return -1;
    }
    if (trama[POSICIO_LONGITUD] <= MIDA_DADES) {
        for (i = POSICIO_DADES + trama[POSICIO_LONGITUD]; i < MIDA_TRAMA; i++) {
            if (trama[i] != 0) {
                return -2;
            }
        }
    }
    return 0;
}

/***********************************************
 * @Finalitat: Respon a una trama que no correspon a un format valid.
 * @Parametres: in: fd = socket de l'emissor; motiu = error de format detectat.
 * @Retorn: Retorna 0 si envia el NACK i -1 si falla la creacio o l'enviament.
 ************************************************/
int enviarNack(int fd, char *motiu) {
    unsigned char trama[MIDA_TRAMA];

    if (crearTrama(trama, TIPUS_NACK, FLAG_RESPOSTA_ERROR, (unsigned char *)motiu, strlen(motiu)) != 0) {
        return -1;
    }
    return enviarTrama(fd, trama);
}

/***********************************************
 * @Finalitat: Espera el socket o l'avis de finalitzacio abans de fer E/S.
 * @Parametres: in: fd = socket; fd_finalitzacio = pipe, o -1 per al client.
 *              in: escriure = 1 per esperar escriptura, 0 per lectura.
 * @Retorn: 1 si es pot operar; -1 si cal finalitzar o falla select.
 ************************************************/
static int esperarSocket(int fd, int fd_finalitzacio, int escriure) {
    fd_set lectura, escriptura;
    int maxim = fd;

    if (fd_finalitzacio < 0) {
        return 1;
    }
    if (fd < 0 || fd >= FD_SETSIZE || fd_finalitzacio >= FD_SETSIZE) {
        return -1;
    }
    if (fd_finalitzacio > maxim) {
        maxim = fd_finalitzacio;
    }
    FD_ZERO(&lectura);
    FD_ZERO(&escriptura);
    FD_SET(fd_finalitzacio, &lectura);
    if (escriure) {
        FD_SET(fd, &escriptura);
    } else {
        FD_SET(fd, &lectura);
    }
    if (select(maxim + 1, &lectura, &escriptura, NULL, NULL) < 0 ||
        FD_ISSET(fd_finalitzacio, &lectura)) {
        return -1;
    }
    return 1;
}

/***********************************************
 * @Finalitat: Envia una trama sense ignorar l'avis de finalitzacio del servidor.
 * @Parametres: in: fd, fd_finalitzacio = socket i pipe (-1 sense pipe); trama = dades.
 * @Retorn: 0 si s'envia sencera; -1 amb error o finalitzacio.
 ************************************************/
int enviarTramaAmbFinalitzacio(int fd, int fd_finalitzacio, unsigned char *trama) {
    if (esperarSocket(fd, fd_finalitzacio, 1) < 0) {
        return -1;
    }
    return enviarTrama(fd, trama);
}

/***********************************************
 * @Finalitat: Rep una trama sense quedar pendent d'un client durant SIGINT.
 * @Parametres: in: fd, fd_finalitzacio = socket i pipe (-1 sense pipe); out: trama.
 * @Retorn: El resultat de rebreTrama, o -1 amb finalitzacio o error de select.
 ************************************************/
int rebreTramaAmbFinalitzacio(int fd, int fd_finalitzacio, unsigned char *trama) {
    int resultat = 0;

    if (esperarSocket(fd, fd_finalitzacio, 0) < 0) {
        return -1;
    }
    resultat = llegirTrama(fd, trama);
    if (resultat == -2) {
        enviarNackAmbFinalitzacio(fd, fd_finalitzacio, "INVALID_DATA");
        return -1;
    }
    return resultat;
}

/***********************************************
 * @Finalitat: Rep una trama al client, sense pipe de finalitzacio.
 * @Parametres: in: fd = socket; out: trama = buffer de 256 bytes.
 * @Retorn: 0 amb trama completa, 1 amb EOF i -1 amb error.
 ************************************************/
int rebreTrama(int fd, unsigned char *trama) {
    return rebreTramaAmbFinalitzacio(fd, -1, trama);
}

/***********************************************
 * @Finalitat: Respon NACK sense bloquejar el tancament del servidor.
 * @Parametres: in: fd, fd_finalitzacio = socket i pipe; motiu = error de format.
 * @Retorn: 0 amb NACK enviat; -1 amb error o finalitzacio.
 ************************************************/
int enviarNackAmbFinalitzacio(int fd, int fd_finalitzacio, char *motiu) {
    unsigned char trama[MIDA_TRAMA];

    if (crearTrama(trama, TIPUS_NACK, FLAG_RESPOSTA_ERROR,
                   (unsigned char *)motiu, strlen(motiu)) != 0) {
        return -1;
    }
    return enviarTramaAmbFinalitzacio(fd, fd_finalitzacio, trama);
}

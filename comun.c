/***********************************************
 *
 * @Proposit: Implementa les operacions comunes dels tres processos.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 21/09/2026
 * @Data ultima modificacio: 21/09/2026
 *
 ************************************************/

#include "comun.h"

/***********************************************
 *
 * @Finalitat: Llegeix una linia d'un descriptor de fitxer.
 * @Parametres: in: fd = descriptor del fitxer.
 *              out: buffer = espai on es guarda la linia.
 *              in: mida = capacitat del buffer.
 * @Retorn: Retorna 1 si llegeix una linia, 0 si arriba al final i -1 si falla.
 *
 ************************************************/
int llegirLinia(int fd, char *buffer, int mida) {
    int posicio = 0, bytes_llegits = 0, final_linia = 0;
    char caracter = '\0';

    while (posicio < mida - 1 && final_linia == 0) {
        bytes_llegits = read(fd, &caracter, 1);
        if (bytes_llegits < 0) {
            return -1;
        }

        if (bytes_llegits == 0 || caracter == '\n') {
            final_linia = 1;
        } else if (caracter != '\r') {
            buffer[posicio] = caracter;
            posicio++;
        }
    }

    buffer[posicio] = '\0';
    if (bytes_llegits == 0 && posicio == 0) {
        return 0;
    }

    return 1;
}

/***********************************************
 *
 * @Finalitat: Reserva l'espai exacte i copia una cadena de text.
 * @Parametres: out: desti = cadena creada.
 *              in: origen = text que es vol copiar.
 * @Retorn: Retorna 0 si es pot copiar i -1 si falla la reserva.
 *
 ************************************************/
int copiarText(char **desti, char *origen) {
    int longitud = 0, i = 0;

    while (origen[longitud] != '\0') {
        longitud++;
    }

    *desti = malloc((longitud + 1) * sizeof(char));
    if (*desti == NULL) {
        return -1;
    }

    for (i = 0; i <= longitud; i++) {
        (*desti)[i] = origen[i];
    }

    return 0;
}

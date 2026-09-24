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
 * @Retorn: Retorna la linia reservada dinamicament o NULL si no pot llegir-la.
 *
 ************************************************/
char *llegirLinia(int fd) {
    char *text = NULL, *temporal = NULL;
    char caracter = '\0';
    int longitud = 0, bytes_llegits = 0;

    text = malloc(sizeof(*text));
    if (text == NULL) {
        return NULL;
    }
    text[0] = '\0';

    while (1) {
        bytes_llegits = read(fd, &caracter, 1);
        if (bytes_llegits <= 0) {
            free(text);
            return NULL;
        }
        if (caracter == '\n') {
            break;
        }

        temporal = realloc(text, (longitud + 2) * sizeof(*temporal));
        if (temporal == NULL) {
            free(text);
            return NULL;
        }

        text = temporal;
        text[longitud] = caracter;
        longitud++;
        text[longitud] = '\0';
    }

    return text;
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

    *desti = malloc((longitud + 1) * sizeof(**desti));
    if (*desti == NULL) {
        return -1;
    }

    for (i = 0; i <= longitud; i++) {
        (*desti)[i] = origen[i];
    }

    return 0;
}

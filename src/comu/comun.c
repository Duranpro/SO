/***********************************************
 *
 * @Proposit: Implementa les operacions comunes dels tres processos.
 * @Autor/s: Antonio Duran Sabates i Arnau González Hortelano
 * @Data creacio: 21/09/2026
 * @Data ultima modificacio: 15/10/2026
 *
 ************************************************/

#include "comun.h"

/***********************************************
 *
 * @Finalitat: Comprova si un text representa un enter no negatiu dins del rang d'int.
 * @Parametres: in: text = text que es vol comprovar.
 * @Retorn: Retorna 1 si es un numero i 0 altrament.
 *
 ************************************************/
int esNumero(char *text) {
    int i = 0, valor = 0;

    if (text == NULL || text[0] == '\0') {
        return 0;
    }
    while (text[i] != '\0') {
        if (text[i] < '0' || text[i] > '9') {
            return 0;
        }
        if (valor > (INT_MAX - (text[i] - '0')) / 10) {
            return 0;
        }
        valor = valor * 10 + text[i] - '0';
        i++;
    }
    return 1;
}

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

    text = malloc(sizeof(*text)); // Revisar: inicialitzacio de text amb malloc(1) per reservar espai per al caràcter nul.
    if (text == NULL) {
        return NULL;
    }
    text[0] = '\0';

    while (1) {
        bytes_llegits = read(fd, &caracter, 1);
        if (bytes_llegits <= 0) { // Revisar: Si la linia no acaba amb '\n', alliberem la memoria i retornem NULL per indicar que no s'ha pogut llegir una linia completa.
            free(text);
            return NULL;
        }
        if (caracter == '\n') {
            break;
        }

        temporal = realloc(text, (longitud + 2) * sizeof(*temporal)); // Revisar: Es podria escriure com temporal = realloc(text, longitud + 2); ja que sizeof(*temporal) és 1
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

    while (origen[longitud] != '\0') { // Revisar: strlen(origen) podria ser util per obtenir la longitud de la cadena origen, però el bucle actual també funciona correctament.
        longitud++;
    }

    *desti = malloc((longitud + 1) * sizeof(**desti));
    if (*desti == NULL) {
        return -1;
    }

    for (i = 0; i <= longitud; i++) {
        (*desti)[i] = origen[i]; // Revisar: Es podria utilitzar strcpy(*desti, origen) per copiar la cadena, però el bucle actual també funciona correctament.
    }

    return 0;
}

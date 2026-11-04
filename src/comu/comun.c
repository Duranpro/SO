/***********************************************
 *
 * @Proposit: Implementa les operacions comunes dels tres processos.
 * @Autor/s: Antonio Duran Sabates i Arnau González Hortelano
 * @Data creacio: 21/09/2026
 * @Data ultima modificacio: 04/11/2026
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

    text = malloc(1); 
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

        temporal = realloc(text, (longitud + 2));
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
    int longitud = (int)strlen(origen);
    *desti = malloc((longitud + 1) * sizeof(**desti));
    if (*desti == NULL) {
        return -1;
    }
    strcpy(*desti, origen);

    return 0;
}

/***********************************************
 *
 * @Finalitat: Obte l'MD5 d'un fitxer executant md5sum amb una pipe i un fill.
 * @Parametres: in: ruta_fitxer = ruta del fitxer que es vol comprovar.
 *              out: md5 = buffer del cridador de 33 caracters com a minim.
 * @Retorn: Retorna 0 amb els 32 caracters i '\0', o -1 si hi ha un error.
 *
 ************************************************/
int calcularMd5(char *ruta_fitxer, char *md5) {
    int fd_pipe[2];
    int pid = -1, bytes_llegits = 0, estat = 0;
    char buffer[256];

    if (ruta_fitxer == NULL || md5 == NULL || ruta_fitxer[0] == '\0') {
        return -1;
    }
    md5[0] = '\0';
    if (pipe(fd_pipe) < 0) {
        return -1;
    }
    pid = fork();
    if (pid < 0) {
        close(fd_pipe[0]);
        close(fd_pipe[1]);
        return -1;
    }
    if (pid == 0) {
        close(fd_pipe[0]);
        if (dup2(fd_pipe[1], STDOUT_FILENO) < 0) {
            close(fd_pipe[1]);
            _exit(1);
        }
        if (fd_pipe[1] != STDOUT_FILENO) {
            close(fd_pipe[1]);
        }
        execlp("md5sum", "md5sum", ruta_fitxer, NULL);
        _exit(1);
    }
    close(fd_pipe[1]);
    bytes_llegits = read(fd_pipe[0], buffer, sizeof(buffer));
    close(fd_pipe[0]);
    /* Si un senyal interromp waitpid, encara hem de recollir aquest fill. */
    while (waitpid(pid, &estat, 0) < 0) {
        if (errno != EINTR) {
            return -1;
        }
    }
    if (estat != 0 || bytes_llegits < 32) {
        return -1;
    }
    memcpy(md5, buffer, 32);
    md5[32] = '\0';
    return 0;
}

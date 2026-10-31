/***********************************************
 * @Proposit: Conserva els fils, els desperta amb una pipe i els recull amb join.
 * @Autor/s: Antonio Duran Sabates i Arnau Gonzalez Hortelano
 * @Data creacio: 19/10/2026
 * @Data ultima modificacio: 19/10/2026
 ************************************************/
#include "gestio_navegants.h"
#include "trames.h"

/***********************************************
 * @Finalitat: Inicialitza el registre i la pipe de finalitzacio del servidor.
 * @Parametres: out: registre = registre de fils que es crea.
 * @Retorn: Retorna 0 si es crea i -1 si falla algun recurs.
 ************************************************/
int iniciarNavegants(RegistreNavegants *registre) {
    registre->primers = NULL;
    if (pthread_mutex_init(&registre->mutex, NULL) != 0) {
        return -1;
    }
    if (pipe(registre->finalitzacio) != 0) {
        pthread_mutex_destroy(&registre->mutex);
        return -1;
    }
    return 0;
}

/***********************************************
 * @Finalitat: Indica que el gestor del servidor ha acabat el seu fil.
 * @Parametres: in/out: navegant = entrada estable del fil dins del registre.
 * @Retorn: ----.
 ************************************************/
void finalitzarNavegant(FilNavegant *navegant) {
    pthread_mutex_lock(&navegant->registre->mutex);
    navegant->acabat = 1;
    pthread_mutex_unlock(&navegant->registre->mutex);
}

/***********************************************
 * @Finalitat: Reserva les dades de control abans del pthread_create del servidor.
 * @Parametres: in: registre = propietari de les dades de control.
 * @Retorn: Retorna l'entrada reservada o NULL si falla la memoria.
 ************************************************/
FilNavegant *reservarNavegant(RegistreNavegants *registre) {
    FilNavegant *navegant = malloc(sizeof(*navegant));

    if (navegant == NULL) {
        return NULL;
    }
    navegant->acabat = 0;
    navegant->registre = registre;
    navegant->seguent = NULL;
    return navegant;
}

/***********************************************
 * @Finalitat: Incorpora al registre un fil que el servidor ja ha creat.
 * @Parametres: in/out: navegant = entrada amb el pthread_t valid.
 * @Retorn: ----.
 ************************************************/
void registrarNavegant(FilNavegant *navegant) {
    RegistreNavegants *registre = navegant->registre;

    pthread_mutex_lock(&registre->mutex);
    navegant->seguent = registre->primers;
    registre->primers = navegant;
    pthread_mutex_unlock(&registre->mutex);
}

/***********************************************
 * @Finalitat: Recull els fils acabats o desperta i espera tots els fils en sortir.
 * @Parametres: in/out: registre = fils; in: finalitzar = 1 per tancar el registre.
 * @Retorn: ----.
 ************************************************/
void recollirNavegants(RegistreNavegants *registre, int finalitzar) {
    /* REVISAR CLASSE: el punter al punter que desenllaca nodes es poc directe per al nivell dels Sx.c; un recorregut amb anterior i actual evita aquesta indireccio, mantenint nodes estables. */
    FilNavegant **posicio = NULL, *navegant = NULL;

    pthread_mutex_lock(&registre->mutex);
    if (finalitzar) {
        /* L'EOF de la pipe desperta tots els selects sense llegir-ne dades. */
        close(registre->finalitzacio[1]);
    }
    posicio = &registre->primers;
    while (*posicio != NULL) {
        navegant = *posicio;
        if (finalitzar || navegant->acabat) {
            *posicio = navegant->seguent;
            pthread_mutex_unlock(&registre->mutex);
            pthread_join(navegant->fil, NULL);
            free(navegant);
            pthread_mutex_lock(&registre->mutex);
        } else {
            posicio = &navegant->seguent;
        }
    }
    pthread_mutex_unlock(&registre->mutex);
    if (finalitzar) {
        close(registre->finalitzacio[0]);
        pthread_mutex_destroy(&registre->mutex);
    }
}

/***********************************************
 * @Finalitat: Espera connexions o l'avis de finalitzacio de la pipe.
 * @Parametres: in: fd_servidor = socket; fd_finalitzacio = lectura de la pipe.
 * @Retorn: 1 amb connexio, 0 amb espera, -1 amb error i 2 amb finalitzacio.
 ************************************************/
int esperarNavegant(int fd_servidor, int fd_finalitzacio) {
    fd_set descriptors;
    /* REVISAR CLASSE: aquest timeout d un segon serveix per recollir fils acabats, no per rebre SIGINT (ja hi ha pipe). Valorar si cal aquesta revisio periodica; no es espera activa. */
    struct timeval temps = {1, 0};
    int resultat = 0;
    int maxim = fd_servidor;

    if (fd_servidor < 0 || fd_servidor >= FD_SETSIZE || fd_finalitzacio < 0 || fd_finalitzacio >= FD_SETSIZE) {
        return -1;
    }
    if (fd_finalitzacio > maxim) {
        maxim = fd_finalitzacio;
    }
    FD_ZERO(&descriptors);
    FD_SET(fd_servidor, &descriptors);
    FD_SET(fd_finalitzacio, &descriptors);
    resultat = select(maxim + 1, &descriptors, NULL, NULL, &temps);
    if (resultat > 0 && FD_ISSET(fd_finalitzacio, &descriptors)) {
        /* No consumim l'avis: tambe ha de despertar els fils navegants. */
        return 2;
    }
    if (resultat < 0 && errno == EINTR) {
    return 0;
    }

    return resultat;
}

/***********************************************
 * @Finalitat: Espera dades del navegant o la finalitzacio indicada per la pipe.
 * @Parametres: in: fd_navegant = socket; fd_finalitzacio = lectura de la pipe.
 * @Retorn: Retorna 1 amb dades, 0 amb finalitzacio i -1 si falla select.
 ************************************************/
int esperarPeticioNavegant(int fd_navegant, int fd_finalitzacio) {
    fd_set descriptors;
    int maxim;

    if (fd_navegant > fd_finalitzacio) {
        maxim = fd_navegant;
    } else {
        maxim = fd_finalitzacio;
    }

    if (fd_navegant < 0 || fd_finalitzacio < 0 || maxim >= FD_SETSIZE) {
        return -1;
    }
    FD_ZERO(&descriptors);
    FD_SET(fd_navegant, &descriptors);
    FD_SET(fd_finalitzacio, &descriptors);
    if (select(maxim + 1, &descriptors, NULL, NULL, NULL) < 0) {
        return -1;
    }
    return FD_ISSET(fd_finalitzacio, &descriptors) ? 0 : 1;
}

/***********************************************
 * @Finalitat: Acumula una trama vigilant la finalitzacio entre lectures parcials.
 * @Parametres: in: fd_navegant = socket; fd_finalitzacio = lectura de la pipe.
 *              out: trama = buffer de MIDA_TRAMA bytes.
 * @Retorn: Retorna 0 amb trama completa, 1 amb finalitzacio o EOF inicial
 *          i -1 amb error o EOF a mitja trama.
 ************************************************/
int rebreTramaNavegant(int fd_navegant, int fd_finalitzacio, unsigned char *trama) {
    int total_llegit = 0, bytes_llegits = 0, resultat = 0;

    while (total_llegit < MIDA_TRAMA) {
        resultat = esperarPeticioNavegant(fd_navegant, fd_finalitzacio);
        if (resultat == 0) {
            return 1;
        }
        if (resultat < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        bytes_llegits = read(fd_navegant, trama + total_llegit,MIDA_TRAMA - total_llegit);
        if (bytes_llegits < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (bytes_llegits == 0) {
            if (total_llegit == 0) {
                return 1;
            }
            return -1;
        }
        total_llegit += bytes_llegits;
    }
    return 0;
}

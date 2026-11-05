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

/***********************************************
 * @Finalitat: Obte la mida en bytes d'un fitxer que cap en un int.
 * @Parametres: in: ruta_fitxer = ruta del fitxer que es consulta.
 * @Retorn: Mida del fitxer, o -1 si no es pot obrir o posicionar.
 ************************************************/
int obtenirMidaFitxer(char *ruta_fitxer) {
    int fd_fitxer = -1, mida_fitxer = 0;

    if (ruta_fitxer == NULL || ruta_fitxer[0] == '\0') {
        return -1;
    }
    fd_fitxer = open(ruta_fitxer, O_RDONLY);
    if (fd_fitxer < 0) {
        return -1;
    }
    mida_fitxer = lseek(fd_fitxer, 0, SEEK_END);
    close(fd_fitxer);
    return mida_fitxer;
}

/***********************************************
 * @Finalitat: Envia una resposta textual READY o ACK de la transferencia.
 * @Parametres: in: fd_socket = connexio; tipus, flags = camps de la trama.
 *              in: text = resposta del protocol, sense dades binaries.
 * @Retorn: 0 si s'envia correctament i -1 si falla.
 ************************************************/
static int respondreFitxer(int fd_socket, unsigned char tipus,
                           unsigned char flags, char *text, int fd_finalitzacio) {
    unsigned char trama[MIDA_TRAMA];

    if (crearTrama(trama, tipus, flags, (unsigned char *)text, strlen(text)) != 0) {
        return -1;
    }
    return enviarTramaAmbFinalitzacio(fd_socket, fd_finalitzacio, trama);
}

/***********************************************
 * @Finalitat: Espera el READY o l'ACK abans de continuar la transferencia.
 * @Parametres: in: fd_socket = connexio; tipus = TYPE esperat.
 *              in: text = contingut correcte de la resposta.
 * @Retorn: 0 amb resposta OK valida i -1 amb error o trama incorrecta.
 ************************************************/
static int esperarRespostaFitxer(int fd_socket, unsigned char tipus, char *text, int fd_finalitzacio) {
    unsigned char trama[MIDA_TRAMA];
    char resposta[MIDA_DADES + 1];
    int longitud = 0, rebuig_operacio = 0;

    if (rebreTramaAmbFinalitzacio(fd_socket, fd_finalitzacio, trama) != 0) {
        return -1;
    }
    /* DELIVER i CLAIM poden ser rebutjats mentre esperem READY. */
    if (tipus == TIPUS_READY_FITXER &&
        (trama[POSICIO_TIPUS] == TIPUS_DELIVER || trama[POSICIO_TIPUS] == TIPUS_CLAIM) &&
        trama[POSICIO_FLAGS] == FLAG_RESPOSTA_ERROR) {
        rebuig_operacio = 1;
    }
    if (trama[POSICIO_TIPUS] != tipus && rebuig_operacio == 0) {
        if (trama[POSICIO_TIPUS] != TIPUS_NACK) {
            enviarNackAmbFinalitzacio(fd_socket, fd_finalitzacio, "INVALID_TYPE");
        }
        return -1;
    }
    if (trama[POSICIO_FLAGS] != FLAG_RESPOSTA_OK &&
        trama[POSICIO_FLAGS] != FLAG_RESPOSTA_ERROR) {
        enviarNackAmbFinalitzacio(fd_socket, fd_finalitzacio, "INVALID_FLAGS");
        return -1;
    }
    longitud = trama[POSICIO_LONGITUD];
    if (longitud < 1 || longitud > MIDA_DADES) {
        enviarNackAmbFinalitzacio(fd_socket, fd_finalitzacio, "INVALID_LENGTH");
        return -1;
    }
    memcpy(resposta, trama + POSICIO_DADES, longitud);
    resposta[longitud] = '\0';
    if ((int)strlen(resposta) != longitud) {
        enviarNackAmbFinalitzacio(fd_socket, fd_finalitzacio, "INVALID_DATA");
        return -1;
    }
    if (trama[POSICIO_FLAGS] == FLAG_RESPOSTA_ERROR) {
        return -1;
    }
    if (strcmp(resposta, text) != 0) {
        enviarNackAmbFinalitzacio(fd_socket, fd_finalitzacio, "INVALID_DATA");
        return -1;
    }
    return 0;
}

/***********************************************
 * @Finalitat: Envia un fitxer seguint HEADER, READY, DATA i ACK final.
 * @Parametres: in: fd_socket = connexio que el cridador conserva i tanca.
 *              in: ruta_fitxer = origen; tipus_transferencia = nom del protocol.
 *              in: flags_capcalera, flags_dades = valors definits a l'Annex II.
 * @Retorn: 0 amb ACK OK i -1 si falla. En error, el cridador tanca la connexio.
 ************************************************/
int enviarFitxer(int fd_socket, char *ruta_fitxer, char *tipus_transferencia,
                 unsigned char flags_capcalera, unsigned char flags_dades, int fd_finalitzacio) {
    unsigned char trama[MIDA_TRAMA], dades[MIDA_DADES];
    char capcalera[MIDA_DADES + 1], md5[33], *nom_fitxer = NULL;
    int mida_fitxer = 0, longitud = 0, fd_fitxer = -1;
    int bytes_llegits = 0, bytes_enviats = 0, resultat = 0;

    if (fd_socket < 0 || ruta_fitxer == NULL || tipus_transferencia == NULL) {
        return -1;
    }
    mida_fitxer = obtenirMidaFitxer(ruta_fitxer);
    if (mida_fitxer <= 0 || calcularMd5(ruta_fitxer, md5) != 0) {
        return -1;
    }
    nom_fitxer = strrchr(ruta_fitxer, '/');
    if (nom_fitxer == NULL) {
        nom_fitxer = ruta_fitxer;
    } else {
        nom_fitxer++;
    }
    if (nom_fitxer[0] == '\0' || strchr(nom_fitxer, '&') != NULL) {
        return -1;
    }
    longitud = snprintf(capcalera, sizeof(capcalera), "%s&%s&%d&%s",
                        tipus_transferencia, nom_fitxer, mida_fitxer, md5);
    if (longitud < 1 || longitud > MIDA_DADES ||
        crearTrama(trama, TIPUS_CAPCALERA_FITXER, flags_capcalera,
                   (unsigned char *)capcalera, longitud) != 0 ||
        enviarTramaAmbFinalitzacio(fd_socket, fd_finalitzacio, trama) != 0 ||
        esperarRespostaFitxer(fd_socket, TIPUS_READY_FITXER, "READY", fd_finalitzacio) != 0) {
        return -1;
    }
    fd_fitxer = open(ruta_fitxer, O_RDONLY);
    if (fd_fitxer < 0) {
        return -1;
    }
    while (bytes_enviats < mida_fitxer && resultat == 0) {
        longitud = MIDA_DADES;
        if (mida_fitxer - bytes_enviats < longitud) {
            longitud = mida_fitxer - bytes_enviats;
        }
        bytes_llegits = read(fd_fitxer, dades, longitud);
        if (bytes_llegits != longitud ||
            crearTrama(trama, TIPUS_DADES_FITXER, flags_dades, dades, bytes_llegits) != 0 ||
            enviarTramaAmbFinalitzacio(fd_socket, fd_finalitzacio, trama) != 0) {
            resultat = -1;
        } else {
            bytes_enviats += bytes_llegits;
        }
    }
    close(fd_fitxer);
    if (resultat != 0) {
        return -1;
    }
    return esperarRespostaFitxer(fd_socket, TIPUS_ACK_FITXER, "OK", fd_finalitzacio);
}

/***********************************************
 * @Finalitat: Separa els quatre camps textuals de la capcalera rebuda.
 * @Parametres: in: trama = capcalera 0x20; tipus_esperat = operacio esperada.
 *              out: nom_fitxer = buffer MIDA_DADES + 1; mida_fitxer = bytes.
 *              out: md5_esperat = buffer de 33 caracters.
 * @Retorn: 0 amb camps valids i -1 si el contingut no es correcte.
 ************************************************/
static int llegirCapcaleraFitxer(unsigned char *trama, char *tipus_esperat,
                                char *nom_fitxer, int *mida_fitxer, char *md5_esperat) {
    char dades[MIDA_DADES + 1], *camps[4];
    int longitud = trama[POSICIO_LONGITUD], i = 0, nombre_camps = 1;

    memcpy(dades, trama + POSICIO_DADES, longitud);
    dades[longitud] = '\0';
    if ((int)strlen(dades) != longitud) {
        return -1;
    }
    camps[0] = dades;
    for (i = 0; i < longitud; i++) {
        if (dades[i] == '&') {
            if (nombre_camps == 4) {
                return -1;
            }
            dades[i] = '\0';
            camps[nombre_camps] = dades + i + 1;
            nombre_camps++;
        }
    }
    if (nombre_camps != 4 || strcmp(camps[0], tipus_esperat) != 0 ||
        camps[1][0] == '\0' || strchr(camps[1], '/') != NULL ||
        esNumero(camps[2]) == 0 || atoi(camps[2]) <= 0 || strlen(camps[3]) != 32) {
        return -1;
    }
    for (i = 0; i < 32; i++) {
        if ((camps[3][i] < '0' || camps[3][i] > '9') &&
            (camps[3][i] < 'a' || camps[3][i] > 'f')) {
            return -1;
        }
    }
    strcpy(nom_fitxer, camps[1]);
    *mida_fitxer = atoi(camps[2]);
    strcpy(md5_esperat, camps[3]);
    return 0;
}

/***********************************************
 * @Finalitat: Valida HEADER i prepara el fitxer real abans d'enviar READY.
 * @Parametres: in: fd_socket = connexio; trama = capcalera ja rebuda.
 *              in: carpeta_desti = carpeta existent.
 *              in: tipus_esperat, flags_capcalera = valors del protocol.
 *              out: ruta_desti = buffer MIDA_RUTA_FITXER; nom_fitxer = nom rebut.
 *              out: mida_fitxer = bytes esperats; md5_esperat = buffer de 33.
 * @Retorn: Descriptor del fitxer obert, o -1 si no pot iniciar la recepcio.
 ************************************************/
static int prepararRecepcioFitxer(int fd_socket, unsigned char *trama, char *carpeta_desti,
                                 char *tipus_esperat, unsigned char flags_capcalera,
                                 char *ruta_desti, char *nom_fitxer,
                                 int *mida_fitxer, char *md5_esperat, int fd_finalitzacio) {
    int longitud = 0, fd_fitxer = -1;
    if (trama[POSICIO_TIPUS] != TIPUS_CAPCALERA_FITXER) {
        if (trama[POSICIO_TIPUS] != TIPUS_NACK) {
            enviarNackAmbFinalitzacio(fd_socket, fd_finalitzacio, "INVALID_TYPE");
        }
        return -1;
    }
    if (trama[POSICIO_FLAGS] != flags_capcalera) {
        respondreFitxer(fd_socket, TIPUS_READY_FITXER, FLAG_RESPOSTA_ERROR, "INVALID_FLAGS", fd_finalitzacio);
        return -1;
    }
    longitud = trama[POSICIO_LONGITUD];
    if (longitud < 1 || longitud > MIDA_DADES) {
        respondreFitxer(fd_socket, TIPUS_READY_FITXER, FLAG_RESPOSTA_ERROR, "INVALID_LENGTH", fd_finalitzacio);
        return -1;
    }
    if (llegirCapcaleraFitxer(trama, tipus_esperat, nom_fitxer,
                            mida_fitxer, md5_esperat) != 0) {
        respondreFitxer(fd_socket, TIPUS_READY_FITXER, FLAG_RESPOSTA_ERROR, "INVALID_DATA", fd_finalitzacio);
        return -1;
    }
    longitud = snprintf(ruta_desti, MIDA_RUTA_FITXER, "%s/%s", carpeta_desti, nom_fitxer);
    if (longitud < 1 || longitud >= MIDA_RUTA_FITXER) {
        respondreFitxer(fd_socket, TIPUS_READY_FITXER, FLAG_RESPOSTA_ERROR, "INVALID_PATH", fd_finalitzacio);
        return -1;
    }
    fd_fitxer = open(ruta_desti, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_fitxer < 0) {
        respondreFitxer(fd_socket, TIPUS_READY_FITXER, FLAG_RESPOSTA_ERROR, "FILE_ERROR", fd_finalitzacio);
        return -1;
    }
    if (respondreFitxer(fd_socket, TIPUS_READY_FITXER, FLAG_RESPOSTA_OK, "READY", fd_finalitzacio) != 0) {
        close(fd_fitxer);
        return -1;
    }
    return fd_fitxer;
}

/***********************************************
 * @Finalitat: Escriu cada bloc binari rebut directament al fitxer desti.
 * @Parametres: in: fd_socket = connexio; fd_fitxer = desti obert.
 *              in: mida_fitxer = bytes totals; flags_dades = FLAGS esperat.
 * @Retorn: Bytes escrits; -1 amb connexio o format invalid. Una mida curta
 *          permet al cridador respondre SIZE_ERROR sense inventar cap ACK.
 ************************************************/
static int rebreDadesFitxer(int fd_socket, int fd_fitxer, int mida_fitxer,
                           unsigned char flags_dades, int fd_finalitzacio) {
    unsigned char trama[MIDA_TRAMA];
    int bytes_rebuts = 0, longitud = 0, bytes_esperats = 0;

    while (bytes_rebuts < mida_fitxer) {
        if (rebreTramaAmbFinalitzacio(fd_socket, fd_finalitzacio, trama) != 0) {
            return -1;
        }
        if (trama[POSICIO_TIPUS] != TIPUS_DADES_FITXER) {
            if (trama[POSICIO_TIPUS] != TIPUS_NACK) {
                enviarNackAmbFinalitzacio(fd_socket, fd_finalitzacio, "INVALID_TYPE");
            }
            return -1;
        }
        if (trama[POSICIO_FLAGS] != flags_dades) {
            enviarNackAmbFinalitzacio(fd_socket, fd_finalitzacio, "INVALID_FLAGS");
            return -1;
        }
        longitud = trama[POSICIO_LONGITUD];
        if (longitud < 1 || longitud > MIDA_DADES) {
            enviarNackAmbFinalitzacio(fd_socket, fd_finalitzacio, "INVALID_LENGTH");
            return -1;
        }
        bytes_esperats = MIDA_DADES;
        if (mida_fitxer - bytes_rebuts < bytes_esperats) {
            bytes_esperats = mida_fitxer - bytes_rebuts;
        }
        if (longitud != bytes_esperats ||
            write(fd_fitxer, trama + POSICIO_DADES, longitud) != longitud) {
            return bytes_rebuts;
        }
        bytes_rebuts += longitud;
    }
    return bytes_rebuts;
}

/***********************************************
 * @Finalitat: Continua des d una capcalera rebuda i verifica mida i MD5.
 * @Parametres: in: fd_socket = connexio del cridador; carpeta_desti = desti existent.
 *              in: trama_capcalera = HEADER rebut, sense tornar-lo a llegir.
 *              in: tipus_esperat, flags_capcalera, flags_dades = valors de l'Annex II.
 *              out: nom_fitxer_rebut = buffer del cridador de MIDA_DADES + 1 bytes.
 * @Retorn: 0 amb recepcio correcta i -1 amb error. El cridador tanca la connexio
 *          en error; el fitxer rebut, encara que sigui incomplet, es el desti real.
 ************************************************/
int rebreFitxerDesDeCapcalera(int fd_socket, unsigned char *trama_capcalera,
                              char *carpeta_desti, char *tipus_esperat,
                              unsigned char flags_capcalera, unsigned char flags_dades,
                              char *nom_fitxer_rebut, int fd_finalitzacio) {
    char ruta_desti[MIDA_RUTA_FITXER], nom_fitxer[MIDA_DADES + 1];
    char md5_esperat[33], md5_rebut[33];
    int fd_fitxer = -1, mida_fitxer = 0, bytes_rebuts = 0;

    if (fd_socket < 0 || trama_capcalera == NULL || carpeta_desti == NULL || carpeta_desti[0] == '\0' ||
        tipus_esperat == NULL || nom_fitxer_rebut == NULL) {
        return -1;
    }
    nom_fitxer_rebut[0] = '\0';
    fd_fitxer = prepararRecepcioFitxer(fd_socket, trama_capcalera, carpeta_desti, tipus_esperat,
                                      flags_capcalera, ruta_desti, nom_fitxer,
                                      &mida_fitxer, md5_esperat, fd_finalitzacio);
    if (fd_fitxer < 0) {
        return -1;
    }
    bytes_rebuts = rebreDadesFitxer(fd_socket, fd_fitxer, mida_fitxer, flags_dades, fd_finalitzacio);
    if (close(fd_fitxer) < 0 && bytes_rebuts >= 0) {
        bytes_rebuts = 0;
    }
    if (bytes_rebuts < 0) {
        return -1;
    }
    if (bytes_rebuts != mida_fitxer) {
        respondreFitxer(fd_socket, TIPUS_ACK_FITXER, FLAG_RESPOSTA_ERROR, "SIZE_ERROR", fd_finalitzacio);
        return -1;
    }
    if (calcularMd5(ruta_desti, md5_rebut) != 0 || strcmp(md5_rebut, md5_esperat) != 0) {
        respondreFitxer(fd_socket, TIPUS_ACK_FITXER, FLAG_RESPOSTA_ERROR, "MD5_ERROR", fd_finalitzacio);
        return -1;
    }
    if (respondreFitxer(fd_socket, TIPUS_ACK_FITXER, FLAG_RESPOSTA_OK, "OK", fd_finalitzacio) != 0) {
        return -1;
    }
    strcpy(nom_fitxer_rebut, nom_fitxer);
    return 0;
}

/***********************************************
 * @Finalitat: Rep la capcalera i delega la recepcio completa del fitxer.
 * @Parametres: in: fd_socket, carpeta_desti, tipus_esperat = connexio i desti.
 *              in: flags_capcalera, flags_dades = FLAGS del protocol.
 *              out: nom_fitxer_rebut = buffer de MIDA_DADES + 1 bytes.
 * @Retorn: 0 amb mida, MD5 i ACK correctes; -1 amb error.
 ************************************************/
int rebreFitxer(int fd_socket, char *carpeta_desti, char *tipus_esperat,
                unsigned char flags_capcalera, unsigned char flags_dades,
                char *nom_fitxer_rebut, int fd_finalitzacio) {
    unsigned char trama[MIDA_TRAMA];

    if (fd_socket < 0 || carpeta_desti == NULL || carpeta_desti[0] == '\0' ||
        tipus_esperat == NULL || nom_fitxer_rebut == NULL) {
        return -1;
    }
    nom_fitxer_rebut[0] = '\0';
    if (rebreTramaAmbFinalitzacio(fd_socket, fd_finalitzacio, trama) != 0) {
        return -1;
    }
    return rebreFitxerDesDeCapcalera(fd_socket, trama, carpeta_desti, tipus_esperat,
                                     flags_capcalera, flags_dades, nom_fitxer_rebut, fd_finalitzacio);
}

#define _GNU_SOURCE

/***********************************************
 *
 * @Proposit: Implementa el terminal i el parser de comandes d'Odysseus.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 23/09/2026
 * @Data ultima modificacio: 23/09/2026
 *
 ************************************************/

#include "comandes.h"

/***********************************************
 *
 * @Finalitat: Escriu un missatge per la sortida estandard.
 * @Parametres: in: text = text que es vol escriure.
 * @Retorn: Retorna 0 si pot escriure i -1 si falla la reserva.
 *
 ************************************************/
int escriureMissatge(char *text) {
    int caracters_escrits = 0;
    char *missatge = NULL;

    caracters_escrits = asprintf(&missatge, "%s", text);
    if (caracters_escrits < 0) {
        return -1;
    }
    write(1, missatge, caracters_escrits);
    free(missatge);
    return 0;
}

/***********************************************
 *
 * @Finalitat: Llegeix una comanda completa des de l'entrada estandard.
 * @Parametres: out: buffer = espai on es guarda la comanda.
 *              in: mida = capacitat del buffer.
 * @Retorn: Retorna 1 si llegeix una comanda, 0 si arriba a EOF i -1 si falla.
 *
 ************************************************/
int llegirComanda(char *buffer, int mida) {
    int posicio = 0, bytes_llegits = 0, final_linia = 0;
    char caracter = '\0';

    while (final_linia == 0) {
        bytes_llegits = read(0, &caracter, 1);
        if (bytes_llegits < 0) {
            return -1;
        }
        if (bytes_llegits == 0 || caracter == '\n') {
            final_linia = 1;
        } else if (caracter != '\r' && posicio < mida - 1) {
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
 * @Finalitat: Comprova si un text conte nomes digits.
 * @Parametres: in: text = text que es vol comprovar.
 * @Retorn: Retorna 1 si es un numero i 0 altrament.
 *
 ************************************************/
int esNumero(char *text) {
    int i = 0;

    if (text == NULL || text[0] == '\0') {
        return 0;
    }
    while (text[i] != '\0') {
        if (text[i] < '0' || text[i] > '9') {
            return 0;
        }
        i++;
    }
    return 1;
}

/***********************************************
 *
 * @Finalitat: Separa una linia en paraules utilitzant espais i tabuladors.
 * @Parametres: in/out: linia = linia que es vol separar.
 *              out: paraules = punters a les paraules trobades.
 *              in: maxim = nombre maxim de paraules que es guarden.
 * @Retorn: Retorna el nombre de paraules guardades.
 *
 ************************************************/
int separarParaules(char *linia, char *paraules[], int maxim) {
    int nombre_paraules = 0;
    char *paraula = NULL;

    paraula = strtok(linia, " ");
    while (paraula != NULL && nombre_paraules < maxim) {
        paraules[nombre_paraules] = paraula;
        nombre_paraules++;
        paraula = strtok(NULL, " ");
    }
    return nombre_paraules;
}

/***********************************************
 *
 * @Finalitat: Analitza la comanda CONNECT.
 * @Parametres: in: paraules = paraules de la comanda.
 *              in: nombre_paraules = nombre de paraules llegides.
 *              out: comanda = comanda reconeguda.
 * @Retorn: Retorna el resultat de l'analisi.
 *
 ************************************************/
int analitzarConnexio(char *paraules[], int nombre_paraules, Comanda *comanda) {
    comanda->tipus = COMANDA_CONNECT;
    if (nombre_paraules == 2 &&
        strcasecmp(paraules[1], "ITHACA") == 0) {
        return RESULTAT_CORRECTE;
    }
    return RESULTAT_SINTAXI_INCORRECTA;
}

/***********************************************
 *
 * @Finalitat: Analitza les comandes LIST VOYAGES i LIST MARKET.
 * @Parametres: in: paraules = paraules de la comanda.
 *              in: nombre_paraules = nombre de paraules llegides.
 *              out: comanda = comanda reconeguda.
 * @Retorn: Retorna el resultat de l'analisi.
 *
 ************************************************/
int analitzarLlista(char *paraules[], int nombre_paraules, Comanda *comanda) {
    if (nombre_paraules < 2) {
        return RESULTAT_DESCONEGUDA;
    }
    if (strcasecmp(paraules[1], "VOYAGES") == 0) {
        comanda->tipus = COMANDA_LIST_VOYAGES;
    } else if (strcasecmp(paraules[1], "MARKET") == 0) {
        comanda->tipus = COMANDA_LIST_MARKET;
    } else {
        return RESULTAT_DESCONEGUDA;
    }

    if (nombre_paraules != 2) {
        return RESULTAT_SINTAXI_INCORRECTA;
    }
    return RESULTAT_CORRECTE;
}

/***********************************************
 *
 * @Finalitat: Analitza la comanda ACCEPT i extreu l'identificador.
 * @Parametres: in: paraules = paraules de la comanda.
 *              in: nombre_paraules = nombre de paraules llegides.
 *              out: comanda = comanda i identificador extret.
 * @Retorn: Retorna el resultat de l'analisi.
 *
 ************************************************/
int analitzarAcceptacio(char *paraules[], int nombre_paraules, Comanda *comanda) {
    int identificador = 0;

    comanda->tipus = COMANDA_ACCEPT;
    if (nombre_paraules == 2 && esNumero(paraules[1]) == 1) {
        identificador = atoi(paraules[1]);
        if (identificador > 0) {
            comanda->valor = identificador;
            return RESULTAT_CORRECTE;
        }
    }
    return RESULTAT_SINTAXI_INCORRECTA;
}

/***********************************************
 *
 * @Finalitat: Analitza la comanda SAIL i extreu el nom de l'illa.
 * @Parametres: in: paraules = paraules de la comanda.
 *              in: nombre_paraules = nombre de paraules llegides.
 *              out: comanda = comanda i illa extreta.
 * @Retorn: Retorna el resultat de l'analisi.
 *
 ************************************************/
int analitzarNavegacio(char *paraules[], int nombre_paraules, Comanda *comanda) {
    comanda->tipus = COMANDA_SAIL;
    if (nombre_paraules == 2) {
        strcpy(comanda->argument, paraules[1]);
        return RESULTAT_CORRECTE;
    }
    return RESULTAT_SINTAXI_INCORRECTA;
}

/***********************************************
 *
 * @Finalitat: Analitza una comanda BUY o SELL.
 * @Parametres: in: paraules = paraules de la comanda.
 *              in: nombre_paraules = nombre de paraules llegides.
 *              in: tipus = tipus de compra o venda.
 *              out: comanda = comanda i arguments extrets.
 * @Retorn: Retorna el resultat de l'analisi.
 *
 ************************************************/
int analitzarCompraVenda(char *paraules[], int nombre_paraules, int tipus, Comanda *comanda) {
    int quantitat = 0;

    comanda->tipus = tipus;
    if (nombre_paraules == 3 && esNumero(paraules[2]) == 1) {
        quantitat = atoi(paraules[2]);
        if (quantitat > 0) {
            strcpy(comanda->argument, paraules[1]);
            comanda->valor = quantitat;
            return RESULTAT_CORRECTE;
        }
    }
    return RESULTAT_SINTAXI_INCORRECTA;
}

/***********************************************
 *
 * @Finalitat: Analitza una comanda que no admet arguments.
 * @Parametres: in: nombre_paraules = nombre de paraules llegides.
 *              in: tipus = tipus de comanda reconeguda.
 *              out: comanda = comanda reconeguda.
 * @Retorn: Retorna el resultat de l'analisi.
 *
 ************************************************/
int analitzarSenseArguments(int nombre_paraules, int tipus, Comanda *comanda) {
    comanda->tipus = tipus;
    if (nombre_paraules == 1) {
        return RESULTAT_CORRECTE;
    }
    return RESULTAT_SINTAXI_INCORRECTA;
}

/***********************************************
 *
 * @Finalitat: Separa i despatxa una comanda al seu analitzador.
 * @Parametres: in/out: linia = linia que conte la comanda.
 *              out: comanda = tipus i arguments extrets.
 * @Retorn: Retorna si la comanda es correcta, incorrecta o desconeguda.
 *
 ************************************************/
int analitzarComanda(char *linia, Comanda *comanda) {
    char *paraules[MAX_PARAULES] = {NULL};
    int nombre_paraules = 0;

    comanda->tipus = COMANDA_DESCONEGUDA;
    comanda->argument[0] = '\0';
    comanda->valor = 0;
    nombre_paraules = separarParaules(linia, paraules, MAX_PARAULES);

    if (nombre_paraules == 0) {
        return RESULTAT_DESCONEGUDA;
    }
    if (strcasecmp(paraules[0], "CONNECT") == 0) {
        return analitzarConnexio(paraules, nombre_paraules, comanda);
    }
    if (strcasecmp(paraules[0], "LIST") == 0) {
        return analitzarLlista(paraules, nombre_paraules, comanda);
    }
    if (strcasecmp(paraules[0], "ACCEPT") == 0) {
        return analitzarAcceptacio(paraules, nombre_paraules, comanda);
    }
    if (strcasecmp(paraules[0], "SAIL") == 0) {
        return analitzarNavegacio(paraules, nombre_paraules, comanda);
    }
    if (strcasecmp(paraules[0], "BUY") == 0) {
        return analitzarCompraVenda(paraules, nombre_paraules, COMANDA_BUY, comanda);
    }
    if (strcasecmp(paraules[0], "SELL") == 0) {
        return analitzarCompraVenda(paraules, nombre_paraules, COMANDA_SELL, comanda);
    }
    if (strcasecmp(paraules[0], "MAP") == 0) {
        return analitzarSenseArguments(nombre_paraules, COMANDA_MAP, comanda);
    }
    if (strcasecmp(paraules[0], "STATUS") == 0) {
        return analitzarSenseArguments(nombre_paraules, COMANDA_STATUS,comanda);
    }
    if (strcasecmp(paraules[0], "DELIVER") == 0) {
        return analitzarSenseArguments(nombre_paraules, COMANDA_DELIVER, comanda);
    }
    if (strcasecmp(paraules[0], "CLAIM") == 0) {
        return analitzarSenseArguments(nombre_paraules, COMANDA_CLAIM, comanda);
    }
    return RESULTAT_DESCONEGUDA;
}

/***********************************************
 *
 * @Finalitat: Mostra la sintaxi esperada per una comanda coneguda.
 * @Parametres: in: tipus = tipus de comanda amb sintaxi incorrecta.
 * @Retorn: Retorna 0 si pot escriure i -1 si es produeix un error.
 *
 ************************************************/
int mostrarUsComanda(int tipus) {
    char *us = NULL;

    switch (tipus) {
    case COMANDA_CONNECT:
        us = "Usage: CONNECT ITHACA\n";
        break;
    case COMANDA_LIST_VOYAGES:
        us = "Usage: LIST VOYAGES\n";
        break;
    case COMANDA_ACCEPT:
        us = "Usage: ACCEPT <voyage_id>\n";
        break;
    case COMANDA_SAIL:
        us = "Usage: SAIL <island>\n";
        break;
    case COMANDA_MAP:
        us = "Usage: MAP\n";
        break;
    case COMANDA_LIST_MARKET:
        us = "Usage: LIST MARKET\n";
        break;
    case COMANDA_BUY:
        us = "Usage: BUY <product> <amount>\n";
        break;
    case COMANDA_SELL:
        us = "Usage: SELL <product> <amount>\n";
        break;
    case COMANDA_STATUS:
        us = "Usage: STATUS\n";
        break;
    case COMANDA_DELIVER:
        us = "Usage: DELIVER\n";
        break;
    case COMANDA_CLAIM:
        us = "Usage: CLAIM\n";
        break;
    }

    if (us == NULL) {
        return -1;
    }
    return escriureMissatge(us);
}

/***********************************************
 *
 * @Finalitat: Mostra el resultat obtingut en analitzar una comanda.
 * @Parametres: in: resultat = resultat de l'analisi.
 *              in: tipus = tipus de comanda reconeguda.
 * @Retorn: Retorna 0 si pot escriure i -1 si es produeix un error.
 *
 ************************************************/
int mostrarResultatComanda(int resultat, int tipus) {
    if (resultat == RESULTAT_CORRECTE) {
        return escriureMissatge("Command OK\n");
    }
    if (resultat == RESULTAT_SINTAXI_INCORRECTA) {
        return mostrarUsComanda(tipus);
    }
    return escriureMissatge("Unknown command\n");
}

/***********************************************
 *
 * @Finalitat: Executa el terminal interactiu fins que arriba EOF.
 * @Parametres: in: finalitzar_programa = indica si s'ha rebut SIGINT.
 * @Retorn: Retorna 0 en arribar a EOF i -1 si es produeix un error.
 *
 ************************************************/
int executarTerminal(int *finalitzar_programa) {
    Comanda comanda = {0};
    int resultat_lectura = 0, resultat_analisi = 0;
    int finalitzat = 0;
    char linia[MIDA_LINIA];

    while (finalitzat == 0 && *finalitzar_programa == 0) {
        if (escriureMissatge("$ ") != 0) {
            return -1;
        }
        resultat_lectura = llegirComanda(linia, MIDA_LINIA);

        if (*finalitzar_programa != 0 || resultat_lectura == 0) {
            finalitzat = 1;
        } else if (resultat_lectura < 0) {
            return -1;
        } else {
            resultat_analisi = analitzarComanda(linia, &comanda);
            if (mostrarResultatComanda(resultat_analisi, comanda.tipus) != 0) {
                return -1;
            }
        }
    }
    return 0;
}

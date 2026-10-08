/***********************************************
 *
 * @Proposit: Defineix els tipus de dades basics de The Nostos System.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 18/09/2026
 * @Data ultima modificacio: 07/10/2026
 *
 ************************************************/

#ifndef TIPUS_H
#define TIPUS_H

#define MIDA_NOM_PRODUCTE 100
#define MAX_ILLES_CONEGUDES 6
#define NOM_ILLA_INICIAL "Aeaea"

typedef struct {
    char *nom;
    int quantitat;
} Aliment;

typedef struct {
    char *nom_illa;
    char *ip;
    int port;
} Ruta;

typedef struct {
    char nom[MIDA_NOM_PRODUCTE];
    int quantitat;
    int preu;
} Producte;

typedef struct {
    char *nom;
    char *ip;
    int port;
    int nombre_connexions;
    char **connexions;
} IllaConeguda;

typedef struct {
    int identificador;
    char *nom_objecte;
    char *ruta_fitxer;
    char *illa_desti;
    char *odisseu_assignat;
    int recompensa;
    int disponible;
    int fracassat;
} Viatge;

typedef struct {
    char *ruta_carpeta;
    char *nom;
    char *ip_itaca;
    int port_itaca;
    char *ip_illa_inicial;
    int port_illa_inicial;
    int diners;
    int nombre_aliments;
    Aliment *aliments;
    char *objecte_viatge;
    char *illa_desti_viatge;
    int identificador_viatge;
    int recompensa_viatge;
    char *ubicacio_actual;
    int tipus_connexio;
    int nombre_productes_mercat;
    Producte *productes_mercat;
    int nombre_illes_conegudes;
    IllaConeguda *illes_conegudes;
    int desti_assolit;
} ConfiguracioOdisseu;

typedef struct {
    char *nom;
    char *ruta_carpeta;
    char *ip;
    int port;
    int nombre_viatges;
    Viatge *viatges;
} ConfiguracioItaca;

typedef struct {
    char *nom;
    char *ruta_carpeta;
    char *ip;
    int port;
    int capacitat_port;
    int nombre_rutes;
    Ruta *rutes;
    int nombre_productes;
    Producte *productes;
} ConfiguracioIlla;

#endif

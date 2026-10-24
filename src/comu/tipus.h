/***********************************************
 *
 * @Proposit: Defineix els tipus de dades basics de The Nostos System.
 * @Autor/s: Antonio Duran Sabates i Arnau González Hortelano
 * @Data creacio: 18/09/2026
 * @Data ultima modificacio: 19/10/2026
 *
 ************************************************/

#ifndef TIPUS_H
#define TIPUS_H

#define MIDA_NOM_PRODUCTE 100
#define MIDA_NOM_ODYSSEUS 254
#define MAX_ILLES 6
#define MIDA_NOM_ILLA 32
#define MIDA_IP 16

typedef struct {
    char nom[MIDA_NOM_ILLA];
    char ip[MIDA_IP];
    int port;
    char connexions[MAX_ILLES][MIDA_NOM_ILLA];
    int num_connexions;
} IllaConeguda;

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
    int identificador;
    char *nom_objecte;
    char *ruta_fitxer;
    char *illa_desti;
    int recompensa;
    int disponible;
    int fracassat;
    char odysseus_assignat[MIDA_NOM_ODYSSEUS];
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
    Producte *mercat_actual;
    int num_productes_mercat;
    IllaConeguda illes_conegudes[MAX_ILLES];
    int num_illes_conegudes;
    char ubicacio_actual[MIDA_NOM_ILLA];
    int *viatges_consultats;
    int nombre_viatges_consultats;
    int te_viatge;
    Viatge viatge_actiu;
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

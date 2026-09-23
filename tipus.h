/***********************************************
 *
 * @Proposit: Defineix els tipus de dades basics de The Nostos System.
 * @Autor/s: Antonio Duran Sabates
 * @Data creacio: 18/09/2026
 * @Data ultima modificacio: 21/09/2026
 *
 ************************************************/

#ifndef TIPUS_H
#define TIPUS_H

#define MIDA_NOM_PRODUCTE 100

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

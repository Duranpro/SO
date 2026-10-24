# The Nostos System

## Estructura

```text
src/
  ithaca/       Servidor Ithaca i la seva configuracio
  island/       Servidor Island i la seva configuracio
  odysseus/     Client, comandes i configuracio
  comu/         Tipus, utilitats, trames, xarxa i gestio dels fils
lib/            SPHRAGIS: capcalera i objecte proporcionats
config/
  ithaca.dat
  illes/        Configuracions de les sis illes
  odysseus/     Configuracions dels navegants i del cas de mort
data/
  voyages.dat
  stocks/       Stocks binaris de les illes
build/          Objectes i dependencies generats per make
entregues/      Entrega original i copia corregida de la Fase 1
teoria/         Apunts i exemples de l'assignatura
```

Les capcaleres `.h` estan al costat del modul `.c` corresponent. El Makefile
configura els directoris d'inclusio i genera les dependencies automaticament.
Els executables es generen a l'arrel del projecte.

## Compilar

Des de l'arrel del projecte, en Linux:

```bash
make
```

Per eliminar els executables i els objectes generats:

```bash
make clean
```

`make clean` conserva `lib/sphragis.o`, que es proporciona ja compilat.

## Executar

Executa sempre les comandes des de l'arrel del projecte. En terminals separats:

```bash
./ithaca config/ithaca.dat data/voyages.dat
./island config/illes/aeaea.dat data/stocks/Aeaea.db
./odysseus config/odysseus/odysseus.dat
```

Un segon navegant:

```bash
./odysseus config/odysseus/odysseus_telemachus.dat
```

Per executar una altra illa, utilitza la seva configuracio i el seu stock, per exemple:

```bash
./island config/illes/scheria.dat data/stocks/Scheria.db
```

Les IP, els ports i el contingut dels `.dat` es conserven. Cada servidor s'ha
d'executar a la maquina que te la IP indicada al seu fitxer de configuracio.
Les rutes de carpetes dins dels `.dat`, com `./aeaea_files`, es resolen des del
directori on s'executa el programa, no des de `config/`.

## Valgrind

```bash
make valgrind-odysseus
make valgrind-ithaca
make valgrind-island
```

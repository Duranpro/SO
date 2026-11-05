# The Nostos System

Compilar amb `make` a cada màquina, des de l’arrel del projecte.
Executar també des d’aquesta carpeta, amb un terminal per procés.

Matagalls (172.16.214.20):

```bash
./ithaca config/ithaca.dat data/voyages.dat
./island config/illes/aeolia.dat data/stocks/Aeolia.db
./island config/illes/scheria.dat data/stocks/Scheria.db
./island config/illes/ogygia.dat data/stocks/Ogygia.db
```

Montserrat (172.16.214.21):

```bash
./island config/illes/aeaea.dat data/stocks/Aeaea.db
./island config/illes/ismarus.dat data/stocks/Ismarus.db
./island config/illes/thrinacia.dat data/stocks/Thrinacia.db
```

Odysseus, des de Matagalls, Montserrat o Puigpedrós:

```bash
./odysseus config/odysseus/odysseus.dat
```

Per a un altre navegant, utilitzar un dels altres fitxers de `config/odysseus/`.

Aquesta distribució correspon a les IP dels fitxers actuals. Si es canvia una
illa o Ithaca de màquina, cal actualitzar les IP corresponents als fitxers de
configuració i mantenir-los iguals a totes les màquines.

Finalitzar amb CTRL+C. Netejar els executables i objectes amb `make clean`.


Abans de la Fase 3, crear les carpetes configurades i copiar les imatges oficials
proporcionades a eStudy dins d'`objects/`, amb els noms de `data/voyages.dat`:

```bash
mkdir -p objects aeaea_files aeolia_files ismarus_files ogygia_files scheria_files thrinacia_files
mkdir -p polyphemus_files telemachus_files laertes_files eumaeus_files mort_files
```

Odysseus desa `state.dat` dins de la seva carpeta. Conserva diners, aliments,
missio, objecte associat, entrega, ubicacio i mapa conegut. Tambe pot llegir el
format anterior de diners i aliments. No desa connexions ni el mercat temporal.

Despres de reiniciar, `STATUS` mostra l'estat recuperat, pero cal reconnectar:
`CONNECT ITHACA` per tornar a Ithaca, o `SAIL Scheria` si la ubicacio guardada
es Scheria i es vol tornar a atracar alla. Aquesta arribada torna a respectar
la capacitat i la FIFO. Per continuar des d'una altra illa guardada, fer primer
`SAIL <nom_de_la_mateixa_illa>` i despres navegar per les connexions conegudes.

El reinici d'Odysseus no restaura assignacions si tambe s'ha reiniciat Ithaca.
Les transferencies interrompudes fallen; no hi ha represa automatica de blocs.

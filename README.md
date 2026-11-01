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

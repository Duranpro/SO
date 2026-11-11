# Testing de fase 3: problemes trobats

Proves del 9 de novembre de 2026, en Docker amb Debian 12 i les imatges oficials.

**El recorregut normal funciona, però hi ha tres problemes funcionals pendents.** També s'ha trobat un problema addicional provocant dues transferències amb el mateix nom de fitxer.

Es van executar 22 escenaris: **17 van passar i 5 van fallar**. Dues de les fallades corresponen al mateix problema de reinici. Els scripts i logs es van eliminar després a petició de l'usuari; aquest document conserva els resultats. Les línies de codi s'han tornat a comprovar en preparar aquesta versió del report. No s'han tornat a executar les proves ni s'ha modificat el C.

## P1. Ithaca paga per un objecte equivocat

### Què va passar?

Vam acceptar la missió del Trident, però vam enviar la imatge d'AegisFragment amb el glif de Scheria. Li vam posar el nom `trident_of_poseidon.png`.

**Ithaca la va acceptar i va pagar 1.500 gold, encara que era un altre objecte.** Hauria d'haver rebutjat la reclamació.

### On és el problema al codi?

A `gestionarClaim`, [ithaca.c, línia 332](C:/Users/Gonho/0-Salle/SO/odysseus/src/ithaca/ithaca.c:332), es comprova el nom:

```c
valida = strcmp(esperat, nom_fitxer) == 0;
```

Després, a la [línia 337](C:/Users/Gonho/0-Salle/SO/odysseus/src/ithaca/ithaca.c:337), es comprova el glif:

```c
valida = longitud > 0 && longitud < MIDA_RUTA_FITXER && SPHRAGIS_has_glyph(desti, ruta_rebuda) == 1;
```

Aquestes dues comprovacions són útils, però **falta comprovar que el contingut correspon a l'objecte de la missió**. Canviar el nom d'una imatge no canvia l'objecte que representa.

Si aquestes comprovacions i les de l'assignació passen, la [línia 345](C:/Users/Gonho/0-Salle/SO/odysseus/src/ithaca/ithaca.c:345) dona la missió per completada:

```c
viatge->completat = 1;
```

L'MD5 de la transferència no resol això: comprova que el fitxer rebut coincideix amb el que ha anunciat qui l'envia. L'objecte equivocat també pot arribar sencer i sense errors.

### Què caldrà corregir?

Afegir una comprovació del contingut respecte a l'objecte esperat amb el segell correcte. Cal tenir present que segellar la imatge en modifica els bytes: no serveix comparar directament l'MD5 amb el de l'original sense segellar.

**Estat: pendent. És un requisit de la pràctica.**

## P2. Després de morir es recupera la partida perduda

### Què va passar?

Vam provar dos casos:

- Odysseus es queda esperant plaça amb 1 kg de menjar. Consumeix la ració, arriba a zero i mor.
- Odysseus es queda sense diners ni menjar per continuar i l'usuari finalitza amb CTRL+C.

**En reiniciar, es mantenen la missió i l'objecte. En el primer cas, també es recupera el menjar a zero.**

Hauria de començar amb els recursos inicials i sense l'objecte ni la missió anterior. Aquest reinici especial és el que demana l'enunciat quan es perd la partida.

### On és el problema al codi?

A `esperarPort`, [comandes.c, línies 1454–1457](C:/Users/Gonho/0-Salle/SO/odysseus/src/odysseus/comandes.c:1454), quan s'acaba el menjar:

```c
if (provisions == 0) {
    close(*fd_illa);
    *fd_illa = -1;
    *finalitzar_programa = 1;
```

Això tanca la connexió i indica que el programa ha d'acabar. **No marca la partida com a perduda ni elimina l'objecte.**

Després, el `main` desa l'estat igualment a [odysseus.c, línia 88](C:/Users/Gonho/0-Salle/SO/odysseus/src/odysseus/odysseus.c:88):

```c
guardarEstatOdysseus(&configuracio);
```

En tornar a executar-lo, la [línia 70](C:/Users/Gonho/0-Salle/SO/odysseus/src/odysseus/odysseus.c:70) recupera aquest estat:

```c
carregarEstatOdysseus(&configuracio);
```

**Per tant, es desa i es torna a carregar la partida en què acabàvem de morir.** En sortir amb CTRL+C sense recursos, també s'arriba al desament sense distingir aquesta situació.

### Què caldrà corregir?

Distingir una sortida normal d'una partida perduda. Si s'ha perdut, cal perdre l'objecte i preparar el següent inici amb els recursos inicials.

No s'ha d'esborrar l'estat en tots els CTRL+C: quan només tanquem el programa per continuar més tard, la persistència sí que s'ha de conservar.

**Estat: pendent. És un requisit de la pràctica i explica dues fallades del testing.**

## P3. Es pot navegar sense haver rebut tot l'objecte
> **Actualització del 10/11/2026: P3 corregit.** En fallar OBJECT es descarta i es desa la missió local com a inactiva; reconnectar i fer LIST VOYAGES permet demanar-ne una altra. SAIL també rebutja una missió activa sense objecte rebut. S'han provat mida incorrecta, MD5 incorrecte, interrupció, reinici, acceptació posterior d'una altra missió, navegació sense missió, bloqueig d'un estat incomplet i recorregut complet: tot passa en Debian 12. La descripció següent explica la fallada original del dia 9.

### Què va passar?

Vam tallar la transferència OBJECT després del primer bloc. Odysseus va detectar l'error i va informar que no podia rebre l'objecte.

Però, després de `CONNECT ITHACA` i `SAIL Aeaea`, **va sortir i va arribar a Aeaea amb la missió activa, sense haver rebut l'objecte complet**.

Hauria d'impedir començar el viatge d'aquella missió fins que l'objecte s'hagi rebut correctament.

### On és el problema al codi?

A `executarAccept`, [comandes.c, línies 1280–1284](C:/Users/Gonho/0-Salle/SO/odysseus/src/odysseus/comandes.c:1280), la missió s'activa abans de rebre OBJECT:

```c
configuracio->viatge_actiu = viatge;
configuracio->te_viatge = 1;
configuracio->entrega_completada = 0;
configuracio->fitxer_objecte[0] = '\0';
guardarEstatOdysseus(configuracio);
```

Si la recepció falla, a les [línies 1297–1302](C:/Users/Gonho/0-Salle/SO/odysseus/src/odysseus/comandes.c:1297) només es tanca la connexió i es mostra l'error:

```c
if (rebreFitxer(*fd_ithaca, configuracio->ruta_carpeta, "OBJECT",
                FLAG_RESPOSTA_OK, FLAG_DADES_RESPOSTA,
                configuracio->fitxer_objecte, -1) != 0) {
    close(*fd_ithaca);
    *fd_ithaca = -1;
    return escriureMissatge("Error: no s'ha pogut rebre l'objecte.\n");
}
```

**La missió continua activa, però `fitxer_objecte` queda buit.**

A més, `executarSail`, a la [línia 1673](C:/Users/Gonho/0-Salle/SO/odysseus/src/odysseus/comandes.c:1673), comprova la ruta i les connexions, però no comprova que una missió activa tingui l'objecte rebut. La combinació d'aquests dos comportaments permet sortir.

### Què caldrà corregir?

Impedir navegar amb una missió activa sense l'objecte vàlid. També cal permetre abandonar l'assignació fallida i tornar a demanar missions, perquè el client no quedi bloquejat per sempre.

Això ha de continuar permetent navegar sense missió quan l'enunciat ho permet. No cal afegir un sistema per reprendre transferències interrompudes.

**Estat actual: corregit i verificat el 10/11/2026 en els casos indicats.**

## P4. Dues transferències amb el mateix nom es trepitgen

**És una prova addicional amb noms alterats deliberadament. Les missions simultànies amb els noms oficials van funcionar.**

### Què va passar?

Vam deixar una DELIVERY a mitges i vam iniciar una altra transferència amb el mateix nom de fitxer. La segona va modificar el fitxer que encara utilitzava la primera. La primera va acabar amb `MD5_ERROR`.

### On és el problema al codi?

A `prepararRecepcioFitxer`, [comun.c, línia 386](C:/Users/Gonho/0-Salle/SO/odysseus/src/comu/comun.c:386), el destí depèn de la carpeta i del nom rebut:

```c
longitud = snprintf(ruta_desti, MIDA_RUTA_FITXER, "%s/%s", carpeta_desti, nom_fitxer);
```

Si dos clients envien el mateix nom a la mateixa carpeta, **tots dos utilitzen el mateix fitxer**.

A la [línia 391](C:/Users/Gonho/0-Salle/SO/odysseus/src/comu/comun.c:391), s'obre així:

```c
fd_fitxer = open(ruta_desti, O_WRONLY | O_CREAT | O_TRUNC, 0644);
```

`O_TRUNC` buida el fitxer si ja existeix. Per això, la segona recepció pot esborrar dades de la primera i després barrejar-hi escriptures.

### Què caldrà decidir?

Si es cobreix aquest cas, cal evitar que dues recepcions simultànies utilitzin el mateix fitxer. Té menys prioritat que els tres problemes anteriors perquè la fallada es va provocar alterant els noms.

**Estat: pendent de decidir si es cobreix.**

## Què va funcionar?

- Rebre l'objecte oficial, navegar a Scheria, entregar-lo, rebre'l segellat i cobrar 1.500 gold una sola vegada.
- Rebutjar DELIVER i CLAIM en els estats incorrectes provats.
- Detectar mida incorrecta, MD5 incorrecte i interrupcions en OBJECT, DELIVERY, SIGNED i CLAIM. El P3 apareix després d'aquest error detectat.
- Recuperar l'estat en reinicis normals després de comprar, rebre OBJECT, rebre SIGNED i cobrar CLAIM.
- Conservar el stock de l'illa després de reiniciar, sense barrejar-lo amb altres illes. BUY MAP no modifica el stock físic.
- Gestionar els errors de fitxer i les dues missions simultànies amb noms diferents provades.
- Finalitzar amb CTRL+C en els casos provats. Valgrind no va detectar errors ni memòria pendent als processos instrumentats.

## Dos detalls que cal recordar

- **Retorn a Ithaca:** el testing inclou `SAIL Ithaca`, però l'enunciat descriu el retorn amb `CONNECT ITHACA`. Aquest últim funciona. Es va considerar una discrepància del document, no un error del programa.
- **Carpeta d'imatges:** al testing, les imatges eren a `data/objects/`, però `voyages.dat` apuntava a `objects/...`. Les proves van fer la còpia indicada al README. Cal mantenir coherents aquestes rutes en preparar l'entrega.

## Ordre per resoldre-ho

1. **P2:** reinici després de morir o quedar sense recursos.
2. **P3:** sortida després d'una recepció OBJECT fallida.
3. **P1:** validació del contingut de l'objecte al CLAIM.
4. **P4:** valorar el cas addicional de noms coincidents.

Els resultats corresponen a les proves fetes dins Docker, no a una execució real a les tres màquines de la universitat. No s'ha repetit tot el testing de les fases 1 i 2.

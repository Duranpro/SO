# Revisió de comprovacions i simplicitat

Revisió estàtica del codi propi de `src/`, contrastada amb
`teoria/Practica_SO_2026-27_corregit.pdf`, `teoria/Plantejament.pdf` i
`lib/sphragis.h`. Les línies corresponen al codi existent el 30/10/2026.
S'han afegit comentaris REVISAR als punts detectats, sense canviar el codi executable
ni executar els processos. Les referencies de linia d'aquest informe son les anteriors
a afegir aquests comentaris; buscar REVISAR als fonts per localitzar els punts actuals.


**Estat dels punts:** REVISAR vol dir pendent de valorar o corregir; no vol dir
que la correcció ja estigui aplicada. Cada punt indica el motiu i, als exemples
en C, la proposta concreta. Els controls marcats com a mantenir no són errors.

## Criteri de l'enunciat

- Pàgines impreses 17–18: es pot assumir correcte el **format dels fitxers de
  configuració**; s'han de gestionar els errors d'obertura, lectura, escriptura
  i reserva de memòria. Format correcte no garanteix que `read` o `malloc` funcionin.
- Pàgines 28–30: cal gestionar concurrència, desconnexions, caigudes i CTRL+C.
- Annex II, pàgines 38–54: protocol de 256 bytes, DATA de fins a 253 bytes,
  formats concrets i NACK per trames invàlides.
- La implementació actual correspon a Fase 2. No proposo afegir transferències,
  persistència d'Odysseus ni mecanismes de recuperació de Fase 3.

**Decisió:** mantenir els controls dels recursos i de les entrades externes;
eliminar comprovacions internes impossibles, validacions repetides i reserves
dinàmiques que no aporten res. No cal fer una llibreria genèrica a prova de tot.

## Punts marcats com a REVISAR

| Estat | Fitxer i línies | Per què s’ha de revisar i canvi proposat |
| --- | --- | --- |
| **REVISAR** | `src/odysseus/comandes.c:719–731` | `escriureMissatge` fa `asprintf("%s", text)` per copiar un text que ja existeix. Sobra la reserva, el control de reserva i el `free`. Escriure directament `text`, comprovant el resultat de `write`. |
| **REVISAR** | `src/odysseus/comandes.c:926–971` | `mostrarUsComanda` reserva còpies de literals amb `copiarText`. Es pot seleccionar el literal i passar-lo directament a `escriureMissatge`, sense heap. |
| **REVISAR** | `src/island/island.c:73–77,192–196` | Els buffers `error` només contenen missatges fixos i curts. Millor seleccionar un literal; desapareixen `malloc`, el seu control, `strcpy` i els `free(error)`. **Mentre existeixi el malloc, el control és necessari.** |
| **REVISAR** | `src/island/island.c:502–507` | `resposta` només serà `DOCKED`, `WAIT` o `Sense memoria`. Es pot seleccionar un literal, sense reservar ni copiar. |
| **REVISAR** | `src/ithaca/ithaca.c:168–172` | A `gestionarAccept` es repeteixen FLAGS, longitud, `memcpy`, terminador, `esNumero` i `strlen`, ja validats a 153–163. Eliminar aquesta segona validació i còpia. |
| **REVISAR** | `src/ithaca/ithaca.c:174–181` | Després d'`esNumero`, convertir amb `atoi` i comprovar `1 <= identificador <= nombre_viatges`. No cal repetir la conversió manual dígit a dígit. El límit de l'array sí que cal. |
| **REVISAR** | `src/ithaca/ithaca.c:190` | `longitud_resposta >= 0` és redundant: amb valor negatiu ja s'ha retornat a 186–188. Conservar el límit superior mentre no es garanteixi en carregar les dades. |
| **REVISAR** | `src/island/island.c:197` | FLAGS ja ha estat comprovat al despatxador (550) i la longitud a 189. La condició exterior és redundant en el flux actual. |
| **REVISAR** | `src/island/island.c:208–214` | `esNumero` ja rebutja valors superiors a `INT_MAX`. Es pot fer `quantitat = atoi(quantitat_text)` després de validar; sobra repetir el control d'overflow de la conversió. |
| **REVISAR** | `src/island/island.c:65,78,189` | El límit superior de longitud ja es comprova a 554 abans de cridar aquests gestors; FLAGS també es valida a 550. Deixar cada control en un únic lloc. Els controls específics de cada operació continuen fent falta. |
| **REVISAR** | `src/island/island.c:78` | En BUY MAP, el control de FLAGS és redundant. Conservar la comprovació que la quantitat sigui exactament 1. |
| **REVISAR** | `src/odysseus/comandes.c:218` | El segon `strlen(dades) != longitud` de BUY MAP repeteix el control de 204. |
| **REVISAR** | `src/odysseus/comandes.c:429` | A LIST MARKET, FLAGS ja és OK: s'han rebutjat els valors desconeguts i la branca ERROR ja ha retornat. |
| **REVISAR** | `src/odysseus/comandes.c:460` | `quantitat < 0` i `preu < 0` sobren després d'`esNumero` i `atoi`. Conservar els controls de MAP, índex i total. |
| **REVISAR** | `src/odysseus/comandes.c:561` | `*import >= 0` és redundant després d'`esNumero(or)` i `atoi(or)`. |
| **REVISAR** | `src/odysseus/comandes.c:590,678` | `quantitat <= 0` ja es rebutja al parser i aquestes funcions es criden des del terminal amb comanda validada. Es pot deixar aquesta responsabilitat al parser. El servidor ha de validar la seva pròpia petició de xarxa. |
| **REVISAR** | `src/odysseus/comandes.c:1035,1241,1579` | TYPE i/o límit superior de longitud es tornen a comprovar després d'haver posat `resultat = -1` en cas d'error. Dins `resultat == 0` aquestes parts són redundants. FLAGS OK sí que pot distingir OK d'ERROR. |
| **REVISAR** | `src/odysseus/comandes.c:1477` | A l'espera del port, TYPE ja ha estat validat a 1461. Conservar el control de resposta OK i del contingut DOCKED. |
| **REVISAR** | `src/odysseus/comandes.c:337–339` | A `sortirIlla`, assignar `resultat = 0` quan ja val 0 no aporta res; la validació posterior és la que rebutja contingut incorrecte. |
| **REVISAR** | `src/island/configuracio_illa.c:196` | `if (punter != NULL) free(punter)` sobra: es pot executar `free(NULL)`. El control exterior a 194 sí que cal per no indexar un array no reservat. |
| **REVISAR** | `src/island/configuracio_illa.c:221` | El filtre SPHRAGIS compacta els supervivents i deixa NULL fora del nou recompte, segons la capçalera. Dins `j < known_island_count`, el control de NULL sobra després d'un filtratge correcte. |
| **REVISAR** | `src/comu/comun.c:22` | Totes les crides actuals passen arrays o tokens comprovats. `text == NULL` es pot eliminar si el contracte d'`esNumero` exigeix una cadena vàlida. Conservar el control de cadena buida i de rang enter. |
| **REVISAR** | `src/comu/trames.c:29,32` | Totes les crides actuals passen un buffer local vàlid; amb longitud positiva passen un literal o dades ja reservades. Els controls de punters NULL són opcionals i eliminables amb aquest contracte intern. Conservar el control de longitud. `dades == NULL` és normal amb longitud 0. |
| **REVISAR** | `src/ithaca/ithaca.c:442` | `asprintf` per al literal de tancament no cal. Escriure el literal directament. |
| **REVISAR** | `src/island/configuracio_illa.c:273` | Igual per al missatge fix d'error de SPHRAGIS. |
| **REVISAR** | `src/odysseus/comandes.c:468` | Igual per a `Map 50 gold`. |

La prevalidació de totes les rutes a `island.c:81–92` les formata i allibera
abans de tornar-les a formatar per enviar-les a 103–115. Té un propòsit: detectar
una ruta massa llarga abans de començar una resposta parcial de compra. Es pot
eliminar només si es garanteix abans que totes les rutes serialitzades caben en
253 bytes; no és una duplicació completament inútil en el disseny actual.

## Inventari de totes les comparacions explícites amb NULL

Cada fila agrupa comprovacions amb el mateix origen. Les expressions `strchr`,
`strstr` i `strtok` també apareixen a l'inventari: el seu NULL significa «no trobat»
o «no queden camps», no pas error de memòria.

| Fitxer | Línies | Decisió |
| --- | --- | --- |
| `comu/comun.c` | 22 | Opcional pel contracte intern; explicat més amunt. |
| `comu/comun.c` | 51,67,97 | Mantenir: errors de malloc/realloc. |
| `comu/trames.c` | 29,32 | Eliminables pel contracte dels cridadors actuals; no eliminar el control de longitud. |
| `comu/gestio_navegants.c` | 46 | Mantenir: malloc. |
| `comu/gestio_navegants.c` | 83 | Mantenir: final de la llista enllaçada. |
| `ithaca/ithaca.c` | 83,421,431 | Mantenir: malloc o reserva del control del fil. A 431 evita desreferenciar NULL abans de pthread_create. |
| `ithaca/ithaca.c` | 296 | Mantenir: exclou el separador del protocol del nom rebut. |
| `ithaca/configuracio_itaca.c` | 37,47,58 | Mantenir: llegirLinia pot fallar encara que el fitxer tingui format correcte. |
| `ithaca/configuracio_itaca.c` | 62,66 | Eliminable la comprovació de camp absent assumint configuració correcta. A 62 conservar el control de copiarText. |
| `ithaca/configuracio_itaca.c` | 103 | Mantenir: realloc. |
| `ithaca/configuracio_itaca.c` | 119,123,127,131 | Controls de camps de voyages.dat. Simplificables si també s'assumeix correcte aquest fitxer proporcionat; la garantia explícita de l'enunciat parla de configuració. Conservar sempre errors de copiarText. |
| `ithaca/configuracio_itaca.c` | 156 | Mantenir la terminació per EOF; la funció actual no diferencia EOF d'error. Vegeu incidències. |
| `island/island.c` | 74,193,503 | Mantenir mentre hi hagi malloc; millor eliminar la reserva sencera. |
| `island/island.c` | 145,319,328,707,720 | Mantenir: malloc/realloc/reserva del fil. |
| `island/island.c` | 201 | Mantenir: cal trobar el separador de producte i quantitat rebuts. |
| `island/island.c` | 496 | Mantenir: separador en el nom rebut. |
| `island/configuracio_illa.c` | 50,256,314 | Mantenir: realloc/malloc. |
| `island/configuracio_illa.c` | 61,65,69,124,128 | Eliminables els controls de token absent sota la garantia de format correcte. Conservar els errors de copiarText. |
| `island/configuracio_illa.c` | 99,109,120,141 | Mantenir: lectura/reserva de la línia. |
| `island/configuracio_illa.c` | 152 | Mantenir `linia == NULL` per error de lectura. Comparar el marcador amb `--- ROUTES ---` és prescindible si assumim format correcte. |
| `island/configuracio_illa.c` | 160 | Mantenir la terminació per EOF; falta diferenciar errors. |
| `island/configuracio_illa.c` | 194 | Mantenir: pot fallar la reserva de l'array temporal amb recompte original positiu. |
| `island/configuracio_illa.c` | 196,221 | Eliminables; justificats més amunt. |
| `odysseus/configuracio_odisseu.c` | 26,37,60,101,109,134 | Mantenir: errors de lectura/reserva de línia. |
| `odysseus/configuracio_odisseu.c` | 41,46,51,64,69,138,144 | Eliminables els controls de token absent sota la garantia de format. Conservar els errors de copiarText. |
| `odysseus/configuracio_odisseu.c` | 123 | Mantenir: malloc. |
| `odysseus/configuracio_odisseu.c` | 171 | Mantenir: nombre_aliments s'assigna abans de malloc; si falla, hi ha recompte positiu i array NULL. No es pot entrar al bucle. |
| `odysseus/comandes.c` | 226,233,237 | Mantenir: estructura dels camps del mapa rebut i nombre exacte de camps. |
| `odysseus/comandes.c` | 418,425,429 | Mantenir els controls de separadors/tokens de LIST MARKET. La part de FLAGS de 429 és redundant. |
| `odysseus/comandes.c` | 441,631,1146 | Mantenir: malloc/realloc. |
| `odysseus/comandes.c` | 540,557,558 | Mantenir: separadors i quatre camps obligatoris del rebut BUY/SELL; rebutjar camps addicionals. |
| `odysseus/comandes.c` | 877 | Mantenir: final dels tokens introduïts al terminal. |
| `odysseus/comandes.c` | 1121,1129,1133 | Mantenir: estructura i sis camps exactes de LIST VOYAGES. |
| `odysseus/comandes.c` | 1255,1257,1261 | Mantenir: estructura i cinc camps exactes de la resposta ACCEPT. |
| `odysseus/comandes.c` | 1499,1718 | Mantenir: EOF/error al terminal o interrupció. |

Les comprovacions dels camps de xarxa no sobren perquè els nostres emissors
construeixin trames correctes: l'Annex II preveu explícitament rebutjar formats
invàlids. Tampoc és duplicació inútil validar al client i al servidor: són processos
independents i les dades compartides poden canviar entre peticions.

## `missatge = NULL`: assignació, no comprovació

No hi ha cap `if (missatge == NULL)` al codi actual. Hi ha inicialitzacions,
assignacions `missatge = NULL` i controls del retorn d'asprintf.

- `asprintf < 0` / `longitud >= 0`: conservar sempre que es mantingui la reserva.
  Cal usar el retorn; posar inicialment el punter a NULL no substitueix aquest control.
- A `ithaca.c:106,204,247` i `island.c:122,268,360`, posar el missatge a NULL
  després de l'últim ús no aporta res al flux actual.
- A `comandes.c:456,483,550,653,698,1166,1422,1590,1601`, els missatges es
  tornen a assignar amb asprintf o ja no s'utilitzen. Aquests resets són prescindibles
  en el flux actual; revisar-ho si es canvia la neteja.
- A `island.c:513,519,525,539,584`, conservar la protecció: hi ha un
  `free(missatge)` comú a 596. El NULL evita alliberar un missatge ja alliberat
  o un punter de resultat no utilitzable després d'un asprintf fallit.
- No aplicar una eliminació global de `punter = NULL`: `nom_nou = NULL` a
  `comandes.c:644` cedeix la propietat a l'inventari i evita que el free final
  l'alliberi. Els resets de mercat, viatges consultats i descriptors també
  representen estat real que es consulta després.
- Les inicialitzacions dels camps dinàmics d'aliments, rutes, viatges i
  SPHRAGIS permeten netejar càrregues parcials quan falla una reserva. Mantenir-les.

## Controls que conservaria

- Retorns de malloc, realloc, copiarText i asprintf; conservar el temporal de
  realloc per no perdre la reserva antiga quan falla.
- Retorns de socket, connect, bind, listen, accept, pipe, select i pthread_create.
- EOF i errors de read/write, i tancament/alliberament dels recursos.
- Validació numèrica del terminal i dels missatges rebuts. `esNumero` ja garanteix
  el rang d'int; aprofitar-ho en comptes de tornar a convertir manualment.
- Límits de memòria i de DATA_LENGTH. Un nom de longitud variable pot fer que
  `crearTrama` falli a `ithaca.c:119–126` encara que el fitxer tingui el format
  correcte: format i longitud màxima no són la mateixa garantia.
- Controls d'estat: connexió, viatge actiu, ID consultat, atracat, stock, diners,
  quantitat positiva, rutes directes i quantitat de mapa igual a 1.
- Mutex, registre de fils, join i pipe de finalització: responen a requisits
  concrets de Fase 2. No els eliminaria per reduir línies.
- `signal(SIGPIPE, SIG_IGN)`: un client que cau no ha de matar el servidor quan
  escriu al socket. `write` retorna error i el codi pot gestionar-lo.
- Ignorar SIGINT abans de desmuntar la pipe evita que el handler escrigui en un
  descriptor ja tancat. La curta durada del tancament no és una garantia.
- El tipus unsigned char de les trames permet representar els flags 0x80/0xC0
  i longituds de fins a 253 sense valors negatius. No el canviaria per char signat.
- Els controls d'overflow de sumes i productes no són els mateixos que validar
  cada operand: `esNumero` no garanteix que dos enters vàlids es puguin multiplicar.

## REVISAR: incidències detectades i motiu

1. **REVISAR — `llegirLinia` confon EOF i error.** A `comun.c:58–60`, un read fallit,
   EOF i un malloc/realloc fallit acaben retornant NULL. A
   `carregarViatges` i al bucle de rutes això pot acceptar una càrrega parcial
   com a correcta. A més, es perd l'última línia si no acaba amb salt de línia.
   Cal distingir final de fitxer i error, no eliminar els controls de NULL.
2. **REVISAR — Errors d'escriptura ignorats.** `escriureMissatge` retorna èxit encara que
   write falli. També hi ha molts write directes sense comprovar a les sortides.
   Això no compleix del tot el requisit d'errors d'E/S. Simplificar la funció
   permet arreglar-ho sense afegir reserves.
3. **REVISAR — Trames parcials: diferència amb el redactat.** `enviarTrama`, `rebreTrama`
   i `rebreTramaNavegant` acumulen escriptures/lectures parcials. Això té sentit
   tècnic en TCP, però la pàgina impresa 39 prescriu una única operació read/write
   de 256 bytes. Cal tenir present aquesta diferència de criteri de l'assignatura;
   no la canviaria automàticament ni afirmaria que un únic read garanteix 256 bytes.
4. **REVISAR — No tota la redundància és defensa útil.** A `gestionarCompraVenda`, després
   d'`esNumero`, el bucle manual d'overflow complica una conversió ja validada;
   en canvi comprovar el producte preu × quantitat continua sent necessari.

## Ordre recomanat per fer una neteja petita

1. Treure les reserves artificials de missatges i literals.
2. Eliminar les validacions repetides dins del mateix flux.
3. Simplificar les conversions aprofitant esNumero.
4. Treure controls de format de configuració emparats per l'enunciat.
5. Conservar els controls de recursos, protocol i estat, i corregir la distinció
   entre EOF i error de lectura.

No proposo afegir frameworks, abstraccions de validació, reintents generals,
timeouts nous ni funcionalitats de fases posteriors per fer aquesta neteja.

## Com quedarien les correccions en C

Són propostes per aplicar més endavant, **no canvis aplicats als `.c`**.
Els fragments parcials indiquen quin bloc substituir; la resta de la funció
es conserva. Els exemples mantenen els noms actuals del projecte.

### REVISAR — 1. Escriure un text sense reservar una còpia

**Per què s’ha de revisar:** Es reserva memòria per copiar un text que ja existeix, i no es comprova si write falla o escriu menys bytes.

A `src/odysseus/comandes.c`, substituir tota `escriureMissatge` per:

```c
int escriureMissatge(char *text) {
    int longitud = strlen(text);

    if (write(1, text, longitud) != longitud) {
        return -1;
    }
    return 0;
}
```

No hi ha `malloc`, `asprintf` ni `free`. Aquesta versió senzilla detecta també
una escriptura incompleta i la retorna com a error; no intenta completar-la.
El contracte continua sent que `text` apunta a una cadena vàlida.
Actualitzar el comentari de retorn: «0 si escriu tot el text, -1 si falla
o l'escriptura és incompleta». Els cridadors han de comprovar aquest retorn.

### REVISAR — 2. Mostrar la sintaxi sense copiar literals

**Per què s’ha de revisar:** Es creen i alliberen còpies dinàmiques de missatges de sintaxi que són literals fixos.

Substituir tota `mostrarUsComanda` a `src/odysseus/comandes.c`:

```c
int mostrarUsComanda(int tipus) {
    switch (tipus) {
    case COMANDA_CONNECT:
        return escriureMissatge("Usage: CONNECT ITHACA\n");
    case COMANDA_LIST_VOYAGES:
        return escriureMissatge("Usage: LIST VOYAGES\n");
    case COMANDA_ACCEPT:
        return escriureMissatge("Usage: ACCEPT <voyage_id>\n");
    case COMANDA_SAIL:
        return escriureMissatge("Usage: SAIL <island>\n");
    case COMANDA_MAP:
        return escriureMissatge("Usage: MAP\n");
    case COMANDA_LIST_MARKET:
        return escriureMissatge("Usage: LIST MARKET\n");
    case COMANDA_BUY:
        return escriureMissatge("Usage: BUY <product> <amount>\n");
    case COMANDA_SELL:
        return escriureMissatge("Usage: SELL <product> <amount>\n");
    case COMANDA_STATUS:
        return escriureMissatge("Usage: STATUS\n");
    case COMANDA_DELIVER:
        return escriureMissatge("Usage: DELIVER\n");
    case COMANDA_CLAIM:
        return escriureMissatge("Usage: CLAIM\n");
    }
    return -1;
}
```

### REVISAR — 3. Errors fixos sense malloc ni strcpy

**Per què s’ha de revisar:** Els missatges d’error són fixos; reservar-los dinàmicament afegeix malloc, controls i free innecessaris.

A `gestionarCompraVenda` d'`island.c`, la declaració i les assignacions
del missatge d'error poden quedar així:

```c
char *error = "Peticio invalida";

/* Quan no es troba el producte: */
error = "Producte desconegut";

/* Quan la quantitat no es pot representar: */
error = "Quantitat fora de rang";

/* Quan no queda prou stock: */
error = "Stock insuficient";

/* Quan l'operacio es pot fer: */
error = "";

/* Quan falla l'actualitzacio del fitxer: */
error = "Error escrivint stock";
```

Són alternatives dins les branques existents, no assignacions per executar
una darrere l'altra. Substituir **cada** `strcpy(error, ...)` per l'assignació
corresponent i **cada** `error[0] = '\0'` per `error = ""`.
Eliminar el malloc, el seu `if` i **tots** els `free(error)` de la funció.
No modificar els literals a través del punter.

El bloc final d'error es conserva sense alliberar el literal:

```c
if (error[0] != '\0') {
    crearTrama(trama, tipus, FLAG_RESPOSTA_ERROR,
               (unsigned char *)error, strlen(error));
} else {
    /* Es conserva la construccio de la resposta correcta actual. */
}
```

A `gestionarBuyMap`, aplicar el mateix patró: començar amb `char *error = ""`,
assignar `"Quantitat de mapa invalida"` o `"Ruta massa llarga"` a les branques
corresponents, i eliminar totes les reserves i alliberaments d'`error`.

### REVISAR — 4. Resposta d'arribada sense reserva

**Per què s’ha de revisar:** La resposta només pot ser un dels tres literals previstos; no necessita una reserva de memòria pròpia.

A `atendreOdysseusIsland`, eliminar el malloc de `resposta` i el bloc d'error
que només existeix per aquesta reserva. A les branques actuals, substituir:

```c
if (resultat_port == 1) {
    resposta = "DOCKED";
    /* Es conserva el log actual d'entrada al port. */
} else if (resultat_port == 2) {
    resposta = "WAIT";
    /* Es conserva el log actual d'espera a la FIFO. */
} else {
    resposta = "Sense memoria";
    flags = FLAG_RESPOSTA_ERROR;
}

crearTrama(trama, TIPUS_ARRIBADA_ILLA, flags,
           (unsigned char *)resposta, strlen(resposta));
resultat = enviarTrama(fd_navegant, trama);
```

Eliminar també `free(resposta)`: ara apunta a un literal.
Conservar la gestió de `missatge` dels logs i els seus resets de NULL.

### REVISAR — 5. ACCEPT: validar una vegada i convertir directament

**Per què s’ha de revisar:** Es validen i copien dues vegades les mateixes dades i es repeteix una conversió numèrica ja coberta per esNumero.

A `gestionarAccept` d'`ithaca.c`, conservar les validacions inicials de FLAGS,
longitud, text numèric i viatge actiu. Substituir el segon bloc de validació
i el bucle manual de conversió per:

```c
identificador = atoi(dades);
if (identificador > configuracio->nombre_viatges) {
    identificador = 0;
}
```

El `if (identificador > 0)` que ja ve després es conserva. Així un ID 0
o fora de l'array continua arribant a la resposta `ACCEPT invalid` existent.
Treure `i` de la declaració de variables si ja no s'utilitza.

Més avall, després de comprovar que asprintf no ha fallat:

```c
pthread_mutex_lock(mutex_viatges);
if (viatge->disponible == 1 && longitud_resposta <= MIDA_DADES) {
    viatge->disponible = 0;
    strcpy(viatge->odysseus_assignat, nom_odysseus);
    acceptat = 1;
}
pthread_mutex_unlock(mutex_viatges);
```

Només desapareix `longitud_resposta >= 0`; el mutex i el límit superior queden.

### REVISAR — 6. BUY/SELL: aprofitar esNumero

**Per què s’ha de revisar:** La longitud i FLAGS ja es comproven abans, i esNumero ja rebutja quantitats fora del rang d’int.

A `gestionarCompraVenda`, després de separar producte i quantitat, substituir
`valida = esNumero(...)` i el bucle manual que calcula la quantitat per:

```c
valida = esNumero(quantitat_text);
if (valida == 1) {
    quantitat = atoi(quantitat_text);
    if (dades[0] != '\0') {
        format_valid = 1;
    }
}
```

Conservar a continuació la condició de `quantitat > 0`, longitud del producte,
exclusió de MAP, stock i overflow del preu per quantitat.
La comprovació de longitud inicial del gestor pot quedar:

```c
/* El despatxador ja valida FLAGS i longitud <= MIDA_DADES. */
if (longitud == 0) {
    return enviarNack(fd_navegant, "INVALID_LENGTH");
}
```

Desfer el `if (peticio[POSICIO_FLAGS] == FLAG_PETICIO && ...)` exterior:
executar directament el seu contingut, perquè el despatxador ja ho garanteix.
No eliminar les validacions dels camps rebuts.

A BUY MAP, el control equivalent específic queda:

```c
if (peticio[POSICIO_LONGITUD] != 1 || peticio[POSICIO_DADES] != '1') {
    error = "Quantitat de mapa invalida";
}
```

Aquest fragment pressuposa el canvi a literals de l'apartat 3 i conserva
la validació prèvia que diferencia text numèric invàlid de quantitat no admesa.

### REVISAR — 7. Condicions repetides del client

**Per què s’ha de revisar:** Hi ha condicions que ja són certes per validacions anteriors; dificulten veure quines comprovacions són necessàries.

A `executarBuyMap`, després del primer control de strlen i de gestionar ERROR:

```c
if (trama[POSICIO_FLAGS] != FLAG_RESPOSTA_OK) {
    resultat = -1;
    break;
}
```

S'ha eliminat només la segona comprovació de strlen.

A `executarListMarket`, la validació de camps queda:

```c
if (nombre_camps != 5 || camp != NULL ||
    !esNumero(camps[0]) || !esNumero(camps[1]) ||
    !esNumero(camps[3]) || !esNumero(camps[4]) ||
    atoi(camps[0]) != index || atoi(camps[1]) < index ||
    strlen(camps[2]) >= MIDA_NOM_PRODUCTE) {
    enviarNack(*fd_illa, "INVALID_DATA");
    resultat = -1;
    break;
}
```

Després de convertir quantitat i preu, la següent validació queda:

```c
if (atoi(camps[1]) != total ||
    (index == total &&
     (strcmp(camps[2], "MAP") != 0 || quantitat != 1 || preu != 50)) ||
    (index < total && strcasecmp(camps[2], "MAP") == 0)) {
    enviarNack(*fd_illa, "INVALID_DATA");
    resultat = -1;
    break;
}
```

A `intercanviarAliments`, dins la branca de rebut ja validat:

```c
*import = atoi(or);
return 1;
```

Substitueix l'assignació i el `if (*import >= 0)`; el control anterior
`esNumero(or)` continua sent obligatori.

Altres substitucions de condicions, mantenint el seu cos actual:

```c
/* executarBuy: el parser ja garanteix quantitat positiva. */
if (*fd_illa < 0) {
    return escriureMissatge("Error: cal estar atracat i comprar un aliment.\n");
}

/* executarSell: mateix contracte amb el parser. */
if (*fd_illa < 0 || strcasecmp(producte, "MAP") == 0) {
    return escriureMissatge("Error: cal estar atracat i vendre un aliment.\n");
}

/* connectarIthaca: TYPE ja validat, ara interessa la resposta OK. */
if (resultat == 0 && trama[POSICIO_FLAGS] == FLAG_RESPOSTA_OK &&
    trama[POSICIO_LONGITUD] == 2) {
    /* Cos actual de confirmacio de connexio. */
}

/* executarAccept: TYPE i longitud ja validats. */
if (resultat == 0) {
    /* Cos actual que copia DATA i diferencia ERROR d'OK. */
}

/* arribarIlla: dins del bloc resultat == 0 existent. */
if (trama[POSICIO_FLAGS] == FLAG_RESPOSTA_OK) {
    /* Cos actual que copia DATA i diferencia DOCKED de WAIT. */
}

/* esperarPort: TYPE ja validat immediatament abans. */
if (resultat != 0 || trama[POSICIO_FLAGS] != FLAG_RESPOSTA_OK ||
    trama[POSICIO_LONGITUD] != 6 ||
    memcmp(trama + POSICIO_DADES, "DOCKED", 6) != 0) {
    /* Cos actual de notificacio d'error i break. */
}
```

A `sortirIlla`, eliminar íntegrament el bloc que només fa `resultat = 0`
quan FLAGS és OK, longitud és 2 i dades és OK. Conservar la comprovació
posterior que envia NACK quan la longitud o el contingut no són correctes.

### REVISAR — 8. Configuració de format correcte: treure només el control de format

**Per què s’ha de revisar:** L’enunciat permet assumir el format correcte de configuració; cal separar aquests controls dels errors reals de lectura i memòria.

Exemple de lectura de la IP i port a `carregarConfiguracioItaca`:

```c
linia = llegirLinia(fd);
if (linia == NULL) {
    resultat = -1;
} else {
    camp = strtok(linia, " ");
    resultat = copiarText(&configuracio->ip, camp);
    if (resultat == 0) {
        camp = strtok(NULL, " ");
        configuracio->port = atoi(camp);
    }
}
free(linia);
linia = NULL;
```

Aquí ja no comprovem si falten camps: ho cobreix la garantia de format.
Sí que es comprova `llegirLinia` i el resultat de `copiarText`.
Aplicar el mateix patró als camps de configuració d'Island i Odysseus
indicats a l'inventari. No aplicar-lo als tokens de xarxa ni del terminal.

Per llegir el marcador de rutes, que sabem que és correcte:

```c
linia = llegirLinia(fd);
if (linia == NULL) {
    resultat = -1;
}
free(linia);
linia = NULL;
```

### REVISAR — 9. free i filtratge de SPHRAGIS

**Per què s’ha de revisar:** free(NULL) és vàlid i el filtre garanteix els supervivents dins del nou recompte; alguns controls interns sobren.

A `alliberarConfiguracioSphragis`:

```c
if (illa->known_islands != NULL) {
    for (i = 0; i < nombre_rutes; i++) {
        free(illa->known_islands[i]);
    }
}
free(illa->known_islands);
illa->known_islands = NULL;
illa->known_island_count = 0;
```

A `conservarRutesValides`, dins del bucle limitat pel recompte retornat
pel filtre correcte:

```c
if (strcmp(configuracio->rutes[i].nom_illa,
           illa_sphragis->known_islands[j]) == 0) {
    ruta_valida = 1;
}
```

### REVISAR — 10. Contractes interns de esNumero i crearTrama

**Per què s’ha de revisar:** Els cridadors actuals ja garanteixen els punters vàlids. Cal decidir i documentar aquest contracte abans de treure els controls.

Si es decideix treure els controls de punters que els cridadors ja garanteixen,
la primera condició d'`esNumero` queda:

```c
/* text ha d'apuntar a una cadena valida. */
if (text[0] == '\0') {
    return 0;
}
```

La resta d'`esNumero`, inclòs el límit INT_MAX, es conserva. `crearTrama`
pot quedar completa així:

```c
/* trama es valid; dades es valid si longitud > 0. */
int crearTrama(unsigned char *trama, unsigned char tipus,
               unsigned char flags, unsigned char *dades, int longitud) {
    if (longitud < 0 || longitud > MIDA_DADES) {
        return -1;
    }

    memset(trama, 0, MIDA_TRAMA);
    trama[POSICIO_TIPUS] = tipus;
    trama[POSICIO_FLAGS] = flags;
    trama[POSICIO_LONGITUD] = longitud;
    if (longitud > 0) {
        memcpy(trama + POSICIO_DADES, dades, longitud);
    }
    return 0;
}
```

No canviar la funció a void: la longitud de dades construïdes dinàmicament
encara pot superar MIDA_DADES.

### REVISAR — 11. Exemple de reset prescindible i reset necessari

**Per què s’ha de revisar:** Alguns resets no tenen cap ús posterior, però altres eviten un doble free. No es poden eliminar tots de cop.

Log final de `gestionarDesconnexio`, sense reset inútil:

```c
longitud = asprintf(&missatge, ">>> %s has left Ithaca. Fair winds!\n",
                    nom_odysseus);
if (longitud >= 0) {
    int escrit = write(1, missatge, longitud);
    free(missatge);
    if (escrit != longitud) {
        return -1;
    }
}
return 1;
```

En canvi, a `atendreOdysseusIsland`, mantenir aquest patró quan hi ha
un `free(missatge)` comú posterior:

```c
if (longitud >= 0) {
    write(1, missatge, longitud);
    free(missatge);
}
missatge = NULL; /* El free comu posterior ha de rebre NULL. */
```

Aquest últim fragment només il·lustra la propietat de la memòria; el write
també s'hauria de comprovar i l'error s'hauria de conduir a la neteja comuna.

### REVISAR — 12. Literals directes i gestió de l'escriptura

**Per què s’ha de revisar:** S’utilitza asprintf per escriure literals, i cal comprovar write sense saltar-se la neteja dels recursos.

Per exemple, al tancament d'Ithaca, substituir asprintf i free per:

```c
if (write(1, "Ithaca closes the harbor.\n",
          strlen("Ithaca closes the harbor.\n")) !=
    (int)strlen("Ithaca closes the harbor.\n")) {
    resultat = 1;
}
/* Continuar amb la neteja actual encara que falli la sortida. */
```

Al client, el cas `Map 50 gold` pot escriure's amb `escriureMissatge` directament.
Cal posar aquesta branca fora del `free(missatge)` que servia per a les dues
branques originals: no alliberar un punter d'una iteració anterior.

```c
if (index == total) {
    if (escriureMissatge("Map 50 gold\n") != 0) {
        free(mercat);
        return -1;
    }
} else {
    strcpy(mercat[index - 1].nom, camps[2]);
    mercat[index - 1].quantitat = quantitat;
    mercat[index - 1].preu = preu;
    if (asprintf(&missatge, "%s %d kg %d gold/kg\n",
                 camps[2], quantitat, preu) < 0) {
        free(mercat);
        return -1;
    }
    resultat = escriureMissatge(missatge);
    free(missatge);
    if (resultat != 0) {
        free(mercat);
        return -1;
    }
}
```

### REVISAR — 13. Distingir EOF i error sense un sistema nou

**Per què s’ha de revisar:** NULL significa tant EOF com error; això pot donar per bona una càrrega incompleta i perdre l’última línia sense salt final.

Una opció petita és canviar el contracte de `llegirLinia` a:

```c
/* 1 = linia llegida; 0 = EOF sense linia; -1 = error. */
int llegirLinia(int fd, char **linia);
```

Això és una proposta alternativa que requereix adaptar **tots** els cridadors
i la declaració de `comun.h`; no es pot copiar només el bucle següent sobre
la funció actual. Una implementació possible és:

```c
int llegirLinia(int fd, char **linia) {
    char *text = NULL, *temporal = NULL;
    char caracter;
    int longitud = 0, bytes_llegits;

    *linia = NULL;
    text = malloc(1);
    if (text == NULL) {
        return -1;
    }
    text[0] = '\0';

    while (1) {
        bytes_llegits = read(fd, &caracter, 1);
        if (bytes_llegits < 0) {
            free(text);
            return -1;
        }
        if (bytes_llegits == 0) {
            if (longitud == 0) {
                free(text);
                return 0;
            }
            break; /* Conservar l'ultima linia sense salt final. */
        }
        if (caracter == '\n') {
            break;
        }
        temporal = realloc(text, longitud + 2);
        if (temporal == NULL) {
            free(text);
            return -1;
        }
        text = temporal;
        text[longitud++] = caracter;
        text[longitud] = '\0';
    }

    *linia = text;
    return 1;
}
```

Exemple del bucle de `carregarViatges`, dins del camí on open ja ha funcionat:

```c
int lectura;

while ((lectura = llegirLinia(fd, &linia)) == 1) {
    resultat = afegirViatge(linia, configuracio);
    free(linia);
    linia = NULL;
    if (resultat != 0) {
        break;
    }
}
if (lectura < 0) {
    resultat = -1;
}
close(fd);
/* Conservar el tractament d'error i el retorn posteriors. */
```

En una lectura obligatòria de configuració, comprovar `lectura != 1`;
al terminal, acabar si `lectura == 0`, i informar o propagar l'error si és -1.
Si el terminal ha estat interromput per CTRL+C, consultar primer l'indicador
de finalització per tractar-ho com el tancament demanat per l'usuari.

No incloc una substitució dels bucles de trames: la diferència entre el
redactat de l'assignatura i les lectures parcials TCP s'ha de resoldre abans
de canviar aquest comportament.

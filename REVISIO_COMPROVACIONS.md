# Revisió de comprovacions i simplicitat

**Ampliació: nivell de classe.** Al final hi ha la revisió contrastada amb tots
els materials de `teoria`. Els comentaris nous són `REVISAR CLASSE`.
La revisió no canvia el comportament del programa. Les funcions de les llibreries absents dels
materials es marquen com a candidates a revisar, no com a prohibicions
expresses de l'enunciat. Les auxiliars particulars de les sessions no es
consideren funcions disponibles al projecte ni motiu per substituir les existents.

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

## Revisió addicional: nivell dels estudiants i material de classe

### Abast i criteri

S'han inventariat els **16 PDFs, 51 fitxers `.c`, 1 `.h` i 7 `.txt`** de
`teoria`. S'han contrastat les crides i els patrons dels fonts amb els materials;
també s'han inspeccionat les diapositives amb codi en imatge. La còpia de
`teoria/File Descriptors/Tema4.FileDescriptors.pdf` també s'ha inclòs.

La referència principal són els S0–S6 de les subcarpetes, els altres exemples
i els PDFs. Els `.txt` de l'arrel són evidència complementària: que una funció
surti en una versió antiga vostra no implica que surti a la solució del professor.

Distingeixo tres casos:

1. **Vist als materials:** conservar, encara que el nom sembli poc habitual.
2. **No localitzat:** substituir si voleu cenyir-vos estrictament a les funcions
   mostrades i hi ha una alternativa clara. No significa que el professor ho
   prohibeixi: aquesta és la restricció de simplicitat demanada en aquesta revisió.
3. **Combinació pròpia més elaborada:** les primitives són conegudes, però el
   disseny no apareix als exemples. Cal simplificar-lo o poder-lo explicar.

Els noms propis com `confirmarEspera`, `reservarNavegant` o
`intercanviarAliments` són funcions vostres. La seva absència dels apunts no
és cap problema per si mateixa: s'ha de valorar què fan per dins.

### Resultat per funció o mecanisme

| Element del projecte | Evidència als materials | Decisió |
| --- | --- | --- |
| `open`, `read`, `write`, `close`, `lseek` | `teoria/File Descriptors/S0.c`, `teoria/Threads/S2.c` i `teoria/File Descriptors/file_descriptors_1.c` | Dins del nivell. Conservar. |
| `malloc`, `realloc`, `free`, punters dobles de sortida | `teoria/File Descriptors/S0.c`, `teoria/Threads/S2.c` i `teoria/Select/S5_sol.c` | Dins del nivell. Un `char **` no és automàticament avançat. |
| `asprintf`, `_GNU_SOURCE` | `teoria/File Descriptors/S0.c`, `teoria/Signals/S1.c` i `teoria/Threads/S2.c` | Vistos. Treure només reserves innecessàries, no substituir tot asprintf. |
| `memcpy`, `memset`, serialització en buffers | `teoria/Sockets/S4_client.c` i `teoria/Sockets/S4_server.c` | Vistos. La serialització no és complexitat extra. |
| `socket`, `bind`, `listen`, `accept`, `connect`, `htons`, `inet_pton`, `IPPROTO_TCP` | `teoria/Sockets/server.c`, `teoria/Sockets/client.c` i `teoria/Select/select.c` | Vistos. No cal canviar inet_pton per una funció menys clara. |
| `pthread_create`, `pthread_join`, paràmetres en structs | `teoria/Threads/S2.c`, `teoria/Threads/example2.c` i `teoria/Sockets/S4_server.c` | Vistos. Un fil per client té base als exemples. |
| `pthread_mutex_t`, `pthread_mutex_lock/unlock/destroy`; inicialització del mutex | `teoria/Tema3-Concurrencia-ExclusioMutua-Mutex-CA-v1.pdf`, diapositives 12–15; `teoria/Tema3-Semafors-Sincronitzacio-CA-v1.pdf`, diapositives 2 i 18 | Conservar els mutex del projecte: són l'eina indicada per protegir memòria entre fils. El PDF mostra PTHREAD_MUTEX_INITIALIZER; pthread_mutex_init és la inicialització dinàmica de la mateixa API. |
| `SEM_constructor`, `SEM_init`, `SEM_wait`, `SEM_signal`, `SEM_destructor` | `teoria/Tema3-Semafors-Sincronitzacio-CA-v1.pdf`, diapositives 7–8, i `lib/semaphore_v2.h` | API del curs explícitament proporcionada i ara disponible a lib. No és un auxiliar particular de S5 ni implica substituir els mutex existents. |
| `select`, `FD_ZERO`, `FD_SET`, `FD_ISSET`, `FD_SETSIZE` | `teoria/Select/S5_sol.c`, `teoria/Select/S5_trader.c`, `teoria/Select/select.c` i `teoria/Tema4.Select.pdf` | Vistos. El límit FD_SETSIZE també està contemplat a `teoria/Select/S5_sol.c`. |
| `struct timeval`, paràmetre timeout | Signatura de select a `teoria/Tema4.Select.pdf`, diapositiva 3 | Té suport al material, encara que `teoria/Select/S5_trader.c` faci servir NULL com a timeout. |
| `gettimeofday` i aritmètica de terminis en microsegons | No localitzats | Principal bloc temporal a revisar. Cal conservar el consum periòdic encara que hi hagi ordres. |
| `signal`, `alarm`, `pause`, `SIG_IGN` | `teoria/Signals/S1.c`, `teoria/Signals/signals_1.c` i `teoria/Signals/signals_2.c` | Vistos. No introduir un mecanisme de senyals nou per simplificar un detall. |
| `signal(SIGPIPE, SIG_IGN)` | `teoria/Select/S5_sol.c` i `teoria/Select/S5_trader.c` | Vist explícitament; conservar. |
| `errno`, `EINTR` | `teoria/Select/select.c` tracta select interromput; `teoria/Tema4.Pipes.pdf` també esmenta errno | Vistos. Guardar/restaurar errno en un handler és un detall addicional del disseny actual. |
| `__attribute__((unused))` | `teoria/Signals/signals_1.c`, `teoria/Threads/example1.c`, `teoria/Threads/slides_example1.c` i `teoria/Select/select.c` | Vist. No marcar-lo com a funció desconeguda ni treure'l per aparença. |
| Lectures parcials acumulades | Última diapositiva de `teoria/Tema4.Sockets.pdf`: llegir els bytes pendents | Vist explícitament; actualitza la valoració anterior dels bucles de lectura. |
| `pipe` | `teoria/Tema4.Pipes.pdf`, `teoria/Pipes/ex1.c`, `teoria/Pipes/ex2.c`, `teoria/Pipes/ex3.c`, `teoria/Pipes/ex4.c` i `teoria/Pipes/ex5.c` | La funció és vista. El seu ús per despertar tots els fils amb EOF és un patró propi més elaborat. |
| `strtok` | Funció de `<string.h>` disponible al projecte | Conservar. Que les sessions facin servir un separador propi no obliga a substituir-la. El parseig amb strtok no es fa als gestors concurrents d'Island/Ithaca. |
| `strcasecmp` | No localitzat | Alternativa estricta: comparació amb conversió ASCII en un bucle. Cal conservar case insensitive. |
| `toupper` | No localitzat; `teoria/Threads/S2.c` converteix ASCII manualment | Substitució fàcil per als noms ASCII de la pràctica. |
| `strstr` per trobar `&&` | No localitzat | Substituïble per un bucle curt que busqui dos separadors seguits. |
| `memcmp` per a `DOCKED` | No localitzat | Substituïble per buffer acabat en NUL i strcmp, després de validar longitud. |
| `strchr`, `strcmp`, `strcpy`, `strlen`, `atoi` | `teoria/Sockets/S4_server.c`, `teoria/Select/S5_sol.c` i `teoria/Select/S5_trader.c` | Vistos. No cal reimplementar-los tots. |
| `SPHRAGIS_*` | Llibreria proporcionada (`lib/sphragis.h`, fora de `teoria/`) i ús requerit a `teoria/Practica_SO_2026-27_corregit.pdf` | Conservar. L'absència dels Sx.c no els fa opcionals. |

### Parts de disseny que més revisaria

**1. Registre de navegants i finalització.**

`gestio_navegants.c/.h` combina una llista enllaçada, entrades estables, un
mutex propi, marca d'acabat, recollida periòdica, pipe i select als fils.
Cada peça té una funció, però el conjunt és força més elaborat que els arrays
de fils de `teoria/Threads/S2.c` i `teoria/Sockets/S4_server.c`.

- Primer canvi petit: substituir el recorregut amb `FilNavegant **posicio`
  per punters `anterior` i `actual`. Manté el mateix esquema de memòria.
- Alternativa de disseny: array dinàmic de **punters** a entrades reservades,
  més semblant als arrays dels exemples. No usar un array de structs mogut
  per realloc si els fils guarden punters als seus elements.
- Conservar que `pthread_join` es fa sense tenir el bloqueig que el fil
  necessita per marcar-se acabat. Fer join amb aquest bloqueig pot encallar-los.
- La pipe és local a cada servidor: no substitueix la xarxa entre màquines
  i no incompleix aquesta restricció del projecte.
- No canviar-ho simplement per `pthread_cancel`: `teoria/Threads/S2.c` adverteix explícitament
  de memòria filtrada en fils cancel·lats. El projecte ha d'alliberar recursos.

**2. Pipe com a avís col·lectiu.**

El handler escriu un byte; el principal detecta la pipe preparada, deixa
d'acceptar connexions i tanca l'extrem d'escriptura durant la recollida.
Els fils detecten la finalització sense consumir la pipe. Aquest esquema
permet que tots es despertin; un sol byte consumit per un sol fil no seria
un substitut equivalent.

És una combinació de primitives conegudes, però no és el patró pare/fill
de `teoria/Pipes/ex1.c`. L'anotació és «revisar el disseny i explicar-lo», no
«pipe desconeguda, eliminar-la». Una alternativa ha de continuar desbloquejant
els clients inactius abans dels join.

**3. Temporització a esperarPort.**

El càlcul amb `gettimeofday`, tres estructures timeval i microsegons és el
bloc menys proper als Sx.c. `select` amb timeout té base a `teoria/Tema4.Select.pdf`; `alarm`
també és conegut de `teoria/Signals/S1.c`. Qualsevol canvi ha de conservar aquests dos requisits:

- MAP/STATUS no poden reiniciar indefinidament els cinc segons de consum.
- No es pot fer `sleep(5)` al mateix camí que ha d'atendre teclat i DOCKED.

No dono una substitució completa de gettimeofday per `temps.tv_sec = 5`
a cada iteració: seria més curta però canviaria el funcionament. Tampoc
proposo introduir altres APIs de rellotge absents dels materials.
Si es manté la funció actual, es pot reduir la repetició del càlcul temporal
amb una funció auxiliar petita, però això no elimina l'API no vista.

**4. Port i ordre de notificacions.**

`places_ocupades` i `num_atracats` s'incrementen i decrementen junts:
es pot conservar només `num_atracats`. Les cues amb arrays i desplaçaments
sí que s'assemblen a l'eliminació de clients de `teoria/Select/S5_sol.c`.

`espera_notificada` i `confirmarEspera` són més subtils: eviten que una
promoció feta per un altre fil enviï DOCKED abans que el fil d'arribada
hagi enviat WAIT. No són controls gratuïts. Revisar qui envia a cada socket
si es vol simplificar el protocol intern; no eliminar la bandera sola.

### Exemples de versions més properes a classe

Són alternatives documentades; no s'han aplicat als fonts.

**Comparació ASCII sense strcasecmp**, seguint la conversió de `teoria/Threads/S2.c`:

```c
int igualSenseMajuscules(char *a, char *b) {
    int i = 0;
    char ca, cb;

    while (a[i] != '\0' && b[i] != '\0') {
        ca = a[i];
        cb = b[i];
        if (ca >= 'a' && ca <= 'z') {
            ca = ca - 'a' + 'A';
        }
        if (cb >= 'a' && cb <= 'z') {
            cb = cb - 'a' + 'A';
        }
        if (ca != cb) {
            return 0;
        }
        i++;
    }
    return a[i] == b[i];
}

/* Abans: strcasecmp(paraules[1], "ITHACA") == 0 */
/* Despres: igualSenseMajuscules(paraules[1], "ITHACA") == 1 */
```

Retorna 1 quan són iguals: no substituir mecànicament només el nom de
strcasecmp deixant la comparació `== 0`. És una versió per a les paraules
ASCII del protocol, no una funció general per a majúscules amb accents.

**Nom en majúscules sense toupper:**

```c
for (i = 0; configuracio->nom[i] != '\0' && i < MIDA_NOM_ODYSSEUS - 1; i++) {
    nom[i] = configuracio->nom[i];
    if (nom[i] >= 'a' && nom[i] <= 'z') {
        nom[i] = nom[i] - 'a' + 'A';
    }
}
nom[i] = '\0';
```

**Buscar separadors dobles sense strstr:**

```c
int teSeparadorsDobles(char *text) {
    int i = 0;

    while (text[i] != '\0') {
        if (text[i] == '&' && text[i + 1] == '&') {
            return 1;
        }
        i++;
    }
    return 0;
}
```

El control existent pot usar `teSeparadorsDobles(dades) == 1`, mantenint
els altres controls de separador inicial/final, nombre de camps i longitud.

**Comparar DOCKED sense memcmp**, només dins un camí on s'ha validat
que la trama és del tipus correcte, FLAGS OK i longitud exactament 6:

```c
char resposta_port[7]; /* Declarar al principi de la funcio. */

memcpy(resposta_port, trama + POSICIO_DADES, 6);
resposta_port[6] = '\0';
if (strcmp(resposta_port, "DOCKED") != 0) {
    enviarNack(fd_espera, "INVALID_DATA");
    break;
}
```

**Fer explícit el retorn d'una interrupció**, sense operador ternari:

```c
if (resultat < 0 && errno == EINTR) {
    return 0;
}
return resultat;
```

Aquest és un canvi de llegibilitat, no una eliminació d'errno/EINTR:
`teoria/Select/select.c` ja mostra aquest tractament.

### Precisió sobre la revisió anterior

Els bucles de lectura de trames **sí que tenen suport explícit als apunts**:
la darrera diapositiva de `teoria/Tema4.Sockets.pdf` explica que read no garanteix
tots els bytes i indica llegir-ne la resta. També `teoria/Threads/S2.c` acumula lectures del fitxer.
Els comentaris antics d'aquests bucles s'han aclarit. La discrepància amb
el redactat de l'Annex II continua existint, però no s'ha de confondre amb
«això no ho hem vist a classe».

Amb els tres PDFs nous del Tema 3, queda corregida la valoració anterior:
`pthread_mutex_*` sí que té suport als apunts i és adequat per als recursos
compartits entre fils. No cal substituir els mutex de viatges, port, stock
ni registre de navegants per semàfors.

Els exemples docents mostren també printf, snprintf, cancel·lació amb fuites
o esperes actives en alguns exercicis. No es poden traslladar automàticament
al projecte: les restriccions de l'enunciat continuen aplicant-se. No he
trobat printf, scanf, system, popen o stat als fonts propis actuals.

### Prioritat de la revisió de classe

1. Conservar els mutex per als recursos compartits entre fils, tal com
   indica el Tema 3. No fer una migració a semàfors per motiu de nivell de classe.
2. Revisar el bloc temporal i justificar o substituir gettimeofday sense
   ajornar el consum amb cada ordre.
3. Fer més directe el registre i el tancament dels fils; no reescriure
   l'arquitectura sencera per reduir unes línies.
4. Si es vol la restricció estricta de funcions, substituir strcasecmp,
   toupper, strstr i memcmp per bucles o funcions de les llibreries disponibles.
   Conservar strtok sense exigir un separador propi d'una sessió.
5. Eliminar estat duplicat i reserves artificials, com ja indicava la primera
   revisió. Conservar els controls exigits d'errors, protocol i recursos.

Els comentaris de nivell de classe porten el prefix `REVISAR CLASSE`.
S'han retirat els que recomanaven auxiliars particulars de les sessions.
També s'han actualitzat els dos comentaris sobre E/S parcial a `trames.c`.
No s'ha canviat el codi executable ni els materials de teoria.


## Actualització amb el Tema 3 i semaphore_v2 a lib

Aquesta actualització substitueix qualsevol valoració anterior que deia que
els mutex no apareixien als apunts. S'han revisat:

- `teoria/Tema3-Concurrencia-ExclusioMutua-Mutex-CA-v1.pdf` (16 pàgines).
- `teoria/Tema3-MecanismesSincronitzacioExclusioMutua.pdf` (20 pàgines).
- `teoria/Tema3-Semafors-Sincronitzacio-CA-v1.pdf` (18 pàgines).
- `lib/semaphore_v2.h`, idèntic a `teoria/Semaphores/semaphore_v2.h`.

### Què canvia en la revisió

**Mutex: conservar-los.** Les diapositives 14–15 de
`teoria/Tema3-Concurrencia-ExclusioMutua-Mutex-CA-v1.pdf` mostren
`pthread_mutex_t`, `PTHREAD_MUTEX_INITIALIZER`, `pthread_mutex_lock`,
`pthread_mutex_unlock` i `pthread_mutex_destroy`, amb creació de fils i join.
No mostren una crida a `pthread_mutex_init` en aquests fragments: la seva
inicialització és estàtica. La funció usada al projecte és la inicialització
dinàmica de la mateixa API, no un mecanisme diferent de sincronització.

Les diapositives 2 i 18 de
`teoria/Tema3-Semafors-Sincronitzacio-CA-v1.pdf` especifiquen:

- Threads, exclusió mútua: pthread_mutex.
- Threads, sincronització: semàfors.
- Processos o fork: semàfors per exclusió mútua i sincronització.

Per tant, els mutex de viatges, port, stock i registre de fils **no s'han de
marcar per substituir-los perquè siguin fora de classe**. Es retira el comentari
antic d'Island que encara afirmava que aquesta API no era als apunts.
La llista enllaçada i la pipe de finalització es poden continuar revisant per
simplicitat del disseny, independentment que els seus mutex siguin correctes.

**semaphore_v2: disponible com a llibreria del curs.** Les diapositives 7–8
de `teoria/Tema3-Semafors-Sincronitzacio-CA-v1.pdf` mostren explícitament
la inclusió de semaphore_v2.h i l'ús de SEM_constructor, SEM_init, SEM_wait,
SEM_signal i SEM_destructor. Ara que també és a `lib/`, aquestes funcions
estan proporcionades al projecte; això les diferencia dels auxiliars
particulars com els separadors o macros d'impressió dels S5.

**Disponibilitat no implica integració automàtica.** El Makefile ja inclou
`-Ilib`, però cap font del projecte inclou actualment semaphore_v2.h. La
capçalera conté definicions de funcions amb enllaç extern, no només prototips:
si s'inclou en diversos .c del mateix executable pot donar errors de símbols
duplicats; l'include guard només evita repeticions dins una mateixa unitat
de compilació. Si algun dia s'utilitza, cal organitzar-ne la inclusió i
comprovar els retorns, inicialitzar una sola vegada i destruir quan ja no
quedin usuaris, tal com expliquen els apunts.

No s'ha canviat la sincronització ni s'ha afegit cap include de semaphore_v2:
amb el material nou, no hi ha motiu per fer aquesta substitució als bloquejos
actuals entre fils.

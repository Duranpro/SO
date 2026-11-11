# Problemes trobats i solucions

Registre per preparar la memòria de la pràctica. De moment només recull els problemes observats; les solucions s'hi afegiran quan s'hagin aplicat i comprovat.

## Testing de fase 3 — 9 de novembre de 2026

Origen: [report del testing](output/fase3/REPORT.md), executat en Docker amb Debian 12 i les imatges oficials. Aquest registre descriu el resultat d'aquella execució, no una nova comprovació de l'estat actual del codi.

### P1. CLAIM accepta un objecte d'una altra missió

- **Què es va provar:** reclamar la missió TridentOfPoseidon amb la imatge d'AegisFragment, segellada amb Scheria i reanomenada `trident_of_poseidon.png`.
- **Què va passar:** Ithaca va acceptar la prova i va concedir 1.500 gold.
- **Problema:** es comprovaven el nom del fitxer i el glif, però no que el contingut fos l'objecte de la missió assignada. Una transferència íntegra també pot transportar l'objecte equivocat.
- **Codi identificat al testing:** `src/ithaca/ithaca.c`, funció `gestionarClaim`.
- **Classificació:** problema funcional respecte als requisits.
- **Estat de la solució:** pendent de documentar quan s'hagi corregit i verificat.

### P2. Es recupera una partida que s'hauria de donar per perduda

- **Què es va provar:** morir per falta de provisions durant WAIT i, en un altre escenari, finalitzar amb CTRL+C perquè no quedaven recursos per continuar.
- **Què va passar:** després de reiniciar, Odysseus conservava la missió i l'objecte. En el cas de mort durant WAIT, també recuperava les provisions a zero.
- **Problema:** la persistència recuperava l'estat de la partida perduda en lloc de començar amb els recursos inicials, sense la missió ni l'objecte anterior.
- **Codi identificat al testing:** `src/odysseus/comandes.c`, gestió de la mort a `esperarPort`; `src/odysseus/odysseus.c`, desament i càrrega de l'estat.
- **Classificació:** problema funcional respecte als requisits. Es va reproduir en dos escenaris.
- **Estat de la solució:** pendent de documentar quan s'hagi corregit i verificat.

### P3. Es pot navegar sense haver rebut l'OBJECT complet

- **Què es va provar:** interrompre OBJECT després del primer bloc i executar després `CONNECT ITHACA` i `SAIL Aeaea`.
- **Què va passar:** Odysseus va informar de l'error de recepció, però posteriorment va sortir i va atracar a Aeaea amb la missió activa i sense l'objecte complet.
- **Problema:** la recepció incompleta es detectava, però no impedia començar el viatge d'aquella missió.
- **Codi identificat al testing:** `src/odysseus/comandes.c`, funcions `executarAccept` i `executarSail`.
- **Classificació:** problema funcional respecte als requisits.
- **Solució aplicada (10/11/2026):** si falla OBJECT, es tanca la connexió, s'alliberen les dades de la missió i es desa l'estat sense missió ni objecte vàlid. El missatge indica reconnectar i executar LIST VOYAGES; Ithaca ja marca l'assignació anterior com a fracassada en tornar a llistar. S'ha afegit una comprovació a executarSail que impedeix navegar amb una missió activa sense objecte rebut.
- **Verificació:** proves en Docker amb Debian 12 de mida incorrecta, MD5 incorrecte i tall durant OBJECT; reinici sense recuperar la missió fallida; acceptació posterior d'una altra missió; navegació sense missió; bloqueig d'un estat antic incomplet; recorregut complet amb entrega, reinicis i cobrament. Totes passen. El cas de MD5 també s'ha executat amb Valgrind sense errors detectats.
- **Estat de la solució:** corregit i verificat en aquests casos.

### P4. Dues recepcions amb el mateix nom interfereixen entre elles

- **Què es va provar:** mantenir una DELIVERY a mitges i iniciar una segona transferència d'una altra missió utilitzant deliberadament el mateix nom de fitxer entrant.
- **Què va passar:** la segona recepció va modificar el fitxer utilitzat per la primera, que va acabar amb `MD5_ERROR`.
- **Problema:** les dues recepcions compartien la mateixa ruta de destí.
- **Codi identificat al testing:** `src/comu/comun.c`, construcció de la ruta i obertura del fitxer a `prepararRecepcioFitxer`.
- **Classificació:** comprovació addicional amb una entrada anòmala. Les transferències simultànies de missions diferents amb els noms oficials van funcionar; no es considera equivalent al cas normal de la pràctica.
- **Estat de la solució:** pendent de decidir si es cobreix i de documentar qualsevol correcció verificada.

## Observacions de preparació i documentació

- **Rutes de les imatges:** en el moment del testing, les imatges eren a `data/objects/`, mentre que `voyages.dat` utilitzava `objects/...`. Les proves van seguir la preparació indicada al README i les van copiar a la ruta esperada. Cal recordar aquesta dependència en explicar com es prepara l'execució.
- **Retorn a Ithaca:** el document de testing incloïa `SAIL Ithaca`, però l'enunciat descrivia el retorn amb `CONNECT ITHACA`. Aquesta discrepància es va separar dels errors del programa.

Quan es resolgui un problema, s'afegiran al seu apartat el canvi aplicat, el motiu de la decisió i la prova que confirma la correcció. No s'ha de donar cap problema per solucionat només perquè hi hagi una proposta de canvi.

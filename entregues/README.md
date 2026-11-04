# Entregues

- `G22_F1.tar`: entrega original de la Fase 1, conservada com a referencia.
- `G22_F1_corregit.tar`: copia amb la correccio de les assignacions de literals
  a `char *` a `mostrarUsComanda()`, dins de `comandes.c`.

La versio corregida reserva el missatge amb `copiarText()`, comprova si la reserva
ha fallat i l'allibera amb `free()` despres d'utilitzar-lo, tambe si no es pot
generar el missatge de sortida.

El codi actual de les fases 1 i 2 es treballa a `src/`. Tambe s'han corregit els
casos de la Fase 2 a `src/island/island.c` i `src/odysseus/comandes.c`.
Les taules i els textos de mida fixa utilitzen arrays de caracters amb espai
propi; els punters que necessiten una cadena propia reserven i alliberen memoria.

La copia corregida de la Fase 1 s'ha extret en una carpeta temporal Linux i s'ha
compilat amb el seu propi Makefile, sense errors ni avisos. S'han comprovat els
onze missatges d'us i les fallades simulades de reserva de memoria.

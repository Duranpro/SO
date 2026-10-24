CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O0
CPPFLAGS = -Isrc/comu -Isrc/ithaca -Isrc/island -Isrc/odysseus -Ilib

COMUNS = build/comu/comun.o build/comu/trames.o build/comu/xarxa.o
NAVEGANTS = build/comu/gestio_navegants.o
ODYSSEUS = build/odysseus/odysseus.o build/odysseus/configuracio_odisseu.o build/odysseus/comandes.o
ITHACA = build/ithaca/ithaca.o build/ithaca/configuracio_itaca.o
ISLAND = build/island/island.o build/island/configuracio_illa.o
OBJECTES = $(sort $(COMUNS) $(NAVEGANTS) $(ODYSSEUS) $(ITHACA) $(ISLAND))
DEPENDENCIES = $(OBJECTES:.o=.d)

.PHONY: all clean valgrind valgrind-odysseus valgrind-ithaca valgrind-island

all: odysseus ithaca island

build/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

odysseus: $(ODYSSEUS) $(COMUNS) lib/sphragis.o
	$(CC) $(CFLAGS) $^ -o $@

ithaca: $(ITHACA) $(COMUNS) $(NAVEGANTS)
	$(CC) $(CFLAGS) $^ -o $@ -lpthread

island: $(ISLAND) $(COMUNS) $(NAVEGANTS) lib/sphragis.o
	$(CC) $(CFLAGS) $^ -o $@ -lpthread

valgrind: valgrind-odysseus

valgrind-odysseus: odysseus
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./odysseus config/odysseus/odysseus.dat

valgrind-ithaca: ithaca
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./ithaca config/ithaca.dat data/voyages.dat

valgrind-island: island
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./island config/illes/aeaea.dat data/stocks/Aeaea.db

clean:
	rm -f $(OBJECTES) $(DEPENDENCIES) odysseus ithaca island

-include $(DEPENDENCIES)

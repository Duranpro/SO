all: odysseus ithaca island

build/odysseus/odysseus.o: src/odysseus/odysseus.c src/odysseus/comandes.h src/comu/comun.h src/comu/tipus.h src/comu/trames.h src/comu/xarxa.h lib/sphragis.h src/odysseus/configuracio_odisseu.h
	mkdir -p build/odysseus
	gcc -Wall -Wextra -std=c99 -g -Isrc/comu -Isrc/ithaca -Isrc/island -Isrc/odysseus -Ilib -c src/odysseus/odysseus.c -o build/odysseus/odysseus.o

build/odysseus/configuracio_odisseu.o: src/odysseus/configuracio_odisseu.c src/odysseus/configuracio_odisseu.h src/comu/comun.h src/comu/tipus.h
	mkdir -p build/odysseus
	gcc -Wall -Wextra -std=c99 -g -Isrc/comu -Isrc/ithaca -Isrc/island -Isrc/odysseus -Ilib -c src/odysseus/configuracio_odisseu.c -o build/odysseus/configuracio_odisseu.o

build/odysseus/comandes.o: src/odysseus/comandes.c src/odysseus/comandes.h src/comu/comun.h src/comu/tipus.h src/comu/trames.h src/comu/xarxa.h lib/sphragis.h
	mkdir -p build/odysseus
	gcc -Wall -Wextra -std=c99 -g -Isrc/comu -Isrc/ithaca -Isrc/island -Isrc/odysseus -Ilib -c src/odysseus/comandes.c -o build/odysseus/comandes.o

build/ithaca/ithaca.o: src/ithaca/ithaca.c src/ithaca/configuracio_itaca.h src/comu/comun.h src/comu/tipus.h src/comu/trames.h src/comu/xarxa.h src/comu/gestio_navegants.h
	mkdir -p build/ithaca
	gcc -Wall -Wextra -std=c99 -g -Isrc/comu -Isrc/ithaca -Isrc/island -Isrc/odysseus -Ilib -c src/ithaca/ithaca.c -o build/ithaca/ithaca.o

build/ithaca/configuracio_itaca.o: src/ithaca/configuracio_itaca.c src/ithaca/configuracio_itaca.h src/comu/comun.h src/comu/tipus.h
	mkdir -p build/ithaca
	gcc -Wall -Wextra -std=c99 -g -Isrc/comu -Isrc/ithaca -Isrc/island -Isrc/odysseus -Ilib -c src/ithaca/configuracio_itaca.c -o build/ithaca/configuracio_itaca.o

build/island/island.o: src/island/island.c src/island/configuracio_illa.h src/comu/comun.h lib/sphragis.h src/comu/tipus.h src/comu/trames.h src/comu/xarxa.h src/comu/gestio_navegants.h
	mkdir -p build/island
	gcc -Wall -Wextra -std=c99 -g -Isrc/comu -Isrc/ithaca -Isrc/island -Isrc/odysseus -Ilib -c src/island/island.c -o build/island/island.o

build/island/configuracio_illa.o: src/island/configuracio_illa.c src/island/configuracio_illa.h src/comu/comun.h lib/sphragis.h src/comu/tipus.h
	mkdir -p build/island
	gcc -Wall -Wextra -std=c99 -g -Isrc/comu -Isrc/ithaca -Isrc/island -Isrc/odysseus -Ilib -c src/island/configuracio_illa.c -o build/island/configuracio_illa.o

build/comu/comun.o: src/comu/comun.c src/comu/comun.h
	mkdir -p build/comu
	gcc -Wall -Wextra -std=c99 -g -Isrc/comu -Isrc/ithaca -Isrc/island -Isrc/odysseus -Ilib -c src/comu/comun.c -o build/comu/comun.o

build/comu/trames.o: src/comu/trames.c src/comu/trames.h
	mkdir -p build/comu
	gcc -Wall -Wextra -std=c99 -g -Isrc/comu -Isrc/ithaca -Isrc/island -Isrc/odysseus -Ilib -c src/comu/trames.c -o build/comu/trames.o

build/comu/xarxa.o: src/comu/xarxa.c src/comu/xarxa.h
	mkdir -p build/comu
	gcc -Wall -Wextra -std=c99 -g -Isrc/comu -Isrc/ithaca -Isrc/island -Isrc/odysseus -Ilib -c src/comu/xarxa.c -o build/comu/xarxa.o

build/comu/gestio_navegants.o: src/comu/gestio_navegants.c src/comu/gestio_navegants.h src/comu/trames.h
	mkdir -p build/comu
	gcc -Wall -Wextra -std=c99 -g -Isrc/comu -Isrc/ithaca -Isrc/island -Isrc/odysseus -Ilib -c src/comu/gestio_navegants.c -o build/comu/gestio_navegants.o

odysseus: build/odysseus/odysseus.o build/odysseus/configuracio_odisseu.o build/odysseus/comandes.o build/comu/comun.o build/comu/trames.o build/comu/xarxa.o lib/sphragis.o
	gcc -Wall -Wextra -std=c99 -g build/odysseus/odysseus.o build/odysseus/configuracio_odisseu.o build/odysseus/comandes.o build/comu/comun.o build/comu/trames.o build/comu/xarxa.o lib/sphragis.o -o odysseus

ithaca: build/ithaca/ithaca.o build/ithaca/configuracio_itaca.o build/comu/comun.o build/comu/trames.o build/comu/xarxa.o build/comu/gestio_navegants.o
	gcc -Wall -Wextra -std=c99 -g build/ithaca/ithaca.o build/ithaca/configuracio_itaca.o build/comu/comun.o build/comu/trames.o build/comu/xarxa.o build/comu/gestio_navegants.o -o ithaca -lpthread

island: build/island/island.o build/island/configuracio_illa.o build/comu/comun.o build/comu/trames.o build/comu/xarxa.o build/comu/gestio_navegants.o lib/sphragis.o
	gcc -Wall -Wextra -std=c99 -g build/island/island.o build/island/configuracio_illa.o build/comu/comun.o build/comu/trames.o build/comu/xarxa.o build/comu/gestio_navegants.o lib/sphragis.o -o island -lpthread

valgrind: valgrind-odysseus

valgrind-odysseus: odysseus
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./odysseus config/odysseus/odysseus.dat

valgrind-ithaca: ithaca
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./ithaca config/ithaca.dat data/voyages.dat

valgrind-island: island
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./island config/illes/aeaea.dat data/stocks/Aeaea.db

clean:
	rm -f build/comu/*.o build/comu/*.d build/odysseus/*.o build/odysseus/*.d build/ithaca/*.o build/ithaca/*.d build/island/*.o build/island/*.d odysseus ithaca island

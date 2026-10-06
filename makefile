all: odysseus ithaca island

odysseus.o: odysseus.c comandes.h configuracio_odisseu.h comun.h tipus.h
	gcc -Wall -Wextra -std=c99 -pthread -g -O0 -c odysseus.c -o odysseus.o

ithaca.o: ithaca.c configuracio_itaca.h comun.h tipus.h trames.h xarxa.h
	gcc -Wall -Wextra -std=c99 -pthread -g -O0 -c ithaca.c -o ithaca.o

island.o: island.c configuracio_illa.h comun.h tipus.h sphragis.h trames.h xarxa.h
	gcc -Wall -Wextra -std=c99 -pthread -g -O0 -c island.c -o island.o

configuracio_odisseu.o: configuracio_odisseu.c configuracio_odisseu.h comun.h tipus.h
	gcc -Wall -Wextra -std=c99 -pthread -g -O0 -c configuracio_odisseu.c -o configuracio_odisseu.o

configuracio_itaca.o: configuracio_itaca.c configuracio_itaca.h comun.h tipus.h
	gcc -Wall -Wextra -std=c99 -pthread -g -O0 -c configuracio_itaca.c -o configuracio_itaca.o

configuracio_illa.o: configuracio_illa.c configuracio_illa.h comun.h tipus.h sphragis.h
	gcc -Wall -Wextra -std=c99 -pthread -g -O0 -c configuracio_illa.c -o configuracio_illa.o

comandes.o: comandes.c comandes.h comun.h tipus.h trames.h xarxa.h
	gcc -Wall -Wextra -std=c99 -pthread -g -O0 -c comandes.c -o comandes.o

comun.o: comun.c comun.h
	gcc -Wall -Wextra -std=c99 -pthread -g -O0 -c comun.c -o comun.o

trames.o: trames.c trames.h
	gcc -Wall -Wextra -std=c99 -pthread -g -O0 -c trames.c -o trames.o

xarxa.o: xarxa.c xarxa.h
	gcc -Wall -Wextra -std=c99 -pthread -g -O0 -c xarxa.c -o xarxa.o

odysseus: odysseus.o configuracio_odisseu.o comandes.o comun.o trames.o xarxa.o
	gcc -Wall -Wextra -std=c99 -pthread -g -O0 odysseus.o configuracio_odisseu.o comandes.o comun.o trames.o xarxa.o -o odysseus

ithaca: ithaca.o configuracio_itaca.o comun.o trames.o xarxa.o
	gcc -Wall -Wextra -std=c99 -pthread -g -O0 ithaca.o configuracio_itaca.o comun.o trames.o xarxa.o -o ithaca

island: island.o configuracio_illa.o comun.o trames.o xarxa.o sphragis.o
	gcc -Wall -Wextra -std=c99 -pthread -g -O0 island.o configuracio_illa.o comun.o trames.o xarxa.o sphragis.o -o island

valgrind: valgrind-odysseus

valgrind-odysseus: odysseus
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./odysseus odysseus.dat

valgrind-ithaca: ithaca
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./ithaca ithaca.dat voyages.dat

valgrind-island: island
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./island aeaea.dat stocks/Aeaea.db

clean:
	rm -f odysseus.o ithaca.o island.o configuracio_odisseu.o configuracio_itaca.o configuracio_illa.o comandes.o comun.o trames.o xarxa.o odysseus ithaca island

all: jeu

jeu: jeu.o
	gcc -o jeu jeu.o

jeu.o: jeu.c
	gcc -o jeu.o -c jeu.c -W -Wall -ansi -pedantic

clean:
	rm -rf *.o
	rm -rf jeu
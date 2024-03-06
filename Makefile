all:
	gcc -Ofast -march=native matrix.c -o ./matrix -lncurses

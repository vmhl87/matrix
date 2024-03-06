all:
	gcc -Ofast -march=native matrix.c -o ./matrix -lncursesw

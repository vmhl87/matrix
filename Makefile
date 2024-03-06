all:
	gcc -Ofast -march=native matrix.c -o ./matrix -lncursesw

install:
	cp ./matrix /usr/local/bin/

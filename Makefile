all:
	gcc -Ofast -march=native matrix.c -o ./matrix -lncursesw

install:
	sudo cp ./matrix /usr/local/bin/

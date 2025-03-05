all: minesweeper

minesweeper: minesweeper.c
	gcc minesweeper.c -o minesweeper.x86_64 -fsanitize=address -lncurses

clean:
	rm -f minesweeper.x86_64 *~

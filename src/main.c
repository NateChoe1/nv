#include <ctype.h>
#include <curses.h>
#include <unistd.h>

#include "buffers.h"
#include "display.h"

void edit(char *path) {
	FILE *file = fopen(path, "r");
	if (file == NULL)
		return;
	Buffer buff;
	if (readFile(file, &buff))
		return;
	gotoLine(4, &buff);
	gotoLine(2, &buff);
	redrawBuffer(buff);
	sleep(10);
}

int main(int argc, char **argv) {
	initscr();
	cbreak();
	noecho();

	for (int i = 1; i < argc; i++)
		edit(argv[i]);

	endwin();
}

#include <ctype.h>
#include <curses.h>
#include <stdlib.h>

#include "buffers.h"
#include "display.h"
#include "controls.h"

int normalCommand(Buffer *buff, int control, int *mode) {
	Line *currentLine = getCurrentLine(buff);
	switch (control) {
		case 'j':
			gotoLine(buff, buff->cursorLine + 1);
			break;
		case 'k':
			gotoLine(buff, buff->cursorLine - 1);
			break;
		case 'h':
			if (--buff->cursorPos < 0)
				buff->cursorPos = 0;
			break;
		case 'l':
			if (++buff->cursorPos > currentLine->len)
				buff->cursorPos = currentLine->len;
			break;
		case 'e' & 31:
			buff->scrollLine++;
			break;
		case 'y' & 31:
			buff->scrollLine--;
			break;
		case 'i':
			*mode = INSERT;
			break;
		case 'd':
			deleteCurrentLine(buff);
			break;
		case 'q':
			return 1;
	}
	return 0;
}

int insertCommand(Buffer *buff, int control, int *mode) {
	Line *currentLine = getCurrentLine(buff);
	switch (control) {
		case 'c' & 31:
			*mode = NORMAL;
			break;
		case 'a' & 31:
			appendLine(buff);
			break;
		case 'i' & 31:
			insertLine(buff);
			break;
		default:
			insertChar(currentLine, buff->cursorPos, control);
			buff->cursorPos++;
			break;
	}
	return 0;
}

char *modeString(int mode) {
	switch (mode) {
		case NORMAL:
			return "-- NORMAL --";
		case INSERT:
			return "-- INSERT --";
		default:
			return "";
	}
}

void edit(char *path) {
	FILE *file = fopen(path, "r");
	if (file == NULL)
		return;
	Buffer *buff = __builtin_alloca(sizeof(Buffer));
	//NOTE: This one line means that this code only works on gcc.
	if (readFile(file, buff))
		return;
	fclose(file);
	int mode = NORMAL;
	for (;;) {
		redrawBuffer(buff, modeString(mode));
		int control = getch();
		switch (mode) {
			case NORMAL:
				if (normalCommand(buff, control, &mode))
					return;
				break;
			case INSERT:
				if (insertCommand(buff, control, &mode))
					return;
				break;
		}
	}
}

int main(int argc, char **argv) {
	initscr();
	raw();
	noecho();

	for (int i = 1; i < argc; i++)
		edit(argv[i]);

	endwin();
}

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
			if (++buff->scrollLine >= buff->lines)
				buff->scrollLine = buff->lines - 1;
			if (buff->cursorLine < buff->scrollLine)
				gotoLine(buff, buff->scrollLine);
			goto scrolledToCursor;
		case 'y' & 31: {
			if (--buff->scrollLine < 0)
				buff->scrollLine = 0;
			int last = lastShown(buff);
			if (buff->cursorLine > last)
				gotoLine(buff, last);
			goto scrolledToCursor;
		}
		case 'i':
			*mode = INSERT;
			break;
		case 'd':
			deleteCurrentLine(buff);
			break;
		case '$':
			buff->cursorPos = currentLine->len;
			break;
		case '0':
			buff->cursorPos = 0;
			break;
		case 'g':
			buff->cursorPos = 0;
			gotoLine(buff, 0);
			break;
		case 'G':
			gotoLine(buff, buff->lines - 1);
			break;
		case 'x':
			deleteLineChar(currentLine, buff->cursorPos);
			break;
		case 'B':
			freeBuffer(buff);
			return 1;
		case 'b':
			writeBuffer(buff);
			break;
	}
	if (buff->cursorLine < buff->scrollLine)
		buff->scrollLine = buff->cursorLine;
	for (;;) {
		int last = lastShown(buff);
		if (last >= buff->cursorLine)
			break;
		buff->scrollLine++;
	}
scrolledToCursor:
	return 0;
}

int insertCommand(Buffer *buff, int control, int *mode) {
	Line *currentLine = getCurrentLine(buff);
	switch (control) {
		case 'c' & 31: case 0x1b:
			*mode = NORMAL;
			break;
		case 'a' & 31:
			appendLine(buff);
			break;
		case 'i' & 31:
			insertLine(buff);
			break;
		case '\n':
			splitLine(buff);
			break;
		case KEY_LEFT:
			if (--buff->cursorPos < 0)
				buff->cursorPos = 0;
			break;
		case KEY_RIGHT:
			if (++buff->cursorPos > currentLine->len)
				buff->cursorPos = currentLine->len;
			break;
		case KEY_UP:
			gotoLine(buff, buff->cursorLine - 1);
			break;
		case KEY_DOWN:
			gotoLine(buff, buff->cursorLine + 1);
			break;
		case KEY_BACKSPACE: case 0x7f:
			deleteChar(buff);
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
	Buffer *buff = __builtin_alloca(sizeof(Buffer));
	if (readFile(file, buff))
		return;
	if (file != NULL)
		fclose(file);
	buff->path = path;
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
	keypad(stdscr, true);

	for (int i = 1; i < argc; i++)
		edit(argv[i]);

	endwin();
}

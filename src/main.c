#include <ctype.h>
#include <curses.h>
#include <stdlib.h>

#include "buffers.h"
#include "display.h"
#include "controls.h"

void scrollBuffer(Buffer *buff) {
	if (buff->cursorLine < buff->scrollLine)
		buff->scrollLine = buff->cursorLine;
	for (;;) {
		int last = lastShown(buff);
		if (last >= buff->cursorLine)
			break;
		buff->scrollLine++;
	}
}

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
			if (buff->cursorPos > currentLine->len)
				buff->cursorPos = currentLine->len;
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
			currentLine = getCurrentLine(buff);
			buff->cursorPos = currentLine->len - 1;
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
		case 'o':
			appendLine(buff);
			*mode = INSERT;
			break;
		case 'O':
			insertLine(buff);
			*mode = INSERT;
			break;	
		case '}':
			for (;;) {
				if (currentLine->len == 0 ||
				    buff->cursorLine >= buff->lines)
					break;
				gotoLine(buff, buff->cursorLine + 1);
			}
			break;
		case '{':
			for (;;) {
				if (currentLine->len == 0 ||
				    buff->cursorLine <= 0)
					break;
				gotoLine(buff, buff->cursorLine - 1);
			}
			break;
	}
	scrollBuffer(buff);
scrolledToCursor:
	return 0;
}

int insertCommand(Buffer *buff, int control, int *mode) {
	Line *currentLine = getCurrentLine(buff);
	switch (control) {
		case 'c' & 31: case 0x1b:
			*mode = NORMAL;
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
		int oldLine = buff->cursorLine;
		int oldCursor = buff->cursorPos;
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
		if (buff->cursorLine != oldLine)
			updateCursorPos(buff);
		else if (buff->cursorPos != oldCursor)
			updateCursorChars(buff);
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

#include <curses.h>

#include "buffers.h"
#include "display.h"

void redrawBuffer(Buffer *buff, char *message) {
	clear();
	int y = 0;
	int currentLine = buff->scrollLine;
	int curx, cury;
	while (y < LINES - 1 && currentLine < buff->lines) {
		int x = 0;
		move(y, x);
		Line *line = getLine(currentLine, buff);
		if (currentLine == buff->cursorLine &&
		    buff->cursorPos >= line->len) {
			curx = line->len;
			cury = y;
		}
		for (int i = 0; i < line->len; i++) {
			if (currentLine == buff->cursorLine && buff->cursorPos == i) {
				curx = x;
				cury = y;
			}
			switch (line->data[i]) {
				case '\t':
					do {
						addch(' ');
						x++;
					} while (x % 8 != 0);
					break;
				default:
					addch(line->data[i]);
					x++;
			}
			if (x / COLS + 1 >= LINES)
				break;
		}
		y += x / COLS + 1;
		currentLine++;
	}
	mvaddstr(LINES - 1, 0, message);
	move(cury, curx);
}

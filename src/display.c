#include <curses.h>

#include "buffers.h"
#include "display.h"

void redrawBuffer(Buffer buff) {
	int y = 0;
	int currentLine = buff.scrollLine;
	int curx, cury;
	while (y < LINES && currentLine < buff.lines) {
		int x = 0;
		move(y, x);
		Line *line = getLine(currentLine, buff);
		for (int i = 0; i < line->len; i++) {
			if (y == buff.cursorLine) {
				if (buff.cursorPos == i ||
				    (line->len < buff.cursorPos &&
				     i == line->len)) {
					curx = x;
					cury = y;
				}
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
	move(cury, curx);
	refresh();
}

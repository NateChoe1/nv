#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define INITIAL_ALLOCATED_LINES 100
#define INITIAL_ALLOCATED_LINE_LEN 100

typedef uint8_t Attributes;
#define ESCAPED (1 << 0)

typedef struct {
	int len;
	int allocatedLen;
	char *data;
} Line;

typedef struct {
	int scrollLine;
	//the first line to display
	int cursorLine;
	//the line the cursor is on
	int cursorPos;
	//how far along that line the cursor is
	int lines;
	//the number of lines in the file

	int startAlloc;
	//the number of allocated lines in startLines
	Line *startLines;
	//the first cursorLine lines of the file
	int endAlloc;
	//the number of allocated lines in endLines
	Line *endLines;
	//all the lines of the file not in startLines

	int height;
	//height of the terminal, not the actual amount of rows displayed
	int displayed;
	//number of displayed rows
} Buffer;
/*
 * NOTE:
 * endLines stores the lines at the end of the buffer, so if there are 2 lines
 * after the cursor in the file and endlines has 5 allocated lines, the lines
 * would be in indexes 3 and 4.
 */

int readFile(FILE *file, Buffer *ret);
Line *getLine(int pos, Buffer buff);
Line *getCurrentLine(Buffer buff);
void gotoLine(int pos, Buffer *buff);

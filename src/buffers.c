#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "buffers.h"

int readFile(FILE *file, Buffer *ret) {
//returns 1 on error
	ret->scrollLine = 0;
	ret->cursorLine = 0;
	ret->cursorPos = 0;

	ret->startAlloc = INITIAL_ALLOCATED_LINES;
	ret->startLines = malloc(sizeof(Line) * ret->startAlloc);
	if (ret->startLines == NULL)
		goto startError;

	int lineCount = 0;
	int allocatedLines = INITIAL_ALLOCATED_LINES;
	Line *content = malloc(sizeof(Line) * allocatedLines);
	if (content == NULL)
		goto startError;
	for (;;) {
		int eofchar = fgetc(file);
		if (eofchar == EOF)
			break;
		ungetc(eofchar, file);

		int len = 0;
		int allocatedLen = INITIAL_ALLOCATED_LINE_LEN;
		char *data = malloc(sizeof(char) * allocatedLen);
		if (data == NULL)
			goto error;
		for (;;) {
			int c = fgetc(file);
			if (c == '\n' || c == EOF)
				break;
			while (len >= allocatedLen) {
				allocatedLen *= 2;
				char *newdata = realloc(data,
				               sizeof(char) * allocatedLen);
				if (newdata == NULL)
					goto error;
				data = newdata;
			}
			data[len++] = c;
		}
		while (lineCount >= allocatedLines) {
			allocatedLines *= 2;
			Line *newcontent = realloc(content,
			                  sizeof(Line) * allocatedLines);
			if (newcontent == NULL)
				goto error;
			content = newcontent;
		}
		content[lineCount].len = len;
		content[lineCount].allocatedLen = allocatedLen;
		content[lineCount].data = data;
		lineCount++;
	}

	memmove(content + allocatedLines - lineCount, content,
			sizeof(Line) * lineCount);
	ret->endAlloc = allocatedLines;
	ret->endLines = content;
	ret->lines = lineCount;
	return 0;

error:
	for (int i = 0; i < lineCount; i++)
		free(content[i].data);
	free(content);
startError:
	free(ret->startLines);
	return 1;
}

Line *getLine(int pos, Buffer buff) {
	if (pos < buff.cursorLine)
		return buff.startLines + pos;
	return buff.endLines + (buff.endAlloc - (buff.lines - pos));
}

Line *getCurrentLine(Buffer buff) {
	return buff.endLines + (buff.endAlloc - (buff.lines - buff.cursorLine));
}

Line *insertLine(Buffer buff) {
//returns the newly created line
	buff.lines++;
	Line *line = getCurrentLine(buff);
	line->len = 0;
	line->allocatedLen = INITIAL_ALLOCATED_LINE_LEN;
	line->data = malloc(sizeof(char) * line->allocatedLen);
	return line;
}

int reallocStart(Buffer *buff, int newlyAllocated) {
	Line *newStart = realloc(buff->startLines,
	                         newlyAllocated * sizeof(Line));
	if (newStart == NULL)
		return 1;
	buff->startLines = newStart;
	buff->startAlloc = newlyAllocated;
	return 0;
}

int reallocEnd(Buffer *buff, int newlyAllocated) {
	Line *newLines = malloc(newlyAllocated * sizeof(Line));
	if (newLines == NULL)
		return 1;
	int toEnd = buff->lines - buff->cursorLine;
	memcpy(newLines + newlyAllocated - toEnd,
	       buff->endLines + buff->endAlloc - toEnd,
	       sizeof(Line) * toEnd);
	buff->endLines = newLines;
	buff->endAlloc = newlyAllocated;
	return 0;
}

void gotoLine(int pos, Buffer *buff) {
	if (pos < 0 || pos >= buff->lines || pos == buff->cursorLine)
		return;
	if (pos < buff->cursorLine) {
		int necessary = buff->lines - pos;
		if (buff->endAlloc < necessary) {
			int newlyAllocated = buff->endAlloc;
			while (newlyAllocated < necessary)
				newlyAllocated *= 2;
			reallocEnd(buff, newlyAllocated);
		}
		memcpy(buff->endLines + buff->endAlloc - necessary,
		       buff->startLines + pos,
		       (buff->cursorLine - pos) * sizeof(Line));
	}
	else {
		if (buff->startAlloc < pos) {
			int newlyAllocated = buff->startAlloc;
			while (newlyAllocated < pos) {
				newlyAllocated *= 2;
			}
			reallocStart(buff, newlyAllocated);
		}
		int endStart = buff->lines - buff->cursorLine;
		memcpy(buff->startLines + buff->cursorLine,
		       buff->endLines + buff->endAlloc - endStart,
		       (pos - buff->cursorLine) * sizeof(Line));
	}
	buff->cursorLine = pos;
}

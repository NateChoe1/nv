#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define MIN(a, b) ((a < b) ? a:b)

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

Line *getLine(int pos, Buffer *buff) {
	if (pos < buff->cursorLine)
		return buff->startLines + pos;
	return buff->endLines + (buff->endAlloc - (buff->lines - pos));
}

Line *getCurrentLine(Buffer *buff) {
	return buff->endLines + (buff->endAlloc - (buff->lines - buff->cursorLine));
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

void gotoLine(Buffer *buff, int pos) {
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

Line *insertLine(Buffer *buff) {
	while (buff->lines - buff->cursorLine + 1 >= buff->endAlloc)
		reallocEnd(buff, buff->endAlloc *= 2);
	buff->lines++;
	Line *line = getCurrentLine(buff);
	line->len = 0;
	line->allocatedLen = INITIAL_ALLOCATED_LINE_LEN;
	line->data = malloc(sizeof(char) * line->allocatedLen);
	return line;
}

Line *appendLine(Buffer *buff) {
	while (buff->lines - buff->cursorLine + 1 >= buff->endAlloc)
		reallocEnd(buff, buff->endAlloc *= 2);
	Line *oldCursor = getCurrentLine(buff);
	buff->lines++;
	Line *line = getCurrentLine(buff);
	line->len = 0;
	line->allocatedLen = INITIAL_ALLOCATED_LINE_LEN;
	line->data = malloc(sizeof(char) * line->allocatedLen);
	Line backup;
	memcpy(&backup, line, sizeof(Line));
	memcpy(line, oldCursor, sizeof(Line));
	memcpy(oldCursor, &backup, sizeof(Line));
	gotoLine(buff, buff->cursorLine + 1);
	return line;
}
//TODO: This function sucks, make it better.

void freeLine(Line *line) {
	free(line->data);
}

static Line *fakeDeleteCurrentLine(Buffer *buff) {
//Deletes a line from the buffer, doesn't free it, and returns it. The line may
//get overwritten by another line if you call gotoLine, so either copy the line
//before using it, or just don't call gotoLine.
	Line *currentLine = getCurrentLine(buff);
	buff->lines--;
	return currentLine;
}

void deleteCurrentLine(Buffer *buff) {
	freeLine(fakeDeleteCurrentLine(buff));
}

int insertChar(Line *line, int pos, char c) {
	pos = MIN(line->len, pos);
	if (line->allocatedLen <= line->len + 1) {
		int newlyAllocated = line->allocatedLen;
		while (newlyAllocated <= line->len + 1)
			newlyAllocated *= 2;
		char *newdata = realloc(line->data,
		                        sizeof(char) * newlyAllocated);
		if (newdata == NULL)
			return 1;
		line->allocatedLen = newlyAllocated;
		line->data = newdata;
	}
	memmove(line->data + pos + 1, line->data + pos, line->len - pos);
	line->data[pos] = c;
	line->len++;
	return 0;
}

int splitLine(Buffer *buff) {
	appendLine(buff);
	Line *prev = getLine(buff->cursorLine - 1, buff);
	Line *curr = getCurrentLine(buff);
	int split = MIN(buff->cursorPos, prev->len);
	int newlen = prev->len - split;
	if (newlen > curr->allocatedLen) {
		int newlyAllocated = curr->allocatedLen;
		while (newlyAllocated < newlen)
			newlyAllocated *= 2;
		char *newdata = realloc(curr->data,
		                        newlyAllocated * sizeof(char));
		if (newdata == NULL)
			return 1;
		curr->data = newdata;
		curr->allocatedLen = newlyAllocated;
	}
	memcpy(curr->data, prev->data + split, newlen * sizeof(char));
	prev->len = split;
	curr->len = newlen;
	buff->cursorPos = 0;
	return 1;
}

void deleteLineChar(Line *line, int pos) {
	if (pos < 0)
		return;
	line->len--;
	if (pos > line->len)
		pos = line->len;
	memmove(line->data + pos, line->data + pos + 1, line->len - pos);
}

void deleteChar(Buffer *buff) {
	if (buff->cursorPos > 0) {
		deleteLineChar(getCurrentLine(buff), --buff->cursorPos);
	}
	else if (buff->cursorLine > 0) {
		Line *deleted = fakeDeleteCurrentLine(buff);
		Line *append = __builtin_alloca(sizeof(Line));
		memcpy(append, deleted, sizeof(Line));
		gotoLine(buff, buff->cursorLine - 1);

		Line *current = getCurrentLine(buff);
		buff->cursorPos = current->len;
		if (current->len + append->len > current->allocatedLen) {
			int newlyAllocated = current->allocatedLen;
			while (current->len + append->len < newlyAllocated)
				newlyAllocated *= 2;
			char *newdata = realloc(current->data,
			                newlyAllocated * sizeof(char));
			if (newdata == NULL) {
				freeLine(append);
				return;
			}
			current->data = newdata;
		}
		memcpy(current->data + current->len, append->data, append->len);
		current->len += append->len;
		freeLine(append);
	}
}

void writeBuffer(Buffer *buff) {
	FILE *file = fopen(buff->path, "w");
	for (int i = 0; i < buff->lines; i++) {
		Line *line = getLine(i, buff);
		fwrite(line->data, sizeof(char), line->len, file);
		fputc('\n', file);
	}
}

void freeBuffer(Buffer *buff) {
	for (int i = 0; i < buff->lines; i++)
		free(getLine(i, buff)->data);
	free(buff->startLines);
	free(buff->endLines);
}

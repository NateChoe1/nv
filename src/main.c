#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nv.h"

int readFile(FILE *file, Buffer *ret) {
//returns 1 on error
	ret->scrollLine = 0;
	ret->cursorLine = 0;
	ret->cursorPos = 0;

	ret->startAlloc = 100;
	ret->startLines = malloc(ret->startAlloc * sizeof(*ret->startLines));
	if (ret->startLines == NULL)
		goto startError;

	int lineCount = 0;
	int allocatedLines = 100;
	Line *content = malloc(sizeof(Line) * allocatedLines);
	if (content == NULL)
		goto startError;
	for (;;) {
		int eofchar = fgetc(file);
		if (eofchar == EOF)
			break;
		ungetc(eofchar, file);

		int len = 0;
		int allocatedLen = 100;
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

int main(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		FILE *file = fopen(argv[i], "r");
		if (file == NULL)
			continue;
		Buffer buff;
		readFile(file, &buff);
		for (int i = 0; i < buff.lines; i++) {
			Line *line = getLine(i, buff);
			fwrite(line->data, sizeof(char), line->len, stdout);
			putchar('\n');
		}
	}
}

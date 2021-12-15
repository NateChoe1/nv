void redrawBuffer(Buffer *buff, char *message);
//message is displayed on the bottom right of the terminal
int displayedLength(Line *line);
int lastShown(Buffer *buff);

void updateCursorPos(Buffer *buff);
void updateCursorChars(Buffer *buff);

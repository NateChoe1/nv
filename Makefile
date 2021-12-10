SRC = $(shell find -name "*.c")
OBJ = $(shell echo $(SRC) | sed "s/src/work/g" | sed "s/\.c/.o/g")
LIBS = $(shell ncurses6-config --libs)
CC := gcc
CFLAGS := -O2 -Wall -Wpedantic -Werror
CFLAGS += $(shell ncurses6-config --cflags)
INSTALLDIR := /usr/bin
OUT = nv

build/$(OUT): $(OBJ)
	$(CC) $(OBJ) -o build/$(OUT) $(LIBS)

work/%.o: src/%.c
	$(CC) $(CFLAGS) $< -c -o $@

install: build/$(OUT)
	cp build/$(OUT) $(INSTALLDIR)

uninstall: $(INSTALLDIR)/$(OUT)
	rm $(INSTALLDIR)/$(OUT)

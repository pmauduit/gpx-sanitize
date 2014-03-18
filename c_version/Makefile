#!/usr/bin/make -f

BIN:=parser
BIN_DEP:=sanitizer_util.c parser.c
BIN_OBJ:=$(BIN_DEP:%.c=%.o)

INSTALL_DIR:=../../sanitize

CC=$(shell which gcc)
GDB=$(shell which gdb)
VALGRIND=$(shell which valgrind)
STRIP=$(shell which strip)

CFLAGS=-Wall -I/usr/include/libxml2/ -g
LDFLAGS=-lm -lxml2

.PHONY: install valgrind clean

all: $(BIN)

# Build rules

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS) $(LDFLAGS)

$(BIN): $(BIN_OBJ)
	$(CC) -o $(BIN) sanitizer_util.o parser.c $(CFLAGS) $(LDFLAGS)

# Installation rules

install: all | $(STRIP)
	cp $(BIN) $(INSTALL_DIR)
	$(STRIP) $(INSTALL_DIR)/$(BIN)

clean:
	rm -f $(BIN) $(BIN_OBJ) out*.gpx

uninstall: clean
	rm -f $(INSTALL_DIR)/$(BIN)

# Run & Instrumentation tools rules
run: $(BIN)
	./$(BIN) sample.gpx

valgrind: $(BIN)
	$(VALGRIND) ./$(BIN) sample.gpx

debug: $(BIN)
	$(GDB) --args ./$(BIN) sample.gpx

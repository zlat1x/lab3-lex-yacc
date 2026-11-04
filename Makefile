
CC = gcc
CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Wno-unused-function -Isrc -Ibuild

.PHONY: all run test clean

all: build/minic

build:
	mkdir -p build

build/parser.tab.c build/parser.tab.h &: src/parser.y src/ast.h | build
	bison -d -Wall -Werror -o build/parser.tab.c src/parser.y

build/lex.yy.c: src/lexer.l build/parser.tab.h | build
	flex -o $@ $<

build/minic: build/parser.tab.c build/lex.yy.c src/ast.c src/runtime.c src/main.c src/ast.h src/runtime.h
	$(CC) $(CFLAGS) build/parser.tab.c build/lex.yy.c src/ast.c src/runtime.c src/main.c -o $@

run: all
	./build/minic examples/demo.c

test: all
	python3 tests/test_minic.py

clean:
	rm -rf build

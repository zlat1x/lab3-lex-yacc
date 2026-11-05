
CC = gcc
CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Wno-unused-function -Isrc -Ibuild

.PHONY: all run test generate generate-ast clean

all: build/minic

build:
	mkdir -p build

build/parser.tab.c build/parser.tab.h &: src/parser.y src/ast.h | build
	bison -d -Wall -Werror -o build/parser.tab.c src/parser.y

build/lex.yy.c: src/lexer.l build/parser.tab.h | build
	flex -o $@ $<

build/minic: build/parser.tab.c build/lex.yy.c src/ast.c src/runtime.c src/codegen.c src/json.c src/main.c src/ast.h src/runtime.h src/codegen.h src/json.h
	$(CC) $(CFLAGS) build/parser.tab.c build/lex.yy.c src/ast.c src/runtime.c src/codegen.c src/json.c src/main.c -o $@

run: all
	./build/minic examples/demo.c

test: all
	python3 -m unittest discover -s tests -p 'test_*.py' -v

generate: all
	mkdir -p out
	./build/minic --emit-python examples/demo.c out/demo.py

generate-ast: all
	mkdir -p out
	./build/minic --emit-json examples/demo.c out/demo-ast.json

clean:
	rm -rf build

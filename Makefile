test: test1
	./test1 100000

CFLAGS=-g -Wall -std=c++11 -Wno-nonnull-compare

iv: Makefile iv.cpp config.cpp handle_command.cpp
	g++ ${CFLAGS}  -o $@ `pkg-config --cflags --libs ncursesw`  iv.cpp

test1: test1.cpp string.h Makefile
	g++ ${CFLAGS} -o $@ test1.cpp

.PHONY: test

test: test1
	./test1 100000

iv: Makefile iv.cpp config.cpp handle_command.cpp
	g++ -g -Wall -std=c++11 -o $@ `pkg-config --cflags --libs ncursesw`  iv.cpp

test1: test1.cpp string.h
	g++ -g -Wall -std=c++11 -o $@ $^

.PHONY: test

iv: Makefile iv.cpp config.cpp handle_command.cpp
	g++ -Wall -std=c++11 -o $@ `pkg-config --cflags --libs ncursesw`  iv.cpp

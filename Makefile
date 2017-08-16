iv: Makefile iv.cpp config.cpp handle_command.cpp
	g++ -std=c++11 -o $@ -lncurses iv.cpp

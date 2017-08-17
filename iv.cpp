#include <algorithm>
#include <cctype> /* isprint */
#include <cstdlib> /* exit() */
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>

#ifndef CTRL
#define CTRL(c) ((c) & 037)
#endif

const int tab_size = 8;

struct buffer
{
	typedef std::map<int, std::string> chars_type;
	chars_type chars;
	chars_type::iterator start, cursor;
	size_t cursor_x;
	std::string filename;

	buffer() : chars(), start(chars.begin()), cursor(chars.begin()) {}
	buffer(const std::string _filename) : filename(_filename)
	{
		r();
	}

	template <class Iterator>
	void assign(Iterator begin, Iterator end)
	{
		chars.clear();
		std::string *line = NULL;
		auto push = [this, &line](char c) {
			if (line == NULL)
				line = &chars.insert(std::make_pair(chars.size(), std::string())).first->second;
			line->push_back(c);
		};
		while (begin != end) {
			switch (*begin) {
			case '\t':
				for (int i = 0; i < tab_size; i++)
					push(' ');
				break;
			case '\n':
				push(*begin);
				line = NULL;
				break;
			default:
				push(*begin);
				break;
			}
			++begin;
		}
		start = cursor = chars.begin();
		cursor_x = 0;
	}

	void adjust_start()
	{
		while (cursor->first >= start->first + LINES - 2)
			++start;
		while (cursor->first < start->first)
			--start;
	}

	void set_start(int _start)
	{
		start = --chars.upper_bound(_start);
		while (cursor->first >= start->first + LINES - 2)
			--cursor;
		while (cursor->first < start->first)
			++cursor;
	}

	void read(std::istream &stream)
	{
		assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
	}

	void write(std::ostream &stream)
	{
		for (auto c : chars)
			stream << c.second;
	}

	void r(std::string _filename = std::string())
	{
		if (_filename.empty())
			_filename = filename;
		std::ifstream stream(_filename);
		stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		read(stream);
	}

	void o(const std::string &_filename = std::string())
	{
		if (_filename.empty())
			throw std::invalid_argument(":o needs an argument");
		std::ifstream stream(_filename);
		stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		read(stream);
		filename = _filename;
	}

	void w(std::string _filename = std::string())
	{
		if (_filename.empty())
			_filename = filename;
		std::ofstream stream(_filename);
		stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		write(stream);
	}

	void saveas(const std::string &_filename = std::string())
	{
		if (_filename.empty())
			throw std::invalid_argument(":saveas needs an argument");
		std::ofstream stream(_filename);
		stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		write(stream);
		filename = _filename;
	}
} buf;

enum class mode_type {
	NORMAL,
	INSERT,
	COMMAND
} mode;

struct screen_initializer
{
	screen_initializer() { initscr(); }
};

struct Window : public screen_initializer
{
	WINDOW *file;
	WINDOW *status;
	WINDOW *cmdline;
	std::string command;

	Window();
	~Window();
	int input() { return wgetch(file); }
	void update();
	void update_file();
	void update_status();
	void update_cmdline();
	void activate_window();
} win;

Window::Window()
	: screen_initializer(),
	file(newwin(LINES - 2, COLS, 0, 0)),
	status(newwin(1, COLS, LINES - 2, 0)),
	cmdline(newwin(1, COLS, LINES - 1, 0))
{
	clear();
	noecho();
	cbreak();
	for (WINDOW *w: {stdscr, file, status, cmdline})
		keypad(w, TRUE);
}

Window::~Window()
{
	endwin();
}

static void activate(WINDOW *w)
{
	wrefresh(w);
}

void Window::update()
{
	clear();
	update_file();
	update_status();
	update_cmdline();
	for (WINDOW *w: {file, status, cmdline})
		wnoutrefresh(w);
	activate_window();
	doupdate();
}

void Window::update_file()
{
	wclear(file);
	int line = buf.start->first;
	for (buffer::chars_type::iterator i = buf.start; i != buf.chars.end(); i++) {
		wmove(file, line++ - buf.start->first, 0);
		waddnstr(file, i->second.c_str(), COLS);
	}
	wmove(file, buf.cursor->first - buf.start->first, buf.cursor_x);
}

void Window::update_status()
{
	wclear(status);
	waddstr(status, buf.filename.empty() ? "Untitled" : buf.filename.c_str());
	if (mode == mode_type::INSERT)
		waddstr(status, " ---INSERT---");
	wrefresh(status);
}

void Window::update_cmdline()
{
	wclear(cmdline);
	if (mode == mode_type::COMMAND) {
		wprintw(cmdline, ":");
		wprintw(cmdline, command.c_str());
	}
	wrefresh(cmdline);
}

void Window::activate_window()
{
	switch (mode) {
	case mode_type::NORMAL:
		activate(file);
		break;
	case mode_type::INSERT:
		activate(file);
		break;
	case mode_type::COMMAND:
		activate(cmdline);
		break;
	}
}

#include "handle_command.cpp"

struct key_bindings
{
	typedef std::pair<const int, std::function<void ()>> binding;
	std::map<binding::first_type, binding::second_type> bindings;
	key_bindings(const std::initializer_list<binding> &_bindings) : bindings(_bindings) { }
	bool handle(int key);
	void add_command_binding(int key, const char *cmd);
};

bool key_bindings::handle(int key)
{
	bool ret = bindings.count(key);
	if (ret)
		bindings[key]();
	return ret;
}

void key_bindings::add_command_binding(int key, const char *cmd)
{
	bindings.emplace(key, std::bind(handle_command, cmd));
}

key_bindings any_bindings({});

key_bindings normal_bindings({});
key_bindings insert_bindings({});
key_bindings command_bindings({});

void handle_key()
{
	int c = win.input();
	do {
		if (any_bindings.handle(c))
			break;
		if (mode == mode_type::NORMAL && normal_bindings.handle(c))
			break;
		if (mode == mode_type::INSERT && insert_bindings.handle(c))
			break;
		if (mode == mode_type::COMMAND && command_bindings.handle(c))
			break;
		if (mode == mode_type::INSERT && std::isprint(c)) {
			buf.cursor->second.insert(buf.cursor_x, 1, c);
			buf.cursor_x++;
			win.update_file();
			break;
		}
		if (mode == mode_type::COMMAND && std::isprint(c)) {
			char string[] = {(char)c, '\0'};
			win.command.push_back(c);
			wprintw(win.cmdline, string);
			wrefresh(win.cmdline);
			break;
		}
		flash();
	} while (false);
}

bool quit_on_sigint = false;

void sigint_handler(int)
{
	if (quit_on_sigint)
		std::exit(0);
	mode = mode_type::NORMAL;
	win.update();
}

int main(int argc, char **argv)
{
	using namespace std::placeholders;

	signal(SIGINT, sigint_handler);

	if (argc > 2) {
		std::cerr << "Usage: " << argv[0] << " [file]" << std::endl;
		return 1;
	} else if (argc < 2){
		wprintw(win.file, "IV -- simple vi clone");
		win.update_status();
	} else if (argv[1] == std::string("--dumpkeys")) {
		quit_on_sigint = true;
		while (true) {
			int c = win.input();
			wprintw(stdscr, "%d %s\n", c, key_name(c));
			wrefresh(stdscr);
		}
		return 0;
	} else {
		buf.o(argv[1]);
		win.update();
	}

	auto map = std::bind(&key_bindings::add_command_binding, &any_bindings, _1, _2);
	auto nmap = std::bind(&key_bindings::add_command_binding, &normal_bindings, _1, _2);
	auto imap = std::bind(&key_bindings::add_command_binding, &insert_bindings, _1, _2);
	auto cmap = std::bind(&key_bindings::add_command_binding, &command_bindings, _1, _2);

#include "config.cpp"

	while (true) {
		try {
			handle_key();
		} catch (const std::exception &exc) {
			wclear(win.status);
			wprintw(win.status, exc.what());
			wrefresh(win.status);
		}
	}
}

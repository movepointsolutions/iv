#include <algorithm>
#include <cctype> /* isprint */
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <list>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <ncurses.h>
#include <stdio.h>

struct buffer
{
	std::list<char> chars;
	std::list<char>::iterator start, cursor;
	std::string filename;

	buffer() : chars(), start(chars.begin()), cursor(chars.begin()) {}
	buffer(const std::string _filename) : filename(_filename)
	{
		r();
	}

	void read(std::istream &stream)
	{
		chars.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
		start = cursor = chars.begin();
	}

	void write(std::ostream &stream)
	{
		std::copy(chars.begin(), chars.end(), std::ostreambuf_iterator<char>(stream));
	}

	void r(std::string _filename = std::string())
	{
		if (_filename.empty())
			_filename = filename;
		std::ifstream stream(_filename);
		read(stream);
	}

	void o(const std::string &_filename = std::string())
	{
		if (_filename.empty())
			throw std::invalid_argument(":o needs an argument");
		std::ifstream stream(_filename);
		read(stream);
		filename = _filename;
	}

	void w(std::string _filename = std::string())
	{
		if (_filename.empty())
			_filename = filename;
		std::ofstream stream(_filename);
		write(stream);
	}

	void saveas(const std::string &_filename = std::string())
	{
		if (_filename.empty())
			throw std::invalid_argument(":saveas needs an argument");
		std::ofstream stream(_filename);
		write(stream);
		filename = _filename;
	}
} buf;

struct screen_initializer
{
	screen_initializer() { initscr(); }
};

class Window : public screen_initializer
{
	WINDOW *file_;
	WINDOW *status_;
	WINDOW *cmdline_;
public:
	Window();
	~Window();
	WINDOW *file() { return file_; }
	WINDOW *status() { return status_; }
	WINDOW *cmdline() { return cmdline_; }
	int input() { return wgetch(file()); }
	void update();
} win;

Window::Window()
	: screen_initializer(),
	file_(newwin(LINES - 2, COLS, 0, 0)),
	status_(newwin(1, COLS, LINES - 2, 0)),
	cmdline_(newwin(1, COLS, LINES - 1, 0))
{
	clear();
	noecho();
	cbreak();
	for (WINDOW *w: {stdscr, file(), status(), cmdline()})
		keypad(w, TRUE);
}

Window::~Window()
{
	endwin();
}

void Window::update()
{
	clear();
	for (WINDOW *w: {file(), status(), cmdline()})
		wrefresh(w);
}

void handle_command(const std::string &command)
{
	std::istringstream args(command);
	std::string arg0;
	args >> arg0;
	if (arg0 == "r") {
		std::string filename;
		if (args >> filename)
			buf.r(filename);
		else
			buf.r();
	} else if (arg0 == "w") {
		std::string filename;
		if (args >> filename)
			buf.w(filename);
		else
			buf.w();
	} else if (arg0 == "o") {
		std::string filename;
		if (args >> filename)
			buf.o(filename);
		else
			buf.o();
	} else if (arg0 == "saveas") {
		std::string filename;
		if (args >> filename)
			buf.saveas(filename);
		else
			buf.saveas();
	}
}

enum class mode_type {
	NORMAL,
	INSERT,
	COMMAND
} mode;

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

int main(int argc, char **argv)
{
	std::string command;
	using namespace std::placeholders;

	key_bindings any_bindings({
		{12, []() { win.update(); }} /* C-L */
	});

	key_bindings normal_bindings({
		{':', [&command]() {
			mode = mode_type::COMMAND;
			wclear(win.cmdline());
			wprintw(win.cmdline(), ":");
			wrefresh(win.cmdline());
			command = std::string();
		}}
	});

	key_bindings insert_bindings({});
	key_bindings command_bindings({});

	auto map = std::bind(&key_bindings::add_command_binding, &any_bindings, _1, _2);
	auto nmap = std::bind(&key_bindings::add_command_binding, &normal_bindings, _1, _2);
	auto imap = std::bind(&key_bindings::add_command_binding, &insert_bindings, _1, _2);
	auto cmap = std::bind(&key_bindings::add_command_binding, &command_bindings, _1, _2);

#include "config.cpp"

	wprintw(win.file(), "Test");
	while (true) {
		int c = win.input();
		wprintw(win.status(), "%d %s ", c, key_name(c));
		do {
			if (any_bindings.handle(c))
				break;
			if (mode == mode_type::NORMAL && normal_bindings.handle(c))
				break;
			if (mode != mode_type::NORMAL && c == 27) {
				mode = mode_type::NORMAL;
				win.update();
				break;
			}
			if (mode == mode_type::COMMAND && std::isprint(c)) {
				char string[] = {(char)c, '\0'};
				command.push_back(c);
				wprintw(win.cmdline(), string);
				wrefresh(win.cmdline());
				break;
			}
			if (mode == mode_type::COMMAND && c == '\n') {
				mode = mode_type::NORMAL;
				wclear(win.cmdline());
				wrefresh(win.cmdline());
				handle_command(command);
				break;
			}
		} while (false);
	}
}

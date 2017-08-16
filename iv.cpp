#include <algorithm>
#include <cctype> /* isprint */
#include <cstdlib> /* exit() */
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
#include <signal.h>
#include <stdio.h>

const int tab_size = 8;

struct buffer
{
	typedef std::map<std::pair<int, int>, char> chars_type;
	chars_type chars;
	chars_type::iterator start, cursor;
	std::string filename;

	buffer() : chars(), start(chars.begin()), cursor(chars.begin()) {}
	buffer(const std::string _filename) : filename(_filename)
	{
		r();
	}

	template <class Iterator>
	void assign(Iterator begin, Iterator end)
	{
		int x = 0, y = 0;
		chars.clear();
		while (begin != end) {
			if (*begin == '\t') {
				for (int i = 0; i < tab_size; i++)
					chars.insert(std::make_pair(std::make_pair(y, x++), ' '));
			} else if (*begin == '\n') {
				chars.insert(std::make_pair(std::make_pair(y, x), *begin));
				x = 0;
				y++;
			} else {
				chars.insert(std::make_pair(std::make_pair(y, x), *begin));
				x++;
			}
			++begin;
		}
		start = cursor = chars.begin();
	}

	void set_cursor(chars_type::iterator _cursor)
	{
		if (_cursor != chars.end()) {
			cursor = _cursor;
			while (cursor->first.first >= start->first.first + LINES - 2)
				do {
					++start;
				} while (start->first.second);
			while (cursor->first.first < start->first.first)
				do {
					--start;
				} while (start->first.second);
		}
	}

	void read(std::istream &stream)
	{
		assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
		start = cursor = chars.begin();
	}

	void write(std::ostream &stream)
	{
		std::ostreambuf_iterator<char> it(stream);
		for (auto c : chars)
			*it++ = c.second;
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

class Window : public screen_initializer
{
	WINDOW *file_;
	WINDOW *status_;
	WINDOW *cmdline_;
	std::string command_;
public:
	Window();
	~Window();
	WINDOW *file() { return file_; }
	WINDOW *status() { return status_; }
	WINDOW *cmdline() { return cmdline_; }
	std::string &command() { return command_; }
	int input() { return wgetch(file()); }
	void update();
	void update_file();
	void update_status();
	void update_cmdline();
	void activate_window();
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
	for (WINDOW *w: {file(), status(), cmdline()})
		wnoutrefresh(w);
	activate_window();
	doupdate();
}

void Window::update_file()
{
	int curx = -1, cury = -1;
	wclear(file());
	wmove(file(), 0, 0);
	for (buffer::chars_type::iterator i = buf.start; i != buf.chars.end(); i++)
		waddch(file(), i->second);
	wmove(file(), buf.cursor->first.first - buf.start->first.first, buf.cursor->first.second - buf.start->first.second);
}

void Window::update_status()
{
	wclear(status());
	waddstr(status(), buf.filename.empty() ? "Untitled" : buf.filename.c_str());
	wrefresh(status());
}

void Window::update_cmdline()
{
	wclear(cmdline());
	if (mode == mode_type::COMMAND) {
		wprintw(cmdline(), ":");
		wprintw(cmdline(), command().c_str());
	}
	wrefresh(cmdline());
}

void Window::activate_window()
{
	switch (mode) {
	case mode_type::NORMAL:
		activate(file());
		break;
	case mode_type::INSERT:
		activate(file());
		break;
	case mode_type::COMMAND:
		activate(cmdline());
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

key_bindings any_bindings({
	{12, []() { win.update(); }} /* C-L */
});

key_bindings normal_bindings({
	{':', []() {
		mode = mode_type::COMMAND;
		wclear(win.cmdline());
		wprintw(win.cmdline(), ":");
		wrefresh(win.cmdline());
		win.command() = std::string();
	}}
});

key_bindings insert_bindings({});
key_bindings command_bindings({
	{127, []() {
		if (win.command().empty())
			mode = mode_type::NORMAL;
		else
			win.command().pop_back();
		win.update_cmdline();
		win.activate_window();
	}}
});

void handle_key()
{
	int c = win.input();
	wprintw(win.status(), "%d %s ", c, key_name(c));
	do {
		if (any_bindings.handle(c))
			break;
		if (mode == mode_type::NORMAL && normal_bindings.handle(c))
			break;
		if (mode == mode_type::INSERT && insert_bindings.handle(c))
			break;
		if (mode == mode_type::COMMAND && command_bindings.handle(c))
			break;
		if (mode != mode_type::NORMAL && c == 27) {
			mode = mode_type::NORMAL;
			win.update();
			break;
		}
		if (mode == mode_type::COMMAND && std::isprint(c)) {
			char string[] = {(char)c, '\0'};
			win.command().push_back(c);
			wprintw(win.cmdline(), string);
			wrefresh(win.cmdline());
			break;
		}
		if (mode == mode_type::COMMAND && c == '\n') {
			mode = mode_type::NORMAL;
			wclear(win.cmdline());
			wrefresh(win.cmdline());
			handle_command(win.command());
			break;
		}
		flash();
	} while (false);
}

void sigint_handler(int)
{
	win.update();
}

int main(int argc, char **argv)
{
	using namespace std::placeholders;

	signal(SIGINT, sigint_handler);

	auto map = std::bind(&key_bindings::add_command_binding, &any_bindings, _1, _2);
	auto nmap = std::bind(&key_bindings::add_command_binding, &normal_bindings, _1, _2);
	auto imap = std::bind(&key_bindings::add_command_binding, &insert_bindings, _1, _2);
	auto cmap = std::bind(&key_bindings::add_command_binding, &command_bindings, _1, _2);

#include "config.cpp"

	wprintw(win.file(), "IV -- simple vi clone");
	win.update_status();
	while (true) {
		try {
			handle_key();
		} catch (const std::exception &exc) {
			wclear(win.status());
			wprintw(win.status(), exc.what());
			wrefresh(win.status());
		}
	}
}

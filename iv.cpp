#include <cctype> /* isprint */
#include <functional>
#include <initializer_list>
#include <map>
#include <string>
#include <utility>
#include <ncurses.h>
#include <stdio.h>

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
} window;

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
	std::string x = "[" + command + "]";
	wprintw(window.file(), x.c_str());
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
		{12, []() { window.update(); }}
	});

	key_bindings normal_bindings({
		{':', [&command]() {
			mode = mode_type::COMMAND;
			wclear(window.cmdline());
			wprintw(window.cmdline(), ":");
			wrefresh(window.cmdline());
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

	wprintw(window.file(), "Test");
	while (true) {
		int c = window.input();
		wprintw(window.status(), "%d %s ", c, key_name(c));
		do {
			if (any_bindings.handle(c))
				break;
			if (mode == mode_type::NORMAL && normal_bindings.handle(c))
				break;
			if (mode != mode_type::NORMAL && c == 27) {
				mode = mode_type::NORMAL;
				window.update();
				break;
			}
			if (mode == mode_type::COMMAND && std::isprint(c)) {
				char string[] = {(char)c, '\0'};
				command.push_back(c);
				wprintw(window.cmdline(), string);
				wrefresh(window.cmdline());
				break;
			}
			if (mode == mode_type::COMMAND && c == 10) {
				mode = mode_type::NORMAL;
				wclear(window.cmdline());
				wrefresh(window.cmdline());
				handle_command(command);
				break;
			}
		} while (false);
	}
}

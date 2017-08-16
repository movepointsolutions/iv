#include <functional>
#include <initializer_list>
#include <map>
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
};

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
	for (WINDOW *w: {file(), status(), cmdline()})
		wrefresh(w);
}

struct environment
{
	Window window;
};

struct key_bindings
{
	typedef std::pair<const int, std::function<void ()>> binding;
	std::map<binding::first_type, binding::second_type> bindings;
	key_bindings(const std::initializer_list<binding> &_bindings) : bindings(_bindings) { }
	void handle(int key) { if (bindings.count(key)) bindings[key](); }
};

int main(int argc, char **argv)
{
	{
		environment e;
		key_bindings b({
			{12, [&e]() { e.window.update(); }}
		});
		wprintw(e.window.status(), "Test");
		while (true) {
			int c = e.window.input();
			//printf("%d %s\n", c, key_name(c));
			b.handle(c);
		}
	}
}

#include <initializer_list>
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

int main(int argc, char **argv)
{
	int c;
	{
		Window win;
		wprintw(win.status(), "Test");
		wrefresh(win.status());
		c = win.input();
	}
	printf("%d\n", c);
}

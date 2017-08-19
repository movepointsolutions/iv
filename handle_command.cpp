void handle_command(const std::string &command)
{
	std::istringstream args(command);
	std::string arg0, arg1;
	args >> arg0;
	if (arg0 == "q" || arg0 == "quit") {
		exit(0);
	} else if (arg0 == "r") {
		std::string filename;
		if (args >> filename)
			buf.r(filename);
		else
			buf.r();
		win.update_file();
	} else if (arg0 == "w") {
		std::string filename;
		if (args >> filename)
			buf.w(filename);
		else
			buf.w();
	} else if (arg0 == "wq") {
		buf.w();
		exit(0);
	} else if (arg0 == "o" || arg0 == "e") {
		std::string filename;
		if (args >> filename)
			buf.o(filename);
		else
			buf.o();
		win.update_file();
		win.update_status();
	} else if (arg0 == "saveas") {
		std::string filename;
		if (args >> filename)
			buf.saveas(filename);
		else
			buf.saveas();
		win.update_status();
	} else if (arg0 == "cursor") {
		std::string direction;
		if (!(args >> direction))
			throw std::invalid_argument(":cursor needs an argument");
		if (direction == "left" && buf.cursor_x > 0)
			buf.cursor_x--;
		else if (direction == "right" && buf.cursor_x < buf.cursor->second.size() - 1 - (mode == mode_type::NORMAL))
			buf.cursor_x++;
		else if (direction == "up" && buf.cursor != buf.chars.begin()) {
			--buf.cursor;
			buf.adjust_start();
		} else if (direction == "down") {
			++buf.cursor;
			if (buf.cursor == buf.chars.end())
				--buf.cursor;
			buf.adjust_start();
		}
		win.update_file();
	} else if (arg0 == "0") {
		buf.cursor = buf.chars.begin();
		win.update_file();
	} else if (arg0 == "100") {
		buf.cursor = buf.chars.find(100);
		if (buf.cursor == buf.chars.end())
			buf.cursor = --buf.chars.upper_bound(100);
		win.update_file();
	} else if (arg0 == "refresh") {
		win.update();
	} else if (arg0 == "mode") {
		std::string _mode;
		if (args >> _mode) {
			mode = _mode == "normal" ? mode_type::NORMAL :
			       _mode == "insert" ? mode_type::INSERT :
			                           mode_type::COMMAND;
			if (mode == mode_type::COMMAND)
				win.command = std::string();
			win.update();
		}
	} else if (arg0 == "page") {
		std::string direction;
		if (args >> direction) {
			if (direction == "up") {
				buf.set_start(std::max(0, buf.start->first - LINES + 2));
			} else if (direction == "down") {
				buf.set_start(buf.start->first + LINES - 2);
			}
			win.update_file();
		}
	} else if (arg0 == "halfpage") {
		std::string direction;
		if (args >> direction) {
			if (direction == "up") {
				buf.set_start(std::max(0, buf.start->first - LINES / 2 + 1));
			} else if (direction == "down") {
				buf.set_start(buf.start->first + LINES / 2 - 1);
			}
			win.update_file();
		}
	} else if (arg0 == "n_0") {
		buf.cursor_x = 0;
		win.update_file();
	} else if (arg0 == "n_$") {
		buf.cursor_x = buf.cursor->second.size() - 2;
		win.update_file();
	} else if (arg0 == "n_i") {
		buf.cursor_x = std::min(buf.cursor->second.size() - 1, buf.cursor_x);
		mode = mode_type::INSERT;
		win.update();
	} else if (arg0 == "n_a") {
		buf.cursor_x = std::min(buf.cursor->second.size() - 1, buf.cursor_x + 1);
		mode = mode_type::INSERT;
		win.update();
	} else if (arg0 == "n_I") {
		buf.cursor_x = 0;
		mode = mode_type::INSERT;
		win.update();
	} else if (arg0 == "n_A") {
		buf.cursor_x = buf.cursor->second.size() - 1;
		mode = mode_type::INSERT;
		win.update();
	} else if (arg0 != "misc") {
		throw std::invalid_argument("unknown command: " + arg0);
	} else if (!(args >> arg1)) {
		throw std::invalid_argument("need argument: " + arg0);
	} else if (arg1 == "i:backspace") {
		if (buf.cursor_x > 0) {
			buf.cursor->second.erase(buf.cursor_x--, 1);
			win.update_file();
		}
	} else if (arg1 == "c:backspace") {
		if (win.command.empty())
			mode = mode_type::NORMAL;
		else
			win.command.pop_back();
		win.update_cmdline();
		win.activate_window();
	} else if (arg1 == "escape") {
		if (mode == mode_type::INSERT)
			buf.cursor_x = std::min((int)buf.cursor->second.size() - 2, std::max((int)buf.cursor_x, 1) - 1);
		mode = mode_type::NORMAL;
		win.update();
	} else if (arg1 == "c:return") {
		mode = mode_type::NORMAL;
		wclear(win.cmdline);
		wrefresh(win.cmdline);
		handle_command(win.command);
	}
}

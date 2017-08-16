void handle_command(const std::string &command)
{
	std::istringstream args(command);
	std::string arg0;
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
		if (direction == "left")
			buf.set_cursor(buf.chars.find(std::make_pair(buf.cursor->first.first, buf.cursor->first.second - 1)));
		else if (direction == "right")
			buf.set_cursor(buf.chars.find(std::make_pair(buf.cursor->first.first, buf.cursor->first.second + 1)));
		else if (direction == "up")
			buf.set_cursor(buf.chars.find(std::make_pair(buf.cursor->first.first - 1, buf.cursor->first.second)));
		else if (direction == "down")
			buf.set_cursor(buf.chars.find(std::make_pair(buf.cursor->first.first + 1, buf.cursor->first.second)));
		win.update_file();
	}
}

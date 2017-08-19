nmap('h', "cursor left");
nmap('l', "cursor right");
nmap('k', "cursor up");
nmap('j', "cursor down");
nmap(KEY_LEFT, "cursor left");
nmap(KEY_RIGHT, "cursor right");
nmap(KEY_UP, "cursor up");
nmap(KEY_DOWN, "cursor down");
imap(KEY_LEFT, "cursor left");
imap(KEY_RIGHT, "cursor right");
imap(KEY_UP, "cursor up");
imap(KEY_DOWN, "cursor down");
map(CTRL('L'), "refresh");

nmap('i', "mode insert");
nmap(':', "mode command");
imap(127, "misc i:backspace");
cmap(127, "misc c:backspace");
map(27, "misc escape");
cmap('\n', "misc c:return");

nmap(339, "page up");
nmap(338, "page down");
imap(339, "page up");
imap(338, "page down");
nmap(21, "halfpage up");
nmap(4, "halfpage down");
imap(21, "halfpage up");
imap(4, "halfpage down");

nmap('0', "n_0");
nmap('$', "n_$");
nmap('i', "n_i");
nmap('a', "n_a");
nmap('I', "n_I");
nmap('A', "n_A");

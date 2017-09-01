#include <cassert>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include "list.h"

// simple stream editor using iv::string as text container
struct f1_type
{
	iv::list<char> text;
	void operator ()(char c)
	{
		if (std::isalpha(c))
			text.push_back(c);
	}
	operator std::string() const
	{
		std::string ret;
		ret.assign(text.begin(), text.end());
		return ret;
	}
} f1;

struct f2_type : public std::string
{
	void operator ()(char c)
	{
		if (std::isalpha(c))
			push_back(c);
	}
} f2;

int main(int argc, char **argv)
{
	std::cout << "Test1" << std::endl;
	{
		iv::list<char> s1;
		assert(s1.begin() == s1.end());
		iv::list<char>::const_iterator end = s1.begin();
		s1.push_back('\0');
		assert(end == s1.end());
		iv::list<char>::const_iterator j = s1.begin();
		assert(j != s1.end());
		assert(j == s1.root());
		assert(!j.left());
		assert(!j.right());
		++j;
		assert(j == end);
		//assert(!s1.begin().expired());
		//assert(!j.expired());
		s1.push_back('\001');
		iv::list<char>::const_iterator k = s1.begin();
		//if (k != end)
			//std::cerr << (int)*k << " " << k.lock() << std::endl;
		++k;
		//if (k != end)
			//std::cerr << (int)*k << " " << k.lock() << std::endl;
		assert(k != end);
		++k;
		//if (k != end)
			//std::cerr << (int)*k << " " << k.lock() << std::endl;
		assert(k == s1.end());
	}
	std::ifstream random("/dev/urandom", std::ios::binary);
	char c;
	int i = 0;
	while (random.get(c)) {
		if (i % 1000 == 0)
			std::cout << i << std::endl;
		f1(c);
		f2(c);
		if (i++ >= std::atoi(argv[1]))
			break;
	}
	if (static_cast<std::string>(f1) != f2) {
		std::cerr << "mismatch: char " << c << " (" << (int)c << ") pos " << i << " ";
		std::cerr << "\"" << static_cast<std::string>(f1) << "\" != \"" << f2 << "\"" << std::endl;
		return 1;
	}
	return 0;
}

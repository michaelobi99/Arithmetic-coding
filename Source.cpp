#include <iostream>

char getChar() {
	static char c;
	std::cin.ignore();
	std::cin.clear();
	std::cin.get(c);
	return c;
}

int main() {
	char ch{};
	const char* str = "I love c, c++, python, and rust\n";
	ch = getChar();
	while (ch != 'v') {
		printf("%s", str);
		ch = getChar();
	}
}
#ifndef LUPS_LEXER_H
#define LUPS_LEXER_H

#include <string>
#include "token.h"

class Lexer {
public:
	Lexer(const std::string &input);
	Token next_token();

private:
	std::string input_;
	int pos_;
	char ch_;
	int read_pos_;

	std::string read_ident();
	std::string read_number();
	std::string read_string();

	void read_char();
	void skip_whitespace();

	char peek_char();
};

#endif

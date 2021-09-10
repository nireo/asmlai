#include "lexer.h"
#include "token.h"
#include <cctype>

Lexer::Lexer(const std::string &input) {
	input_ = input;
	read_pos_ = 0;
	read_char();
}

bool is_letter(char ch) {
	return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_';
}

bool is_digit(char ch) { return '0' <= ch && ch <= '9'; }

void Lexer::read_char() {
	if (read_pos_ >= (int)input_.size())
		ch_ = 0;
	else
		ch_ = input_[read_pos_];

	pos_ = read_pos_;
	read_pos_++;
}

Token new_token(TokenType type, char ch) {
	return Token{type, std::string(1, ch)};
}

TokenType lookup_ident(const std::string &str) {
	auto keyword = tokentypes::KEYWORDS.find(str);
	if (keyword == tokentypes::KEYWORDS.end())
		return tokentypes::IDENT;
	return keyword->second;
}

Token Lexer::next_token() {
	Token tok;

	skip_whitespace();

	switch (ch_) {
	case '=':
		if (peek_char() == '=') {
			tok = Token{.type = tokentypes::EQ, .literal = "=="};
			read_char();
		} else {
			tok = new_token(tokentypes::ASSIGN, ch_);
		}
		read_char();
		break;
	case ';':
		tok = new_token(tokentypes::SEMICOLON, ch_);
		read_char();
		break;
	case ':':
		tok = new_token(tokentypes::COLON, ch_);
		read_char();
		break;
	case '(':
		tok = new_token(tokentypes::LPAREN, ch_);
		read_char();
		break;
	case ')':
		tok = new_token(tokentypes::RPAREN, ch_);
		read_char();
		break;
	case '[':
		tok = new_token(tokentypes::LBRACKET, ch_);
		read_char();
		break;
	case ']':
		tok = new_token(tokentypes::RBRACKET, ch_);
		read_char();
		break;
	case ',':
		tok = new_token(tokentypes::COMMA, ch_);
		read_char();
		break;
	case '+':
		tok = new_token(tokentypes::PLUS, ch_);
		read_char();
		break;
	case '-':
		tok = new_token(tokentypes::MINUS, ch_);
		read_char();
		break;
	case '!':
		if (peek_char() == '=') {
			tok = Token{.type = tokentypes::NEQ, .literal = "!="};
			read_char();
		} else {
			tok = new_token(tokentypes::BANG, ch_);
		}
		read_char();
		break;
	case '/':
		tok = new_token(tokentypes::SLASH, ch_);
		read_char();
		break;
	case '*':
		tok = new_token(tokentypes::ASTERISK, ch_);
		read_char();
		break;
	case '<':
		tok = new_token(tokentypes::LT, ch_);
		read_char();
		break;
	case '>':
		tok = new_token(tokentypes::GT, ch_);
		read_char();
		break;
	case '{':
		tok = new_token(tokentypes::LBRACE, ch_);
		read_char();
		break;
	case '}':
		tok = new_token(tokentypes::RBRACE, ch_);
		read_char();
		break;
	case '"':
		tok.type = tokentypes::STRING;
		tok.literal = read_string();
		read_char();
		break;
	case 0:
		tok.literal = "";
		tok.type = tokentypes::EOFF;
		read_char();
		break;
	default:
		if (is_letter(ch_)) {
			tok.literal = read_ident();
			tok.type = lookup_ident(tok.literal);
		} else if (is_digit(ch_)) {
			tok.type = tokentypes::INT;
			tok.literal = read_number();
		} else {
			tok = new_token(tokentypes::ILLEGAL, ch_);
			read_char();
		}
		break;
	}

	return tok;
}

std::string Lexer::read_string() {
	int start_pos = pos_ + 1;
	for (;;) {
		read_char();
		if (ch_ == '"' || ch_ == 0) {
			break;
		}
	}

	return input_.substr(start_pos, pos_ - start_pos);
}

void Lexer::skip_whitespace() {
	while (ch_ == ' ' || ch_ == '\t' || ch_ == '\n' || ch_ == '\r')
		read_char();
}

std::string Lexer::read_ident() {
	int start_pos = pos_;
	while (is_letter(ch_)) {
		read_char();
	}

	return input_.substr(start_pos, pos_ - start_pos);
}

std::string Lexer::read_number() {
	int start_pos = pos_;
	while (is_digit(ch_)) {
		read_char();
	}

	return input_.substr(start_pos, pos_ - start_pos);
}

char Lexer::peek_char() {
	if (read_pos_ >= (int)input_.size())
		return 0;
	else
		return input_[read_pos_];
}

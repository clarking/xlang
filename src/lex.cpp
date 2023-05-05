/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


// lexical analyzer


#include <iostream>

#include <stack>
#include <algorithm>
#include "token.hpp"
#include "log.hpp"
#include "lex.hpp"

namespace xlang {

	void Lexer::init() {

		if (!file_exists(file.path)) {
			Log::error(file.name, "No such file of directory");
			std::exit(EXIT_FAILURE);
		}

		key_tokens = {{"asm",      KEY_ASM},
					  {"break",    KEY_BREAK},
					  {"char",     KEY_CHAR},
					  {"const",    KEY_CONST},
					  {"continue", KEY_CONTINUE},
					  {"do",       KEY_DO},
					  {"double",   KEY_DOUBLE},
					  {"else",     KEY_ELSE},
					  {"extern",   KEY_EXTERN},
					  {"float",    KEY_FLOAT},
					  {"for",      KEY_FOR},
					  {"global",   KEY_GLOBAL},
					  {"goto",     KEY_GOTO},
					  {"if",       KEY_IF},
					  {"int",      KEY_INT},
					  {"long",     KEY_LONG},
					  {"record",   KEY_RECORD},
					  {"return",   KEY_RETURN},
					  {"short",    KEY_SHORT},
					  {"sizeof",   KEY_SIZEOF},
					  {"static",   KEY_STATIC},
					  {"void",     KEY_VOID},
					  {"while",    KEY_WHILE}};

		std::sort(symbols.begin(), symbols.end());
	}

	std::string Lexer::get_filename() {
		return file.name;
	}

	bool Lexer::is_eof(char ch) {
		return (ch <= 0);
	}

	bool Lexer::file_read(SourceFile &src) {
		std::ifstream input(src.path);

		if (input.is_open()) {
			std::string ln;
			while (std::getline(input, ln)) {
				src.content += ln;
			}
			input.close();
			src.loaded = true;
			buffer_index = 0;
			return true;
		}

		return false;
	}

	bool Lexer::file_exists(const std::string &path) {

		if (path.empty())
			return false;

		std::ifstream test(path);
		if (test.is_open()) {
			test.close();
			return true;
		}

		return false;
	}

	char Lexer::get_next_char() {

		if (!file.loaded) {
			file_read(file);
			eof_flag = false;
		}

		if (buffer_index < file.content.size())
			return file.content[buffer_index++];

		eof_flag = true;
		unget_flag = false;
		return 0;
	}

	void Lexer::unget_char() {

		// putback

		buffer_index--;
		if (buffer_index <= 0) {
			buffer_index = 0;
			unget_flag = true;
			return;
		}

		unget_flag = false;
	}

	void Lexer::consume_chars_till(char end) {
		char ch = end;
		while (!is_eof(ch)) {
			ch = get_next_char();
			if (ch == end) {
				return;
			}
			lexeme.push_back(ch);
			col++;
			if (ch == '\n')
				line++;
		}
	}

	void Lexer::consume_chars_till(std::string chars) {
		char ch;
		while (!is_eof(ch = get_next_char())) {
			if (chars.find(ch)) {
				return;
			}
			else {
				lexeme.push_back(ch);
				col++;
				if (ch == '\n')
					line++;
			}
		}
	}

	void Lexer::consume_chars_till_symbol() {
		char ch;
		while (!is_eof(ch = get_next_char())) {
			if (symbol(ch)) {
				unget_char();
				return;
			}
			else {
				lexeme.push_back(ch);
				col++;
				if (ch == '\n')
					line++;
			}
		}
		unget_char();
	}

	/*
	symbol : one of
	  ! % ^ ~ & * ( ) - + = [ ] { } | : ; < > , . / \ ' " @ # ` ?
	*/
	bool Lexer::symbol(char ch) {
		return std::binary_search(symbols.begin(), symbols.end(), ch);
	}

	/*
	digit : one of
	    0 1 2 3 4 5 6 7 8 9
	*/
	bool Lexer::digit(char ch) {
		return ((ch - '0' >= 0) && (ch - '0' <= 9));
	}

	/*
	nonzero-digit : one of
		1 2 3 4 5 6 7 8 9
	*/
	bool Lexer::nonzero_digit(char ch) {
		return ((ch - '0' >= 1) && (ch - '0' <= 9));
	}

	/*
	octal-digit : one of
	  0 1 2 3 4 5 6 7
	*/
	bool Lexer::octal_digit(char ch) {
		return ((ch - '0' >= 0) && (ch - '0' <= 7));
	}

	/*
	hexadecimal-digit : one of
		0 1 2 3 4 5 6 7 8 9
		a b c d e f
		A B C D E F
	*/
	bool Lexer::hexadecimal_digit(char ch) {
		return (((ch - '0' >= 0) && (ch - '0' <= 9)) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'));
	}

	/*
	non-digit : one of
	  _ $ a b c d e f g h i j k l m n o p q r s t u v w x y z
	  A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
	*/
	bool Lexer::non_digit(char ch) {
		return (ch == '_' || ch == '$' || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'));
	}

	/*
	comment :
	  / / any character except newline
	  / * any character * /
	*/
	bool Lexer::comment() {
		char ch = get_next_char();
		char peek;
		int multicomment_line = 0, mulcmnt_col = 0;
		bool is_comment_complete = false;

		if (is_eof(ch)) {
			unget_char();
			return false;
		}

		// single line comment '//'
		if (ch == '/') {
			col++;
			do {
				ch = get_next_char();
				col++;
				if (is_eof(ch)) {
					unget_char();
					break;
				}
			} while (ch != '\n');

		}
		else if (ch == '*') {    //multi line comment / *  * /
			multicomment_line = line;
			mulcmnt_col = col;
			col++;

			// any character
			while (!is_eof(ch = get_next_char())) {
				col++;
				if (ch == '\n') {
					line++;
					col = 1;
				}
				else if (ch == '*') {
					peek = get_next_char();
					if (peek == '/') {
						col++;
						is_comment_complete = true;
						break;
					}
					else if (peek == '\n') {
						line++;
						col = 1;
					}
					else {
						if (is_eof(peek)) {
							Log::error(get_filename(), "incomplete comment", multicomment_line, mulcmnt_col);

							return false;
						}
					}
				}
			}
			if (is_comment_complete) {
				return true;
			}
			else {
				unget_char();
				Log::error(get_filename(), "incomplete comment", multicomment_line, mulcmnt_col);

				return false;
			}
		}
		else {
			unget_char();
			return false;
		}

		line++;
		return true;
	}

	Token Lexer::make_token(TokenId tok1) {

		// assign lexeme, location and return that Token

		Token tok;
		tok.number = tok1;
		tok.string = lexeme;

		if (col == (unsigned) lexeme.size())
			tok.loc.col = 1;
		else
			tok.loc.col = col - lexeme.size();

		tok.loc.line = line;
		return tok;
	}

	Token Lexer::make_token(std::string lexm, TokenId tok1) {
		Token tok;
		tok.number = tok1;
		tok.string = lexm;

		if (col == (unsigned) lexm.size())
			tok.loc.col = 1;
		else
			tok.loc.col = col - lexm.size();

		tok.loc.line = line;
		return tok;
	}

	Token Lexer::literal() {

		char ch = get_next_char();
		char peek;
		Token tok;

		if (is_eof(ch))
			tok.number = END;
		else {
			if (ch == '0' || nonzero_digit(ch)) {
				unget_char();
				tok = integer_literal();

				peek = get_next_char();
				if (is_eof(peek))
					tok.number = END;
				else {
					if (symbol(peek))
						unget_char();
				}
			}
			else if (ch == '\'')
				tok = character_literal();
			else if (ch == '"')
				tok = string_literal();
		}

		lexeme.clear();
		return tok;
	}

	Token Lexer::character_literal() {

		char ch = get_next_char();
		char peek;
		Token tok;

		if (is_eof(ch))
			tok.number = END;
		else {

			if (ch == '"') {
				lexeme.clear();
				col++;
				tok = make_token(LIT_CHAR);
			}
			else {
				unget_char();
				c_char_sequence();

				tok = make_token(LIT_CHAR);

				if (error_flag) {
					peek = get_next_char();
					if (peek == '\\') {
						consume_chars_till("\n\'");
						Log::error_at(tok.loc, "invalid character incomplete escape sequence", lexeme);

					}
					else if (peek == '\n') {
						consume_chars_till("\n\'");
						Log::error_at(tok.loc, "missing terminating character", lexeme);
					}
					else {
						consume_chars_till("\n\'");
						Log::error_at(tok.loc, "invalid character ", lexeme);
					}
					//	error_count++;
				}
			}
		}

		lexeme.clear();
		return tok;
	}

	void Lexer::c_char_sequence() {

		char ch = get_next_char();
		char peek;

		if (is_eof(ch)) {
			eof_flag = true;
			return;
		}

		if (ch == '\\') {
			peek = get_next_char();

			if (is_eof(peek)) {
				eof_flag = true;
				return;
			}
			else if (peek == '\n') {
				error_flag = true;
				unget_char();
				return;
			}
			else {
				lexeme.push_back(ch);
				lexeme.push_back(peek);
				col += 2;
			}
		}
		else if (ch == '\n') {
			error_flag = true;
			unget_char();
			return;
		}
		else if (ch == '\'') {
			return;
		}
		else {
			lexeme.push_back(ch);
			col++;
		}

		peek = get_next_char();

		if (is_eof(peek)) {
			eof_flag = true;
			return;
		}

		if (ch == '\'') {
			col++;
			return;
		}
		else {
			unget_char();
			c_char_sequence();
		}
	}

	Token Lexer::string_literal() {

		char ch = get_next_char();
		char peek;
		Token tok;

		if (is_eof(ch)) {
			tok.number = END;
		}
		else {

			if (ch == '"') {
				lexeme.clear();
				col++;
				tok = make_token(LIT_STRING);
			}
			else {
				unget_char();
				s_char_sequence();

				tok = make_token(LIT_STRING);

				if (error_flag) {
					peek = get_next_char();
					if (peek == '\\') {
						consume_chars_till("\n\"");
						Log::error_at(tok.loc, "invalid string incomplete escape sequence", lexeme);

					}
					else if (peek == '\n') {
						consume_chars_till("\n\"");
						Log::error_at(tok.loc, "missing terminating string", lexeme);

					}
					else {
						consume_chars_till("\n\"");
						Log::error_at(tok.loc, "invalid string ", lexeme);
					}

					//	error_count++;

				}
			}
		}

		lexeme.clear();
		return tok;
	}

	void Lexer::s_char_sequence() {

		char ch = get_next_char();
		char peek;

		if (is_eof(ch)) {
			eof_flag = true;
			return;
		}

		if (ch == '\\') {
			peek = get_next_char();

			if (is_eof(peek)) {
				eof_flag = true;
				return;
			}
			else if (peek == '\n') {
				error_flag = true;
				unget_char();
				return;
			}
			else {
				lexeme.push_back(ch);
				lexeme.push_back(peek);
				col += 2;
			}
		}
		else if (ch == '\n') {
			error_flag = true;
			unget_char();
			return;
		}
		else if (ch == '"')
			return;
		else {
			lexeme.push_back(ch);
			col++;
		}


		peek = get_next_char();

		if (is_eof(peek)) {
			eof_flag = true;
			return;
		}

		if (ch == '"') {
			col++;
			return;
		}
		else {
			unget_char();
			s_char_sequence();
		}
	}

	Token Lexer::integer_literal() {

		char ch = get_next_char();
		char peek;
		Token tok;

		if (is_eof(ch))
			tok.number = END;
		else {
			if (ch == '0') {
				peek = get_next_char();
				if (peek == 'x' || peek == 'X') {

					lexeme.push_back(ch);
					lexeme.push_back(peek);
					col += 2;
					tok = hexadecimal_literal();

					if (tok.string.size() == 2)
						tok.string = tok.string + "0";
				}
				else if (peek == 'b' || peek == 'B') {
					lexeme.push_back('0');
					lexeme.push_back(peek);
					col += 2;
					tok = binary_literal();
				}
				else if (digit(peek)) {

					unget_char();
					unget_char();
					tok = octal_literal();
				}
				else if (peek == '.') {
					tok = float_literal();
					tok.string.insert(0, "0.");
				}
				else {
					if (symbol(peek)) {
						unget_char();
						//store in lexeme by setting Token to OCTAL
						//because we already checked strating by 0
						lexeme.push_back(ch);
						tok = make_token(LIT_OCTAL);
					}
				}
			}
			else if (nonzero_digit(ch)) {
				unget_char();
				tok = decimal_literal();
			}
		}
		return tok;
	}

	Token Lexer::decimal_literal() {

		char ch = get_next_char();
		char peek;
		Token tok;

		if (is_eof(ch))
			tok.number = END;
		else {
			if (nonzero_digit(ch)) {
				lexeme.push_back(ch);
				col++;

				sub_decimal_literal();
				if (error_flag) {
					consume_chars_till_symbol();
					Log::error(get_filename(), "invalid decimal ", lexeme, line, col - lexeme.size());
				}

				peek = get_next_char();

				if (peek == '.') {
					tok = float_literal();
					lexeme.push_back('.');
					tok.string.insert(0, lexeme);
				}
				else if (symbol(peek)) {
					unget_char();
					if (eof_flag) {
						if (lexeme.size() > 0) {
							tok = make_token(LIT_DECIMAL);
							col++;
						}
						else
							tok.number = END;
					}
					else {
						if (lexeme.size() > 0) {
							tok = make_token(LIT_DECIMAL);
							col++;
						}
					}
				}
			}
		}
		return tok;
	}

	void Lexer::sub_decimal_literal() {

		char ch = get_next_char();
		char peek;

		if (is_eof(ch)) {
			eof_flag = true;
			return;
		}

		if (digit(ch)) {
			lexeme.push_back(ch);
			col++;
		}
		else {
			if (symbol(ch)) {
				unget_char();
				return;
			}
			else {
				error_flag = true;
				return;
			}
		}


		peek = get_next_char();
		if (is_eof(peek)) {
			eof_flag = true;
			return;
		}

		if (digit(peek)) {
			unget_char();
			sub_decimal_literal();
		}
		else {
			if (symbol(peek)) {
				unget_char();
				return;
			}
			else {
				unget_char();
				error_flag = true;
				return;
			}
		}
	}

	Token Lexer::octal_literal() {

		char ch = get_next_char();
		Token tok;

		if (is_eof(ch))
			tok.number = END;
		else {
			if (ch == '0') {
				lexeme.push_back(ch);
				col++;

				sub_octal_literal();
				if (error_flag) {
					consume_chars_till_symbol();
					Log::error(get_filename(), "invalid octal ", lexeme, line, col - lexeme.size());
				}

				if (eof_flag) {
					if (lexeme.size() > 0) {
						tok = make_token(LIT_OCTAL);
						col++;
					}
					else
						tok.number = END;
				}
				else {
					if (lexeme.size() > 0) {
						tok = make_token(LIT_OCTAL);
						col++;
					}
				}
			}
		}
		return tok;
	}

	void Lexer::sub_octal_literal() {

		char ch = get_next_char();
		char peek;

		if (is_eof(ch)) {
			eof_flag = true;
			return;
		}

		if (octal_digit(ch)) {
			lexeme.push_back(ch);
			col++;
		}
		else {
			if (symbol(ch)) {
				unget_char();
				return;
			}
			else {
				error_flag = true;
				return;
			}
		}

		peek = get_next_char();
		if (is_eof(peek)) {
			eof_flag = true;
			return;
		}

		if (octal_digit(peek)) {
			unget_char();
			sub_octal_literal();
		}
		else {
			if (symbol(peek)) {
				unget_char();
				return;
			}
			else {
				unget_char();
				error_flag = true;
				return;
			}
		}

	}

	Token Lexer::hexadecimal_literal() {

		char ch = get_next_char();
		Token tok;

		if (is_eof(ch))
			tok.number = END;
		else {
			unget_char();
			sub_hexadecimal_literal();
			if (error_flag) {
				consume_chars_till_symbol();
				Log::error(get_filename(), "invalid hexadecimal ", lexeme, line, col - lexeme.size());
			}
			if (eof_flag) {
				if (lexeme.size() > 0) {
					tok = make_token(LIT_HEX);
					col++;
				}
				else
					tok.number = END;
			}
			else {
				if (lexeme.size() > 0) {
					tok = make_token(LIT_HEX);
					col++;
				}
			}
		}
		return tok;
	}

	void Lexer::sub_hexadecimal_literal() {

		char ch = get_next_char();
		char peek;

		if (is_eof(ch)) {
			eof_flag = true;
			return;
		}

		if (hexadecimal_digit(ch)) {
			lexeme.push_back(ch);
			col++;
		}
		else {
			if (symbol(ch)) {
				unget_char();
				return;
			}
			else {
				error_flag = true;
				return;
			}
		}

		peek = get_next_char();
		if (is_eof(peek)) {
			eof_flag = true;
			return;
		}

		if (hexadecimal_digit(peek)) {
			unget_char();
			sub_hexadecimal_literal();
		}
		else {
			if (symbol(peek)) {
				unget_char();
				return;
			}

			unget_char();
			error_flag = true;
			return;
		}
	}

	Token Lexer::binary_literal() {

		char ch = get_next_char();
		Token tok;

		if (is_eof(ch))
			tok.number = END;
		else {
			unget_char();
			sub_binary_literal();

			if (error_flag) {
				consume_chars_till_symbol();
				Log::error(get_filename(), "invalid binary ", lexeme, line, col - lexeme.size());
			}
			if (eof_flag) {
				if (lexeme.size() > 0) {
					tok = make_token(LIT_BIN);
					col++;
				}
				else
					tok.number = END;
			}
			else {
				if (lexeme.size() > 0) {
					tok = make_token(LIT_BIN);
					col++;
				}
			}
		}

		lexeme.clear();
		return tok;
	}

	void Lexer::sub_binary_literal() {
		char ch = get_next_char();
		char peek;

		if (is_eof(ch)) {
			eof_flag = true;
			return;
		}

		if (ch == '0' || ch == '1') {
			lexeme.push_back(ch);
			col++;
		}
		else {
			if (symbol(ch)) {
				unget_char();
				return;
			}
			else {
				error_flag = true;
				return;
			}
		}

		peek = get_next_char();
		if (is_eof(peek)) {
			eof_flag = true;
			return;
		}

		if (ch == '0' || ch == '1') {
			unget_char();
			sub_binary_literal();
		}
		else {
			if (symbol(peek)) {
				unget_char();
				return;
			}
			else {
				unget_char();
				error_flag = true;
				return;
			}
		}
	}

	Token Lexer::float_literal() {
		Token tok;
		std::string lexm;
		digit_sequence(lexm);

		if (error_flag) {
			consume_chars_till_symbol();
			Log::error(get_filename(), "invalid float ", lexm, line, col - lexm.size());
		}

		tok = make_token(lexm, LIT_FLOAT);;
		return tok;
	}

	void Lexer::digit_sequence(std::string &lexm) {

		char ch = get_next_char();
		char peek;

		if (is_eof(ch)) {
			eof_flag = true;
			return;
		}

		if (digit(ch)) {
			lexm.push_back(ch);
			col++;
		}
		else {
			error_flag = true;
			return;
		}

		peek = get_next_char();
		if (is_eof(peek)) {
			eof_flag = true;
			return;
		}

		if (digit(peek)) {
			unget_char();
			digit_sequence(lexm);
		}
		else {
			if (symbol(peek)) {
				unget_char();
				return;
			}
			else {
				unget_char();
				error_flag = true;
				return;
			}
		}
	}

	Token Lexer::identifier() {

		char ch = get_next_char();
		char peek;
		Token tok;

		if (is_eof(ch))
			tok.number = END;
		else {
			if (non_digit(ch)) {
				lexeme.push_back(ch);
				tok.loc.col = col;
				tok.loc.line = line;
				col++;
			}
		}

		peek = get_next_char();
		if (is_eof(peek))
			tok.number = END;
		else {
			if (non_digit(peek) || digit(peek)) {
				unget_char();
				sub_identifier();

				if (eof_flag) {
					if (lexeme.size() > 0) {
						tok.number = IDENTIFIER;
						tok.string = lexeme;
					}
					else
						tok.number = END;
				}
				else if (lexeme.size() > 0) {
					tok.number = IDENTIFIER;
					tok.string = lexeme;
				}
			}
			else {
				if (symbol(peek)) {
					unget_char();
					if (lexeme.size() > 0) {
						tok.number = IDENTIFIER;
						tok.string = lexeme;
						col++;
					}
				}
			}
		}

		std::unordered_map<std::string, TokenId>::iterator find_it = key_tokens.find(lexeme);
		if (find_it != key_tokens.end())
			tok.number = find_it->second;

		lexeme.clear();
		return tok;
	}

	void Lexer::sub_identifier() {

		char ch = get_next_char();
		char peek;

		if (is_eof(ch)) {
			eof_flag = true;
			return;
		}

		if (non_digit(ch) || digit(ch)) {
			lexeme.push_back(ch);
			col++;
		}

		peek = get_next_char();
		if (is_eof(peek)) {
			eof_flag = true;
			return;
		}

		if (non_digit(peek) || digit(peek)) {
			unget_char();
			sub_identifier();
		}
		else {
			unget_char();
			return;
		}
	}

	Token Lexer::operator_token() {

		char ch = get_next_char();
		char peek;
		Token tok;

		switch (ch) {
			case '+' : {
				peek = get_next_char();
				if (peek == '=') {
					col += 2;
					return make_token("+=", ASSGN_ADD);
				}
				else if (peek == '+') {
					col += 2;
					return make_token("++", INCR_OP);
				}
				else {
					col++;
					unget_char();
					return make_token("+", ARTHM_ADD);
				}
			}
				break;

			case '-' : {
				peek = get_next_char();
				if (peek == '=') {
					col += 2;
					return make_token("-=", ASSGN_SUB);
				}
				else if (peek == '-') {
					col += 2;
					return make_token("--", DECR_OP);
				}
				else if (peek == '>') {
					col += 2;
					return make_token("->", ARROW_OP);
				}
				else {
					col++;
					unget_char();
					return make_token("-", ARTHM_SUB);
				}
			}
				break;

			case '*' : {
				peek = get_next_char();
				if (peek == '=') {
					col += 2;
					return make_token("*=", ASSGN_MUL);
				}
				else {
					col++;
					unget_char();
					return make_token("*", ARTHM_MUL);
				}
			}
				break;

			case '/' : {
				peek = get_next_char();
				if (peek == '=') {
					col += 2;
					return make_token("/=", ASSGN_DIV);
				}
				else {
					col++;
					unget_char();
					return make_token("/", ARTHM_DIV);
				}
			}
				break;

			case '%' : {
				peek = get_next_char();
				if (peek == '=') {
					col += 2;
					return make_token("%=", ASSGN_MOD);
				}
				else {
					col++;
					unget_char();
					return make_token("%", ARTHM_MOD);
				}
			}
				break;

			case '&' : {
				peek = get_next_char();
				if (peek == '=') {
					col += 2;
					return make_token("&=", ASSGN_BIT_AND);
				}
				if (peek == '&') {
					col += 2;
					return make_token("&&", LOG_AND);
				}
				else {
					col++;
					unget_char();
					return make_token("&", BIT_AND);
				}
			}
				break;

			case '|' : {
				peek = get_next_char();
				if (peek == '=') {
					col += 2;
					return make_token("|=", ASSGN_BIT_OR);
				}
				if (peek == '|') {
					col += 2;
					return make_token("||", LOG_OR);
				}
				else {
					col++;
					unget_char();
					return make_token("|", BIT_OR);
				}
			}
				break;

			case '!' : {
				peek = get_next_char();
				if (peek == '=') {
					col += 2;
					return make_token("!=", COMP_NOT_EQ);
				}
				else {
					col++;
					unget_char();
					return make_token("!", LOG_NOT);
				}
			}
				break;

			case '~' : {
				col++;
				return make_token("~", BIT_COMPL);
			}
				break;

			case '<' : {
				peek = get_next_char();
				if (peek == '=') {
					col += 2;
					return make_token("<=", COMP_LESS_EQ);
				}
				else if (peek == '<') {
					peek = get_next_char();
					if (peek == '=') {
						col += 3;
						return make_token("<<=", ASSGN_LSHIFT);
					}
					else {
						col += 2;
						unget_char();
						return make_token("<<", BIT_LSHIFT);
					}
				}
				else {
					col++;
					unget_char();
					return make_token("<", COMP_LESS);
				}
			}
				break;

			case '>' : {
				peek = get_next_char();
				if (peek == '=') {
					col += 2;
					return make_token(">=", COMP_GREAT_EQ);
				}
				else if (peek == '>') {
					peek = get_next_char();
					if (peek == '=') {
						col += 3;
						return make_token(">>=", ASSGN_RSHIFT);
					}
					else {
						col += 2;
						unget_char();
						return make_token(">>", BIT_RSHIFT);
					}
				}
				else {
					col++;
					unget_char();
					return make_token(">", COMP_GREAT);
				}
			}
				break;

			case '^' : {
				peek = get_next_char();
				if (peek == '=') {
					col += 2;
					return make_token("^=", ASSGN_BIT_EX_OR);
				}
				else {
					col++;
					unget_char();
					return make_token("^", BIT_EXOR);
				}
			}
				break;

			case '=' : {
				peek = get_next_char();
				if (peek == '=') {
					col += 2;
					return make_token("==", COMP_EQ);
				}
				else {
					col++;
					unget_char();
					return make_token("=", ASSGN);
				}
			}
				break;

			default : {
				if (is_eof(ch))
					tok.number = END;
				else
					unget_char();
			}
				break;
		}

		return tok;
	}

	void Lexer::print_processed_tokens() {

		std::stack<Token> temp;
		Token tok;

		while (!processed_tokens.empty()) {
			temp.push(processed_tokens.front());
			processed_tokens.pop();
		}

		while (!temp.empty()) {
			tok = temp.top();
			std::cout << "tok = " << tok.number << " lexeme = " << tok.string << std::endl;
			processed_tokens.push(temp.top());
			temp.pop();
		}
	}

	Token Lexer::get_next() {

		Token tok;
		tok.number = END;
		tok.loc.line = 0;
		tok.loc.col = 0;

		char ch;
		if (this->is_lexing_done)
			return tok;

		if (!processed_tokens.empty()) {
			tok = processed_tokens.front();
			processed_tokens.pop();
			return tok;
		}

		loop_label:
		switch (ch = get_next_char()) {
			case '_':
			case '$':
			case 'a'...'z':
			case 'A'...'Z':
				unget_char();
				tok = identifier();
				break;

			case '0'...'9':
			case '"':
			case '\'':
				unget_char();
				tok = literal();
				error_flag = false;
				break;

			case ' ':
			case '\t':
				col++;
				goto loop_label;

			case '+':
			case '-':
			case '*':
			case '%':
			case '&':
			case '|':
			case '!':
			case '~':
			case '<':
			case '>':
			case '^':
			case '=':
				unget_char();
				tok = operator_token();
				break;

			case '/':
				if (comment())
					goto loop_label;

				unget_char();
				tok = operator_token();
				break;

			case '.':
				col++;
				tok = make_token(".", DOT_OP);
				break;

			case ',':
				col++;
				tok = make_token(",", COMMA_OP);
				break;

			case ':':
				col++;
				tok = make_token(":", COLON_OP);
				break;

			case '{':
				col++;
				tok = make_token("{", CURLY_OPEN);
				break;

			case '}':
				col++;
				tok = make_token("}", CURLY_CLOSE);
				break;

			case '(':
				col++;
				tok = make_token("(", PARENTH_OPEN);
				break;

			case ')':
				col++;
				tok = make_token(")", PARENTH_CLOSE);
				break;

			case '[':
				col++;
				tok = make_token("[", SQUARE_OPEN);
				break;

			case ']':
				col++;
				tok = make_token("]", SQUARE_CLOSE);
				break;

			case ';':
				col++;
				tok = make_token(";", SEMICOLON);
				break;

			case '\n':
				line++;
				col = 1;
				goto loop_label;

			default:
				if (is_eof(ch)) {
					this->is_lexing_done = true;
					return tok;
				}

				Log::error("invalid character", std::string(1, ch));
				break;
		}
		return tok;
	}

	void Lexer::put_back(Token &tok) {
		processed_tokens.push(tok);
	}

	void Lexer::put_back(Token &tok, bool high_priority) {
		Token tok2;
		if (!processed_tokens.empty()) {
			if (high_priority) {
				tok2 = processed_tokens.front();
				processed_tokens.pop();
				processed_tokens.push(tok);
				processed_tokens.push(tok2);
			}
			else
				processed_tokens.push(tok);
		}
	}

	void Lexer::reverse_tokens_queue() {

		std::stack<Token> temp;
		Token tok;

		while (!processed_tokens.empty()) {
			temp.push(processed_tokens.front());
			processed_tokens.pop();
		}

		while (!temp.empty()) {
			tok = temp.top();
			processed_tokens.push(temp.top());
			temp.pop();
		}
	}
}
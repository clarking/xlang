/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <queue>
#include "file.hpp"
#include "token.hpp"

namespace xlang {
	
	class Lexer {
	public:

		Lexer(SourceFile src) : file(src) {};
		
		void init();
		
		Token get_next();
		
		void put_back(Token &);
		
		void put_back(Token &, bool);
		
		std::string get_filename();
		
		void print_processed_tokens();
		
		void reverse_tokens_queue();
		
    private:
		
		SourceFile file;
		std::unordered_map<std::string, TokenId> key_tokens;
		std::queue<Token> processed_tokens;

		std::vector<char> symbols = {
            ' ', '\t', '\n', '!', '%', '^', '~', '&', 
            '*', '(', ')', '-', '+', '=', '[', ']', '{', 
            '}', '|', ':', ';', '<', '>', ',', '.', '/', 
            '\\', '\'', '"', '@', '`', '?'
        };

		std::string lexeme;
		size_t buffer_index = 0;
		unsigned line = 1;
        unsigned col = 1;

		bool unget_flag = false;
		bool is_lexing_done = false;
		bool eof_flag = false;
		bool error_flag = false;
		
		bool is_eof(char);
		
		bool file_read(SourceFile &src);
		
		bool file_exists(const std::string &path);
		
		char get_next_char();
		
		void unget_char();
		
		void consume_chars_till(char);
		
		void consume_chars_till(std::string);
		
		void consume_chars_till_symbol();
		
		Token make_token(TokenId);
		
		Token make_token(std::string, TokenId);
		
		Token operator_token();
		
		bool symbol(char);
		
		bool digit(char);
		
		bool nonzero_digit(char);
		
		bool octal_digit(char);
		
		bool hexadecimal_digit(char);
		
		bool non_digit(char);
		
		Token identifier();
		
		void sub_identifier();
		
		Token literal();
		
		Token integer_literal();
		
		Token float_literal();
		
		Token character_literal();
		
		void c_char_sequence();
		
		Token string_literal();
		
		void s_char_sequence();
		
		Token decimal_literal();
		
		void sub_decimal_literal();
		
		Token octal_literal();
		
		void sub_octal_literal();
		
		Token hexadecimal_literal();
		
		void sub_hexadecimal_literal();
		
		Token binary_literal();
		
		void sub_binary_literal();
		
		void digit_sequence(std::string &);
		
		bool comment();
	};
}

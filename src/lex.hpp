/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

//  data/operations used by class lexer.

#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <queue>
#include "file.hpp"
#include "token.hpp"

namespace xlang {
	
	//buffer size used to read block from a file
	#define BUFFER_SIZE 512
	
	class lexer {
		
		public:
		lexer(src_file src) : file(src) {};
		
		void init();
		
		token get_next();
		
		void put_back(token &);
		
		void put_back(token &, bool);
		
		std::string get_filename();
		
		void print_processed_tokens();
		
		void reverse_tokens_queue();
		
		private:
		
		src_file file;
		bool is_lexing_done = false;
		
		bool is_eof(char);
		
		bool file_read(src_file &src);
		
		bool file_exists(const std::string &path);
		
		char buffer[BUFFER_SIZE];
		size_t buffer_index = 0;
		
		char get_next_char();
		
		void unget_char();
		
		bool unget_flag = false;
		
		int line = 1, col = 1;
		
		std::unordered_map<std::string, token_t> key_tokens;
		std::vector<char> symbols = {' ', '\t', '\n', '!', '%', '^', '~', '&', '*', '(', ')', '-', '+', '=', '[', ']', '{', '}', '|', ':', ';', '<', '>', ',', '.', '/', '\\', '\'', '"', '@', '`', '?'};
		
		std::string lexeme;
		
		bool eof_flag = false;
		
		bool error_flag = false;
		
		void consume_chars_till(char);
		
		void consume_chars_till(std::string);
		
		void consume_chars_till_symbol();
		
		token make_token(token_t);
		
		token make_token(std::string, token_t);
		
		token operator_token();
		
		std::queue<token> processed_tokens;
		
		//lexer grammar
		bool symbol(char);
		
		bool digit(char);
		
		bool nonzero_digit(char);
		
		bool octal_digit(char);
		
		bool hexadecimal_digit(char);
		
		bool non_digit(char);
		
		token identifier();
		
		void sub_identifier();
		
		token literal();
		
		token integer_literal();
		
		token float_literal();
		
		token character_literal();
		
		void c_char_sequence();
		
		token string_literal();
		
		void s_char_sequence();
		
		token decimal_literal();
		
		void sub_decimal_literal();
		
		token octal_literal();
		
		void sub_octal_literal();
		
		token hexadecimal_literal();
		
		void sub_hexadecimal_literal();
		
		token binary_literal();
		
		void sub_binary_literal();
		
		void digit_sequence(std::string &);
		
		bool comment();
		
	};
	
	
}

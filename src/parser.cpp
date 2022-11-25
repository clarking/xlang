/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


// Recursive Descent Parser for xlang

#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdarg>
#include <cassert>
#include "token.hpp"
#include "compiler.hpp"
#include "log.hpp"
#include "tree.hpp"
#include "parser.hpp"
#include "symtab.hpp"

namespace xlang {
	
	
	parser::parser() {
		//token_lexeme_table used for string of special symbols
		this->token_lexeme_table = {{PTR_OP,               "*"},
		                            {LOG_NOT,              "!"},
		                            {ADDROF_OP,            "&"},
		                            {ARROW_OP,             "->"},
		                            {DOT_OP,               "."},
		                            {COMMA_OP,             ","},
		                            {COLON_OP,             ":"},
		                            {CURLY_OPEN_BRACKET,   "{"},
		                            {CURLY_CLOSE_BRACKET,  "}"},
		                            {PARENTH_OPEN,         "("},
		                            {PARENTH_CLOSE,        ")"},
		                            {SQUARE_OPEN_BRACKET,  "["},
		                            {SQUARE_CLOSE_BRACKET, "]"},
		                            {SEMICOLON,            ";"}};
		
		
		compiler::symtab = symtable::get_node_mem();
		compiler::record_table = symtable::get_record_symtab_mem();
		compiler::func_table = symtable::get_func_table_mem();
		
		consumed_terminator.number = NONE;
		consumed_terminator.string = "";
		nulltoken.number = NONE;
		nulltoken.string = "";
	}
	
	//get token from lexer and match it with tk and return token again to lex
	bool parser::peek_token(token_t tk) {
		token tok = compiler::lex->get_next();
		if (tok.number == tk) {
			compiler::lex->put_back(tok);
			return true;
		}
		compiler::lex->put_back(tok);
		return false;
	}
	
	//same as above function just match with a vector of tokens
	bool parser::peek_token(std::vector<token_t> &tkv) {
		token tok = compiler::lex->get_next();
		std::vector<token_t>::iterator it = tkv.begin();
		while (it != tkv.end()) {
			if (tok.number == *it) {
				compiler::lex->put_back(tok);
				return true;
			}
			it++;
		}
		compiler::lex->put_back(tok);
		return false;
	}
	
	//peek token with variable number of provided tokens
	bool parser::peek_token(const char *format...) {
		va_list args;
		va_start(args, format);
		token tok = compiler::lex->get_next();
		
		while (*format != '\0') {
			if (*format == 'd') {
				if (va_arg(args, int) == tok.number) {
					compiler::lex->put_back(tok);
					return true;
				}
			}
			++format;
		}
		
		va_end(args);
		compiler::lex->put_back(tok);
		return false;
	}
	
	bool parser::peek_nth_token(token_t tk, int n) {
		token *tok = new token[n];
		token_t tk2;
		int i;
		for (i = 0; i < n; i++)
			tok[i] = compiler::lex->get_next();
		
		tk2 = tok[n - 1].number;
		
		for (i = n - 1; i >= 0; i--)
			compiler::lex->put_back(tok[i]);
		
		delete[] tok;
		return (tk == tk2);
	}
	
	token_t parser::get_peek_token() {
		token tok = compiler::lex->get_next();
		token_t tk = tok.number;
		compiler::lex->put_back(tok);
		return tk;
	}
	
	token_t parser::get_nth_token(int n) {
		token *tok = new token[n];
		token_t tk;
		int i;
		
		for (i = 0; i < n; i++)
			tok[i] = compiler::lex->get_next();
		
		tk = tok[n - 1].number;
		for (i = n - 1; i >= 0; i--)
			compiler::lex->put_back(tok[i]);
		
		delete[] tok;
		return tk;
	}
	
	//expression literal
	bool parser::expr_literal(token_t tkt) {
		return (tkt == LIT_DECIMAL ||
		        tkt == LIT_OCTAL ||
		        tkt == LIT_HEX ||
		        tkt == LIT_BIN ||
		        tkt == LIT_FLOAT ||
		        tkt == LIT_CHAR);
	}
	
	bool parser::peek_expr_literal() {
		token tok = compiler::lex->get_next();
		token_t tkt = tok.number;
		compiler::lex->put_back(tok);
		return (expr_literal(tkt));
	}
	
	//get token from lexer
	//if it is not matched with the provided token then display error
	bool parser::expect(token_t tk) {
		token tok = compiler::lex->get_next();
		if (tok.number != tk) {
			std::map<token_t, std::string>::iterator find_it = token_lexeme_table.find(tk);
			if (find_it != token_lexeme_table.end()) {
				auto it = std::prev(expr_list.end());
				loc_t loc;
				if (!expr_list.empty())
					loc = (*it).loc;
				log::error_at(loc, "expected ", find_it->second);
				log::print_tokens(expr_list);
				return false;
			}
		}
		compiler::lex->put_back(tok);
		return true;
	}
	
	//same as above just to determine whether to consume token or return it to lexer
	bool parser::expect(token_t tk, bool consume_token) {
		token tok = compiler::lex->get_next();
		if (tok.number == END_OF_FILE)
			return false;
		
		if (tok.number != tk) {
			std::map<token_t, std::string>::iterator find_it = token_lexeme_table.find(tk);
			if (find_it != token_lexeme_table.end()) {
				auto it = std::prev(expr_list.end());
				loc_t loc;
				if (!expr_list.empty())
					loc = (*it).loc;
				if (tok.number != END_OF_FILE && expr_list.empty())
					loc = tok.loc;
				else {
					loc.line = 0;
					loc.col = 0;
				}
				log::error_at(loc, "expected ", find_it->second, " but found " + s_quotestring(tok.string));
				log::print_tokens(expr_list);
				return false;
			}
		}
		
		if (!consume_token)
			compiler::lex->put_back(tok);
		return true;
	}
	
	bool parser::expect(token_t tk, bool consume_token, std::string str) {
		token tok = compiler::lex->get_next();
		if (tok.number != tk) {
			log::error_at(tok.loc, "expected ", str);
			log::print_tokens(expr_list);
			return false;
		}
		if (!consume_token)
			compiler::lex->put_back(tok);
		return true;
	}
	
	bool parser::expect(token_t tk, bool consume_token, std::string str, std::string arg) {
		token tok = compiler::lex->get_next();
		
		if (tok.number != tk) {
			log::error_at(tok.loc, "expected ", str, arg);
			log::print_tokens(expr_list);
			return false;
		}
		if (!consume_token)
			compiler::lex->put_back(tok);
		return true;
	}
	
	bool parser::expect(const char *format...) {
		va_list args;
		va_start(args, format);
		token tok = compiler::lex->get_next();
		
		while (*format != '\0') {
			if (*format == 'd') {
				if (va_arg(args, int) == tok.number) {
					compiler::lex->put_back(tok);
					return true;
				}
			}
			++format;
		}
		
		va_end(args);
		log::error_at(tok.loc, "expected ");
		return false;
	}
	
	void parser::consume_next() {
		compiler::lex->get_next();
	}
	
	void parser::consume_n(int n) {
		while (n > 0) {
			compiler::lex->get_next();
			n--;
		}
	}
	
	void parser::consume_till(terminator_t &terminator) {
		token tok;
		std::sort(terminator.begin(), terminator.end());
		while ((tok = compiler::lex->get_next()).number != END_OF_FILE) {
			if (std::binary_search(terminator.begin(), terminator.end(), tok.number))
				break;
		}
		compiler::lex->put_back(tok);
	}
	
	//used in primary expression for () checking using stack
	bool parser::check_parenth() {
		if (parenth_stack.size() > 0) {
			parenth_stack.pop();
			return true;
		}
		return false;
	}
	
	//matches with the terminator vector
	//terminator is the vector of tokens which is used for expression terminator
	bool parser::matches_terminator(terminator_t &tkv, token_t tk) {
		for (auto x: tkv) {
			if (x == tk)
				return true;
		}
		return false;
	}
	
	std::string parser::get_terminator(terminator_t &terminator) {
		std::string st;
		size_t i;
		std::map<token_t, std::string>::iterator find_it;
		for (i = 0; i < terminator.size(); i++) {
			find_it = token_lexeme_table.find(terminator[i]);
			if (find_it != token_lexeme_table.end())
				st = st + find_it->second + " ";
		}
		return st;
	}
	
	
	// unary-operator : + | - | ! | ~
	bool parser::unary_operator(token_t tk) {
		return (tk == ARTHM_ADD || tk == ARTHM_SUB || tk == LOG_NOT || tk == BIT_COMPL);
	}
	
	bool parser::peek_unary_operator() {
		token tok = compiler::lex->get_next();
		token_t tk = tok.number;
		compiler::lex->put_back(tok);
		return unary_operator(tk);
	}
	
	// binary-operator : arithmetic-op | logical-op | comparison-op bitwise-op |
	bool parser::binary_operator(token_t tk) {
		return (arithmetic_operator(tk) || logical_operator(tk) || comparison_operator(tk) || bitwise_operator(tk));
	}
	
	// arithmetic-operator :  + | - | * | / | %
	bool parser::arithmetic_operator(token_t tk) {
		return (tk == ARTHM_ADD || tk == ARTHM_SUB || tk == ARTHM_MUL || tk == ARTHM_DIV || tk == ARTHM_MOD);
	}
	
	// logical-operator : one of && ||
	bool parser::logical_operator(token_t tk) {
		return (tk == LOG_AND || tk == LOG_OR);
	}
	
	// comparison-operator : one of < <= > >= == !=
	bool parser::comparison_operator(token_t tk) {
		return (tk == COMP_LESS || tk == COMP_LESS_EQ || tk == COMP_GREAT || tk == COMP_GREAT_EQ || tk == COMP_EQ || tk == COMP_NOT_EQ);
	}
	
	// bitwise-operator : one of  | & ^ << >> bit_and bit_or bit_xor
	bool parser::bitwise_operator(token_t tk) {
		return (tk == BIT_OR || tk == BIT_AND || tk == BIT_EXOR || tk == BIT_LSHIFT || tk == BIT_RSHIFT);
	}
	
	// assignment-operator : one of = += -= *= /= %= |= &= ^= <<= >>=
	bool parser::assignment_operator(token_t tk) {
		return (tk == ASSGN ||
		        tk == ASSGN_ADD ||
		        tk == ASSGN_SUB ||
		        tk == ASSGN_MUL ||
		        tk == ASSGN_DIV ||
		        tk == ASSGN_MOD ||
		        tk == ASSGN_BIT_OR ||
		        tk == ASSGN_BIT_AND ||
		        tk == ASSGN_BIT_EX_OR ||
		        tk == ASSGN_LSHIFT ||
		        tk == ASSGN_RSHIFT);
	}
	
	bool parser::peek_binary_operator() {
		token tok = compiler::lex->get_next();
		token_t tk = tok.number;
		compiler::lex->put_back(tok);
		return binary_operator(tk);
		
	}
	
	bool parser::peek_literal() {
		token_t tk = get_peek_token();
		return (tk == LIT_DECIMAL || tk == LIT_OCTAL || tk == LIT_HEX || tk == LIT_BIN || tk == LIT_FLOAT || tk == LIT_CHAR);
		
	}
	
	bool parser::peek_literal_string() {
		token_t tk = get_peek_token();
		return (tk == LIT_DECIMAL ||
		        tk == LIT_OCTAL ||
		        tk == LIT_HEX ||
		        tk == LIT_BIN ||
		        tk == LIT_FLOAT ||
		        tk == LIT_CHAR ||
		        tk == LIT_STRING);
	}
	
	bool parser::integer_literal(token_t tk) {
		return (tk == LIT_DECIMAL || tk == LIT_OCTAL || tk == LIT_HEX || tk == LIT_BIN);
	}
	
	bool parser::character_literal(token_t tk) {
		return (tk == LIT_CHAR);
	}
	
	bool parser::constant_expr(token_t tk) {
		return (integer_literal(tk) || character_literal(tk));
	}
	
	bool parser::peek_constant_expr() {
		return constant_expr(get_peek_token());
	}
	
	bool parser::peek_assignment_operator() {
		token_t tk = get_peek_token();
		return assignment_operator(tk);
	}
	
	bool parser::peek_identifier() {
		return (get_peek_token() == IDENTIFIER);
	}
	
	bool parser::expect_binary_operator() {
		return expect(
				"dddddddddddddddddddd",
				ARTHM_ADD,
				ARTHM_SUB,
				ARTHM_MUL,
				ARTHM_DIV,
				ARTHM_MOD,
				LOG_AND,
				LOG_OR,
				COMP_LESS,
				COMP_LESS_EQ,
				COMP_GREAT,
				COMP_GREAT_EQ,
				COMP_EQ,
				COMP_NOT_EQ,
				BIT_AND,
				BIT_OR,
				BIT_EXOR,
				BIT_LSHIFT,
				BIT_RSHIFT);
	}
	
	bool parser::expect_literal() {
		return expect("ddddddddd", LIT_DECIMAL, LIT_OCTAL, LIT_HEX, LIT_BIN, LIT_FLOAT, LIT_CHAR);
	}
	
	bool parser::expect_assignment_operator() {
		return expect("ddddddddddd", ASSGN, ASSGN_ADD, ASSGN_SUB, ASSGN_MUL, ASSGN_DIV, ASSGN_MOD, ASSGN_BIT_OR, ASSGN_BIT_AND, ASSGN_BIT_EX_OR, ASSGN_LSHIFT, ASSGN_RSHIFT);
	}
	
	bool parser::member_access_operator(token_t tk) {
		return (tk == DOT_OP || tk == ARROW_OP);
	}
	
	bool parser::peek_member_access_operator() {
		token_t tk = get_peek_token();
		return member_access_operator(tk);
	}
	
	bool parser::expression_token(token_t tk) {
		return (tk == LIT_DECIMAL ||
		        tk == LIT_OCTAL ||
		        tk == LIT_HEX ||
		        tk == LIT_BIN ||
		        tk == LIT_FLOAT ||
		        tk == LIT_CHAR ||
		        tk == ARTHM_ADD ||
		        tk == ARTHM_SUB ||
		        tk == LOG_NOT ||
		        tk == BIT_COMPL ||
		        tk == IDENTIFIER ||
		        tk == PARENTH_OPEN ||
		        tk == ARTHM_MUL ||
		        tk == INCR_OP ||
		        tk == DECR_OP ||
		        tk == BIT_AND ||
		        tk == KEY_SIZEOF);
	}
	
	bool parser::peek_expr_token() {
		return expression_token(get_peek_token());
	}
	
	//	type-specifier : simple-type-specifier | record-name
	//	simple-type-specifier :  void | char | double | float | int | short | long
	bool parser::peek_type_specifier(std::vector<token> &tokens) {
		token tok;
		
		tok = compiler::lex->get_next();
		if (tok.number == KEY_VOID ||
		    tok.number == KEY_CHAR ||
		    tok.number == KEY_DOUBLE ||
		    tok.number == KEY_FLOAT ||
		    tok.number == KEY_INT ||
		    tok.number == KEY_SHORT ||
		    tok.number == KEY_LONG ||
		    tok.number == IDENTIFIER) {
			tokens.push_back(tok);
			compiler::lex->put_back(tok);
			return true;
		}
		
		compiler::lex->put_back(tok);
		return false;
	}
	
	bool parser::type_specifier(token_t tk) {
		return (tk == KEY_CHAR ||
		        tk == KEY_DOUBLE ||
		        tk == KEY_FLOAT ||
		        tk == KEY_INT ||
		        tk == KEY_SHORT ||
		        tk == KEY_LONG ||
		        tk == KEY_VOID);
	}
	
	bool parser::peek_type_specifier() {
		token_t tk = get_peek_token();
		return type_specifier(tk);
	}
	
	void parser::get_type_specifier(std::vector<token> &types) {
		if (peek_type_specifier(types))
			return;
		
		types.clear();
	}
	
	bool parser::peek_type_specifier_from(int n) {
		token *tok = new token[n];
		token_t tk;
		
		for (int i = 0; i < n; i++)
			tok[i] = compiler::lex->get_next();
		
		tk = tok[n - 1].number;
		
		for (int i = n - 1; i >= 0; i--) {
			compiler::lex->put_back(tok[i]);
		}
		
		delete[] tok;
		return (type_specifier(tk));
	}
	
	//	primary-expression :
	//	  literal
	//	  identifier
	//	  ( primary-expr )
	//	  ( primary-expr ) primary-expr
	//	  unary-op primary-expr
	//	  literal binary-op primary-expr
	//	  id-expr binary-op primary-expr
	//	  sub-primary-expression
	
	// infix expressions are hard to parse so there is the possibility that we may
	// discard some valid expressions. Also some unary operators are not handled
	// at the final parsing state. Some are hardcoded for expecting the tokens
	// because of recursion.
	
	void parser::primary_expr(terminator_t &terminator) {
		terminator_t terminator2;
		token tok = compiler::lex->get_next();
		
		//check if token is terminator or not
		if (matches_terminator(terminator, tok.number)) {
			expr_list.push_back(tok);
			return;
		}
		
		switch (tok.number) {
			
			case PARENTH_OPEN : {
				//push open parentheses in expression list and in stack
				expr_list.push_back(tok);
				parenth_stack.push(tok);
				
				//check for closed parenthesis
				if (peek_token(PARENTH_CLOSE)) {
					token tok2 = compiler::lex->get_next();
					log::error_at(tok2.loc, "expression expected ", tok2.string);
					return;
				}
				
				//call same function as per the grammar
				primary_expr(terminator);
				
				//check for parenthesis and expect token )
				if (parenth_stack.size() > 0 && expect(PARENTH_CLOSE)) {
					if (!check_parenth())
						log::error("unbalanced parenthesis");
					else {
						token tok2 = compiler::lex->get_next();
						expr_list.push_back(tok2);
					}
					
					//peek for binary/unary operator
					if (peek_binary_operator() || peek_unary_operator()) {
						sub_primary_expr(terminator);
					}
					else if (peek_token(terminator)) {
						if (check_parenth())
							log::error("unbalanced parenthesis");
						token tok2 = compiler::lex->get_next();
						is_expr_terminator_consumed = true;
						consumed_terminator = tok2;
						is_expr_terminator_got = true;
					}
					else if (peek_token(PARENTH_CLOSE)) {
						token tok2 = compiler::lex->get_next();
						if (!check_parenth())
							log::error_at(tok2.loc, "unbalanced parenthesis ", tok2.string);
						else {
							expr_list.push_back(tok2);
							primary_expr(terminator);
						}
					}
					else {
						tok = compiler::lex->get_next();
						if (!is_expr_terminator_consumed || !is_expr_terminator_got)
							log::error_at(tok.loc, get_terminator(terminator) + "expected");
						if (check_parenth())
							log::error("unbalanced parenthesis");
						else {
							if (tok.number == END_OF_FILE)
								return;
							log::error(get_terminator(terminator) + "expected but found " + tok.string);
						}
					}
				}
			}
				break;
			
			case PARENTH_CLOSE : {
				if (!check_parenth()) {
					log::error("unbalanced parenthesis");
				}
				else {
					//push ) on expression list
					expr_list.push_back(tok);
					
					//peek for binary operator
					if (peek_binary_operator())
						primary_expr(terminator);
					else if (peek_token(terminator)) {
						is_expr_terminator_got = true;
						token tok2 = compiler::lex->get_next();
						is_expr_terminator_consumed = true;
						consumed_terminator = tok2;
						return;
					}
					else if (peek_token(PARENTH_CLOSE))
						primary_expr(terminator);
					else {
						log::error(get_terminator(terminator) + "expected ");
						log::print_tokens(expr_list);
						return;
					}
				}
				return;
			}
				break;
			
			case LIT_DECIMAL :
			case LIT_OCTAL :
			case LIT_HEX :
			case LIT_BIN :
			case LIT_FLOAT :
			case LIT_CHAR : {
				//push literal
				expr_list.push_back(tok);
				//expect binary/unary operator
				if (peek_binary_operator() || peek_unary_operator()) {
					if (expect_binary_operator()) {
						token tok2 = compiler::lex->get_next();
						expr_list.push_back(tok2);
					}
					//if parethesis/identifier is found
					if (peek_token(PARENTH_OPEN) || peek_token(IDENTIFIER)) {
						primary_expr(terminator);
					}
					else if (peek_expr_literal()) {
						if (expect_literal()) {
							token tok2 = compiler::lex->get_next();
							expr_list.push_back(tok2);
						}
					}
					else if (peek_unary_operator()) {
						sub_primary_expr(terminator);
					}
					else {
						token tok2 = compiler::lex->get_next();
						log::error_at(tok2.loc, "literal or expression expected ", tok2.string);
						for (const auto &e: expr_list) {
							log::error(e.number, e.string, "\n");
						}
						std::cout << std::endl;
						return;
					}
					//if binary operator not found then peek for terminator
				}
				else if (peek_token(terminator)) {
					if (check_parenth()) {
						log::error("unbalanced parenthesis");
					}
					else {
						token tok2 = compiler::lex->get_next();
						//expr_list.push_back(tok2);
						is_expr_terminator_got = true;
						is_expr_terminator_consumed = true;
						consumed_terminator = tok2;
						return;
					}
				}
				else if (peek_token(PARENTH_CLOSE)) {
					primary_expr(terminator);
				}
				else {
					token tok2 = compiler::lex->get_next();
					if (!is_expr_terminator_got) {
						log::error(get_terminator(terminator) + " expected ");
						log::print_tokens(expr_list);
						compiler::lex->put_back(tok2);
						return;
					}
					else {
					
					}
					if (!check_parenth()) {
						log::error("unbalanced parenthesis");
						return;
					}
				}
				
				//peek for terminator or binary operator
				if (peek_token(terminator)) {
					token tok2 = compiler::lex->get_next();
					is_expr_terminator_got = true;
					is_expr_terminator_consumed = true;
					consumed_terminator = tok2;
					return;
				}
				else if (peek_binary_operator())
					sub_primary_expr(terminator);
				else {
					if (peek_token(PARENTH_CLOSE)) {
						if (parenth_stack.size() == 0) {
							token tok2 = compiler::lex->get_next();
							log::error_at(tok2.loc, "error ", tok2.string);
						}
					}
					else if (peek_token(END_OF_FILE)) {
						token tok2 = compiler::lex->get_next();
						if (check_parenth())
							log::error("unbalanced parenthesis");
						if (!is_expr_terminator_consumed) {
							log::error_at(tok2.loc, get_terminator(terminator) + "expected");
							return;
						}
					}
					else if (peek_expr_literal()) {
						token tok2 = compiler::lex->get_next();
						if (check_parenth())
							log::error("unbalanced parenthesis");
						if (!is_expr_terminator_got)
							log::error_at(tok2.loc, get_terminator(terminator) + "expected");
						compiler::lex->put_back(tok2);
					}
					else {
						if (!is_expr_terminator_consumed) {
							log::error(get_terminator(terminator) + "expected ");
							log::print_tokens(expr_list);
							return;
						}
					}
				}
			}
				break;
			
			case ARTHM_ADD :
			case ARTHM_SUB :
			case ARTHM_MUL :
			case ARTHM_DIV :
			case ARTHM_MOD :
			case LOG_AND :
			case LOG_OR :
			case COMP_LESS :
			case COMP_LESS_EQ :
			case COMP_GREAT :
			case COMP_GREAT_EQ :
			case COMP_EQ :
			case COMP_NOT_EQ :
			case LOG_NOT :
			case BIT_AND :
			case BIT_OR :
			case BIT_EXOR :
			case BIT_LSHIFT :
			case BIT_RSHIFT :
			case BIT_COMPL : {
				
				if (is_expr_terminator_got) {
					compiler::lex->put_back(tok);
					return;
				}
				
				if (unary_operator(tok.number)) {
					expr_list.push_back(tok);
					if (peek_token(PARENTH_OPEN) ||
					    peek_expr_literal() ||
					    peek_binary_operator() ||
					    peek_unary_operator() ||
					    peek_token(IDENTIFIER)) {
						sub_primary_expr(terminator);
					}
					else if (peek_token(INCR_OP))
						prefix_incr_expr(terminator);
					else if (peek_token(DECR_OP))
						prefix_decr_expr(terminator);
					else {
						token tok2 = compiler::lex->get_next();
						log::error_at(tok2.loc, "expression expected ", tok2.string);
					}
				}
				else {
					if (peek_token(PARENTH_OPEN) || peek_expr_literal() || peek_token(IDENTIFIER)) {
						expr_list.push_back(tok);
						sub_primary_expr(terminator);
					}
					else {
						token tok2 = compiler::lex->get_next();
						log::error_at(tok2.loc, "literal expected ", tok2.string);
						return;
					}
				}
			}
				break;
			
			case IDENTIFIER : {
				//if identifier
				//peek for binary operator otherwise call id_expr() function
				if (peek_binary_operator()) {
					expr_list.push_back(tok);
					sub_primary_expr(terminator);
				}
				else if (peek_token(terminator)) {
					expr_list.push_back(tok);
					tok = compiler::lex->get_next();
					is_expr_terminator_consumed = true;
					consumed_terminator = tok;
					return;
				}
				else if (peek_token(END_OF_FILE)) {
					expr_list.push_back(tok);
					log::error_at(tok.loc, get_terminator(terminator) + "expected");
					return;
				}
				else {
					compiler::lex->put_back(tok, true);
					if (parenth_stack.size() > 0) {
						terminator2.push_back(PARENTH_CLOSE);
						id_expr(terminator2);
					}
					else
						id_expr(terminator);
				}
				return;
			}
				break;
			
			default : {
				log::error_at(tok.loc, "primaryexpr invalid token ", tok.string);
				return;
			}
				break;
		}
	}
	
	void parser::sub_primary_expr(terminator_t &terminator) {
		if (expr_list.size() > 0)
			primary_expr(terminator);
	}
	
	// ()                 Parentheses: grouping or function call
	// [ ]                Brackets (array subscript)
	// .                  Member selection via object name
	// ->                 Member selection via pointer
	// ++ --              Postfix increment/decrement
	// ++ --              Prefix increment/decrement
	// + -                Unary plus/minus
	// ! ~                Logical negation/bitwise complement
	// (type)             Cast (convert value to temporary value of type)
	// *                  Dereference
	// &                  Address (of operand)
	// sizeof             Determine size in bytes on this implementation
	// * / %              Multiplication/division/modulus
	// + -                Addition/subtraction
	// << >>              Bitwise shift left, Bitwise shift right
	// < <=               Relational less than/less than or equal to
	// > >=               Relational greater than/greater than or equal to
	// == !=              Relational is equal to/is not equal to
	// &                  Bitwise AND
	// ^                  Bitwise exclusive OR
	// |                  Bitwise inclusive OR
	// &&                 Logical AND
	// ||                 Logical OR
	// =                  Assignment
	// += -=              Addition/subtraction assignment
	// *= /=              Multiplication/division assignment
	// %= &=              Modulus/bitwise AND assignment
	// ^= |=              Bitwise exclusive/inclusive OR assignment
	// <<= >>=            Bitwise shift left/right assignment
	// ,                  Comma (separate expressions)
	
	int parser::precedence(token_t opr) {
		switch (opr) {
			case DOT_OP : return 24;
			case ARROW_OP : return 23;
			case INCR_OP :
			case DECR_OP : return 22;
			case LOG_NOT :
			case BIT_COMPL: return 21;
			case ADDROF_OP : return 20;
			case KEY_SIZEOF : return 19;
			case ARTHM_MUL :
			case ARTHM_DIV :
			case ARTHM_MOD : return 18;
			case ARTHM_ADD :
			case ARTHM_SUB : return 17;
			case BIT_LSHIFT :
			case BIT_RSHIFT : return 16;
			case COMP_LESS :
			case COMP_LESS_EQ : return 15;
			case COMP_GREAT :
			case COMP_GREAT_EQ : return 14;
			case COMP_EQ :
			case COMP_NOT_EQ : return 13;
			case BIT_AND : return 12;
			case BIT_EXOR : return 11;
			case BIT_OR : return 10;
			case LOG_AND : return 9;
			case LOG_OR : return 8;
			case ASSGN : return 7;
			case ASSGN_ADD :
			case ASSGN_SUB : return 6;
			case ASSGN_MUL :
			case ASSGN_DIV : return 5;
			case ASSGN_MOD :
			case ASSGN_BIT_AND : return 4;
			case ASSGN_BIT_EX_OR :
			case ASSGN_BIT_OR : return 3;
			case ASSGN_LSHIFT :
			case ASSGN_RSHIFT : return 2;
			case COMMA_OP : return 1;
			default: return 0;
		}
	}
	
	//converts primary expression into its reverse polish notation
	//by checking the precedency of an operator
	void parser::postfix_expression(std::list<token> &postfix_expr) {
		std::stack<token> post_stack;
		auto expr_it = expr_list.begin();
		
		while (expr_it != expr_list.end()) {
			switch ((*expr_it).number) {
				case LIT_DECIMAL :
				case LIT_OCTAL :
				case LIT_HEX :
				case LIT_BIN :
				case LIT_FLOAT :
				case LIT_CHAR :
				case IDENTIFIER : postfix_expr.push_back(*expr_it);
					break;
				case ARTHM_ADD :
				case ARTHM_SUB :
				case ARTHM_MUL :
				case ARTHM_DIV :
				case ARTHM_MOD :
				case LOG_AND :
				case LOG_OR :
				case COMP_LESS :
				case COMP_LESS_EQ :
				case COMP_GREAT :
				case COMP_GREAT_EQ :
				case COMP_EQ :
				case COMP_NOT_EQ :
				case LOG_NOT :
				case BIT_AND :
				case BIT_OR :
				case BIT_EXOR :
				case BIT_LSHIFT :
				case BIT_RSHIFT :
				case BIT_COMPL :
				case DOT_OP :
				case ARROW_OP :
				case INCR_OP :
				case DECR_OP :
				case ADDROF_OP :
					if (post_stack.empty() || post_stack.top().number == PARENTH_OPEN)
						post_stack.push(*expr_it);
					else {
						if (!post_stack.empty() && (precedence((*expr_it).number) > precedence(post_stack.top().number))) {
							post_stack.push(*expr_it);
						}
						else {
							while (!post_stack.empty() && (precedence((*expr_it).number) <= precedence(post_stack.top().number))) {
								postfix_expr.push_back(post_stack.top());
								post_stack.pop();
							}
							post_stack.push(*expr_it);
						}
					}
					break;
				case PARENTH_OPEN : post_stack.push(*expr_it);
					break;
				case PARENTH_CLOSE :
					while (!post_stack.empty() && post_stack.top().number != PARENTH_OPEN) {
						postfix_expr.push_back(post_stack.top());
						post_stack.pop();
					}
					if (!post_stack.empty() && post_stack.top().number == PARENTH_OPEN)
						post_stack.pop();
					break;
				case SQUARE_OPEN_BRACKET :
					while (expr_it != expr_list.end() && (*expr_it).number != SQUARE_CLOSE_BRACKET) {
						postfix_expr.push_back(*expr_it);
						expr_it++;
					}
					postfix_expr.push_back(*expr_it);
					break;
					
					//if ; , then exit
				case SEMICOLON :
				case COMMA_OP : goto exit;
				
				default : log::error_at((*expr_it).loc, "error in conversion into postfix expression ", (*expr_it).string);
					return;
			}
			expr_it++;
		}
		
		exit :
		while (!post_stack.empty()) {
			postfix_expr.push_back(post_stack.top());
			post_stack.pop();
		}
	}
	
	//returns primary expression tree by building it from reverse polish notation
	//of an primary expression
	primary_expr_t *parser::get_primary_expr_tree() {
		std::stack<primary_expr_t *> extree_stack;
		std::list<token>::iterator post_it;
		primary_expr_t *expr = nullptr, *oprtr = nullptr;
		std::list<token> postfix_expr;
		token unary_tok = nulltoken;
		
		postfix_expression(postfix_expr);
		
		//first check for size, there could be only an identifier
		if (postfix_expr.size() == 1) {
			expr = tree::get_primary_expr_mem();
			expr->tok = postfix_expr.front();
			expr->is_oprtr = false;
			post_it = postfix_expr.begin();
			if (post_it->number == IDENTIFIER)
				expr->is_id = true;
			else
				expr->is_id = false;
			return expr;
		}
		
		for (post_it = postfix_expr.begin(); post_it != postfix_expr.end(); post_it++) {
			//if literal/identifier is found, push it onto stack
			if (expr_literal((*post_it).number)) {
				expr = tree::get_primary_expr_mem();
				expr->tok = *post_it;
				expr->is_id = false;
				expr->is_oprtr = false;
				extree_stack.push(expr);
				expr = nullptr;
			}
			else if ((*post_it).number == IDENTIFIER) {
				expr = tree::get_primary_expr_mem();
				expr->tok = *post_it;
				expr->is_id = true;
				expr->is_oprtr = false;
				extree_stack.push(expr);
				expr = nullptr;
				//if operator is found, pop last two entries from stack,
				//assign left and right nodes and push the generated tree into stack again
			}
			else if (binary_operator((*post_it).number) || (*post_it).number == DOT_OP || (*post_it).number == ARROW_OP) {
				oprtr = tree::get_primary_expr_mem();
				oprtr->tok = *post_it;
				oprtr->is_id = false;
				oprtr->is_oprtr = true;
				oprtr->oprtr_kind = BINARY_OP;
				if (extree_stack.size() > 1) {
					oprtr->right = extree_stack.top();
					extree_stack.pop();
					oprtr->left = extree_stack.top();
					extree_stack.pop();
					extree_stack.push(oprtr);
				}
			}
			else if ((*post_it).number == BIT_COMPL || (*post_it).number == LOG_NOT) {
				unary_tok = *post_it;
			}
		}
		
		postfix_expr.clear();
		
		if (unary_tok.number != NONE) {
			oprtr = tree::get_primary_expr_mem();
			oprtr->tok = unary_tok;
			oprtr->is_id = false;
			oprtr->is_oprtr = true;
			oprtr->oprtr_kind = UNARY_OP;
			
			if (!extree_stack.empty())
				oprtr->unary_node = extree_stack.top();
			
			return oprtr;
		}
		
		if (!extree_stack.empty())
			return extree_stack.top();
		
		return nullptr;
	}
	
	//returns identifier expression tree
	//this is same as primary expression tree generation
	//only just handling member access operators as well as
	//increment/decrement/addressof operators
	id_expr_t *parser::get_id_expr_tree() {
		std::stack<id_expr_t *> extree_stack;
		std::list<token>::iterator post_it;
		std::list<token> postfix_expr;
		id_expr_t *expr = nullptr, *oprtr = nullptr;
		id_expr_t *temp = nullptr;
		
		postfix_expression(postfix_expr);
		
		for (post_it = postfix_expr.begin(); post_it != postfix_expr.end(); post_it++) {
			if ((*post_it).number == IDENTIFIER) {
				expr = tree::get_id_expr_mem();
				expr->tok = *post_it;
				expr->is_id = true;
				expr->is_oprtr = false;
				post_it++;
				if (post_it != postfix_expr.end()) {
					if ((*post_it).number == SQUARE_OPEN_BRACKET) {
						expr->is_subscript = true;
					}
				}
				post_it--;
				extree_stack.push(expr);
				expr = nullptr;
			}
			else if (binary_operator((*post_it).number) || (*post_it).number == DOT_OP || (*post_it).number == ARROW_OP) {
				oprtr = tree::get_id_expr_mem();
				oprtr->tok = *post_it;
				oprtr->is_id = false;
				oprtr->is_oprtr = true;
				oprtr->is_subscript = false;
				if (extree_stack.size() > 1) {
					oprtr->right = extree_stack.top();
					extree_stack.pop();
					oprtr->left = extree_stack.top();
					extree_stack.pop();
					extree_stack.push(oprtr);
				}
			}
			else if ((*post_it).number == INCR_OP || (*post_it).number == DECR_OP || (*post_it).number == ADDROF_OP) {
				oprtr = tree::get_id_expr_mem();
				oprtr->tok = *post_it;
				oprtr->is_id = false;
				oprtr->is_oprtr = true;
				oprtr->is_subscript = false;
				oprtr->unary = tree::get_id_expr_mem();
				if (extree_stack.size() > 0) {
					oprtr->unary = extree_stack.top();
					extree_stack.pop();
					extree_stack.push(oprtr);
				}
			}
			else if ((*post_it).number == SQUARE_OPEN_BRACKET) {
				post_it++;
				if (!extree_stack.empty()) {
					temp = extree_stack.top();
					(temp->subscript).push_back(*post_it);
				}
				post_it++;
			}
		}
		
		postfix_expr.clear();
		
		if (!extree_stack.empty())
			return extree_stack.top();
		
		return nullptr;
	}
	
	/*
	id-expression :
	  identifier
	  identifier . id-expression
	  identifier -> id-expression
	  identifier subscript-id-access
	  pointer-indirection-access
	*/
	void parser::id_expr(terminator_t &terminator) {
		token tok = compiler::lex->get_next();
		
		if (tok.number == IDENTIFIER) {
			expr_list.push_back(tok);
			
			if (peek_token(terminator)) {
				token tok2 = compiler::lex->get_next();
				if (parenth_stack.size() > 0) {
					compiler::lex->put_back(tok2);
					return;
				}
				is_expr_terminator_consumed = true;
				consumed_terminator = tok2;
				return;
			}
			else if (peek_token(SQUARE_OPEN_BRACKET))
				subscript_id_access(terminator);
				//peek for binary/unary operators
			else if (peek_binary_operator() || peek_unary_operator())
				primary_expr(terminator);
				//peek for ++
			else if (peek_token(INCR_OP))
				postfix_incr_expr(terminator);
				//peek for --
			else if (peek_token(DECR_OP))
				postfix_decr_expr(terminator);
				//peek for . ->
			else if (peek_token(DOT_OP) || peek_token(ARROW_OP)) {
				token tok2 = compiler::lex->get_next();
				expr_list.push_back(tok2);
				id_expr(terminator);
			}
			else if (peek_assignment_operator() || peek_token(PARENTH_OPEN)) {
				return;    //if found do nothing
			}
			else {
				tok = compiler::lex->get_next();
				std::string st = get_terminator(terminator);
				log::error_at(tok.loc, st + " expected in id expression but found ", tok.string);
				log::print_tokens(expr_list);
				return;
			}
		}
		else {
			log::error_at(tok.loc, " identifier expected but found ", tok.string);
			log::print_tokens(expr_list);
		}
	}
	
	/*
	subscript-id-access :
	  [ identifier ]
	  [ constant-expression ]
	  [ id-expression ] subscript-id-access
	  [ constant-expression ] subscript-id-access
	  [ identifier ] . id-expression
	  [ constant-expression ] -> id-expression
	*/
	void parser::subscript_id_access(terminator_t &terminator) {
		//expect [
		if (expect(SQUARE_OPEN_BRACKET)) {
			token tok = compiler::lex->get_next();
			expr_list.push_back(tok);
			
			//peek constant expression or identifier
			if (peek_constant_expr() || peek_identifier()) {
				tok = compiler::lex->get_next();
				expr_list.push_back(tok);
				
				//expect ]
				if (expect(SQUARE_CLOSE_BRACKET)) {
					tok = compiler::lex->get_next();
					expr_list.push_back(tok);
				}
				
				//peek [ for multi dimensional array
				if (peek_token(SQUARE_OPEN_BRACKET))
					subscript_id_access(terminator);
				else if (peek_token(DOT_OP) || peek_token(ARROW_OP)) {
					token tok2 = compiler::lex->get_next();
					expr_list.push_back(tok2);
					id_expr(terminator);
				}
				else if (peek_token(terminator)) {
					is_expr_terminator_consumed = false;
					return;
				}
				else if (peek_assignment_operator())
					return;
				else {
					log::error("; , ) expected ");
					log::print_tokens(expr_list);
					return;
				}
			}
			else {
				token tok2 = compiler::lex->get_next();
				log::error("constant expression expected ", tok2.string);
				log::print_tokens(expr_list);
				
			}
		}
	}
	
	/*
	pointer-operator-sequence :
	  pointer-operator
	  pointer-operator pointer-operator-sequence

	pointer-operator :
	  *
	*/
	void parser::pointer_operator_sequence() {
		token tok;
		//here ARTHM_MUL token will be changed to PTR_OP
		while ((tok = compiler::lex->get_next()).number == ARTHM_MUL) {
			tok.number = PTR_OP;
			expr_list.push_back(tok);
		}
		compiler::lex->put_back(tok);
	}
	
	int parser::get_pointer_operator_sequence() {
		int ptr_count = 0;
		token tok;
		//here ARTHM_MUL token will be changed to PTR_OP
		while ((tok = compiler::lex->get_next()).number == ARTHM_MUL) {
			ptr_count++;
		}
		compiler::lex->put_back(tok);
		return ptr_count;
	}
	
	// pointer-indirection-access : pointer-operator-sequence id-expression
	void parser::pointer_indirection_access(terminator_t &terminator) {
		pointer_operator_sequence();
		if (peek_token(IDENTIFIER))
			id_expr(terminator);
		else {
			log::error("identifier expected in pointer indirection");
			log::print_tokens(expr_list);
			
		}
	}
	
	// prefix-incr-expression : incr-operator id-expression
	id_expr_t *parser::prefix_incr_expr(terminator_t &terminator) {
		id_expr_t *pridexpr = nullptr;
		if (expect(INCR_OP)) {
			token tok = compiler::lex->get_next();
			expr_list.push_back(tok);
		}
		if (peek_token(IDENTIFIER)) {
			id_expr(terminator);
			pridexpr = get_id_expr_tree();
			return pridexpr;
		}
		else {
			log::error("identifier expected ");
			log::print_tokens(expr_list);
			
			return nullptr;
		}
		return nullptr;
	}
	
	// prefix-decr-expression : decr-operator id-expression
	id_expr_t *parser::prefix_decr_expr(terminator_t &terminator) {
		id_expr_t *pridexpr = nullptr;
		if (expect(DECR_OP)) {
			token tok = compiler::lex->get_next();
			expr_list.push_back(tok);
		}
		if (peek_token(IDENTIFIER)) {
			id_expr(terminator);
			pridexpr = get_id_expr_tree();
			return pridexpr;
		}
		else {
			log::error("identifier expected ");
			log::print_tokens(expr_list);
			
			return nullptr;
		}
		return nullptr;
	}
	
	// postfix-incr-expression : id-expression incr-operator
	void parser::postfix_incr_expr(terminator_t &terminator) {
		token tok;
		if (expect(INCR_OP)) {
			tok = compiler::lex->get_next();
			expr_list.push_back(tok);
		}
		if (peek_token(terminator)) {
			tok = compiler::lex->get_next();
			is_expr_terminator_consumed = true;
			consumed_terminator = tok;
			return;
		}
		else {
			tok = compiler::lex->get_next();
			log::error_at(tok.loc, "; , ) expected but found " + tok.string);
			log::print_tokens(expr_list);
			
			return;
		}
	}
	
	// postfix-decr-expression : id-expression decr-operator
	void parser::postfix_decr_expr(terminator_t &terminator) {
		if (expect(DECR_OP)) {
			token tok = compiler::lex->get_next();
			expr_list.push_back(tok);
		}
		if (peek_token(terminator)) {
			token tok = compiler::lex->get_next();
			//expr_list.push_back(tok);
			is_expr_terminator_consumed = true;
			consumed_terminator = tok;
			return;
		}
		else {
			log::error("; , ) expected ");
			log::print_tokens(expr_list);
			return;
		}
	}
	
	// address-of-expression : & id-expression
	id_expr_t *parser::address_of_expr(terminator_t &terminator) {
		id_expr_t *addrexpr = nullptr;
		if (expect(BIT_AND)) {
			token tok = compiler::lex->get_next();
			//change token bitwise and to address of operator
			tok.number = ADDROF_OP;
			expr_list.push_back(tok);
			id_expr(terminator);
			addrexpr = tree::get_id_expr_mem();
			addrexpr = get_id_expr_tree();
			return addrexpr;
		}
		return nullptr;
	}
	
	// sizeof-expression : sizeof ( simple-type-specifier ) | sizeof ( identifier )
	sizeof_expr_t *parser::sizeof_expr(terminator_t &terminator) {
		sizeof_expr_t *sizeofexpr = tree::get_sizeof_expr_mem();
		std::vector<token> simple_types;
		terminator_t terminator2;
		size_t i;
		token tok;
		int ptr_count = 0;
		
		expect(KEY_SIZEOF, true);
		expect(PARENTH_OPEN, true);
		
		if (peek_type_specifier(simple_types)) {
			if (simple_types.size() == 1 && simple_types[0].number == IDENTIFIER) {
				sizeofexpr->is_simple_type = false;
				sizeofexpr->identifier = simple_types[0];
			}
			else {
				sizeofexpr->is_simple_type = true;
				for (i = 0; i < simple_types.size(); i++)
					sizeofexpr->simple_type.push_back(simple_types[i]);
			}
			consume_n(simple_types.size());
			simple_types.clear();
			if (peek_token(ARTHM_MUL)) {
				ptr_count = get_pointer_operator_sequence();
				sizeofexpr->is_ptr = true;
				sizeofexpr->ptr_oprtr_count = ptr_count;
			}
		}
		else {
			log::error("simple types, class names or identifier expected for sizeof ");
			terminator2.clear();
			terminator2.push_back(PARENTH_CLOSE);
			terminator2.push_back(SEMICOLON);
			terminator2.push_back(COMMA_OP);
			consume_till(terminator2);
		}
		expect(PARENTH_CLOSE, true);
		if (peek_token(terminator)) {
			is_expr_terminator_consumed = true;
			consumed_terminator = compiler::lex->get_next();
			return sizeofexpr;
		}
		else {
			tok = compiler::lex->get_next();
			log::error_at(tok.loc, " ; , expected but found ", tok.string);
		}
		delete sizeofexpr;
		return nullptr;
	}
	
	/*
	cast-expression :
	  ( cast-type-specifier ) identifier

	cast-type-specifier :
	  simple-type-specifier
	  identifier
	  simple-type-specifier pointer-operator-sequence
	  identifier pointer-operator-sequence
	*/
	cast_expr_t *parser::cast_expr(terminator_t &terminator) {
		cast_expr_t *cstexpr = tree::get_cast_expr_mem();
		token tok;
		expect(PARENTH_OPEN, true);
		cast_type_specifier(&cstexpr);
		expect(PARENTH_CLOSE, true);
		if (peek_token(IDENTIFIER)) {
			id_expr(terminator);
			cstexpr->target = get_id_expr_tree();
			return cstexpr;
		}
		else {
			tok = compiler::lex->get_next();
			log::error_at(tok.loc, " identifier expected in cast expression");
		}
		delete cstexpr;
		return nullptr;
	}
	
	void parser::cast_type_specifier(cast_expr_t **expr) {
		cast_expr_t *cstexpr = *expr;
		std::vector<token> simple_types;
		terminator_t terminator2;
		size_t i;
		token tok;
		
		if (peek_type_specifier(simple_types)) {
			if (simple_types.size() > 0 && simple_types[0].number == IDENTIFIER) {
				cstexpr->is_simple_type = false;
				cstexpr->identifier = simple_types[0];
			}
			else {
				cstexpr->is_simple_type = true;
				for (i = 0; i < simple_types.size(); i++) {
					cstexpr->simple_type.push_back(simple_types[i]);
				}
			}
			consume_n(simple_types.size());
			simple_types.clear();
		}
		else {
			tok = compiler::lex->get_next();
			log::error_at(tok.loc, "simple type or record name for casting ");
			terminator2.clear();
			terminator2.push_back(PARENTH_CLOSE);
			terminator2.push_back(SEMICOLON);
			terminator2.push_back(COMMA_OP);
			consume_till(terminator2);
			
		}
		if (peek_token(ARTHM_MUL))
			cstexpr->ptr_oprtr_count = get_pointer_operator_sequence();
	}
	
	// assignment-expression : id-expression assignment-operator expression
	assgn_expr_t *parser::assignment_expr(terminator_t &terminator, bool is_left_side_handled) {
		assgn_expr_t *assexpr = nullptr;
		id_expr_t *idexprtree = nullptr;
		expr *_expr = nullptr;
		id_expr_t *ptr_ind = nullptr;
		
		token tok;
		if (expect_assignment_operator()) {
			tok = compiler::lex->get_next();
			assexpr = tree::get_assgn_expr_mem();
			assexpr->tok = tok;
			
			if (!is_left_side_handled) {
				idexprtree = get_id_expr_tree();
				
				if (ptr_oprtr_count > 0) {
					ptr_ind = tree::get_id_expr_mem();
					ptr_ind->is_ptr = true;
					ptr_ind->ptr_oprtr_count = ptr_oprtr_count;
					ptr_ind->unary = idexprtree;
					idexprtree = ptr_ind;
				}
				assexpr->id_expr = idexprtree;
			}
			
			expr_list.clear();
			_expr = expression(terminator);
			assexpr->expression = _expr;
			return assexpr;
		}
		else {
			tok = compiler::lex->get_next();
			log::error_at(tok.loc, " assignment operator expected but found ", tok.string);
		}
		return nullptr;
	}
	
	/*
	function-call-expression :
	  identifier ( )
	  identifier ( expression-list )
	*/
	call_expr_t *parser::call_expr(terminator_t &terminator) {
		call_expr_t *funccallexp = nullptr;
		std::list<expr *> exprlist;
		id_expr_t *idexpr = nullptr;
		token tok;
		
		idexpr = get_id_expr_tree();
		
		funccallexp = tree::get_func_call_expr_mem();
		funccallexp->function = idexpr;
		
		expect(PARENTH_OPEN, true);
		
		if (peek_token(PARENTH_CLOSE)) {
			consume_next();
			if (peek_token(terminator)) {
				consume_next();
				return funccallexp;
			}
			else {
				tok = compiler::lex->get_next();
				log::error_at(tok.loc, get_terminator(terminator) + " expected in function call but found: " + tok.string);
			}
		}
		else {
			is_expr_terminator_consumed = false;
			expr_list.clear();
			func_call_expr_list(exprlist, terminator);
			
			if (is_expr_terminator_consumed) {
				if (consumed_terminator.number == PARENTH_CLOSE) {
					if (peek_token(terminator)) {
						consume_next();
						funccallexp->expression_list = exprlist;
						return funccallexp;
					}
					else {
						tok = compiler::lex->get_next();
						log::error_at(tok.loc,
								get_terminator(terminator) + " expected in function call but found " + tok.string);
					}
				}
				else {
					tok = compiler::lex->get_next();
					log::error_at(tok.loc, get_terminator(terminator) + " expected in function call but found " + tok.string);
				}
			}
			else {
				expect(PARENTH_CLOSE, true);
				if (peek_token(terminator)) {
					consume_next();
					funccallexp->expression_list = exprlist;
					return funccallexp;
				}
				else {
					tok = compiler::lex->get_next();
					log::error_at(tok.loc, get_terminator(terminator) + " expected in function call but found " + tok.string);
				}
			}
		}
		
		tree::delete_func_call_expr(&funccallexp);
		return nullptr;
	}
	
	/*
	func-call-expression-list :
	  expression
	  expression , func-call-expression-list
	*/
	void parser::func_call_expr_list(std::list<expr *> &exprlist, terminator_t &orig_terminator) {
		expr *_expr = nullptr;
		token tok;
		terminator_t terminator = {COMMA_OP, PARENTH_CLOSE};
		
		if (peek_expr_token() || peek_token(LIT_STRING)) {
			is_expr_terminator_consumed = false;
			_expr = expression(terminator);
			
			if (is_expr_terminator_consumed) {
				if (consumed_terminator.number == PARENTH_CLOSE) {
					exprlist.push_back(_expr);
					//is_expr_terminator_consumed = true;
					//consumed_terminator = compiler::lex->get_next();
					return;
				}
				else if (consumed_terminator.number == COMMA_OP) {
					exprlist.push_back(_expr);
					func_call_expr_list(exprlist, orig_terminator);
				}
			}
			else if (peek_token(COMMA_OP)) {
				consume_next();
				exprlist.push_back(_expr);
				func_call_expr_list(exprlist, orig_terminator);
			}
			else if (peek_token(PARENTH_CLOSE)) {
				exprlist.push_back(_expr);
				is_expr_terminator_consumed = false;
				return;
			}
			else {
				tok = compiler::lex->get_next();
				if (is_expr_terminator_consumed) {
					if (consumed_terminator.number == PARENTH_CLOSE) {
						return;
					}
					else {
						tok = compiler::lex->get_next();
						log::error_at(tok.loc, "invalid token found in function call parameters " + tok.string);
					}
				}
				else {
					tok = compiler::lex->get_next();
					log::error_at(tok.loc, get_terminator(terminator) + " expected in function call but found " + tok.string);
				}
			}
		}
		else {
			if (is_expr_terminator_consumed) {
				if (consumed_terminator.number == PARENTH_CLOSE)
					return;
				else {
					tok = compiler::lex->get_next();
					log::error_at(tok.loc, "invalid token found in function call parameters " + tok.string);
				}
			}
			else {
				tok = compiler::lex->get_next();
				log::error_at(tok.loc, get_terminator(terminator) + " expected in function call but found " + tok.string);
			}
		}
	}
	
	/*
	expression :
	  primary-expression
	  assignment-expression
	  sizeof-expression
	  cast-expression
	  id-expression
	  function-call-expression
	*/
	expr *parser::expression(terminator_t &terminator) {
		token tok, tok2;
		std::vector<token> specifier;
		std::vector<token>::iterator lst_it;
		terminator_t terminator2;
		sizeof_expr_t *sizeofexpr = nullptr;
		cast_expr_t *castexpr = nullptr;
		primary_expr_t *pexpr = nullptr;
		id_expr_t *idexpr = nullptr;
		assgn_expr_t *assgnexpr = nullptr;
		call_expr_t *funcclexpr = nullptr;
		expr *_expr = tree::get_expr_mem();
		
		if (peek_token(terminator))
			return nullptr;
		
		tok = compiler::lex->get_next();
		
		switch (tok.number) {
			case LIT_DECIMAL :
			case LIT_OCTAL :
			case LIT_HEX :
			case LIT_BIN :
			case LIT_FLOAT :
			case LIT_CHAR :
			case ARTHM_ADD :
			case ARTHM_SUB :
			case LOG_NOT :
			case BIT_COMPL : compiler::lex->put_back(tok);
				primary_expr(terminator);
				pexpr = get_primary_expr_tree();
				if (pexpr == nullptr) {
					log::error("error to parse primary expression");
					tree::delete_expr(&_expr);
					return nullptr;
				}
				_expr->expr_kind = PRIMARY_EXPR;
				_expr->primary_expr = pexpr;
				expr_list.clear();
				is_expr_terminator_got = false;
				break;
			case LIT_STRING : pexpr = tree::get_primary_expr_mem();
				pexpr->is_id = false;
				pexpr->tok = tok;
				pexpr->is_oprtr = false;
				_expr->expr_kind = PRIMARY_EXPR;
				_expr->primary_expr = pexpr;
				if (!peek_token(terminator)) {
					log::error_at(tok.loc, "semicolon expected " + tok.string);
					tree::delete_expr(&_expr);
					return nullptr;
				}
				expr_list.clear();
				is_expr_terminator_got = false;
				break;
			case IDENTIFIER :
				//peek for . -> [
				if (peek_token(DOT_OP) || peek_token(ARROW_OP) || peek_token(SQUARE_OPEN_BRACKET)) {
					compiler::lex->put_back(tok, true);
					//get id expression
					id_expr(terminator);
					//peek for assignment operator(e.g id-expression = expression)
					if (peek_assignment_operator()) {
						//get assignment expression
						assgnexpr = assignment_expr(terminator, false);
						if (assgnexpr == nullptr) {
							log::error("error to parse assignment expression");
							tree::delete_expr(&_expr);
							return nullptr;
						}
						_expr->expr_kind = ASSGN_EXPR;
						_expr->assgn_expr = assgnexpr;
						//peek terminator
					}
					else if (peek_token(terminator)) {
						tok2 = compiler::lex->get_next();
						is_expr_terminator_consumed = true;
						consumed_terminator = tok2;
						//get id expression tree
						idexpr = get_id_expr_tree();
						if (idexpr == nullptr) {
							log::error("error to parse id expression");
							tree::delete_expr(&_expr);
							return nullptr;
						}
						_expr->expr_kind = ID_EXPR;
						_expr->id_expr = idexpr;
						//peek (
					}
					else if (peek_token(PARENTH_OPEN)) {
						//get function call expression
						funcclexpr = call_expr(terminator);
						if (funcclexpr == nullptr) {
							log::error("error to parse function call expression");
							tree::delete_expr(&_expr);
							return nullptr;
						}
						_expr->expr_kind = FUNC_CALL_EXPR;
						_expr->call_expr = funcclexpr;
					}
					else if (peek_token(PARENTH_CLOSE)) {
					}
					else {
						idexpr = get_id_expr_tree();
						if (idexpr == nullptr) {
							log::error("error to parse id expression");
							tree::delete_expr(&_expr);
							return nullptr;
						}
						_expr->expr_kind = ID_EXPR;
						_expr->id_expr = idexpr;
					}
					expr_list.clear();
					is_expr_terminator_got = false;
					//peek for (
				}
				else if (peek_token(PARENTH_OPEN)) {
					compiler::lex->put_back(tok, true);
					//get id expression
					id_expr(terminator);
					//get function call expression
					funcclexpr = call_expr(terminator);
					if (funcclexpr == nullptr) {
						log::error("error to parse function call expression");
						tree::delete_expr(&_expr);
						return nullptr;
					}
					_expr->expr_kind = FUNC_CALL_EXPR;
					_expr->call_expr = funcclexpr;
					//peek for ++ --
				}
				else if (peek_token(INCR_OP) || peek_token(DECR_OP)) {
					compiler::lex->put_back(tok, true);
					id_expr(terminator);
					idexpr = get_id_expr_tree();
					if (idexpr == nullptr) {
						log::error("error to parse id expression");
						tree::delete_expr(&_expr);
						return nullptr;
					}
					_expr->expr_kind = ID_EXPR;
					_expr->id_expr = idexpr;
				}
				else {
					compiler::lex->put_back(tok, true);
					primary_expr(terminator);
					if (peek_assignment_operator()) {
						assgnexpr = assignment_expr(terminator, false);
						if (assgnexpr == nullptr) {
							log::error("error to parse assignment expression");
							tree::delete_expr(&_expr);
							return nullptr;
						}
						_expr->expr_kind = ASSGN_EXPR;
						_expr->assgn_expr = assgnexpr;
					}
					else {
						pexpr = get_primary_expr_tree();
						if (pexpr == nullptr) {
							log::error("error to parse primary expression");
							tree::delete_expr(&_expr);
							return nullptr;
						}
						_expr->expr_kind = PRIMARY_EXPR;
						_expr->primary_expr = pexpr;
						is_expr_terminator_got = false;
					}
				}
				expr_list.clear();
				is_expr_terminator_got = false;
				break;
			
			case PARENTH_OPEN : tok2 = compiler::lex->get_next();
				//peek for type specifier for cast expression
				if (type_specifier(tok2.number) || symtable::search_record(compiler::record_table, tok2.string)) {
					compiler::lex->put_back(tok);
					compiler::lex->put_back(tok2);
					castexpr = cast_expr(terminator);
					if (castexpr == nullptr) {
						log::error("error to parse cast expression");
						tree::delete_expr(&_expr);
						return nullptr;
					}
					_expr->expr_kind = CAST_EXPR;
					_expr->cast_expr = castexpr;
				}
				else if (tok2.number == END_OF_FILE)
					return nullptr;
					//otherwise primarry expression
				else {
					compiler::lex->put_back(tok);
					compiler::lex->put_back(tok2);
					primary_expr(terminator);
					pexpr = get_primary_expr_tree();
					if (pexpr == nullptr) {
						log::error("error to parse primary expression");
						tree::delete_expr(&_expr);
						return nullptr;
					}
					_expr->expr_kind = PRIMARY_EXPR;
					_expr->primary_expr = pexpr;
				}
				expr_list.clear();
				is_expr_terminator_got = false;
				break;
			case ARTHM_MUL : compiler::lex->put_back(tok);
				//get pointer indirection id expression
				pointer_indirection_access(terminator);
				
				//get pointer operator count from expression list
				lst_it = expr_list.begin();
				while (lst_it != expr_list.end()) {
					if ((*lst_it).number == PTR_OP) {
						ptr_oprtr_count++;
						expr_list.erase(lst_it);
						lst_it = expr_list.begin();
					}
					else
						break;
				}
				//peek for assignment operator
				if (peek_assignment_operator()) {
					assgnexpr = assignment_expr(terminator, false);
					if (assgnexpr == nullptr) {
						log::error("error to parse assignment expression");
						tree::delete_expr(&_expr);
						return nullptr;
					}
					_expr->expr_kind = ASSGN_EXPR;
					_expr->assgn_expr = assgnexpr;
				}
				else {
					idexpr = get_id_expr_tree();
					if (idexpr == nullptr) {
						log::error("error to parse pointer indirection expression");
						tree::delete_expr(&_expr);
						return nullptr;
					}
					idexpr->is_ptr = true;
					idexpr->ptr_oprtr_count = ptr_oprtr_count;
					_expr->expr_kind = ID_EXPR;
					_expr->id_expr = idexpr;
					ptr_oprtr_count = 0;
				}
				expr_list.clear();
				is_expr_terminator_got = false;
				break;
			case INCR_OP : compiler::lex->put_back(tok);
				//get prefix increment expression
				idexpr = prefix_incr_expr(terminator);
				if (idexpr == nullptr) {
					log::error("error to parse increment expression");
					tree::delete_expr(&_expr);
					return nullptr;
				}
				//peek for assignment operator
				if (peek_assignment_operator()) {
					assgnexpr = assignment_expr(terminator, true);
					if (assgnexpr == nullptr) {
						log::error("error to parse passignment expression");
						tree::delete_expr(&_expr);
						return nullptr;
					}
					_expr->expr_kind = ASSGN_EXPR;
					assgnexpr->id_expr = idexpr;
					_expr->assgn_expr = assgnexpr;
				}
				else {
					_expr->expr_kind = ID_EXPR;
					_expr->id_expr = idexpr;
				}
				expr_list.clear();
				is_expr_terminator_got = false;
				break;
			case DECR_OP : compiler::lex->put_back(tok);
				//get prefix decrement operator
				idexpr = prefix_decr_expr(terminator);
				if (idexpr == nullptr) {
					log::error("error to parse decrement expression");
					tree::delete_expr(&_expr);
					
					return nullptr;
				}
				//peek for assignment operator
				if (peek_assignment_operator()) {
					assgnexpr = assignment_expr(terminator, true);
					if (assgnexpr == nullptr) {
						log::error("error to parse assignment expression");
						tree::delete_expr(&_expr);
						return nullptr;
					}
					_expr->expr_kind = ASSGN_EXPR;
					assgnexpr->id_expr = idexpr;
					_expr->assgn_expr = assgnexpr;
				}
				else {
					_expr->expr_kind = ID_EXPR;
					_expr->id_expr = idexpr;
				}
				expr_list.clear();
				is_expr_terminator_got = false;
				break;
			case BIT_AND : compiler::lex->put_back(tok);
				//get address of expression
				idexpr = address_of_expr(terminator);
				if (idexpr == nullptr) {
					log::error("error to parse addressof expression");
					tree::delete_expr(&_expr);
					return nullptr;
				}
				_expr->expr_kind = ID_EXPR;
				_expr->id_expr = idexpr;
				expr_list.clear();
				is_expr_terminator_got = false;
				break;
			case KEY_SIZEOF : compiler::lex->put_back(tok);
				//get sizeof expression
				sizeofexpr = sizeof_expr(terminator);
				if (sizeofexpr == nullptr) {
					log::error("error to parse sizeof expression");
					tree::delete_expr(&_expr);
					return nullptr;
				}
				_expr->expr_kind = SIZEOF_EXPR;
				_expr->sizeof_expr = sizeofexpr;
				break;
			case PARENTH_CLOSE :
			case SEMICOLON : tree::delete_expr(&_expr);
				expr_list.clear();
				is_expr_terminator_got = false;
				is_expr_terminator_consumed = true;
				consumed_terminator = tok;
				return nullptr;
			default : log::error_at(tok.loc, "invalid token found in expression ", tok.string);
				consume_next();
				return nullptr;
		}
		return _expr;
	}
	
	/*
	record-specifier :
	  record-head { record-member-definition }
	*/
	void parser::record_specifier() {
		st_record_node *rec = nullptr;
		token tok;
		bool isglob = false;
		bool isextrn = false;
		//get record head
		if (record_head(&tok, &isglob, &isextrn)) {
			//search recordname in record table
			if (symtable::search_record(compiler::record_table, tok.string)) {
				log::error_at(tok.loc, "record " + tok.string + " already exists");
				return;
			}
			//otherwise insert record into record table
			symtable::insert_record(&compiler::record_table, tok.string);
			rec = last_rec_node;
			rec->is_global = isglob;
			rec->is_extern = isextrn;
			rec->recordtok = tok;
			rec->recordname = tok.string;
			expect(CURLY_OPEN_BRACKET, true);
			//get record member definitions and store them in record symbol table
			record_member_definition(&rec);
			expect(CURLY_CLOSE_BRACKET, true);
		}
		else
			log::error("invalid record definition");
	}
	
	/*
	record-head :
	  global record record-name
	  record record-name
	*/
	bool parser::record_head(token *tok, bool *isglob, bool *isextern) {
		if (peek_token(KEY_GLOBAL)) {
			expect(KEY_GLOBAL, true);
			*isglob = true;
		}
		else if (peek_token(KEY_EXTERN)) {
			expect(KEY_EXTERN, true);
			*isextern = true;
		}
		if (expect(KEY_RECORD, true)) {
			if (expect(IDENTIFIER, false)) {
				*tok = compiler::lex->get_next();
				return true;
			}
		}
		return false;
	}
	
	/*
	record-member-definition :
	  type-specifier rec-id-list
	  void rec-id-list
	*/
	void parser::record_member_definition(st_record_node **rec) {
		token tok;
		std::vector<token> types;
		st_type_info *typeinf = nullptr;
		
		while ((tok = compiler::lex->get_next()).number != END_OF_FILE) {
			compiler::lex->put_back(tok);
			//peek type specifier or record type identifier
			if (peek_type_specifier() || peek_token(IDENTIFIER)) {
				get_type_specifier(types);
				typeinf = symtable::get_type_info_mem();
				typeinf->type = SIMPLE_TYPE;
				typeinf->type_specifier.simple_type.clear();
				typeinf->type_specifier.simple_type.assign(types.begin(), types.end());
				if (types.size() == 1 && types[0].number == IDENTIFIER) {
					if (symtable::search_record(compiler::record_table, types[0].string)) {
						typeinf->type = RECORD_TYPE;
						typeinf->type_specifier.record_type = types[0];
						typeinf->type_specifier.simple_type.clear();
					}
					else
						log::error_at(types[0].loc, "record '" + types[0].string + "' does not exists");
				}
				consume_n(types.size());
				rec_id_list(rec, &typeinf);
				expect(SEMICOLON, true);
				types.clear();
			}
			else
				break;
		}
	}
	
	/*
	rec-id-list :
	  identifier
	  identifier rec-subscript-member
	  identifier , rec-id-list
	  identifier rec-subscript-member , rec-id-list
	  pointer-operator-sequence rec-id-list
	  rec-func-pointer-member
	  pointer-operator-sequence rec-func-pointer-member
	*/
	void parser::rec_id_list(st_record_node **rec, st_type_info **typeinf) {
		token tok;
		int ptr_seq = 0;
		st_node *symt = (*rec)->symtab;
		std::list<token> sublst;
		
		//peek for identifier
		if (peek_token(IDENTIFIER)) {
			expect(IDENTIFIER, false);
			tok = compiler::lex->get_next();
			if (symtable::search_symbol((*rec)->symtab, tok.string)) {
				log::error_at(tok.loc, "redeclaration of " + tok.string);
				return;
			}
			else {
				symtable::insert_symbol(&symt, tok.string);
				assert(last_symbol != nullptr);
				last_symbol->type_info = *typeinf;
				last_symbol->symbol = tok.string;
				last_symbol->tok = tok;
			}
			if (peek_token(SQUARE_OPEN_BRACKET)) {
				sublst.clear();
				rec_subscript_member(sublst);
				assert(last_symbol != nullptr);
				last_symbol->is_array = true;
				last_symbol->arr_dimension_list.assign(sublst.begin(), sublst.end());
				sublst.clear();
			}
			else if (peek_token(COMMA_OP)) {
				consume_next();
				rec_id_list(&(*rec), &(*typeinf));
			}
			//peek for *
		}
		else if (peek_token(ARTHM_MUL)) {
			//get pointer operator sequence count
			ptr_seq = get_pointer_operator_sequence();
			//peek for (, otherwise pointer variable
			if (peek_token(PARENTH_OPEN))
				//record function pointer
				rec_func_pointer_member(&(*rec), &ptr_seq, &(*typeinf));
			else {
				expect(IDENTIFIER, false);
				tok = compiler::lex->get_next();
				if (symtable::search_symbol((*rec)->symtab, tok.string)) {
					log::error_at(tok.loc, "redeclaration of " + tok.string);
					return;
				}
				else {
					symtable::insert_symbol(&symt, tok.string);
					assert(last_symbol != nullptr);
					last_symbol->type_info = *typeinf;
					last_symbol->symbol = tok.string;
					last_symbol->tok = tok;
					last_symbol->is_ptr = true;
					last_symbol->ptr_oprtr_count = ptr_seq;
				}
				//peek for [, record subscript array member
				if (peek_token(SQUARE_OPEN_BRACKET)) {
					sublst.clear();
					rec_subscript_member(sublst);
					assert(last_symbol != nullptr);
					last_symbol->is_array = true;
					last_symbol->arr_dimension_list.assign(sublst.begin(), sublst.end());
					sublst.clear();
				}
				else if (peek_token(COMMA_OP)) {
					consume_next();
					rec_id_list(&(*rec), &(*typeinf));
				}
			}
		}
		else if (peek_token(PARENTH_OPEN))
			rec_func_pointer_member(&(*rec), &ptr_seq, &(*typeinf));
		else {
			tok = compiler::lex->get_next();
			log::error_at(tok.loc, "identifier expected in record member definition but found " + tok.string);
			
			return;
		}
	}
	
	/*
	rec-subscript-member :
	  [ constant-expression ]
	  [ constant-expression ] rec-subscript-member
	*/
	void parser::rec_subscript_member(std::list<token> &sublst) {
		token tok;
		expect(SQUARE_OPEN_BRACKET, true);
		if (peek_constant_expr()) {
			tok = compiler::lex->get_next();
			sublst.push_back(tok);
		}
		else {
			tok = compiler::lex->get_next();
			log::error_at(tok.loc, "constant expression expected but found " + tok.string);
		}
		expect(SQUARE_CLOSE_BRACKET, true);
		if (peek_token(SQUARE_OPEN_BRACKET))
			rec_subscript_member(sublst);
	}
	
	/*
	rec-func-pointer-member :
	  ( pointer-operator identifier ) ( rec-func-pointer-params )
	*/
	void parser::rec_func_pointer_member(st_record_node **rec, int *ptrseq, st_type_info **typeinf) {
		token tok;
		st_node *symt = (*rec)->symtab;
		std::list<token> sublst;
		
		expect(PARENTH_OPEN, true);
		expect(ARTHM_MUL, true);
		
		if (peek_token(IDENTIFIER)) {
			expect(IDENTIFIER, false);
			tok = compiler::lex->get_next();
			if (symtable::search_symbol((*rec)->symtab, tok.string)) {
				log::error_at(tok.loc, "redeclaration of func pointer " + tok.string);
				return;
			}
			else {
				symtable::insert_symbol(&symt, tok.string);
				assert(last_symbol != nullptr);
				last_symbol->type_info = *typeinf;
				last_symbol->is_func_ptr = true;
				last_symbol->symbol = tok.string;
				last_symbol->tok = tok;
				last_symbol->ret_ptr_count = *ptrseq;
				
				expect(PARENTH_CLOSE, true);
				expect(PARENTH_OPEN, true);
				
				if (peek_token(PARENTH_CLOSE))
					consume_next();
				else {
					rec_func_pointer_params(&(last_symbol));
					expect(PARENTH_CLOSE, true);
				}
			}
		}
		else {
			tok = compiler::lex->get_next();
			log::error_at(tok.loc, "identifier expected in record func pointer member definition");
			return;
		}
	}
	
	/*
	rec-func-pointer-params :
	  type-specifier
	  type-specifier pointer-operator-sequence
	  type-specifier , rec-func-pointer-params
	  type-specifier pointer-operator-sequence , rec-func-pointer-params
	*/
	void parser::rec_func_pointer_params(st_symbol_info **stinf) {
		token tok;
		std::vector<token> types;
		int ptr_seq = 0;
		st_rec_type_info *rectype = nullptr;
		
		if (*stinf == nullptr)
			return;
		
		rectype = symtable::get_rec_type_info_mem();
		
		if (peek_token(KEY_CONST)) {
			consume_next();
			rectype->is_const = true;
		}
		
		if (peek_type_specifier()) {
			get_type_specifier(types);
			consume_n(types.size());
			rectype->type = SIMPLE_TYPE;
			rectype->type_specifier.simple_type.assign(types.begin(), types.end());
			types.clear();
			(*stinf)->func_ptr_params_list.push_back(rectype);
			if (peek_token(ARTHM_MUL)) {
				ptr_seq = get_pointer_operator_sequence();
				rectype->is_ptr = true;
				rectype->ptr_oprtr_count = ptr_seq;
			}
			if (peek_token(COMMA_OP)) {
				consume_next();
				rec_func_pointer_params(&(*stinf));
			}
		}
		else if (peek_token(IDENTIFIER)) {
			tok = compiler::lex->get_next();
			rectype->type = RECORD_TYPE;
			rectype->type_specifier.record_type = tok;
			(*stinf)->func_ptr_params_list.push_back(rectype);
			if (peek_token(ARTHM_MUL)) {
				ptr_seq = get_pointer_operator_sequence();
				rectype->is_ptr = true;
				rectype->ptr_oprtr_count = ptr_seq;
			}
			if (peek_token(COMMA_OP)) {
				consume_next();
				rec_func_pointer_params(&(*stinf));
			}
		}
		else {
			symtable::delete_rec_type_info(&rectype);
			tok = compiler::lex->get_next();
			log::error_at(tok.loc, "type specifier expected in record func ptr member definition but found " + tok.string);
			
			return;
		}
	}
	
	/*
	simple-declaration :
	  type-specifier simple-declarator-list
	  const type-specifier simple-declarator-list
	  extern type-specifier simple-declarator-list
	  static type-specifier simple-declarator-list
	  global type-specifier simple-declarator-list
	*/
	/*
	starting part of the declaration is handled by other functions
	because the declaration could be locally or globally
	also some part of declaration is a function declaration/defintion
	*/
	void parser::simple_declaration(token scope, std::vector<token> &types, bool is_record_type, st_node **st) {
		token tok;
		st_type_info *stype = symtable::get_type_info_mem();
		
		//check for scope
		switch (scope.number) {
			case KEY_CONST : stype->is_const = true;
				break;
			case KEY_EXTERN : stype->is_extern = true;
				break;
			case KEY_STATIC : stype->is_static = true;
				break;
			case KEY_GLOBAL : stype->is_global = true;
				break;
			
			default: break;
		}
		
		//if simple type specifier
		if (!is_record_type) {
			stype->type = SIMPLE_TYPE;
			stype->type_specifier.simple_type.assign(types.begin(), types.end());
			simple_declarator_list(&(*st), &stype);
			//types.clear();
			//if ( do nothing and return, it is a function definition
			//that we encountered
			if (peek_token(PARENTH_OPEN))
				return;
			else
				expect(SEMICOLON, true);
		}
		else {
			if (types.size() > 0) {
				stype->type = RECORD_TYPE;
				stype->type_specifier.record_type = types[0];
				simple_declarator_list(&(*st), &stype);
				//types.clear();
				//if ( do nothing and return, it is a function definition
				//that has encountered
				if (peek_token(PARENTH_OPEN))
					return;
				else
					expect(SEMICOLON, true);
			}
		}
		
	}
	
	/*
	simple-declarator-list :
	  identifier
	  identifier subscript-declarator
	  identifier , simple-declarator-list
	  identifier subscript-declarator , simple-declarator-list
	  pointer-operator-sequence simple-declarator-list
	*/
	void parser::simple_declarator_list(st_node **st, st_type_info **stinf) {
		token tok;
		int ptr_seq = 0;
		
		if (*st == nullptr)
			return;
		if (*stinf == nullptr)
			return;
		
		if (peek_token(IDENTIFIER)) {
			compiler::lex->reverse_tokens_queue();
			tok = compiler::lex->get_next();
			if (symtable::search_symbol((*st), tok.string)) {
				log::error_at(tok.loc, "redeclaration/conflicting types of " + tok.string);
				return;
			}
			else {
				symtable::insert_symbol(&(*st), tok.string);
				if (last_symbol == nullptr)
					return;
				last_symbol->symbol = tok.string;
				last_symbol->tok = tok;
				last_symbol->type_info = *stinf;
			}
			if (peek_token(SQUARE_OPEN_BRACKET)) {
				last_symbol->is_array = true;
				subscript_declarator(&last_symbol);
			}
			if (peek_token(COMMA_OP)) {
				consume_next();
				simple_declarator_list(&(*st), &(*stinf));
			}
			if (peek_token(ASSGN)) {
			}
		}
		else if (peek_token(ARTHM_MUL)) {
			ptr_seq = get_pointer_operator_sequence();
			ptr_oprtr_count = ptr_seq;
			if (peek_token(IDENTIFIER)) {
				tok = compiler::lex->get_next();
				if (symtable::search_symbol((*st), tok.string)) {
					log::error_at(tok.loc, "redeclaration/conflicting types of " + tok.string);
					return;
				}
				else {
					symtable::insert_symbol(&(*st), tok.string);
					if (last_symbol == nullptr)
						return;
					last_symbol->symbol = tok.string;
					last_symbol->tok = tok;
					last_symbol->type_info = *stinf;
					last_symbol->is_ptr = true;
					last_symbol->ptr_oprtr_count = ptr_seq;
				}
				if (peek_token(SQUARE_OPEN_BRACKET)) {
					last_symbol->is_array = true;
					subscript_declarator(&last_symbol);
				}
				else if (peek_token(ASSGN)) {
					consume_next();
					subscript_initializer(last_symbol->arr_init_list);
				}
				else if (peek_token(SEMICOLON)) {
					return;
				}
				if (peek_token(COMMA_OP)) {
					consume_next();
					simple_declarator_list(&(*st), &(*stinf));
				}
				else if (peek_token(PARENTH_OPEN)) {
					funcname = tok;
					return;
				}
			}
			else {
				tok = compiler::lex->get_next();
				log::error_at(tok.loc, "identifier expected in declaration");
				return;
			}
		}
		else {
			tok = compiler::lex->get_next();
			log::error_at(tok.loc, "identifier expected in declaration but found " + tok.string);
			tok = compiler::lex->get_next();
			return;
		}
	}
	
	/*
	subscript-declarator :
	  [ constant-expression ]
	  [ constant-expression ] subscript-declarator
	*/
	void parser::subscript_declarator(st_symbol_info **stsinf) {
		token tok;
		expect(SQUARE_OPEN_BRACKET, true);
		if (peek_constant_expr()) {
			tok = compiler::lex->get_next();
			(*stsinf)->arr_dimension_list.push_back(tok);
		}
		else if (peek_token(SQUARE_CLOSE_BRACKET)) {
		}
		else {
			tok = compiler::lex->get_next();
			log::error_at(tok.loc, "constant expression expected but found " + tok.string);
		}
		expect(SQUARE_CLOSE_BRACKET, true);
		if (peek_token(SQUARE_OPEN_BRACKET))
			subscript_declarator(&(*stsinf));
		else if (peek_token(ASSGN)) {
			consume_next();
			subscript_initializer((*stsinf)->arr_init_list);
		}
		else
			return;
	}
	
	/*
	subscript-initializer :
	  { literal-list }
	  { literal-list } , subscript-initializer
	  { subscript-initializer }
	*/
	void parser::subscript_initializer(std::vector<std::vector<token>> &arrinit) {
		token tok;
		std::vector<token> ltrl;
		
		if (peek_token(LIT_STRING)) {
			tok = compiler::lex->get_next();
			ltrl.push_back(tok);
			arrinit.push_back(ltrl);
			ltrl.clear();
		}
		else {
			expect(CURLY_OPEN_BRACKET, true);
			if (peek_literal_string()) {
				literal_list(ltrl);
				arrinit.push_back(ltrl);
				ltrl.clear();
			}
			else if (peek_token(CURLY_OPEN_BRACKET))
				subscript_initializer(arrinit);
			else {
				tok = compiler::lex->get_next();
				log::error_at(tok.loc, "literal expected in array initializer but found " + tok.string);
			}
			expect(CURLY_CLOSE_BRACKET, true);
			if (peek_token(COMMA_OP)) {
				consume_next();
				subscript_initializer(arrinit);
			}
		}
	}
	
	/*
	literal-list :
	  literal
	  literal , literal-list
	*/
	void parser::literal_list(std::vector<token> &ltrl) {
		token tok;
		if (peek_literal_string()) {
			tok = compiler::lex->get_next();
			ltrl.push_back(tok);
		}
		else {
			tok = compiler::lex->get_next();
			log::error_at(tok.loc, "literal expected in array initializer but found " + tok.string);
			return;
		}
		if (peek_token(COMMA_OP)) {
			consume_next();
			literal_list(ltrl);
		}
	}
	
	/*
	func-head :
	  type-specifier function-name ( func-params )
	  extern type-specifier function-name ( func-params )
	  global type-specifier function-name ( func-params )

	function-name :
	  identifier
	*/
	void parser::func_head(st_func_info **stfinf, token _funcname, token scope, std::vector<token> &types, bool is_record_type) {
		token tok;
		
		*stfinf = symtable::get_func_info_mem();
		
		if (*stfinf == nullptr)
			return;
		
		switch (scope.number) {
			case KEY_EXTERN : (*stfinf)->is_extern = true;
				break;
			case KEY_GLOBAL : (*stfinf)->is_global = true;
				break;
			
			default: break;
		}
		
		if (!is_record_type) {
			(*stfinf)->return_type = symtable::get_type_info_mem();
			(*stfinf)->return_type->type = SIMPLE_TYPE;
			(*stfinf)->return_type->type_specifier.simple_type.assign(types.begin(), types.end());
			
			tok = compiler::lex->get_next();
			(*stfinf)->func_name = _funcname.string;
			(*stfinf)->tok = _funcname;
			
			//functions can only be defined globally
			// so while figuring out whether the declaration is the function or not
			// we might lost some tokens while returning it to the lexer
			//even with high priority
			//so we dont need to expect ( token here
			//expect(PARENTH_OPEN, true); will cause error
			
			if (peek_token(PARENTH_CLOSE)) {
				consume_next();
			}
			else {
				func_params((*stfinf)->param_list);
				expect(PARENTH_CLOSE, true);
			}
		}
		else {
			(*stfinf)->return_type = symtable::get_type_info_mem();
			(*stfinf)->return_type->type = RECORD_TYPE;
			(*stfinf)->return_type->type_specifier.record_type = types[0];
			
			tok = compiler::lex->get_next();
			(*stfinf)->func_name = _funcname.string;
			(*stfinf)->tok = _funcname;
			
			//expect(PARENTH_OPEN, true); will cause error
			if (peek_token(PARENTH_CLOSE))
				consume_next();
			else {
				func_params((*stfinf)->param_list);
				expect(PARENTH_CLOSE, true);
			}
		}
	}
	
	/*
	func-params :
	  type-specifier
	  type-specifier identifier
	  type-specifier pointer-operator-sequence
	  type-specifier pointer-operator-sequence identifier
	  type-specifier , func-params
	  type-specifier identifier , func-params
	  type-specifier pointer-operator-sequence , func-params
	  type-specifier pointer-operator-sequence identifier , func-params
	*/
	void parser::func_params(std::list<st_func_param_info *> &fparams) {
		token tok;
		std::vector<token> types;
		int ptr_seq = 0;
		st_func_param_info *funcparam = nullptr;
		
		funcparam = symtable::get_func_param_info_mem();
		
		if (peek_type_specifier()) {
			get_type_specifier(types);
			consume_n(types.size());
			funcparam->type_info->type = SIMPLE_TYPE;
			funcparam->type_info->type_specifier.simple_type.assign(types.begin(), types.end());
			funcparam->symbol_info->type_info = funcparam->type_info;
			funcparam->symbol_info->ptr_oprtr_count = 0;
			types.clear();
			fparams.push_back(funcparam);
			if (peek_token(ARTHM_MUL)) {
				ptr_seq = get_pointer_operator_sequence();
				funcparam->symbol_info->is_ptr = true;
				funcparam->symbol_info->ptr_oprtr_count = ptr_seq;
			}
			if (peek_token(IDENTIFIER)) {
				tok = compiler::lex->get_next();
				funcparam->symbol_info->symbol = tok.string;
				funcparam->symbol_info->tok = tok;
			}
			if (peek_token(COMMA_OP)) {
				consume_next();
				func_params(fparams);
			}
		}
		else if (peek_token(IDENTIFIER)) {
			tok = compiler::lex->get_next();
			funcparam->type_info->type = RECORD_TYPE;
			funcparam->type_info->type_specifier.record_type = tok;
			funcparam->symbol_info->type_info = funcparam->type_info;
			funcparam->symbol_info->ptr_oprtr_count = 0;
			fparams.push_back(funcparam);
			if (peek_token(ARTHM_MUL)) {
				ptr_seq = get_pointer_operator_sequence();
				funcparam->symbol_info->is_ptr = true;
				funcparam->symbol_info->ptr_oprtr_count = ptr_seq;
			}
			if (peek_token(IDENTIFIER)) {
				tok = compiler::lex->get_next();
				funcparam->symbol_info->symbol = tok.string;
				funcparam->symbol_info->tok = tok;
			}
			if (peek_token(COMMA_OP)) {
				consume_next();
				func_params(fparams);
			}
		}
		else {
			symtable::delete_func_param_info(&funcparam);
			tok = compiler::lex->get_next();
			log::error_at(tok.loc, "type specifier expected in function declaration parameters but found " + tok.string);
			return;
		}
	}
	
	/*
	labled-statement :
	  identifier :
	*/
	labled_stmt *parser::labled_statement() {
		labled_stmt *labstmt = tree::get_label_stmt_mem();
		token tok;
		expect(IDENTIFIER, false);
		tok = compiler::lex->get_next();
		labstmt->label = tok;
		expect(COLON_OP, true);
		return labstmt;
	}
	
	/*
	expression-statement :
	  expression
	*/
	expr_stmt *parser::expression_statement() {
		expr_stmt *expstmt = tree::get_expr_stmt_mem();
		terminator_t terminator = {SEMICOLON};
		expstmt->expression = expression(terminator);
		return expstmt;
	}
	
	/*
	selection-statement :
	  if ( condition ) { statement-list }
	  if ( condition ) { statement-list } else { statement-list }
	*/
	select_stmt *parser::selection_statement(st_node **symtab) {
		token tok;
		terminator_t terminator = {PARENTH_CLOSE};
		select_stmt *selstmt = tree::get_select_stmt_mem();
		
		expect(KEY_IF, false);
		tok = compiler::lex->get_next();
		selstmt->iftok = tok;
		expect(PARENTH_OPEN, true);
		selstmt->condition = expression(terminator);
		expect(CURLY_OPEN_BRACKET, true);
		if (peek_token(CURLY_CLOSE_BRACKET))
			consume_next();
		else {
			selstmt->if_statement = statement(&(*symtab));
			expect(CURLY_CLOSE_BRACKET, true);
		}
		if (peek_token(KEY_ELSE)) {
			tok = compiler::lex->get_next();
			selstmt->elsetok = tok;
			expect(CURLY_OPEN_BRACKET, true);
			if (peek_token(CURLY_CLOSE_BRACKET))
				consume_next();
			else {
				selstmt->else_statement = statement(&(*symtab));
				expect(CURLY_CLOSE_BRACKET, true);
			}
		}
		return selstmt;
	}
	
	/*
	iteration-statement :
	  while ( condition ) { statement-list }
	  do { statement-list } while ( condition ) ;
	  for ( init-expression ; condition ; update-expression ) { statement-list }
	*/
	/*
	where statement-list is the doubly linked list of statements
	*/
	iter_stmt *parser::iteration_statement(st_node **symtab) {
		token tok;
		terminator_t terminator = {PARENTH_CLOSE};
		iter_stmt *itstmt = tree::get_iter_stmt_mem();
		
		//peek for while
		if (peek_token(KEY_WHILE)) {
			//while ( condition ) { statement-list }
			expect(KEY_WHILE, false);
			itstmt->type = WHILE_STMT;
			tok = compiler::lex->get_next();
			itstmt->_while.whiletok = tok;
			expect(PARENTH_OPEN, true);
			itstmt->_while.condition = expression(terminator);
			if (is_expr_terminator_consumed && consumed_terminator.number == PARENTH_CLOSE) {
			}
			else
				expect(PARENTH_CLOSE, true);
			
			if (peek_token(SEMICOLON))
				consume_next();
			else {
				expect(CURLY_OPEN_BRACKET, true);
				if (peek_token(CURLY_CLOSE_BRACKET))
					consume_next();
				else {
					itstmt->_while.statement = statement(&(*symtab));
					expect(CURLY_CLOSE_BRACKET, true);
				}
			}
			//peek for do
		}
		else if (peek_token(KEY_DO)) {
			//do { statement-list } while ( condition ) ;
			expect(KEY_DO, false);
			itstmt->type = DOWHILE_STMT;
			tok = compiler::lex->get_next();
			itstmt->_dowhile.dotok = tok;
			expect(CURLY_OPEN_BRACKET, true);
			if (peek_token(CURLY_CLOSE_BRACKET))
				consume_next();
			else {
				itstmt->_dowhile.statement = statement(&(*symtab));
				expect(CURLY_CLOSE_BRACKET, true);
			}
			expect(KEY_WHILE, false);
			tok = compiler::lex->get_next();
			itstmt->_dowhile.whiletok = tok;
			expect(PARENTH_OPEN, true);
			itstmt->_dowhile.condition = expression(terminator);
			if (is_expr_terminator_consumed && consumed_terminator.number == PARENTH_CLOSE)
				expect(SEMICOLON, true);
			else {
				expect(PARENTH_CLOSE, true);
				expect(SEMICOLON, true);
			}
			//peek for for
		}
		else if (peek_token(KEY_FOR)) {
			//for ( init-expression ; condition ; update-expression ) { statement-list }
			itstmt->type = FOR_STMT;
			expect(KEY_FOR, false);
			tok = compiler::lex->get_next();
			itstmt->_for.fortok = tok;
			expect(PARENTH_OPEN, true);
			terminator.clear();
			terminator.push_back(SEMICOLON);
			
			if (peek_token(SEMICOLON))
				consume_next();
			else if (peek_expr_token())
				itstmt->_for.init_expr = expression(terminator);
			else {
				tok = compiler::lex->get_next();
				log::error_at(tok.loc, "expression or ; expected in for()");
			}
			
			itstmt->_for.condition = expression(terminator);
			terminator.clear();
			terminator.push_back(PARENTH_CLOSE);
			
			if (peek_token(PARENTH_CLOSE)) {
				tok = compiler::lex->get_next();
				is_expr_terminator_consumed = true;
				consumed_terminator = tok;
			}
			else
				itstmt->_for.update_expr = expression(terminator);
			
			if (is_expr_terminator_consumed && consumed_terminator.number == PARENTH_CLOSE) {
				if (peek_token(SEMICOLON))
					consume_next();
				else {
					expect(CURLY_OPEN_BRACKET, true);
					if (peek_token(CURLY_CLOSE_BRACKET))
						consume_next();
					else {
						itstmt->_for.statement = statement(&(*symtab));
						expect(CURLY_CLOSE_BRACKET, true);
					}
				}
			}
			else {
				expect(PARENTH_CLOSE, true);
				if (peek_token(SEMICOLON))
					consume_next();
				else {
					expect(CURLY_OPEN_BRACKET, true);
					if (peek_token(CURLY_CLOSE_BRACKET))
						consume_next();
					else {
						itstmt->_for.statement = statement(&(*symtab));
						expect(CURLY_CLOSE_BRACKET, true);
					}
				}
			}
			
		}
		return itstmt;
	}
	
	/*
	jump-statement :
	  break ;
	  continue ;
	  return expression
	  goto identifier
	*/
	jump_stmt *parser::jump_statement() {
		token tok;
		terminator_t terminator = {SEMICOLON};
		jump_stmt *jmpstmt = tree::get_jump_stmt_mem();
		
		switch (get_peek_token()) {
			case KEY_BREAK : jmpstmt->type = BREAK_JMP;
				tok = compiler::lex->get_next();
				jmpstmt->tok = tok;
				expect(SEMICOLON, true, ";", " in break statement");
				break;
			case KEY_CONTINUE : jmpstmt->type = CONTINUE_JMP;
				tok = compiler::lex->get_next();
				jmpstmt->tok = tok;
				expect(SEMICOLON, true, ";", " in continue statement");
				break;
			case KEY_RETURN : jmpstmt->type = RETURN_JMP;
				tok = compiler::lex->get_next();
				jmpstmt->tok = tok;
				if (peek_token(SEMICOLON))
					consume_next();
				else
					jmpstmt->expression = expression(terminator);
				break;
			case KEY_GOTO : jmpstmt->type = GOTO_JMP;
				tok = compiler::lex->get_next();
				jmpstmt->tok = tok;
				expect(IDENTIFIER, false, "", "label in goto statement");
				tok = compiler::lex->get_next();
				jmpstmt->goto_id = tok;
				expect(SEMICOLON, true, ";", " in goto statement");
				break;
			
			default: break;
		}
		return jmpstmt;
	}
	
	/*
	asm-statement :
	  asm { asm-statement-sequence }
	*/
	asm_stmt *parser::asm_statement() {
		asm_stmt *asmhead = nullptr;
		token tok;
		expect(KEY_ASM, true);
		expect(CURLY_OPEN_BRACKET, true);
		asm_statement_sequence(&asmhead);
		if (peek_token(CURLY_CLOSE_BRACKET))
			consume_next();
		else {
			tok = compiler::lex->get_next();
			log::error_at(tok.loc, ", or } expected before \"" + tok.string + "\" in asm statement ");
		}
		return asmhead;
	}
	
	/*
	asm-statement-sequence :
	  string-literal [ asm-operand : asm-operand ]
	  string-literal [ asm-operand : asm-operand ] , asm-statement-sequence
	*/
	void parser::asm_statement_sequence(asm_stmt **asmhead) {
		token tok;
		asm_stmt *asmstmt = tree::get_asm_stmt_mem();
		
		expect(LIT_STRING, false);
		tok = compiler::lex->get_next();
		asmstmt->asm_template = tok;
		
		if (peek_token(SQUARE_OPEN_BRACKET)) {
			consume_next();
			//check for output operand
			//if no output operand
			if (peek_token(COLON_OP)) {
				consume_next();
			}
			else if (peek_token(LIT_STRING)) {
				asm_operand(asmstmt->output_operand);
				expect(COLON_OP, true);
			}
			else {
				tok = compiler::lex->get_next();
				log::error_at(tok.loc, "output operand expected " + tok.string);
				return;
			}
			
			//check for input operand
			if (peek_token(SQUARE_CLOSE_BRACKET))
				consume_next();
			else if (peek_token(LIT_STRING)) {
				asm_operand(asmstmt->input_operand);
				expect(SQUARE_CLOSE_BRACKET, true);
			}
			else {
				tok = compiler::lex->get_next();
				log::error_at(tok.loc, "input operand expected " + tok.string);
				return;
			}
			
			tree::add_asm_statement(&(*asmhead), &asmstmt);
			if (peek_token(COMMA_OP)) {
				consume_next();
				asm_statement_sequence(&(*asmhead));
			}
		}
		else {
			tree::add_asm_statement(&(*asmhead), &asmstmt);
			if (peek_token(COMMA_OP)) {
				consume_next();
				asm_statement_sequence(&(*asmhead));
			}
		}
	}
	
	/*
	asm-operand :
	  string-literal ( expression )
	  string-literal ( expression ) , asm-operand
	*/
	void parser::asm_operand(std::vector<st_asm_operand *> &operand) {
		token tok;
		terminator_t terminator = {PARENTH_CLOSE};
		st_asm_operand *asmoprd = tree::get_asm_operand_mem();
		expect(LIT_STRING, false);
		tok = compiler::lex->get_next();
		asmoprd->constraint = tok;
		expect(PARENTH_OPEN, true);
		if (peek_expr_token()) {
			asmoprd->expression = expression(terminator);
			if (is_expr_terminator_consumed && consumed_terminator.number == PARENTH_CLOSE)
				operand.push_back(asmoprd);
			else {
				expect(PARENTH_CLOSE, true);
				operand.push_back(asmoprd);
			}
			if (peek_token(COMMA_OP)) {
				consume_next();
				asm_operand(operand);
			}
		}
		else if (peek_token(PARENTH_CLOSE)) {
			consume_next();
			operand.push_back(asmoprd);
			return;
		}
		else {
			tok = compiler::lex->get_next();
			log::error_at(tok.loc, " expression expected " + tok.string);
			return;
		}
	}
	
	/*
	statement :
	  labled-statement
	  expression-statement
	  selection-statement
	  iteration-statement
	  jump-statement
	  simple-declaration

	statement-list :
	  statement
	  statement statement-list
	*/
	stmt *parser::statement(st_node **symtab) {
		token tok;
		std::vector<token> types;
		token scope = nulltoken;
		stmt *stmthead = nullptr;
		stmt *statement = nullptr;
		
		while ((tok = compiler::lex->get_next()).number != END_OF_FILE) {
			//if type specifier, the call declaration
			if (type_specifier(tok.number)) {
				compiler::lex->put_back(tok);
				get_type_specifier(types);
				consume_n(types.size());
				simple_declaration(scope, types, false, &(*symtab));
				types.clear();
				if (peek_token(END_OF_FILE))
					return stmthead;
				continue;
				//if identifier
			}
			else if (tok.number == IDENTIFIER) {
				//if peek for identifier, call declaration
				if (peek_token(IDENTIFIER)) {
					types.push_back(tok);
					simple_declaration(scope, types, true, &(*symtab));
					types.clear();
					if (peek_token(END_OF_FILE))
						return stmthead;
					//if peek for :, call labled statement
				}
				else if (peek_token(COLON_OP)) {
					compiler::lex->put_back(tok);
					statement = tree::get_stmt_mem();
					statement->type = LABEL_STMT;
					statement->labled_statement = labled_statement();
					tree::add_statement(&stmthead, &statement);
					if (peek_token(END_OF_FILE))
						return stmthead;
					//otherwise expression statement
				}
				else {
					compiler::lex->put_back(tok);
					statement = tree::get_stmt_mem();
					statement->type = EXPR_STMT;
					statement->expression_statement = expression_statement();
					tree::add_statement(&stmthead, &statement);
					if (peek_token(END_OF_FILE))
						return stmthead;
				}
				//if expression token, get expression statement
			}
			else if (expression_token(tok.number)) {
				compiler::lex->put_back(tok);
				statement = tree::get_stmt_mem();
				statement->type = EXPR_STMT;
				statement->expression_statement = expression_statement();
				tree::add_statement(&stmthead, &statement);
				if (peek_token(END_OF_FILE))
					return stmthead;
				//if if token, get selection statement
			}
			else if (tok.number == KEY_IF) {
				compiler::lex->put_back(tok);
				statement = tree::get_stmt_mem();
				statement->type = SELECT_STMT;
				statement->selection_statement = selection_statement(&(*symtab));
				tree::add_statement(&stmthead, &statement);
				if (peek_token(END_OF_FILE))
					return stmthead;
				//if while,do or for, get iteration statement
			}
			else if (tok.number == KEY_WHILE || tok.number == KEY_DO || tok.number == KEY_FOR) {
				compiler::lex->put_back(tok);
				statement = tree::get_stmt_mem();
				statement->type = ITER_STMT;
				statement->iteration_statement = iteration_statement(&(*symtab));
				tree::add_statement(&stmthead, &statement);
				if (peek_token(END_OF_FILE))
					return stmthead;
				//if break, continue, return or goto, get jump statement
			}
			else if (tok.number == KEY_BREAK || tok.number == KEY_CONTINUE || tok.number == KEY_RETURN || tok.number == KEY_GOTO) {
				compiler::lex->put_back(tok);
				statement = tree::get_stmt_mem();
				statement->type = JUMP_STMT;
				statement->jump_statement = jump_statement();
				tree::add_statement(&stmthead, &statement);
				if (peek_token(END_OF_FILE))
					return stmthead;
				//if { or ) then return because these tokens are belongs to other productions
			}
			else if (tok.number == KEY_ASM) {
				compiler::lex->put_back(tok);
				statement = tree::get_stmt_mem();
				statement->type = ASM_STMT;
				statement->asm_statement = asm_statement();
				tree::add_statement(&stmthead, &statement);
				if (peek_token(END_OF_FILE))
					return stmthead;
			}
			else if (tok.number == CURLY_CLOSE_BRACKET || tok.number == PARENTH_CLOSE) {
				compiler::lex->put_back(tok);
				return stmthead;
			}
			else if (tok.number == SEMICOLON)
				continue;
			else {
				log::error_at(tok.loc, "invalid token in statement " + tok.string);
				return nullptr;
			}
		}
		return stmthead;
	}
	
	void parser::get_func_info(st_func_info **func_info, token tok, int type, std::vector<token> &types, bool is_extern, bool is_glob) {
		
		if (*func_info == nullptr)
			*func_info = symtable::get_func_info_mem();
		
		(*func_info)->func_name = tok.string;
		(*func_info)->tok = tok;
		(*func_info)->return_type = symtable::get_type_info_mem();
		(*func_info)->return_type->type = type;
		
		if (type == SIMPLE_TYPE)
			(*func_info)->return_type->type_specifier.simple_type.assign(types.begin(), types.end());
		else if (type == RECORD_TYPE)
			(*func_info)->return_type->type_specifier.record_type = types[0];
		
		(*func_info)->is_extern = is_extern;
		(*func_info)->is_global = is_glob;
	}
	
	// parse returns AST
	tree_node *parser::parse() {
		
		token tok[5];
		std::vector<token> types;
		std::map<std::string, st_func_info *>::iterator funcit;
		terminator_t terminator = {SEMICOLON};
		stmt *_stmt = nullptr;
		st_node *symtab = nullptr;
		st_func_info *funcinfo = nullptr;
		tree_node *tree_head = nullptr;
		tree_node *_tree = nullptr;
		
		//continue taking tokens from lexer until END_OF_FILE
		while ((tok[0] = compiler::lex->get_next()).number != END_OF_FILE) {
			
			//if global
			if (tok[0].number == KEY_GLOBAL) {
				
				tok[1] = compiler::lex->get_next();
				
				if (tok[1].number == END_OF_FILE)
					return tree_head;
				
				//if global record, call record specifier
				if (tok[1].number == KEY_RECORD) {
					compiler::lex->put_back(tok[0]);
					compiler::lex->put_back(tok[1]);
					record_specifier();
					//if global type-specifier
				}
				else if (type_specifier(tok[1].number)) {
					//get type specifier
					compiler::lex->put_back(tok[1]);
					types.clear();
					get_type_specifier(types);
					consume_n(types.size());
					
					tok[2] = compiler::lex->get_next();
					
					if (tok[2].number == END_OF_FILE)
						return tree_head;
					
					//if global type-specifier identifier
					if (tok[2].number == IDENTIFIER) {
						tok[3] = compiler::lex->get_next();
						
						if (tok[3].number == END_OF_FILE)
							return tree_head;
						
						// check for function definition (
						if (tok[3].number == PARENTH_OPEN) {
							compiler::lex->put_back(tok[3]);
							
							symtab = symtable::get_node_mem();
							funcinfo = symtable::get_func_info_mem();
							func_head(&funcinfo, tok[2], tok[0], types, false);
							funcit = compiler::func_table->find(tok[2].string);
							
							if (funcit == compiler::func_table->end()) {
								compiler::func_table->insert(std::pair<std::string, st_func_info *>(tok[2].string, funcinfo));
								expect(CURLY_OPEN_BRACKET, true);
								_tree = tree::get_tree_node_mem();
								_tree->symtab = symtab;
								get_func_info(&funcinfo, tok[2], SIMPLE_TYPE, types, false, true);
								_tree->symtab->func_info = funcinfo;
								_stmt = statement(&symtab);
								_tree->statement = _stmt;
								_tree->symtab = symtab;
								tree::add_tree_node(&tree_head, &_tree);
								expect(CURLY_CLOSE_BRACKET, true);
							}
							else {
								log::error_at(tok[2].loc, "redeclaration of function " + tok[2].string);
								symtable::delete_func_info(&funcinfo);
								return tree_head;
							}
							types.clear();
						}
						else {
							//otherwise declaration
							compiler::lex->put_back(tok[2]);
							compiler::lex->put_back(tok[3]);
							simple_declaration(tok[0], types, false, &compiler::symtab);
							types.clear();
							ptr_oprtr_count = 0;
						}
						//if * operator
					}
					else if (tok[2].number == ARTHM_MUL) {
						compiler::lex->put_back(tok[2]);
						//get declaration
						simple_declaration(tok[0], types, false, &compiler::symtab);
						//if peek ( , get function definition
						if (peek_token(PARENTH_OPEN)) {
							symtable::remove_symbol(&compiler::symtab, funcname.string);
							
							symtab = symtable::get_node_mem();
							funcinfo = symtable::get_func_info_mem();
							func_head(&funcinfo, funcname, tok[0], types, false);
							funcinfo->ptr_oprtr_count = ptr_oprtr_count;
							symtab->func_info = funcinfo;
							
							funcit = compiler::func_table->find(funcname.string);
							if (funcit == compiler::func_table->end()) {
								compiler::func_table->insert(std::pair<std::string, st_func_info *>(funcname.string, funcinfo));
								expect(CURLY_OPEN_BRACKET, true);
								_tree = tree::get_tree_node_mem();
								_tree->symtab = symtab;
								get_func_info(&funcinfo, funcname, SIMPLE_TYPE, types, false, true);
								_tree->symtab->func_info = funcinfo;
								_stmt = statement(&symtab);
								_tree->statement = _stmt;
								_tree->symtab = symtab;
								tree::add_tree_node(&tree_head, &_tree);
								expect(CURLY_CLOSE_BRACKET, true);
							}
							else {
								log::error_at(funcname.loc, "redeclaration of function " + funcname.string);
								symtable::delete_func_info(&funcinfo);
								return tree_head;
							}
						}
						ptr_oprtr_count = 0;
						funcname = nulltoken;
						types.clear();
					}
					
				}
					
					//if global identifier
				
				else if (tok[1].number == IDENTIFIER) {
					types.push_back(tok[1]);
					tok[2] = compiler::lex->get_next();
					
					if (tok[2].number == END_OF_FILE)
						return tree_head;
					
					//if global identifier identifier
					if (tok[2].number == IDENTIFIER) {
						tok[3] = compiler::lex->get_next();
						
						if (tok[3].number == END_OF_FILE)
							return tree_head;
						
						// check for function definition (
						if (tok[3].number == PARENTH_OPEN) {
							compiler::lex->put_back(tok[3]);
							
							symtab = symtable::get_node_mem();
							funcinfo = symtable::get_func_info_mem();
							func_head(&funcinfo, tok[2], tok[0], types, true);
							funcit = compiler::func_table->find(tok[2].string);
							if (funcit == compiler::func_table->end()) {
								compiler::func_table->insert(std::pair<std::string, st_func_info *>(tok[2].string, funcinfo));
								expect(CURLY_OPEN_BRACKET, true);
								_tree = tree::get_tree_node_mem();
								_tree->symtab = symtab;
								get_func_info(&funcinfo, tok[2], RECORD_TYPE, types, false, true);
								_tree->symtab->func_info = funcinfo;
								_stmt = statement(&symtab);
								_tree->statement = _stmt;
								_tree->symtab = symtab;
								tree::add_tree_node(&tree_head, &_tree);
								expect(CURLY_CLOSE_BRACKET, true);
							}
							else {
								log::error_at(tok[2].loc, "redeclaration of function " + tok[2].string);
								symtable::delete_func_info(&funcinfo);
								
								return tree_head;
							}
							types.clear();
							//otherwise delcaration
						}
						else {
							compiler::lex->put_back(tok[2]);
							compiler::lex->put_back(tok[3]);
							simple_declaration(tok[0], types, true, &compiler::symtab);
							types.clear();
							ptr_oprtr_count = 0;
						}
						
					}
						
						//if * operator
					
					else if (tok[2].number == ARTHM_MUL) {
						compiler::lex->put_back(tok[2]);
						simple_declaration(tok[0], types, false, &compiler::symtab);
						
						if (peek_token(PARENTH_OPEN)) {
							symtable::remove_symbol(&compiler::symtab, funcname.string);
							
							symtab = symtable::get_node_mem();
							funcinfo = symtable::get_func_info_mem();
							func_head(&funcinfo, funcname, tok[0], types, true);
							funcinfo->ptr_oprtr_count = ptr_oprtr_count;
							symtab->func_info = funcinfo;
							
							funcit = compiler::func_table->find(funcname.string);
							if (funcit == compiler::func_table->end()) {
								compiler::func_table->insert(std::pair<std::string, st_func_info *>(funcname.string, funcinfo));
								expect(CURLY_OPEN_BRACKET, true);
								_tree = tree::get_tree_node_mem();
								_tree->symtab = symtab;
								get_func_info(&funcinfo, funcname, RECORD_TYPE, types, false, true);
								_tree->symtab->func_info = funcinfo;
								_stmt = statement(&symtab);
								_tree->statement = _stmt;
								_tree->symtab = symtab;
								tree::add_tree_node(&tree_head, &_tree);
								expect(CURLY_CLOSE_BRACKET, true);
							}
							else {
								log::error_at(funcname.loc, "redeclaration of function " + funcname.string);
								symtable::delete_func_info(&funcinfo);
								
								return tree_head;
							}
						}
						ptr_oprtr_count = 0;
						funcname = nulltoken;
						types.clear();
					}
				}
				
				
			}
				
				//if extern
			
			else if (tok[0].number == KEY_EXTERN) {
				tok[1] = compiler::lex->get_next();
				
				if (tok[1].number == END_OF_FILE)
					return tree_head;
				
				//if extern record
				if (tok[1].number == KEY_RECORD) {
					compiler::lex->put_back(tok[1]);
					compiler::lex->put_back(tok[0]);
					record_specifier();
					//if extern type-specifier
				}
				else if (type_specifier(tok[1].number)) {
					//get type specifier
					compiler::lex->put_back(tok[1]);
					types.clear();
					get_type_specifier(types);
					consume_n(types.size());
					
					tok[2] = compiler::lex->get_next();
					
					if (tok[2].number == END_OF_FILE)
						return tree_head;
					
					//if extern type-specifier identifier
					if (tok[2].number == IDENTIFIER) {
						
						tok[3] = compiler::lex->get_next();
						
						if (tok[3].number == END_OF_FILE)
							return tree_head;
						
						// check for function declaration (
						if (tok[3].number == PARENTH_OPEN) {
							compiler::lex->put_back(tok[3]);
							funcinfo = symtable::get_func_info_mem();
							func_head(&funcinfo, tok[2], tok[0], types, false);
							funcit = compiler::func_table->find(tok[2].string);
							if (funcit == compiler::func_table->end()) {
								expect(SEMICOLON, true);
								compiler::func_table->insert(std::pair<std::string, st_func_info *>(tok[2].string, funcinfo));
								get_func_info(&funcinfo, tok[2], SIMPLE_TYPE, types, true, false);
								_tree = tree::get_tree_node_mem();
								symtab = symtable::get_node_mem();
								_tree->symtab = symtab;
								_tree->symtab->func_info = funcinfo;
								tree::add_tree_node(&tree_head, &_tree);
							}
							else {
								log::error_at(tok[2].loc, "redeclaration of function " + tok[2].string);
								symtable::delete_func_info(&funcinfo);
								return tree_head;
							}
							types.clear();
						}
						else {
							compiler::lex->put_back(tok[2]);
							compiler::lex->put_back(tok[3]);
							simple_declaration(tok[0], types, false, &compiler::symtab);
							types.clear();
							ptr_oprtr_count = 0;
						}
						//if * operator
					}
					else if (tok[2].number == ARTHM_MUL) {
						compiler::lex->put_back(tok[2]);
						simple_declaration(tok[0], types, false, &compiler::symtab);
						
						if (peek_token(PARENTH_OPEN)) {
							symtable::remove_symbol(&compiler::symtab, funcname.string);
							
							funcinfo = symtable::get_func_info_mem();
							func_head(&funcinfo, funcname, tok[0], types, false);
							funcinfo->ptr_oprtr_count = ptr_oprtr_count;
							
							funcit = compiler::func_table->find(funcname.string);
							if (funcit == compiler::func_table->end()) {
								compiler::func_table->insert(std::pair<std::string, st_func_info *>(funcname.string, funcinfo));
								expect(SEMICOLON, true);
								get_func_info(&funcinfo, funcname, SIMPLE_TYPE, types, true, false);
								_tree = tree::get_tree_node_mem();
								symtab = symtable::get_node_mem();
								_tree->symtab = symtab;
								_tree->symtab->func_info = funcinfo;
								tree::add_tree_node(&tree_head, &_tree);
							}
							else {
								log::error_at(funcname.loc, "redeclaration of function " + funcname.string);
								symtable::delete_func_info(&funcinfo);
								
								return tree_head;
							}
						}
						ptr_oprtr_count = 0;
						funcname = nulltoken;
						types.clear();
					}
					//if extern identifier
				}
				else if (tok[1].number == IDENTIFIER) {
					types.push_back(tok[1]);
					
					tok[2] = compiler::lex->get_next();
					
					if (tok[2].number == END_OF_FILE)
						return tree_head;
					
					if (tok[2].number == IDENTIFIER) {
						tok[3] = compiler::lex->get_next();
						
						if (tok[3].number == END_OF_FILE)
							return tree_head;
						
						// check for function definition (
						if (tok[3].number == PARENTH_OPEN) {
							compiler::lex->put_back(tok[3]);
							funcinfo = symtable::get_func_info_mem();
							func_head(&funcinfo, tok[2], tok[0], types, true);
							funcit = compiler::func_table->find(tok[2].string);
							if (funcit == compiler::func_table->end()) {
								expect(SEMICOLON, true);
								compiler::func_table->insert(std::pair<std::string, st_func_info *>(tok[2].string, funcinfo));
								get_func_info(&funcinfo, tok[2], RECORD_TYPE, types, true, false);
								_tree = tree::get_tree_node_mem();
								symtab = symtable::get_node_mem();
								_tree->symtab = symtab;
								_tree->symtab->func_info = funcinfo;
								tree::add_tree_node(&tree_head, &_tree);
							}
							else {
								log::error_at(tok[2].loc, "redeclaration of function " + tok[2].string);
								symtable::delete_func_info(&funcinfo);
								
								return tree_head;
							}
							types.clear();
							ptr_oprtr_count = 0;
							funcname = nulltoken;
						}
						else {
							compiler::lex->put_back(tok[2]);
							compiler::lex->put_back(tok[3]);
							simple_declaration(tok[0], types, true, &compiler::symtab);
							types.clear();
							ptr_oprtr_count = 0;
							funcname = nulltoken;
						}
						//if * operator
					}
					else if (tok[2].number == ARTHM_MUL) {
						compiler::lex->put_back(tok[2]);
						simple_declaration(tok[0], types, true, &compiler::symtab);
						
						//peek for (, for function declaration
						if (peek_token(PARENTH_OPEN)) {
							symtable::remove_symbol(&compiler::symtab, funcname.string);
							
							funcinfo = symtable::get_func_info_mem();
							func_head(&funcinfo, funcname, tok[0], types, true);
							funcinfo->ptr_oprtr_count = ptr_oprtr_count;
							
							funcit = compiler::func_table->find(funcname.string);
							if (funcit == compiler::func_table->end()) {
								compiler::func_table->insert(std::pair<std::string, st_func_info *>(funcname.string, funcinfo));
								expect(SEMICOLON, true);
								get_func_info(&funcinfo, funcname, RECORD_TYPE, types, true, false);
								_tree = tree::get_tree_node_mem();
								symtab = symtable::get_node_mem();
								_tree->symtab = symtab;
								_tree->symtab->func_info = funcinfo;
								tree::add_tree_node(&tree_head, &_tree);
							}
							else {
								log::error_at(funcname.loc, "redeclaration of function " + funcname.string);
								symtable::delete_func_info(&funcinfo);
								
								return tree_head;
							}
						}
						ptr_oprtr_count = 0;
						funcname = nulltoken;
						types.clear();
					}
				}
				//if type-specifier
			}
			else if (type_specifier(tok[0].number)) {
				compiler::lex->put_back(tok[0]);
				types.clear();
				get_type_specifier(types);
				consume_n(types.size());
				
				tok[1] = compiler::lex->get_next();
				
				if (tok[1].number == END_OF_FILE)
					return tree_head;
				
				if (tok[1].number == IDENTIFIER) {
					tok[2] = compiler::lex->get_next();
					
					if (tok[2].number == END_OF_FILE)
						return tree_head;
					
					// check for function definition (
					if (tok[2].number == PARENTH_OPEN) {
						compiler::lex->put_back(tok[2]);
						
						symtab = symtable::get_node_mem();
						funcinfo = symtable::get_func_info_mem();
						func_head(&funcinfo, tok[1], tok[0], types, false);
						funcit = compiler::func_table->find(tok[1].string);
						if (funcit == compiler::func_table->end()) {
							compiler::func_table->insert(std::pair<std::string, st_func_info *>(tok[1].string, funcinfo));
							expect(CURLY_OPEN_BRACKET, true);
							_tree = tree::get_tree_node_mem();
							_tree->symtab = symtab;
							get_func_info(&funcinfo, tok[1], SIMPLE_TYPE, types, false, false);
							_tree->symtab->func_info = funcinfo;
							_stmt = statement(&symtab);
							_tree->statement = _stmt;
							_tree->symtab = symtab;
							tree::add_tree_node(&tree_head, &_tree);
							expect(CURLY_CLOSE_BRACKET, true);
						}
						else {
							log::error_at(tok[1].loc, "redeclaration of function " + tok[1].string);
							
							return tree_head;
						}
						types.clear();
						ptr_oprtr_count = 0;
						funcname = nulltoken;
						
					}
					else {
						compiler::lex->put_back(tok[1]);
						compiler::lex->put_back(tok[2]);
						simple_declaration(tok[0], types, false, &compiler::symtab);
						types.clear();
						ptr_oprtr_count = 0;
						funcname = nulltoken;
					}
					//if * operator
				}
				else if (tok[1].number == ARTHM_MUL) {
					compiler::lex->put_back(tok[1]);
					simple_declaration(tok[0], types, false, &compiler::symtab);
					
					if (peek_token(PARENTH_OPEN) && funcname.number != NONE) {
						symtable::remove_symbol(&compiler::symtab, funcname.string);
						
						symtab = symtable::get_node_mem();
						funcinfo = symtable::get_func_info_mem();
						func_head(&funcinfo, funcname, tok[0], types, false);
						funcinfo->ptr_oprtr_count = ptr_oprtr_count;
						symtab->func_info = funcinfo;
						
						funcit = compiler::func_table->find(funcname.string);
						if (funcit == compiler::func_table->end()) {
							compiler::func_table->insert(std::pair<std::string, st_func_info *>(funcname.string, funcinfo));
							expect(CURLY_OPEN_BRACKET, true);
							_tree = tree::get_tree_node_mem();
							_tree->symtab = symtab;
							get_func_info(&funcinfo, funcname, SIMPLE_TYPE, types, false, false);
							_tree->symtab->func_info = funcinfo;
							_stmt = statement(&symtab);
							_tree->statement = _stmt;
							_tree->symtab = symtab;
							tree::add_tree_node(&tree_head, &_tree);
							expect(CURLY_CLOSE_BRACKET, true);
						}
						else {
							log::error_at(funcname.loc, "redeclaration of function " + funcname.string);
							symtable::delete_func_info(&funcinfo);
							return tree_head;
						}
					}
					ptr_oprtr_count = 0;
					funcname = nulltoken;
					types.clear();
				}
				//if record type identifier
			}
			else if (tok[0].number == IDENTIFIER) {
				types.clear();
				types.push_back(tok[0]);
				
				tok[1] = compiler::lex->get_next();
				
				if (tok[1].number == END_OF_FILE)
					return tree_head;
				
				//if identifier identifier
				if (tok[1].number == IDENTIFIER) {
					
					tok[2] = compiler::lex->get_next();
					
					if (tok[2].number == END_OF_FILE)
						return tree_head;
					
					// check for function definition (
					if (tok[2].number == PARENTH_OPEN) {
						compiler::lex->put_back(tok[2]);
						
						symtab = symtable::get_node_mem();
						funcinfo = symtable::get_func_info_mem();
						func_head(&funcinfo, tok[1], tok[0], types, true);
						funcit = compiler::func_table->find(tok[2].string);
						if (funcit == compiler::func_table->end()) {
							compiler::func_table->insert(std::pair<std::string, st_func_info *>(tok[1].string, funcinfo));
							expect(CURLY_OPEN_BRACKET, true);
							_tree = tree::get_tree_node_mem();
							_tree->symtab = symtab;
							get_func_info(&funcinfo, tok[1], RECORD_TYPE, types, false, false);
							_tree->symtab->func_info = funcinfo;
							_stmt = statement(&symtab);
							_tree->statement = _stmt;
							_tree->symtab = symtab;
							tree::add_tree_node(&tree_head, &_tree);
							expect(CURLY_CLOSE_BRACKET, true);
						}
						else {
							log::error_at(tok[1].loc, "redeclaration of function " + tok[1].string);
							symtable::delete_func_info(&funcinfo);
							
							return tree_head;
						}
						types.clear();
						
					}
					else {
						compiler::lex->put_back(tok[1]);
						compiler::lex->put_back(tok[2]);
						simple_declaration(tok[0], types, true, &compiler::symtab);
						types.clear();
						ptr_oprtr_count = 0;
					}
					//if * operator
				}
				else if (tok[1].number == ARTHM_MUL) {
					if (!symtable::search_record(compiler::record_table, tok[0].string)) {
						compiler::lex->put_back(tok[1]);
						compiler::lex->put_back(tok[0]);
						_tree = tree::get_tree_node_mem();
						_tree->statement = tree::get_stmt_mem();
						_tree->statement->type = EXPR_STMT;
						_tree->statement->expression_statement = tree::get_expr_stmt_mem();
						_tree->statement->expression_statement->expression = expression(terminator);
						if (peek_token(SEMICOLON))
							consume_next();
						else if (is_expr_terminator_consumed) {
						}
						else if (peek_token(END_OF_FILE))
							return tree_head;
						else {
							if (!is_expr_terminator_consumed)
								expect(SEMICOLON, true);
							else
								return tree_head;
						}
						tree::add_tree_node(&tree_head, &_tree);
					}
					else {
						compiler::lex->put_back(tok[1]);
						simple_declaration(tok[0], types, true, &compiler::symtab);
						
						if (peek_token(PARENTH_OPEN)) {
							symtable::remove_symbol(&compiler::symtab, funcname.string);
							symtab = symtable::get_node_mem();
							funcinfo = symtable::get_func_info_mem();
							func_head(&funcinfo, funcname, tok[0], types, true);
							funcinfo->ptr_oprtr_count = ptr_oprtr_count;
							symtab->func_info = funcinfo;
							
							funcit = compiler::func_table->find(funcname.string);
							if (funcit == compiler::func_table->end()) {
								compiler::func_table->insert(std::pair<std::string, st_func_info *>(funcname.string, funcinfo));
								expect(CURLY_OPEN_BRACKET, true);
								_tree = tree::get_tree_node_mem();
								_tree->symtab = symtab;
								get_func_info(&funcinfo, funcname, SIMPLE_TYPE, types, false, false);
								_tree->symtab->func_info = funcinfo;
								_stmt = statement(&symtab);
								_tree->statement = _stmt;
								_tree->symtab = symtab;
								tree::add_tree_node(&tree_head, &_tree);
								expect(CURLY_CLOSE_BRACKET, true);
							}
							else {
								log::error_at(funcname.loc, "redeclaration of function " + funcname.string);
								symtable::delete_func_info(&funcinfo);
								return tree_head;
							}
						}
					}
					ptr_oprtr_count = 0;
					funcname = nulltoken;
					types.clear();
				}
				else if (assignment_operator(tok[1].number) || tok[1].number == SQUARE_OPEN_BRACKET) {
					compiler::lex->put_back(tok[1]);
					compiler::lex->put_back(tok[0]);
					_tree = tree::get_tree_node_mem();
					symtable::delete_node(&(_tree->symtab));
					_tree->statement = tree::get_stmt_mem();
					_tree->statement->type = EXPR_STMT;
					_tree->statement->expression_statement = tree::get_expr_stmt_mem();
					_tree->statement->expression_statement->expression = expression(terminator);
					if (peek_token(SEMICOLON))
						consume_next();
					else if (is_expr_terminator_consumed && consumed_terminator.number == SEMICOLON) {
					}
					else {
						expect(SEMICOLON, true);
					}
					tree::add_tree_node(&tree_head, &_tree);
				}
				else if (binary_operator(tok[1].number) || tok[1].number == INCR_OP || tok[1].number == DECR_OP) {
					compiler::lex->put_back(tok[1]);
					compiler::lex->put_back(tok[0]);
					_tree = tree::get_tree_node_mem();
					_tree->statement = tree::get_stmt_mem();
					_tree->statement->type = EXPR_STMT;
					_tree->statement->expression_statement = tree::get_expr_stmt_mem();
					_tree->statement->expression_statement->expression = expression(terminator);
					if (peek_token(SEMICOLON))
						consume_next();
					else if (is_expr_terminator_consumed) {
					}
					else if (peek_token(END_OF_FILE))
						return tree_head;
					else {
						if (!is_expr_terminator_consumed)
							expect(SEMICOLON, true);
						else
							return tree_head;
					}
					tree::add_tree_node(&tree_head, &_tree);
				}
				else if (tok[1].number == PARENTH_OPEN) {
					compiler::lex->put_back(tok[1]);
					compiler::lex->put_back(tok[0]);
					_tree = tree::get_tree_node_mem();
					_tree->statement = tree::get_stmt_mem();
					_tree->statement->type = EXPR_STMT;
					_tree->statement->expression_statement = tree::get_expr_stmt_mem();
					_tree->statement->expression_statement->expression = expression(terminator);
					tree::add_tree_node(&tree_head, &_tree);
				}
				else {
					log::error_at(tok[1].loc, "invalid token found while parsing '" + tok[1].string + "'");
					return tree_head;
				}
				//if record, record-specifier
			}
			else if (tok[0].number == KEY_RECORD) {
				compiler::lex->put_back(tok[0]);
				record_specifier();
				//if expression token, for allowing global variables for modifications
			}
			else if (expression_token(tok[0].number)) {
				compiler::lex->put_back(tok[0]);
				_tree = tree::get_tree_node_mem();
				symtable::delete_node(&(_tree->symtab));
				_tree->statement = tree::get_stmt_mem();
				_tree->statement->type = EXPR_STMT;
				_tree->statement->expression_statement = tree::get_expr_stmt_mem();
				_tree->statement->expression_statement->expression = expression(terminator);
				if (peek_token(SEMICOLON))
					consume_next();
				else if (is_expr_terminator_consumed) {
				}
				else if (peek_token(END_OF_FILE))
					return tree_head;
				else {
					if (!is_expr_terminator_consumed)
						expect(SEMICOLON, true);
					else
						return tree_head;
				}
				tree::add_tree_node(&tree_head, &_tree);
			}
			else if (tok[0].number == KEY_ASM) {
				compiler::lex->put_back(tok[0]);
				_tree = tree::get_tree_node_mem();
				symtable::delete_node(&(_tree->symtab));
				_tree->statement = tree::get_stmt_mem();
				_tree->statement->type = ASM_STMT;
				_tree->statement->asm_statement = asm_statement();
				tree::add_tree_node(&tree_head, &_tree);
			}
			else if (tok[0].number == SEMICOLON) {
				consume_next();
			}
			else {
				log::error_at(tok[0].loc, "invalid token found while parsing '" + tok[0].string + "'");
				return tree_head;
			}
		}
		return tree_head;
	}
}
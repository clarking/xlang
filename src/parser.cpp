/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

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
	
	Parser::Parser() {

		
		Compiler::symtab = SymbolTable::get_node_mem();
		Compiler::record_table = SymbolTable::get_record_symtab_mem();
		Compiler::func_table = SymbolTable::get_func_table_mem();
		
		consumed_terminator.number = NONE;
		consumed_terminator.string = "";
		nulltoken.number = NONE;
		nulltoken.string = "";
	}
	
	bool Parser::peek_token(TokenId tk) {
	    
        //get Token from lexer and match it with tk and return Token again to lex

		Token tok = Compiler::lex->get_next();
		if (tok.number == tk) {
			Compiler::lex->put_back(tok);
			return true;
		}
		Compiler::lex->put_back(tok);
		return false;
	}
	
	bool Parser::peek_token(std::vector<TokenId> &tkv) {
	    
        // get Token from lexer and match it with a vector of tokens

		Token tok = Compiler::lex->get_next();
		std::vector<TokenId>::iterator it = tkv.begin();
		while (it != tkv.end()) {
			if (tok.number == *it) {
				Compiler::lex->put_back(tok);
				return true;
			}
			it++;
		}
		Compiler::lex->put_back(tok);
		return false;
	}
	
	bool Parser::peek_token(const char *format...) {

	    // peek Token with variable number of provided tokens

		va_list args;
		va_start(args, format);
		Token tok = Compiler::lex->get_next();
		
		while (*format != '\0') {
			if (*format == 'd') {
				if (va_arg(args, int) == tok.number) {
					Compiler::lex->put_back(tok);
					return true;
				}
			}
			++format;
		}
		
		va_end(args);
		Compiler::lex->put_back(tok);
		return false;
	}
	
	bool Parser::peek_nth_token(TokenId tk, int n) {
		Token *tok = new Token[n];
		TokenId tk2;
		int i;
		for (i = 0; i < n; i++)
			tok[i] = Compiler::lex->get_next();
		
		tk2 = tok[n - 1].number;
		
		for (i = n - 1; i >= 0; i--)
			Compiler::lex->put_back(tok[i]);
		
		delete[] tok;
		return (tk == tk2);
	}
	
	TokenId Parser::get_peek_token() {
		Token tok = Compiler::lex->get_next();
		TokenId tk = tok.number;
		Compiler::lex->put_back(tok);
		return tk;
	}
	
	TokenId Parser::get_nth_token(int n) {
		Token *tok = new Token[n];
		TokenId tk;
		int i;
		
		for (i = 0; i < n; i++)
			tok[i] = Compiler::lex->get_next();
		
		tk = tok[n - 1].number;
		for (i = n - 1; i >= 0; i--)
			Compiler::lex->put_back(tok[i]);
		
		delete[] tok;
		return tk;
	}
	
	bool Parser::expr_literal(TokenId tkt) {
		return (tkt == LIT_DECIMAL  ||
		        tkt == LIT_OCTAL    ||
		        tkt == LIT_HEX      ||
		        tkt == LIT_BIN      ||
		        tkt == LIT_FLOAT    ||
		        tkt == LIT_CHAR);
	}
	
	bool Parser::peek_expr_literal() {
		Token tok = Compiler::lex->get_next();
		TokenId tkt = tok.number;
		Compiler::lex->put_back(tok);
		return (expr_literal(tkt));
	}
	
	bool Parser::expect(TokenId tk) {

		Token tok = Compiler::lex->get_next();

		if (tok.number != tk) {
			std::map<TokenId, std::string>::iterator find_it = token_lexeme_table.find(tk);
			if (find_it != token_lexeme_table.end()) {
				auto it = std::prev(expr_list.end());
				TokenLocation loc;
				if (!expr_list.empty())
					loc = (*it).loc;
				Log::error_at(loc, "expected ", find_it->second);
				Log::print_tokens(expr_list);
				return false;
			}
		}
		Compiler::lex->put_back(tok);
		return true;
	}

	bool Parser::expect(TokenId tk, bool consume_token) {
	    
        //determine whether to consume Token or return it to lexer
		
        Token tok = Compiler::lex->get_next();
		if (tok.number == END)
			return false;
		
		if (tok.number != tk) {
			std::map<TokenId, std::string>::iterator find_it = token_lexeme_table.find(tk);
			if (find_it != token_lexeme_table.end()) {
				auto it = std::prev(expr_list.end());
				TokenLocation loc;
				
                if (!expr_list.empty())
					loc = (*it).loc;

				if (tok.number != END && expr_list.empty())
					loc = tok.loc;
				else {
					loc.line = 0;
					loc.col = 0;
				}

				Log::error_at(loc, "expected ", find_it->second, " but found " + s_quotestring(tok.string));
				Log::print_tokens(expr_list);
				return false;
			}
		}
		
		if (!consume_token)
			Compiler::lex->put_back(tok);
		return true;
	}
	
	bool Parser::expect(TokenId tk, bool consume_token, std::string str) {
		Token tok = Compiler::lex->get_next();
		if (tok.number != tk) {
			Log::error_at(tok.loc, "expected ", str);
			Log::print_tokens(expr_list);
			return false;
		}
		if (!consume_token)
			Compiler::lex->put_back(tok);
		return true;
	}
	
	bool Parser::expect(TokenId tk, bool consume_token, std::string str, std::string arg) {
		Token tok = Compiler::lex->get_next();
		
		if (tok.number != tk) {
			Log::error_at(tok.loc, "expected ", str, arg);
			Log::print_tokens(expr_list);
			return false;
		}
		if (!consume_token)
			Compiler::lex->put_back(tok);
		return true;
	}
	
	bool Parser::expect(const char *format...) {
		va_list args;
		va_start(args, format);
		Token tok = Compiler::lex->get_next();
		
		while (*format != '\0') {
			if (*format == 'd') {
				if (va_arg(args, int) == tok.number) {
					Compiler::lex->put_back(tok);
					return true;
				}
			}
			++format;
		}
		
		va_end(args);
		Log::error_at(tok.loc, "expected ");
		return false;
	}
	
	void Parser::consume_next() {
		Compiler::lex->get_next();
	}
	
	void Parser::consume_n(int n) {
		while (n > 0) {
			Compiler::lex->get_next();
			n--;
		}
	}
	
	void Parser::consume_till(terminator_t &terminator) {
		Token tok;
		std::sort(terminator.begin(), terminator.end());
		while ((tok = Compiler::lex->get_next()).number != END) {
			if (std::binary_search(terminator.begin(), terminator.end(), tok.number))
				break;
		}
		Compiler::lex->put_back(tok);
	}
	
	bool Parser::check_parenth() {
	    
        //used in primary expression for () checking using stack

		if (parenth_stack.size() > 0) {
			parenth_stack.pop();
			return true;
		}
		return false;
	}
	
	bool Parser::matches_terminator(terminator_t &tkv, TokenId tk) {
    	//matches with the terminator vector
	    //terminator is the vector of tokens which is used for expression terminator
		for (const auto& x: tkv) {
			if (x == tk)
				return true;
		}
		return false;
	}
	
	std::string Parser::get_terminator(terminator_t &terminator) {
		std::string st;
		size_t i;
		std::map<TokenId, std::string>::iterator find_it;
		for (i = 0; i < terminator.size(); i++) {
			find_it = token_lexeme_table.find(terminator[i]);
			if (find_it != token_lexeme_table.end())
				st = st + find_it->second + " ";
		}
		return st;
	}
	
	bool Parser::unary_operator(TokenId tk) {
		return (tk == ARTHM_ADD ||
                tk == ARTHM_SUB ||
                tk == LOG_NOT   ||
                tk == BIT_COMPL);
	}
	
	bool Parser::peek_unary_operator() {
		Token tok = Compiler::lex->get_next();
		TokenId tk = tok.number;
		Compiler::lex->put_back(tok);
		return unary_operator(tk);
	}
	
	bool Parser::binary_operator(TokenId tk) {
		return (arithmetic_operator(tk) ||
                logical_operator(tk)    ||
                comparison_operator(tk) ||
                bitwise_operator(tk));
	}
	
	bool Parser::arithmetic_operator(TokenId tk) {
		return (tk == ARTHM_ADD || 
                tk == ARTHM_SUB || 
                tk == ARTHM_MUL || 
                tk == ARTHM_DIV || 
                tk == ARTHM_MOD);
	}
	
	bool Parser::logical_operator(TokenId tk) {
		return (tk == LOG_AND || tk == LOG_OR);
	}
	
	bool Parser::comparison_operator(TokenId tk) {
		return (tk == COMP_LESS     ||
                tk == COMP_LESS_EQ  ||
                tk == COMP_GREAT    ||
                tk == COMP_GREAT_EQ ||
                tk == COMP_EQ       ||
                tk == COMP_NOT_EQ);
	}
	
	bool Parser::bitwise_operator(TokenId tk) {
		return (tk == BIT_OR     ||
                tk == BIT_AND    ||
                tk == BIT_EXOR   ||
                tk == BIT_LSHIFT ||
                tk == BIT_RSHIFT);
	}
	
	bool Parser::assignment_operator(TokenId tk) {
		return (tk == ASSGN           ||
		        tk == ASSGN_ADD       ||
		        tk == ASSGN_SUB       ||
		        tk == ASSGN_MUL       ||
		        tk == ASSGN_DIV       ||
		        tk == ASSGN_MOD       ||
		        tk == ASSGN_BIT_OR    ||
		        tk == ASSGN_BIT_AND   ||
		        tk == ASSGN_BIT_EX_OR ||
		        tk == ASSGN_LSHIFT    ||
		        tk == ASSGN_RSHIFT);
	}
	
	bool Parser::peek_binary_operator() {
		Token tok = Compiler::lex->get_next();
		TokenId tk = tok.number;
		Compiler::lex->put_back(tok);
		return binary_operator(tk);
		
	}
	
	bool Parser::peek_literal() {
		TokenId tk = get_peek_token();
		return (tk == LIT_DECIMAL   ||
                tk == LIT_OCTAL     ||
                tk == LIT_HEX       ||
                tk == LIT_BIN       ||
                tk == LIT_FLOAT     ||
                tk == LIT_CHAR);
	}
	
	bool Parser::peek_literal_string() {
		TokenId tk = get_peek_token();
		return (tk == LIT_DECIMAL   ||
		        tk == LIT_OCTAL     ||
		        tk == LIT_HEX       ||
		        tk == LIT_BIN       ||
		        tk == LIT_FLOAT     ||
		        tk == LIT_CHAR      ||
		        tk == LIT_STRING);
	}
	
	bool Parser::integer_literal(TokenId tk) {
		return (tk == LIT_DECIMAL   ||
                tk == LIT_OCTAL     ||
                tk == LIT_HEX       ||
                tk == LIT_BIN);
	}
	
	bool Parser::character_literal(TokenId tk) {
		return (tk == LIT_CHAR);
	}
	
	bool Parser::constant_expr(TokenId tk) {
		return (integer_literal(tk) || character_literal(tk));
	}
	
	bool Parser::peek_constant_expr() {
		return constant_expr(get_peek_token());
	}
	
	bool Parser::peek_assignment_operator() {
		TokenId tk = get_peek_token();
		return assignment_operator(tk);
	}
	
	bool Parser::peek_identifier() {
		return (get_peek_token() == IDENTIFIER);
	}
	
	bool Parser::expect_binary_operator() {
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
	
	bool Parser::expect_literal() {
		return expect("ddddddddd",
            LIT_DECIMAL, 
            LIT_OCTAL, 
            LIT_HEX, 
            LIT_BIN, 
            LIT_FLOAT, 
            LIT_CHAR);
	}
	
	bool Parser::expect_assignment_operator() {
		return expect("ddddddddddd", 
            ASSGN, 
            ASSGN_ADD, 
            ASSGN_SUB, 
            ASSGN_MUL, 
            ASSGN_DIV, 
            ASSGN_MOD, 
            ASSGN_BIT_OR, 
            ASSGN_BIT_AND, 
            ASSGN_BIT_EX_OR, 
            ASSGN_LSHIFT, 
            ASSGN_RSHIFT);
	}
	
	bool Parser::member_access_operator(TokenId tk) {
		return (tk == DOT_OP || tk == ARROW_OP);
	}
	
	bool Parser::peek_member_access_operator() {
		TokenId tk = get_peek_token();
		return member_access_operator(tk);
	}
	
	bool Parser::expression_token(TokenId tk) {
		return (tk == LIT_DECIMAL   ||
		        tk == LIT_OCTAL     ||
		        tk == LIT_HEX       ||
		        tk == LIT_BIN       ||
		        tk == LIT_FLOAT     ||
		        tk == LIT_CHAR      ||
		        tk == ARTHM_ADD     ||
		        tk == ARTHM_SUB     ||
		        tk == LOG_NOT       ||
		        tk == BIT_COMPL     ||
		        tk == IDENTIFIER    ||
		        tk == PARENTH_OPEN  ||
		        tk == ARTHM_MUL     ||
		        tk == INCR_OP       ||
		        tk == DECR_OP       ||
		        tk == BIT_AND       ||
		        tk == KEY_SIZEOF);
	}
	
	bool Parser::peek_expr_token() {
		return expression_token(get_peek_token());
	}
	
	bool Parser::peek_type_specifier(std::vector<Token> &tokens) {

		Token tok;
		tok = Compiler::lex->get_next();

		if (tok.number == KEY_VOID  ||
		    tok.number == KEY_CHAR  ||
		    tok.number == KEY_DOUBLE||
		    tok.number == KEY_FLOAT ||
		    tok.number == KEY_INT   ||
		    tok.number == KEY_SHORT ||
		    tok.number == KEY_LONG  ||
		    tok.number == IDENTIFIER) {
			
            tokens.push_back(tok);
			Compiler::lex->put_back(tok);
			return true;
		}
		
		Compiler::lex->put_back(tok);
		return false;
	}
	
	bool Parser::type_specifier(TokenId tk) {
		return (tk == KEY_CHAR  ||
		        tk == KEY_DOUBLE||
		        tk == KEY_FLOAT ||
		        tk == KEY_INT   ||
		        tk == KEY_SHORT ||
		        tk == KEY_LONG  ||
		        tk == KEY_VOID);
	}
	
	bool Parser::peek_type_specifier() {
		TokenId tk = get_peek_token();
		return type_specifier(tk);
	}
	
	void Parser::get_type_specifier(std::vector<Token> &types) {
		if (peek_type_specifier(types))
			return;
		types.clear();
	}
	
	bool Parser::peek_type_specifier_from(int n) {
		Token *tok = new Token[n];

		TokenId tk;
		for (int i = 0; i < n; i++)
			tok[i] = Compiler::lex->get_next();
		
		tk = tok[n - 1].number;

		for (int i = n - 1; i >= 0; i--) {
			Compiler::lex->put_back(tok[i]);
		}
		
		delete[] tok;
		return (type_specifier(tk));
	}
	
	void Parser::primary_expr(terminator_t &terminator) {

        // infix expressions are hard to parse so there is the possibility that we may
        // discard some valid expressions. Also some unary operators are not handled
        // at the final parsing state. Some are hardcoded for expecting the tokens
        // because of recursion.

		terminator_t terminator2;
		Token tok = Compiler::lex->get_next();
		
		if (matches_terminator(terminator, tok.number)) {
			expr_list.push_back(tok);
			return;
		}
		
		switch (tok.number) {
			
			case PARENTH_OPEN : {
				expr_list.push_back(tok);
				parenth_stack.push(tok);
				
				if (peek_token(PARENTH_CLOSE)) {
					Token tok2 = Compiler::lex->get_next();
					Log::error_at(tok2.loc, "expression expected ", tok2.string);
					return;
				}
				

				primary_expr(terminator); //call same function as per the grammar
				
				if (parenth_stack.size() > 0 && expect(PARENTH_CLOSE)) {
					if (!check_parenth())
						Log::error("unbalanced parenthesis");
					else {
						Token tok2 = Compiler::lex->get_next();
						expr_list.push_back(tok2);
					}
					
					if (peek_binary_operator() || peek_unary_operator()) {
						sub_primary_expr(terminator);
					}
					else if (peek_token(terminator)) {

						if (check_parenth())
							Log::error("unbalanced parenthesis");

						Token tok2 = Compiler::lex->get_next();
						is_expr_terminator_consumed = true;
						consumed_terminator = tok2;
						is_expr_terminator_got = true;
					}
					else if (peek_token(PARENTH_CLOSE)) {
						Token tok2 = Compiler::lex->get_next();

						if (!check_parenth())
							Log::error_at(tok2.loc, "unbalanced parenthesis ", tok2.string);
						else {
							expr_list.push_back(tok2);
							primary_expr(terminator);
						}
					}
					else {

						tok = Compiler::lex->get_next();
						if (!is_expr_terminator_consumed || !is_expr_terminator_got)
							Log::error_at(tok.loc, get_terminator(terminator) + "expected");

						if (check_parenth())
							Log::error("unbalanced parenthesis");
						else {
							if (tok.number == END)
								return;
							Log::error(get_terminator(terminator) + "expected but found " + tok.string);
						}
					}
				}
			}
				break;
			
			case PARENTH_CLOSE : {
                
				if (!check_parenth()) 
					Log::error("unbalanced parenthesis");
				else {
				
					expr_list.push_back(tok); // push ')' on expression list
					
					if (peek_binary_operator())
						primary_expr(terminator);
					else if (peek_token(terminator)) {
						is_expr_terminator_got = true;
						Token tok2 = Compiler::lex->get_next();
						is_expr_terminator_consumed = true;
						consumed_terminator = tok2;
						return;
					}
					else if (peek_token(PARENTH_CLOSE))
						primary_expr(terminator);
					else {
						Log::error(get_terminator(terminator) + "expected ");
						Log::print_tokens(expr_list);
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
				
				expr_list.push_back(tok); // push literal
				
				if (peek_binary_operator() || peek_unary_operator()) {
					if (expect_binary_operator()) {
						Token tok2 = Compiler::lex->get_next();
						expr_list.push_back(tok2);
					}
					
					if (peek_token(PARENTH_OPEN) || peek_token(IDENTIFIER)) {
						primary_expr(terminator);
					}
					else if (peek_expr_literal()) {
						if (expect_literal()) {
							Token tok2 = Compiler::lex->get_next();
							expr_list.push_back(tok2);
						}
					}
					else if (peek_unary_operator()) {
						sub_primary_expr(terminator);
					}
					else {
						Token tok2 = Compiler::lex->get_next();
						Log::error_at(tok2.loc, "literal or expression expected ", tok2.string);
						for (const auto &e: expr_list) {
							Log::error(e.number, e.string, "\n");
						}
						std::cout << std::endl;
						return;
					}
				}
				else if (peek_token(terminator)) {

					if (check_parenth()) {
						Log::error("unbalanced parenthesis");
					}
					else {
						Token tok2 = Compiler::lex->get_next();
						//expr_list.push_back(tok2);
						is_expr_terminator_got = true;
						is_expr_terminator_consumed = true;
						consumed_terminator = tok2;
						return;
					}
				}
				else if (peek_token(PARENTH_CLOSE)) 
					primary_expr(terminator);
				else {

					Token tok2 = Compiler::lex->get_next();
					if (!is_expr_terminator_got) {
						Log::error(get_terminator(terminator) + " expected ");
						Log::print_tokens(expr_list);
						Compiler::lex->put_back(tok2);
						return;
					}
					
					if (!check_parenth()) {
						Log::error("unbalanced parenthesis");
						return;
					}
				}
				

				if (peek_token(terminator)) {
					Token tok2 = Compiler::lex->get_next();
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
							Token tok2 = Compiler::lex->get_next();
							Log::error_at(tok2.loc, "error ", tok2.string);
						}
					}
					else if (peek_token(END)) {
						Token tok2 = Compiler::lex->get_next();
						if (check_parenth())
							Log::error("unbalanced parenthesis");

						if (!is_expr_terminator_consumed) {
							Log::error_at(tok2.loc, get_terminator(terminator) + "expected");
							return;
						}
					}
					else if (peek_expr_literal()) {
						Token tok2 = Compiler::lex->get_next();
						if (check_parenth())
							Log::error("unbalanced parenthesis");

						if (!is_expr_terminator_got)
							Log::error_at(tok2.loc, get_terminator(terminator) + "expected");

						Compiler::lex->put_back(tok2);
					}
					else {
						if (!is_expr_terminator_consumed) {
							Log::error(get_terminator(terminator) + "expected ");
							Log::print_tokens(expr_list);
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
					Compiler::lex->put_back(tok);
					return;
				}
				
				if (unary_operator(tok.number)) {
					expr_list.push_back(tok);
					if (peek_token(PARENTH_OPEN) ||
					    peek_expr_literal()      ||
					    peek_binary_operator()   ||
					    peek_unary_operator()    ||
					    peek_token(IDENTIFIER)) {
						sub_primary_expr(terminator);
					}
					else if (peek_token(INCR_OP))
						prefix_incr_expr(terminator);
					else if (peek_token(DECR_OP))
						prefix_decr_expr(terminator);
					else {
						Token tok2 = Compiler::lex->get_next();
						Log::error_at(tok2.loc, "expression expected ", tok2.string);
					}
				}
				else {
					if (peek_token(PARENTH_OPEN) || peek_expr_literal() || peek_token(IDENTIFIER)) {
						expr_list.push_back(tok);
						sub_primary_expr(terminator);
					}
					else {
						Token tok2 = Compiler::lex->get_next();
						Log::error_at(tok2.loc, "literal expected ", tok2.string);
						return;
					}
				}
			}
				break;
			
			case IDENTIFIER : {
				
				if (peek_binary_operator()) {
					expr_list.push_back(tok);
					sub_primary_expr(terminator);
				}
				else if (peek_token(terminator)) {
					expr_list.push_back(tok);
					tok = Compiler::lex->get_next();
					is_expr_terminator_consumed = true;
					consumed_terminator = tok;
					return;
				}
				else if (peek_token(END)) {
					expr_list.push_back(tok);
					Log::error_at(tok.loc, get_terminator(terminator) + "expected");
					return;
				}
				else {
					Compiler::lex->put_back(tok, true);
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
				Log::error_at(tok.loc, "primaryexpr invalid Token ", tok.string);
				return;
			}
				break;
		}
	}
	
	void Parser::sub_primary_expr(terminator_t &terminator) {
		if (expr_list.size() > 0)
			primary_expr(terminator);
	}
	
	int Parser::precedence(TokenId opr) {

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
        // &                  Address (of Operand)
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

		switch (opr) {
			case DOT_OP : 
                return 24;
			case ARROW_OP : 
                return 23;
			case INCR_OP :
			case DECR_OP : 
                return 22;
			case LOG_NOT :
			case BIT_COMPL: 
                return 21;
			case ADDROF_OP : 
                return 20;
			case KEY_SIZEOF : 
                return 19;
			case ARTHM_MUL :
			case ARTHM_DIV :
			case ARTHM_MOD : 
                return 18;
			case ARTHM_ADD :
			case ARTHM_SUB : 
                return 17;
			case BIT_LSHIFT :
			case BIT_RSHIFT : 
                return 16;
			case COMP_LESS :
			case COMP_LESS_EQ : 
                return 15;
			case COMP_GREAT :
			case COMP_GREAT_EQ : 
                return 14;
			case COMP_EQ :
			case COMP_NOT_EQ : 
                return 13;
			case BIT_AND : 
                return 12;
			case BIT_EXOR : 
                return 11;
			case BIT_OR : 
                return 10;
			case LOG_AND : 
                return 9;
			case LOG_OR : 
                return 8;
			case ASSGN : 
                return 7;
			case ASSGN_ADD :
			case ASSGN_SUB : 
                return 6;
			case ASSGN_MUL :
			case ASSGN_DIV : 
                return 5;
			case ASSGN_MOD :
			case ASSGN_BIT_AND : 
                return 4;
			case ASSGN_BIT_EX_OR :
			case ASSGN_BIT_OR :
                return 3;
			case ASSGN_LSHIFT :
			case ASSGN_RSHIFT : 
                return 2;
			case COMMA_OP :
                return 1;
			default:
                return 0;
		}
	}
	
	void Parser::postfix_expression(std::list<Token> &postfix_expr) {

        //converts primary expression into its reverse polish notation
        //by checking the precedency of an operator

		std::stack<Token> post_stack;
		auto expr_it = expr_list.begin();
		
		while (expr_it != expr_list.end()) {
			switch ((*expr_it).number) {
				case LIT_DECIMAL :
				case LIT_OCTAL :
				case LIT_HEX :
				case LIT_BIN :
				case LIT_FLOAT :
				case LIT_CHAR :
				case IDENTIFIER : 
                    postfix_expr.push_back(*expr_it);
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
						if (!post_stack.empty() && (precedence((*expr_it).number) > precedence(post_stack.top().number))) 
							post_stack.push(*expr_it);
						else {
							while (!post_stack.empty() && (precedence((*expr_it).number) <= precedence(post_stack.top().number))) {
								postfix_expr.push_back(post_stack.top());
								post_stack.pop();
							}
							post_stack.push(*expr_it);
						}
					}
					break;
				case PARENTH_OPEN : 
                    post_stack.push(*expr_it);
					break;
				case PARENTH_CLOSE :
					while (!post_stack.empty() && post_stack.top().number != PARENTH_OPEN) {
						postfix_expr.push_back(post_stack.top());
						post_stack.pop();
					}

					if (!post_stack.empty() && post_stack.top().number == PARENTH_OPEN)
						post_stack.pop();

					break;
				case SQUARE_OPEN :
					while (expr_it != expr_list.end() && (*expr_it).number != SQUARE_CLOSE) {
						postfix_expr.push_back(*expr_it);
						expr_it++;
					}

					postfix_expr.push_back(*expr_it);
					break;
					
					
				case SEMICOLON :
				case COMMA_OP : 
                    goto exit; //if ; , then exit
				
				default : 
                    Log::error_at((*expr_it).loc, "error in conversion into postfix expression ", (*expr_it).string);
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
	
	PrimaryExpression *Parser::get_primary_expr_tree() {

        // returns primary expression tree 
        // by building it from reverse polish notation
        // of an primary expression

		std::stack<PrimaryExpression *> extree_stack;
		std::list<Token>::iterator post_it;
		PrimaryExpression *Expression = nullptr, *oprtr = nullptr;
		std::list<Token> postfix_expr;
		Token unary_tok = nulltoken;
		
		postfix_expression(postfix_expr);
		
		//first check for size, there could be only an identifier
		if (postfix_expr.size() == 1) {
			Expression = Tree::get_primary_expr_mem();
			Expression->tok = postfix_expr.front();
			Expression->is_oprtr = false;
			post_it = postfix_expr.begin();
			if (post_it->number == IDENTIFIER)
				Expression->is_id = true;
			else
				Expression->is_id = false;
			return Expression;
		}
		
		for (post_it = postfix_expr.begin(); post_it != postfix_expr.end(); post_it++) {
			//if literal/identifier is found, push it onto stack
			if (expr_literal((*post_it).number)) {
				Expression = Tree::get_primary_expr_mem();
				Expression->tok = *post_it;
				Expression->is_id = false;
				Expression->is_oprtr = false;
				extree_stack.push(Expression);
				Expression = nullptr;
			}
			else if ((*post_it).number == IDENTIFIER) {
				Expression = Tree::get_primary_expr_mem();
				Expression->tok = *post_it;
				Expression->is_id = true;
				Expression->is_oprtr = false;
				extree_stack.push(Expression);
				Expression = nullptr;
				//if operator is found, pop last two entries from stack,
				//assign left and right nodes and push the generated tree into stack again
			}
			else if (binary_operator((*post_it).number) || (*post_it).number == DOT_OP || (*post_it).number == ARROW_OP) {
				oprtr = Tree::get_primary_expr_mem();
				oprtr->tok = *post_it;
				oprtr->is_id = false;
				oprtr->is_oprtr = true;
				oprtr->oprtr_kind = OperatorType::BINARY;
				if (extree_stack.size() > 1) {
					oprtr->right = extree_stack.top();
					extree_stack.pop();
					oprtr->left = extree_stack.top();
					extree_stack.pop();
					extree_stack.push(oprtr);
				}
			}

			else if ((*post_it).number == BIT_COMPL || (*post_it).number == LOG_NOT) 
				unary_tok = *post_it;
		}
		
		postfix_expr.clear();
		
		if (unary_tok.number != NONE) {
			oprtr = Tree::get_primary_expr_mem();
			oprtr->tok = unary_tok;
			oprtr->is_id = false;
			oprtr->is_oprtr = true;
			oprtr->oprtr_kind = OperatorType::UNARY;
			
			if (!extree_stack.empty())
				oprtr->unary_node = extree_stack.top();
			
			return oprtr;
		}
		
		if (!extree_stack.empty())
			return extree_stack.top();
		
		return nullptr;
	}

	IdentifierExpression *Parser::get_id_expr_tree() {

        // returns identifier expression tree
        // this is same as primary expression tree generation
        // only just handling member access operators as well as
        // increment/decrement/addressof operators

		std::stack<IdentifierExpression *> extree_stack;
		std::list<Token>::iterator post_it;
		std::list<Token> postfix_expr;
		IdentifierExpression *Expression = nullptr, *oprtr = nullptr;
		IdentifierExpression *temp = nullptr;
		
		postfix_expression(postfix_expr);
		
		for (post_it = postfix_expr.begin(); post_it != postfix_expr.end(); post_it++) {
			if ((*post_it).number == IDENTIFIER) {
				Expression = Tree::get_id_expr_mem();
				Expression->tok = *post_it;
				Expression->is_id = true;
				Expression->is_oprtr = false;
				post_it++;
				if (post_it != postfix_expr.end()) {
					if ((*post_it).number == SQUARE_OPEN) {
						Expression->is_subscript = true;
					}
				}
				post_it--;
				extree_stack.push(Expression);
				Expression = nullptr;
			}
			else if (binary_operator((*post_it).number) || (*post_it).number == DOT_OP || (*post_it).number == ARROW_OP) {
				oprtr = Tree::get_id_expr_mem();
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
				oprtr = Tree::get_id_expr_mem();
				oprtr->tok = *post_it;
				oprtr->is_id = false;
				oprtr->is_oprtr = true;
				oprtr->is_subscript = false;
				oprtr->unary = Tree::get_id_expr_mem();
				if (extree_stack.size() > 0) {
					oprtr->unary = extree_stack.top();
					extree_stack.pop();
					extree_stack.push(oprtr);
				}
			}
			else if ((*post_it).number == SQUARE_OPEN) {
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
	
	void Parser::id_expr(terminator_t &terminator) {
		Token tok = Compiler::lex->get_next();
		
		if (tok.number == IDENTIFIER) {
			expr_list.push_back(tok);
			
			if (peek_token(terminator)) {
				Token tok2 = Compiler::lex->get_next();
				if (parenth_stack.size() > 0) {
					Compiler::lex->put_back(tok2);
					return;
				}
				is_expr_terminator_consumed = true;
				consumed_terminator = tok2;
				return;
			}
			else if (peek_token(SQUARE_OPEN))
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
				Token tok2 = Compiler::lex->get_next();
				expr_list.push_back(tok2);
				id_expr(terminator);
			}
			else if (peek_assignment_operator() || peek_token(PARENTH_OPEN)) {
				return;    //if found do nothing
			}
			else {
				tok = Compiler::lex->get_next();
				std::string st = get_terminator(terminator);
				Log::error_at(tok.loc, st + " expected in id expression but found ", tok.string);
				Log::print_tokens(expr_list);
				return;
			}
		}
		else {
			Log::error_at(tok.loc, " identifier expected but found ", tok.string);
			Log::print_tokens(expr_list);
		}
	}

	void Parser::subscript_id_access(terminator_t &terminator) {
		
		if (expect(SQUARE_OPEN)) {
			Token tok = Compiler::lex->get_next();
			expr_list.push_back(tok);
			
			if (peek_constant_expr() || peek_identifier()) {
				tok = Compiler::lex->get_next();
				expr_list.push_back(tok);
				
				if (expect(SQUARE_CLOSE)) {
					tok = Compiler::lex->get_next();
					expr_list.push_back(tok);
				}
				
				//check for multi-dimensional array
				if (peek_token(SQUARE_OPEN))
					subscript_id_access(terminator);
				else if (peek_token(DOT_OP) || peek_token(ARROW_OP)) {
					Token tok2 = Compiler::lex->get_next();
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
					Log::error("; , ) expected ");
					Log::print_tokens(expr_list);
					return;
				}
			}
			else {
				Token tok2 = Compiler::lex->get_next();
				Log::error("constant expression expected ", tok2.string);
				Log::print_tokens(expr_list);
				
			}
		}
	}
	
	void Parser::pointer_operator_sequence() {
		Token tok;
		//here ARTHM_MUL Token will be changed to PTR_OP
		while ((tok = Compiler::lex->get_next()).number == ARTHM_MUL) {
			tok.number = PTR_OP;
			expr_list.push_back(tok);
		}
		Compiler::lex->put_back(tok);
	}
	
	int Parser::get_pointer_operator_sequence() {
		int ptr_count = 0;
		Token tok;
		//here ARTHM_MUL Token will be changed to PTR_OP
		while ((tok = Compiler::lex->get_next()).number == ARTHM_MUL) {
			ptr_count++;
		}
		Compiler::lex->put_back(tok);
		return ptr_count;
	}
	
	void Parser::pointer_indirection_access(terminator_t &terminator) {
	    // pointer-indirection-access : pointer-operator-sequence id-expression
		pointer_operator_sequence();

		if (peek_token(IDENTIFIER))
			id_expr(terminator);
		else {
			Log::error("identifier expected in pointer indirection");
			Log::print_tokens(expr_list);
		}
	}
	
	IdentifierExpression *Parser::prefix_incr_expr(terminator_t &terminator) {
	    // prefix-incr-expression : incr-operator id-expression

		IdentifierExpression *pridexpr = nullptr;
		if (expect(INCR_OP)) {
			Token tok = Compiler::lex->get_next();
			expr_list.push_back(tok);
		}
		if (peek_token(IDENTIFIER)) {
			id_expr(terminator);
			pridexpr = get_id_expr_tree();
			return pridexpr;
		}
		else {
			Log::error("identifier expected ");
			Log::print_tokens(expr_list);
			
			return nullptr;
		}
		return nullptr;
	}
	
	IdentifierExpression *Parser::prefix_decr_expr(terminator_t &terminator) {
		IdentifierExpression *pridexpr = nullptr;
		if (expect(DECR_OP)) {
			Token tok = Compiler::lex->get_next();
			expr_list.push_back(tok);
		}
		if (peek_token(IDENTIFIER)) {
			id_expr(terminator);
			pridexpr = get_id_expr_tree();
			return pridexpr;
		}
		else {
			Log::error("identifier expected ");
			Log::print_tokens(expr_list);
			
			return nullptr;
		}
		return nullptr;
	}
	
	void Parser::postfix_incr_expr(terminator_t &terminator) {
		Token tok;
		if (expect(INCR_OP)) {
			tok = Compiler::lex->get_next();
			expr_list.push_back(tok);
		}
		if (peek_token(terminator)) {
			tok = Compiler::lex->get_next();
			is_expr_terminator_consumed = true;
			consumed_terminator = tok;
			return;
		}
		else {
			tok = Compiler::lex->get_next();
			Log::error_at(tok.loc, "; , ) expected but found " + tok.string);
			Log::print_tokens(expr_list);
			
			return;
		}
	}
	
	void Parser::postfix_decr_expr(terminator_t &terminator) {
		if (expect(DECR_OP)) {
			Token tok = Compiler::lex->get_next();
			expr_list.push_back(tok);
		}
		if (peek_token(terminator)) {
			Token tok = Compiler::lex->get_next();
			//expr_list.push_back(tok);
			is_expr_terminator_consumed = true;
			consumed_terminator = tok;
			return;
		}
		else {
			Log::error("; , ) expected ");
			Log::print_tokens(expr_list);
			return;
		}
	}
	
	IdentifierExpression *Parser::address_of_expr(terminator_t &terminator) {
		IdentifierExpression *addrexpr = nullptr;
		if (expect(BIT_AND)) {
			Token tok = Compiler::lex->get_next();
			//change Token bitwise and to address of operator
			tok.number = ADDROF_OP;
			expr_list.push_back(tok);
			id_expr(terminator);
			addrexpr = Tree::get_id_expr_mem();
			addrexpr = get_id_expr_tree();
			return addrexpr;
		}
		return nullptr;
	}
	
	SizeOfExpression *Parser::sizeof_expr(terminator_t &terminator) {

		SizeOfExpression *sizeofexpr = Tree::get_sizeof_expr_mem();
		std::vector<Token> simple_types;
		terminator_t terminator2;
		size_t i;
		Token tok;
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
			Log::error("simple types, class names or identifier expected for sizeof ");
			terminator2.clear();
			terminator2.push_back(PARENTH_CLOSE);
			terminator2.push_back(SEMICOLON);
			terminator2.push_back(COMMA_OP);
			consume_till(terminator2);
		}
		expect(PARENTH_CLOSE, true);
		if (peek_token(terminator)) {
			is_expr_terminator_consumed = true;
			consumed_terminator = Compiler::lex->get_next();
			return sizeofexpr;
		}
		else {
			tok = Compiler::lex->get_next();
			Log::error_at(tok.loc, " ; , expected but found ", tok.string);
		}
		delete sizeofexpr;
		return nullptr;
	}
	
	CastExpression *Parser::cast_expr(terminator_t &terminator) {
		CastExpression *cstexpr = Tree::get_cast_expr_mem();
		Token tok;
		expect(PARENTH_OPEN, true);
		cast_type_specifier(&cstexpr);
		expect(PARENTH_CLOSE, true);
		if (peek_token(IDENTIFIER)) {
			id_expr(terminator);
			cstexpr->target = get_id_expr_tree();
			return cstexpr;
		}
		else {
			tok = Compiler::lex->get_next();
			Log::error_at(tok.loc, " identifier expected in cast expression");
		}
		delete cstexpr;
		return nullptr;
	}
	
	void Parser::cast_type_specifier(CastExpression **Expression) {
		CastExpression *cstexpr = *Expression;
		std::vector<Token> simple_types;
		terminator_t terminator2;
		size_t i;
		Token tok;
		
		if (peek_type_specifier(simple_types)) {
			if (simple_types.size() > 0 && simple_types[0].number == IDENTIFIER) {
				cstexpr->is_simple_type = false;
				cstexpr->identifier = simple_types[0];
			}
			else {
				cstexpr->is_simple_type = true;
				for (i = 0; i < simple_types.size(); i++) 
					cstexpr->simple_type.push_back(simple_types[i]);
			}
			consume_n(simple_types.size());
			simple_types.clear();
		}
		else {
			tok = Compiler::lex->get_next();
			Log::error_at(tok.loc, "simple type or record name for casting ");
			terminator2.clear();
			terminator2.push_back(PARENTH_CLOSE);
			terminator2.push_back(SEMICOLON);
			terminator2.push_back(COMMA_OP);
			consume_till(terminator2);
			
		}

		if (peek_token(ARTHM_MUL))
			cstexpr->ptr_oprtr_count = get_pointer_operator_sequence();
	}
	
	AssignmentExpression *Parser::assignment_expr(terminator_t &terminator, bool is_left_side_handled) {

		AssignmentExpression *assexpr = nullptr;
		IdentifierExpression *idexprtree = nullptr;
		Expression *_expr = nullptr;
		IdentifierExpression *ptr_ind = nullptr;
		
		Token tok;
		if (expect_assignment_operator()) {
			tok = Compiler::lex->get_next();
			assexpr = Tree::get_assgn_expr_mem();
			assexpr->tok = tok;
			
			if (!is_left_side_handled) {
				idexprtree = get_id_expr_tree();
				
				if (ptr_oprtr_count > 0) {
					ptr_ind = Tree::get_id_expr_mem();
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
			tok = Compiler::lex->get_next();
			Log::error_at(tok.loc, " assignment operator expected but found ", tok.string);
		}
		return nullptr;
	}
	
	CallExpression *Parser::call_expr(terminator_t &terminator) {

		CallExpression *funccallexp = nullptr;
		std::list<Expression *> exprlist;
		IdentifierExpression *idexpr = nullptr;
		Token tok;
		
		idexpr = get_id_expr_tree();
		funccallexp = Tree::get_func_call_expr_mem();
		funccallexp->function = idexpr;
		
		expect(PARENTH_OPEN, true);
		
		if (peek_token(PARENTH_CLOSE)) {
			consume_next();
			if (peek_token(terminator)) {
				consume_next();
				return funccallexp;
			}
			else {
				tok = Compiler::lex->get_next();
				Log::error_at(tok.loc, get_terminator(terminator) + " expected in function call but found: " + tok.string);
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
						tok = Compiler::lex->get_next();
						Log::error_at(tok.loc, get_terminator(terminator) + " expected in function call but found " + tok.string);
					}
				}
				else {
					tok = Compiler::lex->get_next();
					Log::error_at(tok.loc, get_terminator(terminator) + " expected in function call but found " + tok.string);
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
					tok = Compiler::lex->get_next();
					Log::error_at(tok.loc, get_terminator(terminator) + " expected in function call but found " + tok.string);
				}
			}
		}
		
		Tree::delete_func_call_expr(&funccallexp);
		return nullptr;
	}

	void Parser::func_call_expr_list(std::list<Expression *> &exprlist, terminator_t &orig_terminator) {

		Expression *_expr = nullptr;
		Token tok;
		terminator_t terminator = {COMMA_OP, PARENTH_CLOSE};
		
		if (peek_expr_token() || peek_token(LIT_STRING)) {
			is_expr_terminator_consumed = false;
			_expr = expression(terminator);
			
			if (is_expr_terminator_consumed) {
				if (consumed_terminator.number == PARENTH_CLOSE) {
					exprlist.push_back(_expr);
					//is_expr_terminator_consumed = true;
					//consumed_terminator = Compiler::lex->get_next();
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
				tok = Compiler::lex->get_next();
				if (is_expr_terminator_consumed) {
					if (consumed_terminator.number == PARENTH_CLOSE) {
						return;
					}
					else {
						tok = Compiler::lex->get_next();
						Log::error_at(tok.loc, "invalid Token found in function call parameters " + tok.string);
					}
				}
				else {
					tok = Compiler::lex->get_next();
					Log::error_at(tok.loc, get_terminator(terminator) + " expected in function call but found " + tok.string);
				}
			}
		}
		else {
			if (is_expr_terminator_consumed) {
				if (consumed_terminator.number == PARENTH_CLOSE)
					return;
				else {
					tok = Compiler::lex->get_next();
					Log::error_at(tok.loc, "invalid Token found in function call parameters " + tok.string);
				}
			}
			else {
				tok = Compiler::lex->get_next();
				Log::error_at(tok.loc, get_terminator(terminator) + " expected in function call but found " + tok.string);
			}
		}
	}
	
	Expression *Parser::expression(terminator_t &terminator) {
	
    	Token tok, tok2;
		std::vector<Token> specifier;
		std::vector<Token>::iterator lst_it;
		terminator_t terminator2;
		SizeOfExpression *sizeofexpr = nullptr;
		CastExpression *castexpr = nullptr;
		PrimaryExpression *pexpr = nullptr;
		IdentifierExpression *idexpr = nullptr;
		AssignmentExpression *assgnexpr = nullptr;
		CallExpression *funcclexpr = nullptr;
		Expression *_expr = Tree::get_expr_mem();
		
		if (peek_token(terminator))
			return nullptr;
		
		tok = Compiler::lex->get_next();
		
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
			case BIT_COMPL : 
                Compiler::lex->put_back(tok);
				primary_expr(terminator);
				pexpr = get_primary_expr_tree();

				if (pexpr == nullptr) {
					Log::error("error to parse primary expression");
					Tree::delete_expr(&_expr);
					return nullptr;
				}

				_expr->expr_kind = ExpressionType::PRIMARY_EXPR;
				_expr->primary_expr = pexpr;
				expr_list.clear();
				is_expr_terminator_got = false;
				break;

			case LIT_STRING :
                pexpr = Tree::get_primary_expr_mem();
				pexpr->is_id = false;
				pexpr->tok = tok;
				pexpr->is_oprtr = false;
				_expr->expr_kind = ExpressionType::PRIMARY_EXPR;
				_expr->primary_expr = pexpr;

				if (!peek_token(terminator)) {
					Log::error_at(tok.loc, "semicolon expected " + tok.string);
					Tree::delete_expr(&_expr);
					return nullptr;
				}

				expr_list.clear();
				is_expr_terminator_got = false;
				break;

			case IDENTIFIER :
				//peek for . -> [
				if (peek_token(DOT_OP) || peek_token(ARROW_OP) || peek_token(SQUARE_OPEN)) {
					Compiler::lex->put_back(tok, true);
				
					id_expr(terminator); 	//get id expression
					if (peek_assignment_operator()) {
						assgnexpr = assignment_expr(terminator, false);
						if (assgnexpr == nullptr) {
							Log::error("error to parse assignment expression");
							Tree::delete_expr(&_expr);
							return nullptr;
						}
						_expr->expr_kind = ExpressionType::ASSGN_EXPR;
						_expr->assgn_expr = assgnexpr;
					}
					else if (peek_token(terminator)) {
						tok2 = Compiler::lex->get_next();
						is_expr_terminator_consumed = true;
						consumed_terminator = tok2;
						//get id expression tree
						idexpr = get_id_expr_tree();
						if (idexpr == nullptr) {
							Log::error("error to parse id expression");
							Tree::delete_expr(&_expr);
							return nullptr;
						}
						_expr->expr_kind = ExpressionType::ID_EXPR;
						_expr->id_expr = idexpr;
					}
					else if (peek_token(PARENTH_OPEN)) {
						funcclexpr = call_expr(terminator);
						if (funcclexpr == nullptr) {
							Log::error("error to parse function call expression");
							Tree::delete_expr(&_expr);
							return nullptr;
						}

						_expr->expr_kind = ExpressionType::FUNC_CALL_EXPR;
						_expr->call_expr = funcclexpr;
					}
					else if (peek_token(PARENTH_CLOSE)) {
                        // nothing
					}
					else {
						idexpr = get_id_expr_tree();
						if (idexpr == nullptr) {
							Log::error("error to parse id expression");
							Tree::delete_expr(&_expr);
							return nullptr;
						}
						_expr->expr_kind = ExpressionType::ID_EXPR;
						_expr->id_expr = idexpr;
					}

					expr_list.clear();
					is_expr_terminator_got = false;
					
				}
				else if (peek_token(PARENTH_OPEN)) {

					Compiler::lex->put_back(tok, true);
					id_expr(terminator);
					funcclexpr = call_expr(terminator);
					if (funcclexpr == nullptr) {
						Log::error("error to parse function call expression");
						Tree::delete_expr(&_expr);
						return nullptr;
					}
					_expr->expr_kind = ExpressionType::FUNC_CALL_EXPR;
					_expr->call_expr = funcclexpr;
				}
				else if (peek_token(INCR_OP) || peek_token(DECR_OP)) {

					Compiler::lex->put_back(tok, true);
					id_expr(terminator);
					idexpr = get_id_expr_tree();
					if (idexpr == nullptr) {
						Log::error("error to parse id expression");
						Tree::delete_expr(&_expr);
						return nullptr;
					}
					_expr->expr_kind = ExpressionType::ID_EXPR;
					_expr->id_expr = idexpr;
				}
				else {

					Compiler::lex->put_back(tok, true);
					primary_expr(terminator);
					if (peek_assignment_operator()) {
						assgnexpr = assignment_expr(terminator, false);
						if (assgnexpr == nullptr) {
							Log::error("error to parse assignment expression");
							Tree::delete_expr(&_expr);
							return nullptr;
						}
						_expr->expr_kind = ExpressionType::ASSGN_EXPR;
						_expr->assgn_expr = assgnexpr;
					}
					else {
						pexpr = get_primary_expr_tree();
						if (pexpr == nullptr) {
							Log::error("error to parse primary expression");
							Tree::delete_expr(&_expr);
							return nullptr;
						}
						_expr->expr_kind = ExpressionType::PRIMARY_EXPR;
						_expr->primary_expr = pexpr;
						is_expr_terminator_got = false;
					}
				}
				expr_list.clear();
				is_expr_terminator_got = false;
				break;
			
			case PARENTH_OPEN : tok2 = Compiler::lex->get_next();

				if (type_specifier(tok2.number) || SymbolTable::search_record(Compiler::record_table, tok2.string)) {
					Compiler::lex->put_back(tok);
					Compiler::lex->put_back(tok2);
					castexpr = cast_expr(terminator);
					if (castexpr == nullptr) {
						Log::error("error to parse cast expression");
						Tree::delete_expr(&_expr);
						return nullptr;
					}
					_expr->expr_kind = ExpressionType::CAST_EXPR;
					_expr->cast_expr = castexpr;
				}
				else if (tok2.number == END)
					return nullptr;
				else {
					Compiler::lex->put_back(tok);
					Compiler::lex->put_back(tok2);
					primary_expr(terminator);
					pexpr = get_primary_expr_tree();
					if (pexpr == nullptr) {
						Log::error("error to parse primary expression");
						Tree::delete_expr(&_expr);
						return nullptr;
					}
					_expr->expr_kind = ExpressionType::PRIMARY_EXPR;
					_expr->primary_expr = pexpr;
				}

				expr_list.clear();
				is_expr_terminator_got = false;
				break;

			case ARTHM_MUL : 
                Compiler::lex->put_back(tok);
				pointer_indirection_access(terminator);
				
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
				
				if (peek_assignment_operator()) {
					assgnexpr = assignment_expr(terminator, false);
					if (assgnexpr == nullptr) {
						Log::error("error to parse assignment expression");
						Tree::delete_expr(&_expr);
						return nullptr;
					}
					_expr->expr_kind = ExpressionType::ASSGN_EXPR;
					_expr->assgn_expr = assgnexpr;
				}
				else {
					idexpr = get_id_expr_tree();
					if (idexpr == nullptr) {
						Log::error("error to parse pointer indirection expression");
						Tree::delete_expr(&_expr);
						return nullptr;
					}

					idexpr->is_ptr = true;
					idexpr->ptr_oprtr_count = ptr_oprtr_count;
					_expr->expr_kind = ExpressionType::ID_EXPR;
					_expr->id_expr = idexpr;
					ptr_oprtr_count = 0;
				}
				expr_list.clear();
				is_expr_terminator_got = false;
				break;

			case INCR_OP : 
                Compiler::lex->put_back(tok);
				idexpr = prefix_incr_expr(terminator);
				if (idexpr == nullptr) {
					Log::error("error to parse increment expression");
					Tree::delete_expr(&_expr);
					return nullptr;
				}
				
				if (peek_assignment_operator()) {
					assgnexpr = assignment_expr(terminator, true);
					if (assgnexpr == nullptr) {
						Log::error("error to parse passignment expression");
						Tree::delete_expr(&_expr);
						return nullptr;
					}
					_expr->expr_kind = ExpressionType::ASSGN_EXPR;
					assgnexpr->id_expr = idexpr;
					_expr->assgn_expr = assgnexpr;
				}
				else {
					_expr->expr_kind = ExpressionType::ID_EXPR;
					_expr->id_expr = idexpr;
				}
				expr_list.clear();
				is_expr_terminator_got = false;
				break;

			case DECR_OP : 
                Compiler::lex->put_back(tok);
				idexpr = prefix_decr_expr(terminator);
				if (idexpr == nullptr) {
					Log::error("error to parse decrement expression");
					Tree::delete_expr(&_expr);
					
					return nullptr;
				}
				//peek for assignment operator
				if (peek_assignment_operator()) {
					assgnexpr = assignment_expr(terminator, true);
					if (assgnexpr == nullptr) {
						Log::error("error to parse assignment expression");
						Tree::delete_expr(&_expr);
						return nullptr;
					}
					_expr->expr_kind = ExpressionType::ASSGN_EXPR;
					assgnexpr->id_expr = idexpr;
					_expr->assgn_expr = assgnexpr;
				}
				else {
					_expr->expr_kind = ExpressionType::ID_EXPR;
					_expr->id_expr = idexpr;
				}
				expr_list.clear();
				is_expr_terminator_got = false;
				break;

			case BIT_AND : 
                Compiler::lex->put_back(tok);
				idexpr = address_of_expr(terminator);
				if (idexpr == nullptr) {
					Log::error("error to parse addressof expression");
					Tree::delete_expr(&_expr);
					return nullptr;
				}

				_expr->expr_kind = ExpressionType::ID_EXPR;
				_expr->id_expr = idexpr;
				expr_list.clear();
				is_expr_terminator_got = false;
				break;

			case KEY_SIZEOF : 
                Compiler::lex->put_back(tok);
				sizeofexpr = sizeof_expr(terminator);
				if (sizeofexpr == nullptr) {
					Log::error("error to parse sizeof expression");
					Tree::delete_expr(&_expr);
					return nullptr;
				}
				_expr->expr_kind = ExpressionType::SIZEOF_EXPR;
				_expr->sizeof_expr = sizeofexpr;
				break;
			case PARENTH_CLOSE :
			case SEMICOLON : 
                Tree::delete_expr(&_expr);
				expr_list.clear();
				is_expr_terminator_got = false;
				is_expr_terminator_consumed = true;
				consumed_terminator = tok;
				return nullptr;

			default : 
                Log::error_at(tok.loc, "invalid Token found in expression ", tok.string);
				consume_next();
				return nullptr;
		}

		return _expr;
	}
	
	void Parser::record_specifier() {

		RecordNode *rec = nullptr;
		Token tok;
		bool isglob = false;
		bool isextrn = false;
		
		if (record_head(&tok, &isglob, &isextrn)) {
			if (SymbolTable::search_record(Compiler::record_table, tok.string)) {
				Log::error_at(tok.loc, "record " + tok.string + " already exists");
				return;
			}

			SymbolTable::insert_record(&Compiler::record_table, tok.string);
			rec = Compiler::last_rec_node;
			rec->is_global = isglob;
			rec->is_extern = isextrn;
			rec->recordtok = tok;
			rec->recordname = tok.string;
			expect(CURLY_OPEN, true);
			record_member_definition(&rec);
			expect(CURLY_CLOSE, true);
            return;
		}
		
		Log::error("invalid record definition");
	}
	
	bool Parser::record_head(Token *tok, bool *isglob, bool *isextern) {
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
				*tok = Compiler::lex->get_next();
				return true;
			}
		}
		return false;
	}
	
	void Parser::record_member_definition(RecordNode **rec) {

		Token tok;
		std::vector<Token> types;
		TypeInfo *typeinf = nullptr;
		
		while ((tok = Compiler::lex->get_next()).number != END) {
			Compiler::lex->put_back(tok);
			if (peek_type_specifier() || peek_token(IDENTIFIER)) {
				get_type_specifier(types);
				typeinf = SymbolTable::get_type_info_mem();
				typeinf->type = NodeType::SIMPLE;
				typeinf->type_specifier.simple_type.clear();
				typeinf->type_specifier.simple_type.assign(types.begin(), types.end());
				if (types.size() == 1 && types[0].number == IDENTIFIER) {
					if (SymbolTable::search_record(Compiler::record_table, types[0].string)) {
						typeinf->type = NodeType::RECORD;
						typeinf->type_specifier.record_type = types[0];
						typeinf->type_specifier.simple_type.clear();
					}
					else
						Log::error_at(types[0].loc, "record '" + types[0].string + "' does not exists");
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
	
	void Parser::rec_id_list(RecordNode **rec, TypeInfo **typeinf) {
		
        Token tok;
		int ptr_seq = 0;
		Node *symt = (*rec)->symtab;
		std::list<Token> sublst;
		
		if (peek_token(IDENTIFIER)) {

			expect(IDENTIFIER, false);
			tok = Compiler::lex->get_next();

			if (SymbolTable::search_symbol((*rec)->symtab, tok.string)) {
				Log::error_at(tok.loc, "redeclaration of " + tok.string);
				return;
			}
			else {
				SymbolTable::insert_symbol(&symt, tok.string);
				assert(Compiler::last_symbol != nullptr);
				Compiler::last_symbol->type_info = *typeinf;
				Compiler::last_symbol->symbol = tok.string;
				Compiler::last_symbol->tok = tok;
			}
			if (peek_token(SQUARE_OPEN)) {
				sublst.clear();
				rec_subscript_member(sublst);
				assert(Compiler::last_symbol != nullptr);
				Compiler::last_symbol->is_array = true;
				Compiler::last_symbol->arr_dimension_list.assign(sublst.begin(), sublst.end());
				sublst.clear();
			}
			else if (peek_token(COMMA_OP)) {
				consume_next();
				rec_id_list(&(*rec), &(*typeinf));
			}
		}
		else if (peek_token(ARTHM_MUL)) {
			ptr_seq = get_pointer_operator_sequence();

			if (peek_token(PARENTH_OPEN))
				rec_func_pointer_member(&(*rec), &ptr_seq, &(*typeinf));
			else {
				expect(IDENTIFIER, false);
				tok = Compiler::lex->get_next();
				if (SymbolTable::search_symbol((*rec)->symtab, tok.string)) {
					Log::error_at(tok.loc, "redeclaration of " + tok.string);
					return;
				}
				else {
					SymbolTable::insert_symbol(&symt, tok.string);
					assert(Compiler::last_symbol != nullptr);
					Compiler::last_symbol->type_info = *typeinf;
					Compiler::last_symbol->symbol = tok.string;
					Compiler::last_symbol->tok = tok;
					Compiler::last_symbol->is_ptr = true;
					Compiler::last_symbol->ptr_oprtr_count = ptr_seq;
				}
				
				if (peek_token(SQUARE_OPEN)) {
					sublst.clear();
					rec_subscript_member(sublst);
					assert(Compiler::last_symbol != nullptr);
					Compiler::last_symbol->is_array = true;
					Compiler::last_symbol->arr_dimension_list.assign(sublst.begin(), sublst.end());
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
			tok = Compiler::lex->get_next();
			Log::error_at(tok.loc, "identifier expected in record member definition but found " + tok.string);
			return;
		}
	}
	
	void Parser::rec_subscript_member(std::list<Token> &sublst) {

		Token tok;
		expect(SQUARE_OPEN, true);
		if (peek_constant_expr()) {
			tok = Compiler::lex->get_next();
			sublst.push_back(tok);
		}
		else {
			tok = Compiler::lex->get_next();
			Log::error_at(tok.loc, "constant expression expected but found " + tok.string);
		}

		expect(SQUARE_CLOSE, true);
		if (peek_token(SQUARE_OPEN))
			rec_subscript_member(sublst);
	}
	
	void Parser::rec_func_pointer_member(RecordNode **rec, int *ptrseq, TypeInfo **typeinf) {

		Token tok;
		Node *symt = (*rec)->symtab;
		std::list<Token> sublst;
		
		expect(PARENTH_OPEN, true);
		expect(ARTHM_MUL, true);
		
		if (peek_token(IDENTIFIER)) {
			expect(IDENTIFIER, false);
			tok = Compiler::lex->get_next();
			if (SymbolTable::search_symbol((*rec)->symtab, tok.string)) {
				Log::error_at(tok.loc, "redeclaration of func pointer " + tok.string);
				return;
			}
			else {

				SymbolTable::insert_symbol(&symt, tok.string);
				assert(Compiler::last_symbol != nullptr);
				Compiler::last_symbol->type_info = *typeinf;
				Compiler::last_symbol->is_func_ptr = true;
				Compiler::last_symbol->symbol = tok.string;
				Compiler::last_symbol->tok = tok;
				Compiler::last_symbol->ret_ptr_count = *ptrseq;
				
				expect(PARENTH_CLOSE, true);
				expect(PARENTH_OPEN, true);
				
				if (peek_token(PARENTH_CLOSE))
					consume_next();
				else {
					rec_func_pointer_params(&(Compiler::last_symbol));
					expect(PARENTH_CLOSE, true);
				}
			}
		}
		else {
			tok = Compiler::lex->get_next();
			Log::error_at(tok.loc, "identifier expected in record func pointer member definition");
			return;
		}
	}
	
	void Parser::rec_func_pointer_params(SymbolInfo **stinf) {
		Token tok;
		std::vector<Token> types;
		int ptr_seq = 0;
		RecordTypeInfo *rectype = nullptr;
		
		if (*stinf == nullptr)
			return;
		
		rectype = SymbolTable::get_rec_type_info_mem();
		
		if (peek_token(KEY_CONST)) {
			consume_next();
			rectype->is_const = true;
		}
		
		if (peek_type_specifier()) {
			get_type_specifier(types);
			consume_n(types.size());
			rectype->type = NodeType::SIMPLE;
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
			tok = Compiler::lex->get_next();
			rectype->type = NodeType::RECORD;
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
			SymbolTable::delete_rec_type_info(&rectype);
			tok = Compiler::lex->get_next();
			Log::error_at(tok.loc, "type specifier expected in record func ptr member definition but found " + tok.string);
			return;
		}
	}

	void Parser::simple_declaration(Token scope, std::vector<Token> &types, bool is_record_type, Node **st) {

		Token tok;
		TypeInfo *stype = SymbolTable::get_type_info_mem();

		switch (scope.number) {
			case KEY_CONST : 
                stype->is_const = true;
				break;
			case KEY_EXTERN : 
                stype->is_extern = true;
				break;
			case KEY_STATIC : 
                stype->is_static = true;
				break;
			case KEY_GLOBAL : 
                stype->is_global = true;
				break;
			default: 
                break;
		}
		
		if (!is_record_type) {
			stype->type = NodeType::SIMPLE;
			stype->type_specifier.simple_type.assign(types.begin(), types.end());
			simple_declarator_list(&(*st), &stype);

			if (peek_token(PARENTH_OPEN))
				return;
			else
				expect(SEMICOLON, true);
		}
		else {
			if (types.size() > 0) {
				stype->type = NodeType::RECORD;
				stype->type_specifier.record_type = types[0];
				simple_declarator_list(&(*st), &stype);

				if (peek_token(PARENTH_OPEN))
					return;
				else
					expect(SEMICOLON, true);
			}
		}
		
	}
	
	void Parser::simple_declarator_list(Node **st, TypeInfo **stinf) {
	
    	Token tok;
		int ptr_seq = 0;
		
		if (*st == nullptr)
			return;
		if (*stinf == nullptr)
			return;
		
		if (peek_token(IDENTIFIER)) {
			Compiler::lex->reverse_tokens_queue();
			tok = Compiler::lex->get_next();
			if (SymbolTable::search_symbol((*st), tok.string)) {
				Log::error_at(tok.loc, "redeclaration/conflicting types of " + tok.string);
				return;
			}
			else {
				SymbolTable::insert_symbol(&(*st), tok.string);
				if (Compiler::last_symbol == nullptr)
					return;
				Compiler::last_symbol->symbol = tok.string;
				Compiler::last_symbol->tok = tok;
				Compiler::last_symbol->type_info = *stinf;
			}
			if (peek_token(SQUARE_OPEN)) {
				Compiler::last_symbol->is_array = true;
				subscript_declarator(&Compiler::last_symbol);
			}
			if (peek_token(COMMA_OP)) {
				consume_next();
				simple_declarator_list(&(*st), &(*stinf));
			}
			if (peek_token(ASSGN)) { // ?
			}
		}
		else if (peek_token(ARTHM_MUL)) {
			ptr_seq = get_pointer_operator_sequence();
			ptr_oprtr_count = ptr_seq;
			if (peek_token(IDENTIFIER)) {
				tok = Compiler::lex->get_next();
				if (SymbolTable::search_symbol((*st), tok.string)) {
					Log::error_at(tok.loc, "redeclaration/conflicting types of " + tok.string);
					return;
				}
				else {
					SymbolTable::insert_symbol(&(*st), tok.string);
					if (Compiler::last_symbol == nullptr)
						return;
					Compiler::last_symbol->symbol = tok.string;
					Compiler::last_symbol->tok = tok;
					Compiler::last_symbol->type_info = *stinf;
					Compiler::last_symbol->is_ptr = true;
					Compiler::last_symbol->ptr_oprtr_count = ptr_seq;
				}

				if (peek_token(SQUARE_OPEN)) {
					Compiler::last_symbol->is_array = true;
					subscript_declarator(&Compiler::last_symbol);
				}
				else if (peek_token(ASSGN)) {
					consume_next();
					subscript_initializer(Compiler::last_symbol->arr_init_list);
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
				tok = Compiler::lex->get_next();
				Log::error_at(tok.loc, "identifier expected in declaration");
				return;
			}
		}
		else {
			tok = Compiler::lex->get_next();
			Log::error_at(tok.loc, "identifier expected in declaration but found " + tok.string);
			tok = Compiler::lex->get_next();
			return;
		}
	}
	
	void Parser::subscript_declarator(SymbolInfo **stsinf) {
		Token tok;
		expect(SQUARE_OPEN, true);
		if (peek_constant_expr()) {
			tok = Compiler::lex->get_next();
			(*stsinf)->arr_dimension_list.push_back(tok);
		}
		else if (peek_token(SQUARE_CLOSE)) {
		}
		else {
			tok = Compiler::lex->get_next();
			Log::error_at(tok.loc, "constant expression expected but found " + tok.string);
		}

		expect(SQUARE_CLOSE, true);
		if (peek_token(SQUARE_OPEN))
			subscript_declarator(&(*stsinf));

		else if (peek_token(ASSGN)) {
			consume_next();
			subscript_initializer((*stsinf)->arr_init_list);
		}
		else
			return;
	}
	
	void Parser::subscript_initializer(std::vector<std::vector<Token>> &arrinit) {

		Token tok;
		std::vector<Token> ltrl;
		if (peek_token(LIT_STRING)) {
			tok = Compiler::lex->get_next();
			ltrl.push_back(tok);
			arrinit.push_back(ltrl);
			ltrl.clear();
		}
		else {
			expect(CURLY_OPEN, true);
			if (peek_literal_string()) {
				literal_list(ltrl);
				arrinit.push_back(ltrl);
				ltrl.clear();
			}
			else if (peek_token(CURLY_OPEN))
				subscript_initializer(arrinit);
			else {
				tok = Compiler::lex->get_next();
				Log::error_at(tok.loc, "literal expected in array initializer but found " + tok.string);
			}

			expect(CURLY_CLOSE, true);
			if (peek_token(COMMA_OP)) {
				consume_next();
				subscript_initializer(arrinit);
			}
		}
	}
	
	void Parser::literal_list(std::vector<Token> &ltrl) {
		Token tok;
		if (peek_literal_string()) {
			tok = Compiler::lex->get_next();
			ltrl.push_back(tok);
		}
		else {
			tok = Compiler::lex->get_next();
			Log::error_at(tok.loc, "literal expected in array initializer but found " + tok.string);
			return;
		}

		if (peek_token(COMMA_OP)) {
			consume_next();
			literal_list(ltrl);
		}
	}

	void Parser::func_head(FunctionInfo **stfinf, Token _funcname, Token scope, std::vector<Token> &types, bool is_record_type) {

		Token tok;
		*stfinf = SymbolTable::get_func_info_mem();
		if (*stfinf == nullptr)
			return;
		
		switch (scope.number) {
			case KEY_EXTERN : 
                (*stfinf)->is_extern = true;
				break;
			case KEY_GLOBAL : 
                (*stfinf)->is_global = true;
				break;
			default: 
                break;
		}
		
		if (!is_record_type) {
			(*stfinf)->return_type = SymbolTable::get_type_info_mem();
			(*stfinf)->return_type->type = NodeType::SIMPLE;
			(*stfinf)->return_type->type_specifier.simple_type.assign(types.begin(), types.end());
			
			tok = Compiler::lex->get_next();
			(*stfinf)->func_name = _funcname.string;
			(*stfinf)->tok = _funcname;
			
			//functions can only be defined globally
			// so while figuring out whether the declaration is the function or not
			// we might lost some tokens while returning it to the lexer
			//even with high priority
			//so we dont need to expect ( Token here
			//expect(PARENTH_OPEN, true); will cause error
			
			if (peek_token(PARENTH_CLOSE)) 
				consume_next();
			else {
				func_params((*stfinf)->param_list);
				expect(PARENTH_CLOSE, true);
			}
		}
		else {
			(*stfinf)->return_type = SymbolTable::get_type_info_mem();
			(*stfinf)->return_type->type = NodeType::RECORD;
			(*stfinf)->return_type->type_specifier.record_type = types[0];
			
			tok = Compiler::lex->get_next();
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
	
	void Parser::func_params(std::list<FuncParamInfo *> &fparams) {

		Token tok;
		std::vector<Token> types;
		int ptr_seq = 0;
		FuncParamInfo *funcparam = nullptr;
		funcparam = SymbolTable::get_func_param_info_mem();
		
		if (peek_type_specifier()) {
			get_type_specifier(types);
			consume_n(types.size());
			funcparam->type_info->type = NodeType::SIMPLE;
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
				tok = Compiler::lex->get_next();
				funcparam->symbol_info->symbol = tok.string;
				funcparam->symbol_info->tok = tok;
			}

			if (peek_token(COMMA_OP)) {
				consume_next();
				func_params(fparams);
			}
		}
		else if (peek_token(IDENTIFIER)) {
			tok = Compiler::lex->get_next();
			funcparam->type_info->type = NodeType::RECORD;
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
				tok = Compiler::lex->get_next();
				funcparam->symbol_info->symbol = tok.string;
				funcparam->symbol_info->tok = tok;
			}

			if (peek_token(COMMA_OP)) {
				consume_next();
				func_params(fparams);
			}
		}
		else {
			SymbolTable::delete_func_param_info(&funcparam);
			tok = Compiler::lex->get_next();
			Log::error_at(tok.loc, "type specifier expected in function declaration parameters but found " + tok.string);
			return;
		}
	}
	
	LabelStatement *Parser::labled_statement() {
		LabelStatement *labstmt = Tree::get_label_stmt_mem();
		Token tok;
		expect(IDENTIFIER, false);
		tok = Compiler::lex->get_next();
		labstmt->label = tok;
		expect(COLON_OP, true);
		return labstmt;
	}
	
	ExpressionStatement *Parser::expression_statement() {
		ExpressionStatement *expstmt = Tree::get_expr_stmt_mem();
		terminator_t terminator = {SEMICOLON};
		expstmt->expression = expression(terminator);
		return expstmt;
	}
	
	SelectStatement *Parser::selection_statement(Node **symtab) {
		Token tok;
		terminator_t terminator = {PARENTH_CLOSE};
		SelectStatement *selstmt = Tree::get_select_stmt_mem();
		
		expect(KEY_IF, false);
		tok = Compiler::lex->get_next();
		selstmt->iftok = tok;
		expect(PARENTH_OPEN, true);
		selstmt->condition = expression(terminator);
		expect(CURLY_OPEN, true);
		if (peek_token(CURLY_CLOSE))
			consume_next();
		else {
			selstmt->if_statement = statement(&(*symtab));
			expect(CURLY_CLOSE, true);
		}
		if (peek_token(KEY_ELSE)) {
			tok = Compiler::lex->get_next();
			selstmt->elsetok = tok;
			expect(CURLY_OPEN, true);
			if (peek_token(CURLY_CLOSE))
				consume_next();
			else {
				selstmt->else_statement = statement(&(*symtab));
				expect(CURLY_CLOSE, true);
			}
		}
		return selstmt;
	}
	
	IterationStatement *Parser::iteration_statement(Node **symtab) {
	
    	Token tok;
		terminator_t terminator = {PARENTH_CLOSE};
		IterationStatement *itstmt = Tree::get_iter_stmt_mem();
		
		if (peek_token(KEY_WHILE)) {

			expect(KEY_WHILE, false);
			itstmt->type = IterationType::WHILE;
			tok = Compiler::lex->get_next();
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
				expect(CURLY_OPEN, true);
				if (peek_token(CURLY_CLOSE))
					consume_next();
				else {
					itstmt->_while.statement = statement(&(*symtab));
					expect(CURLY_CLOSE, true);
				}
			}
		}
		else if (peek_token(KEY_DO)) {
			expect(KEY_DO, false);
			itstmt->type = IterationType::DOWHILE;
			tok = Compiler::lex->get_next();
			itstmt->_dowhile.dotok = tok;
			expect(CURLY_OPEN, true);

			if (peek_token(CURLY_CLOSE))
				consume_next();
			else {
				itstmt->_dowhile.statement = statement(&(*symtab));
				expect(CURLY_CLOSE, true);
			}

			expect(KEY_WHILE, false);
			tok = Compiler::lex->get_next();
			itstmt->_dowhile.whiletok = tok;
			expect(PARENTH_OPEN, true);
			itstmt->_dowhile.condition = expression(terminator);

			if (is_expr_terminator_consumed && consumed_terminator.number == PARENTH_CLOSE)
				expect(SEMICOLON, true);
			else {
				expect(PARENTH_CLOSE, true);
				expect(SEMICOLON, true);
			}
			
		}
		else if (peek_token(KEY_FOR)) {
			itstmt->type = IterationType::FOR;
			expect(KEY_FOR, false);
			tok = Compiler::lex->get_next();
			itstmt->_for.fortok = tok;
			expect(PARENTH_OPEN, true);
			terminator.clear();
			terminator.push_back(SEMICOLON);
			
			if (peek_token(SEMICOLON))
				consume_next();
			else if (peek_expr_token())
				itstmt->_for.init_expr = expression(terminator);
			else {
				tok = Compiler::lex->get_next();
				Log::error_at(tok.loc, "expression or ; expected in for()");
			}
			
			itstmt->_for.condition = expression(terminator);
			terminator.clear();
			terminator.push_back(PARENTH_CLOSE);
			
			if (peek_token(PARENTH_CLOSE)) {
				tok = Compiler::lex->get_next();
				is_expr_terminator_consumed = true;
				consumed_terminator = tok;
			}
			else
				itstmt->_for.update_expr = expression(terminator);
			
			if (is_expr_terminator_consumed && consumed_terminator.number == PARENTH_CLOSE) {
				if (peek_token(SEMICOLON))
					consume_next();
				else {
					expect(CURLY_OPEN, true);
					if (peek_token(CURLY_CLOSE))
						consume_next();
					else {
						itstmt->_for.statement = statement(&(*symtab));
						expect(CURLY_CLOSE, true);
					}
				}
			}
			else {
				expect(PARENTH_CLOSE, true);
				if (peek_token(SEMICOLON))
					consume_next();
				else {
					expect(CURLY_OPEN, true);
					if (peek_token(CURLY_CLOSE))
						consume_next();
					else {
						itstmt->_for.statement = statement(&(*symtab));
						expect(CURLY_CLOSE, true);
					}
				}
			}
			
		}
		return itstmt;
	}
	
	JumpStatement *Parser::jump_statement() {
	
    	Token tok;
		terminator_t terminator = {SEMICOLON};
		JumpStatement *jmpstmt = Tree::get_jump_stmt_mem();
		
		switch (get_peek_token()) {
			case KEY_BREAK : 
                jmpstmt->type = JumpType::BREAK;
				tok = Compiler::lex->get_next();
				jmpstmt->tok = tok;
				expect(SEMICOLON, true, ";", " in break statement");
				break;

			case KEY_CONTINUE : 
                jmpstmt->type = JumpType::CONTINUE;
				tok = Compiler::lex->get_next();
				jmpstmt->tok = tok;
				expect(SEMICOLON, true, ";", " in continue statement");
				break;

			case KEY_RETURN : 
                jmpstmt->type = JumpType::RETURN;
				tok = Compiler::lex->get_next();
				jmpstmt->tok = tok;

				if (peek_token(SEMICOLON))
					consume_next();
				else
					jmpstmt->expression = expression(terminator);
				break;

			case KEY_GOTO : 
                jmpstmt->type = JumpType::GOTO;
				tok = Compiler::lex->get_next();
				jmpstmt->tok = tok;
				expect(IDENTIFIER, false, "", "label in goto statement");
				tok = Compiler::lex->get_next();
				jmpstmt->goto_id = tok;
				expect(SEMICOLON, true, ";", " in goto statement");
				break;
			
			default: break;
		}
		return jmpstmt;
	}
	
	AsmStatement *Parser::asm_statement() {
		
        AsmStatement *asmhead = nullptr;
		Token tok;
		expect(KEY_ASM, true);
		expect(CURLY_OPEN, true);
		asm_statement_sequence(&asmhead);
		
        if (peek_token(CURLY_CLOSE))
			consume_next();
		else {
			tok = Compiler::lex->get_next();
			Log::error_at(tok.loc, ", or } expected before \"" + tok.string + "\" in asm statement ");
		}

		return asmhead;
	}
	
	void Parser::asm_statement_sequence(AsmStatement **asmhead) {

		Token tok;
		AsmStatement *asmstmt = Tree::get_asm_stmt_mem();
		
		expect(LIT_STRING, false);
		tok = Compiler::lex->get_next();
		asmstmt->asm_template = tok;
		
		if (peek_token(SQUARE_OPEN)) {
			consume_next();

			if (peek_token(COLON_OP)) 
				consume_next();
			else if (peek_token(LIT_STRING)) {
				asm_operand(asmstmt->output_operand);
				expect(COLON_OP, true);
			}
			else {
				tok = Compiler::lex->get_next();
				Log::error_at(tok.loc, "output Operand expected " + tok.string);
				return;
			}
			
			if (peek_token(SQUARE_CLOSE))
				consume_next();
			else if (peek_token(LIT_STRING)) {
				asm_operand(asmstmt->input_operand);
				expect(SQUARE_CLOSE, true);
			}
			else {
				tok = Compiler::lex->get_next();
				Log::error_at(tok.loc, "input Operand expected " + tok.string);
				return;
			}
			
			Tree::add_asm_statement(&(*asmhead), &asmstmt);
			if (peek_token(COMMA_OP)) {
				consume_next();
				asm_statement_sequence(&(*asmhead));
			}
		}
		else {
			Tree::add_asm_statement(&(*asmhead), &asmstmt);
			if (peek_token(COMMA_OP)) {
				consume_next();
				asm_statement_sequence(&(*asmhead));
			}
		}
	}
	
	void Parser::asm_operand(std::vector<AsmOperand *> &Operand) {

		Token tok;
		terminator_t terminator = {PARENTH_CLOSE};
		AsmOperand *asmoprd = Tree::get_asm_operand_mem();
		expect(LIT_STRING, false);
		tok = Compiler::lex->get_next();
		asmoprd->constraint = tok;
		expect(PARENTH_OPEN, true);

		if (peek_expr_token()) {
			asmoprd->expression = expression(terminator);
			if (is_expr_terminator_consumed && consumed_terminator.number == PARENTH_CLOSE)
				Operand.push_back(asmoprd);
			else {
				expect(PARENTH_CLOSE, true);
				Operand.push_back(asmoprd);
			}

			if (peek_token(COMMA_OP)) {
				consume_next();
				asm_operand(Operand);
			}
		}
		else if (peek_token(PARENTH_CLOSE)) {
			consume_next();
			Operand.push_back(asmoprd);
			return;
		}
		else {
			tok = Compiler::lex->get_next();
			Log::error_at(tok.loc, " expression expected " + tok.string);
			return;
		}
	}
	
	Statement *Parser::statement(Node **symtab) {

		Token tok;
		std::vector<Token> types;
		Token scope = nulltoken;
		Statement *stmthead = nullptr;
		Statement *statement = nullptr;
		
		while ((tok = Compiler::lex->get_next()).number != END) {
		
			if (type_specifier(tok.number)) {

				Compiler::lex->put_back(tok);
				get_type_specifier(types);
				consume_n(types.size());
				simple_declaration(scope, types, false, &(*symtab));
				types.clear();
				
                if (peek_token(END))
					return stmthead;
				continue;
			}
			else if (tok.number == IDENTIFIER) {
				if (peek_token(IDENTIFIER)) {
					types.push_back(tok);
					simple_declaration(scope, types, true, &(*symtab));
					types.clear();

					if (peek_token(END))
						return stmthead;
				}
				else if (peek_token(COLON_OP)) {
					Compiler::lex->put_back(tok);
					statement = Tree::get_stmt_mem();
					statement->type = StatementType::LABEL;
					statement->labled_statement = labled_statement();
					Tree::add_statement(&stmthead, &statement);

					if (peek_token(END))
						return stmthead;
				}
				else {
					Compiler::lex->put_back(tok);
					statement = Tree::get_stmt_mem();
					statement->type = StatementType::EXPR;
					statement->expression_statement = expression_statement();
					Tree::add_statement(&stmthead, &statement);

					if (peek_token(END))
						return stmthead;
				}
			}
			else if (expression_token(tok.number)) {
				Compiler::lex->put_back(tok);
				statement = Tree::get_stmt_mem();
				statement->type = StatementType::EXPR;
				statement->expression_statement = expression_statement();
				Tree::add_statement(&stmthead, &statement);

				if (peek_token(END))
					return stmthead;
			}
			else if (tok.number == KEY_IF) {
				Compiler::lex->put_back(tok);
				statement = Tree::get_stmt_mem();
				statement->type = StatementType::SELECT;
				statement->selection_statement = selection_statement(&(*symtab));
				Tree::add_statement(&stmthead, &statement);

				if (peek_token(END))
					return stmthead;
			}
			else if (tok.number == KEY_WHILE || 
                     tok.number == KEY_DO    || 
                     tok.number == KEY_FOR) {

				Compiler::lex->put_back(tok);
				statement = Tree::get_stmt_mem();
				statement->type = StatementType::ITER;
				statement->iteration_statement = iteration_statement(&(*symtab));
				Tree::add_statement(&stmthead, &statement);

				if (peek_token(END))
					return stmthead;
			}
			else if (tok.number == KEY_BREAK    || 
                     tok.number == KEY_CONTINUE ||
                     tok.number == KEY_RETURN   ||
                     tok.number == KEY_GOTO) {

				Compiler::lex->put_back(tok);
				statement = Tree::get_stmt_mem();
				statement->type = StatementType::JUMP;
				statement->jump_statement = jump_statement();
				Tree::add_statement(&stmthead, &statement);
				
                if (peek_token(END))
					return stmthead;
				
			}
			else if (tok.number == KEY_ASM) {
				Compiler::lex->put_back(tok);
				statement = Tree::get_stmt_mem();
				statement->type = StatementType::ASM;
				statement->asm_statement = asm_statement();
				Tree::add_statement(&stmthead, &statement);
				if (peek_token(END))
					return stmthead;
			}
			else if (tok.number == CURLY_CLOSE || tok.number == PARENTH_CLOSE) {
				Compiler::lex->put_back(tok);
				return stmthead;
			}
			else if (tok.number == SEMICOLON)
				continue;
			else {
				Log::error_at(tok.loc, "invalid Token in statement " + tok.string);
				return nullptr;
			}
		}
		return stmthead;
	}
	
	void Parser::get_func_info(FunctionInfo **func_info, Token tok, NodeType type, std::vector<Token> &types, bool is_extern, bool is_glob) {
		
		if (*func_info == nullptr)
			*func_info = SymbolTable::get_func_info_mem();
		
		(*func_info)->func_name = tok.string;
		(*func_info)->tok = tok;
		(*func_info)->return_type = SymbolTable::get_type_info_mem();
		(*func_info)->return_type->type = type;
		
		if (type == NodeType::SIMPLE)
			(*func_info)->return_type->type_specifier.simple_type.assign(types.begin(), types.end());
		else if (type == NodeType::RECORD)
			(*func_info)->return_type->type_specifier.record_type = types[0];
		
		(*func_info)->is_extern = is_extern;
		(*func_info)->is_global = is_glob;
	}
	
	TreeNode *Parser::parse() {
		
		Token tok[5];
		std::vector<Token> types;
		std::map<std::string, FunctionInfo *>::iterator funcit;
		terminator_t terminator = {SEMICOLON};
		Statement *_stmt = nullptr;
		Node *symtab = nullptr;
		FunctionInfo *funcinfo = nullptr;
		TreeNode *tree_head = nullptr;
		TreeNode *_tree = nullptr;
		

		while ((tok[0] = Compiler::lex->get_next()).number != END) {
			if (tok[0].number == KEY_GLOBAL) {
				tok[1] = Compiler::lex->get_next();
				
				if (tok[1].number == END)
					return tree_head;
				
				if (tok[1].number == KEY_RECORD) {
					Compiler::lex->put_back(tok[0]);
					Compiler::lex->put_back(tok[1]);
					record_specifier();
				}
				else if (type_specifier(tok[1].number)) {
					Compiler::lex->put_back(tok[1]);
					types.clear();
					get_type_specifier(types);
					consume_n(types.size());

					tok[2] = Compiler::lex->get_next();
					if (tok[2].number == END)
						return tree_head;
					
					if (tok[2].number == IDENTIFIER) {
						tok[3] = Compiler::lex->get_next();
						
						if (tok[3].number == END)
							return tree_head;
						
						if (tok[3].number == PARENTH_OPEN) {
							Compiler::lex->put_back(tok[3]);
							
							symtab = SymbolTable::get_node_mem();
							funcinfo = SymbolTable::get_func_info_mem();
							func_head(&funcinfo, tok[2], tok[0], types, false);
							funcit = Compiler::func_table->find(tok[2].string);
							
							if (funcit == Compiler::func_table->end()) {
								Compiler::func_table->insert(std::pair<std::string, FunctionInfo *>(tok[2].string, funcinfo));
								expect(CURLY_OPEN, true);
								_tree = Tree::get_tree_node_mem();
								_tree->symtab = symtab;
								get_func_info(&funcinfo, tok[2], NodeType::SIMPLE, types, false, true);
								_tree->symtab->func_info = funcinfo;
								_stmt = statement(&symtab);
								_tree->statement = _stmt;
								_tree->symtab = symtab;
								Tree::add_tree_node(&tree_head, &_tree);
								expect(CURLY_CLOSE, true);
							}
							else {
								Log::error_at(tok[2].loc, "redeclaration of function " + tok[2].string);
								SymbolTable::delete_func_info(&funcinfo);
								return tree_head;
							}
							types.clear();
						}
						else {
							Compiler::lex->put_back(tok[2]);
							Compiler::lex->put_back(tok[3]);
							simple_declaration(tok[0], types, false, &Compiler::symtab);
							types.clear();
							ptr_oprtr_count = 0;
						}
					}
					else if (tok[2].number == ARTHM_MUL) {
						Compiler::lex->put_back(tok[2]);
						simple_declaration(tok[0], types, false, &Compiler::symtab);
						if (peek_token(PARENTH_OPEN)) {
							SymbolTable::remove_symbol(&Compiler::symtab, funcname.string);
							symtab = SymbolTable::get_node_mem();
							funcinfo = SymbolTable::get_func_info_mem();
							func_head(&funcinfo, funcname, tok[0], types, false);
							funcinfo->ptr_oprtr_count = ptr_oprtr_count;
							symtab->func_info = funcinfo;
							
							funcit = Compiler::func_table->find(funcname.string);
							if (funcit == Compiler::func_table->end()) {
								Compiler::func_table->insert(std::pair<std::string, FunctionInfo *>(funcname.string, funcinfo));
								expect(CURLY_OPEN, true);
								_tree = Tree::get_tree_node_mem();
								_tree->symtab = symtab;
								get_func_info(&funcinfo, funcname, NodeType::SIMPLE, types, false, true);
								_tree->symtab->func_info = funcinfo;
								_stmt = statement(&symtab);
								_tree->statement = _stmt;
								_tree->symtab = symtab;
								Tree::add_tree_node(&tree_head, &_tree);
								expect(CURLY_CLOSE, true);
							}
							else {
								Log::error_at(funcname.loc, "redeclaration of function " + funcname.string);
								SymbolTable::delete_func_info(&funcinfo);
								return tree_head;
							}
						}
						ptr_oprtr_count = 0;
						funcname = nulltoken;
						types.clear();
					}
				}
				else if (tok[1].number == IDENTIFIER) {

					types.push_back(tok[1]);
					tok[2] = Compiler::lex->get_next();
					
					if (tok[2].number == END)
						return tree_head;
					
					if (tok[2].number == IDENTIFIER) {
						tok[3] = Compiler::lex->get_next();
						
						if (tok[3].number == END)
							return tree_head;
						
						if (tok[3].number == PARENTH_OPEN) {
							Compiler::lex->put_back(tok[3]);
							
							symtab = SymbolTable::get_node_mem();
							funcinfo = SymbolTable::get_func_info_mem();
							func_head(&funcinfo, tok[2], tok[0], types, true);
							funcit = Compiler::func_table->find(tok[2].string);
							if (funcit == Compiler::func_table->end()) {
								Compiler::func_table->insert(std::pair<std::string, FunctionInfo *>(tok[2].string, funcinfo));
								expect(CURLY_OPEN, true);
								_tree = Tree::get_tree_node_mem();
								_tree->symtab = symtab;
								get_func_info(&funcinfo, tok[2], NodeType::RECORD, types, false, true);
								_tree->symtab->func_info = funcinfo;
								_stmt = statement(&symtab);
								_tree->statement = _stmt;
								_tree->symtab = symtab;
								Tree::add_tree_node(&tree_head, &_tree);
								expect(CURLY_CLOSE, true);
							}
							else {
								Log::error_at(tok[2].loc, "redeclaration of function " + tok[2].string);
								SymbolTable::delete_func_info(&funcinfo);
								
								return tree_head;
							}
							types.clear();
						}
						else {
							Compiler::lex->put_back(tok[2]);
							Compiler::lex->put_back(tok[3]);
							simple_declaration(tok[0], types, true, &Compiler::symtab);
							types.clear();
							ptr_oprtr_count = 0;
						}
						
					}
					else if (tok[2].number == ARTHM_MUL) {
						Compiler::lex->put_back(tok[2]);
						simple_declaration(tok[0], types, false, &Compiler::symtab);
						
						if (peek_token(PARENTH_OPEN)) {
							SymbolTable::remove_symbol(&Compiler::symtab, funcname.string);
							
							symtab = SymbolTable::get_node_mem();
							funcinfo = SymbolTable::get_func_info_mem();
							func_head(&funcinfo, funcname, tok[0], types, true);
							funcinfo->ptr_oprtr_count = ptr_oprtr_count;
							symtab->func_info = funcinfo;
							
							funcit = Compiler::func_table->find(funcname.string);
							if (funcit == Compiler::func_table->end()) {
								Compiler::func_table->insert(std::pair<std::string, FunctionInfo *>(funcname.string, funcinfo));
								expect(CURLY_OPEN, true);
								_tree = Tree::get_tree_node_mem();
								_tree->symtab = symtab;
								get_func_info(&funcinfo, funcname, NodeType::RECORD, types, false, true);
								_tree->symtab->func_info = funcinfo;
								_stmt = statement(&symtab);
								_tree->statement = _stmt;
								_tree->symtab = symtab;
								Tree::add_tree_node(&tree_head, &_tree);
								expect(CURLY_CLOSE, true);
							}
							else {
								Log::error_at(funcname.loc, "redeclaration of function " + funcname.string);
								SymbolTable::delete_func_info(&funcinfo);
								
								return tree_head;
							}
						}
						ptr_oprtr_count = 0;
						funcname = nulltoken;
						types.clear();
					}
				}
			}
			else if (tok[0].number == KEY_EXTERN) {
				tok[1] = Compiler::lex->get_next();
				
				if (tok[1].number == END)
					return tree_head;
				
				if (tok[1].number == KEY_RECORD) {
					Compiler::lex->put_back(tok[1]);
					Compiler::lex->put_back(tok[0]);
					record_specifier();
				}
				else if (type_specifier(tok[1].number)) {

					Compiler::lex->put_back(tok[1]);
					types.clear();
					get_type_specifier(types);
					consume_n(types.size());

					tok[2] = Compiler::lex->get_next();
					if (tok[2].number == END)
						return tree_head;
					
					if (tok[2].number == IDENTIFIER) {
						tok[3] = Compiler::lex->get_next();
						if (tok[3].number == END)
							return tree_head;

						if (tok[3].number == PARENTH_OPEN) {
							Compiler::lex->put_back(tok[3]);
							funcinfo = SymbolTable::get_func_info_mem();
							func_head(&funcinfo, tok[2], tok[0], types, false);
							funcit = Compiler::func_table->find(tok[2].string);
							if (funcit == Compiler::func_table->end()) {
								expect(SEMICOLON, true);
								Compiler::func_table->insert(std::pair<std::string, FunctionInfo *>(tok[2].string, funcinfo));
								get_func_info(&funcinfo, tok[2], NodeType::SIMPLE, types, true, false);
								_tree = Tree::get_tree_node_mem();
								symtab = SymbolTable::get_node_mem();
								_tree->symtab = symtab;
								_tree->symtab->func_info = funcinfo;
								Tree::add_tree_node(&tree_head, &_tree);
							}
							else {
								Log::error_at(tok[2].loc, "redeclaration of function " + tok[2].string);
								SymbolTable::delete_func_info(&funcinfo);
								return tree_head;
							}
							types.clear();
						}
						else {
							Compiler::lex->put_back(tok[2]);
							Compiler::lex->put_back(tok[3]);
							simple_declaration(tok[0], types, false, &Compiler::symtab);
							types.clear();
							ptr_oprtr_count = 0;
						}
					}
					else if (tok[2].number == ARTHM_MUL) {

						Compiler::lex->put_back(tok[2]);
						simple_declaration(tok[0], types, false, &Compiler::symtab);
						
						if (peek_token(PARENTH_OPEN)) {
							SymbolTable::remove_symbol(&Compiler::symtab, funcname.string);
							
							funcinfo = SymbolTable::get_func_info_mem();
							func_head(&funcinfo, funcname, tok[0], types, false);
							funcinfo->ptr_oprtr_count = ptr_oprtr_count;
							
							funcit = Compiler::func_table->find(funcname.string);
							if (funcit == Compiler::func_table->end()) {
								Compiler::func_table->insert(std::pair<std::string, FunctionInfo *>(funcname.string, funcinfo));
								expect(SEMICOLON, true);
								get_func_info(&funcinfo, funcname, NodeType::SIMPLE, types, true, false);
								_tree = Tree::get_tree_node_mem();
								symtab = SymbolTable::get_node_mem();
								_tree->symtab = symtab;
								_tree->symtab->func_info = funcinfo;
								Tree::add_tree_node(&tree_head, &_tree);
							}
							else {
								Log::error_at(funcname.loc, "redeclaration of function " + funcname.string);
								SymbolTable::delete_func_info(&funcinfo);
								
								return tree_head;
							}
						}
						ptr_oprtr_count = 0;
						funcname = nulltoken;
						types.clear();
					}
				}
				else if (tok[1].number == IDENTIFIER) {
					types.push_back(tok[1]);

					tok[2] = Compiler::lex->get_next();
					if (tok[2].number == END)
						return tree_head;
					
					if (tok[2].number == IDENTIFIER) {
						tok[3] = Compiler::lex->get_next();
						
						if (tok[3].number == END)
							return tree_head;
						
						if (tok[3].number == PARENTH_OPEN) {
							Compiler::lex->put_back(tok[3]);
							funcinfo = SymbolTable::get_func_info_mem();
							func_head(&funcinfo, tok[2], tok[0], types, true);
							funcit = Compiler::func_table->find(tok[2].string);
							if (funcit == Compiler::func_table->end()) {
								expect(SEMICOLON, true);
								Compiler::func_table->insert(std::pair<std::string, FunctionInfo *>(tok[2].string, funcinfo));
								get_func_info(&funcinfo, tok[2], NodeType::RECORD, types, true, false);
								_tree = Tree::get_tree_node_mem();
								symtab = SymbolTable::get_node_mem();
								_tree->symtab = symtab;
								_tree->symtab->func_info = funcinfo;
								Tree::add_tree_node(&tree_head, &_tree);
							}
							else {
								Log::error_at(tok[2].loc, "redeclaration of function " + tok[2].string);
								SymbolTable::delete_func_info(&funcinfo);
								
								return tree_head;
							}
							types.clear();
							ptr_oprtr_count = 0;
							funcname = nulltoken;
						}
						else {
							Compiler::lex->put_back(tok[2]);
							Compiler::lex->put_back(tok[3]);
							simple_declaration(tok[0], types, true, &Compiler::symtab);
							types.clear();
							ptr_oprtr_count = 0;
							funcname = nulltoken;
						}
					}
					else if (tok[2].number == ARTHM_MUL) {
						Compiler::lex->put_back(tok[2]);

						simple_declaration(tok[0], types, true, &Compiler::symtab);
						if (peek_token(PARENTH_OPEN)) {
							SymbolTable::remove_symbol(&Compiler::symtab, funcname.string);
							
							funcinfo = SymbolTable::get_func_info_mem();
							func_head(&funcinfo, funcname, tok[0], types, true);
							funcinfo->ptr_oprtr_count = ptr_oprtr_count;
							
							funcit = Compiler::func_table->find(funcname.string);
							if (funcit == Compiler::func_table->end()) {
								Compiler::func_table->insert(std::pair<std::string, FunctionInfo *>(funcname.string, funcinfo));
								expect(SEMICOLON, true);
								get_func_info(&funcinfo, funcname, NodeType::RECORD, types, true, false);
								_tree = Tree::get_tree_node_mem();
								symtab = SymbolTable::get_node_mem();
								_tree->symtab = symtab;
								_tree->symtab->func_info = funcinfo;
								Tree::add_tree_node(&tree_head, &_tree);
							}
							else {
								Log::error_at(funcname.loc, "redeclaration of function " + funcname.string);
								SymbolTable::delete_func_info(&funcinfo);
								
								return tree_head;
							}
						}
						ptr_oprtr_count = 0;
						funcname = nulltoken;
						types.clear();
					}
				}
			}
			else if (type_specifier(tok[0].number)) {

				Compiler::lex->put_back(tok[0]);
				types.clear();
				get_type_specifier(types);
				consume_n(types.size());
				
				tok[1] = Compiler::lex->get_next();
				if (tok[1].number == END)
					return tree_head;
				
				if (tok[1].number == IDENTIFIER) {
					tok[2] = Compiler::lex->get_next();
					
					if (tok[2].number == END)
						return tree_head;
					
					if (tok[2].number == PARENTH_OPEN) {
						Compiler::lex->put_back(tok[2]);
						
						symtab = SymbolTable::get_node_mem();
						funcinfo = SymbolTable::get_func_info_mem();
						func_head(&funcinfo, tok[1], tok[0], types, false);
						funcit = Compiler::func_table->find(tok[1].string);

						if (funcit == Compiler::func_table->end()) {
							Compiler::func_table->insert(std::pair<std::string, FunctionInfo *>(tok[1].string, funcinfo));
							expect(CURLY_OPEN, true);
							_tree = Tree::get_tree_node_mem();
							_tree->symtab = symtab;
							get_func_info(&funcinfo, tok[1], NodeType::SIMPLE, types, false, false);
							_tree->symtab->func_info = funcinfo;
							_stmt = statement(&symtab);
							_tree->statement = _stmt;
							_tree->symtab = symtab;
							Tree::add_tree_node(&tree_head, &_tree);
							expect(CURLY_CLOSE, true);
						}
						else {
							Log::error_at(tok[1].loc, "redeclaration of function " + tok[1].string);
							
							return tree_head;
						}
						types.clear();
						ptr_oprtr_count = 0;
						funcname = nulltoken;
						
					}
					else {
						Compiler::lex->put_back(tok[1]);
						Compiler::lex->put_back(tok[2]);
						simple_declaration(tok[0], types, false, &Compiler::symtab);
						types.clear();
						ptr_oprtr_count = 0;
						funcname = nulltoken;
					}
				}
				else if (tok[1].number == ARTHM_MUL) {
					Compiler::lex->put_back(tok[1]);
					simple_declaration(tok[0], types, false, &Compiler::symtab);
					
					if (peek_token(PARENTH_OPEN) && funcname.number != NONE) {
						SymbolTable::remove_symbol(&Compiler::symtab, funcname.string);
						
						symtab = SymbolTable::get_node_mem();
						funcinfo = SymbolTable::get_func_info_mem();
						func_head(&funcinfo, funcname, tok[0], types, false);
						funcinfo->ptr_oprtr_count = ptr_oprtr_count;
						symtab->func_info = funcinfo;
						
						funcit = Compiler::func_table->find(funcname.string);
						if (funcit == Compiler::func_table->end()) {
							Compiler::func_table->insert(std::pair<std::string, FunctionInfo *>(funcname.string, funcinfo));
							expect(CURLY_OPEN, true);
							_tree = Tree::get_tree_node_mem();
							_tree->symtab = symtab;
							get_func_info(&funcinfo, funcname, NodeType::SIMPLE, types, false, false);
							_tree->symtab->func_info = funcinfo;
							_stmt = statement(&symtab);
							_tree->statement = _stmt;
							_tree->symtab = symtab;
							Tree::add_tree_node(&tree_head, &_tree);
							expect(CURLY_CLOSE, true);
						}
						else {
							Log::error_at(funcname.loc, "redeclaration of function " + funcname.string);
							SymbolTable::delete_func_info(&funcinfo);
							return tree_head;
						}
					}
					ptr_oprtr_count = 0;
					funcname = nulltoken;
					types.clear();
				}
			}
			else if (tok[0].number == IDENTIFIER) {
				types.clear();
				types.push_back(tok[0]);

				tok[1] = Compiler::lex->get_next();
				if (tok[1].number == END)
					return tree_head;
				
				if (tok[1].number == IDENTIFIER) {

					tok[2] = Compiler::lex->get_next();
					if (tok[2].number == END)
						return tree_head;
					
					if (tok[2].number == PARENTH_OPEN) {
						Compiler::lex->put_back(tok[2]);
						
						symtab = SymbolTable::get_node_mem();
						funcinfo = SymbolTable::get_func_info_mem();
						func_head(&funcinfo, tok[1], tok[0], types, true);
						funcit = Compiler::func_table->find(tok[2].string);
						if (funcit == Compiler::func_table->end()) {
							Compiler::func_table->insert(std::pair<std::string, FunctionInfo *>(tok[1].string, funcinfo));
							expect(CURLY_OPEN, true);
							_tree = Tree::get_tree_node_mem();
							_tree->symtab = symtab;
							get_func_info(&funcinfo, tok[1], NodeType::RECORD, types, false, false);
							_tree->symtab->func_info = funcinfo;
							_stmt = statement(&symtab);
							_tree->statement = _stmt;
							_tree->symtab = symtab;
							Tree::add_tree_node(&tree_head, &_tree);
							expect(CURLY_CLOSE, true);
						}
						else {
							Log::error_at(tok[1].loc, "redeclaration of function " + tok[1].string);
							SymbolTable::delete_func_info(&funcinfo);
							
							return tree_head;
						}
						types.clear();
						
					}
					else {
						Compiler::lex->put_back(tok[1]);
						Compiler::lex->put_back(tok[2]);
						simple_declaration(tok[0], types, true, &Compiler::symtab);
						types.clear();
						ptr_oprtr_count = 0;
					}
				}
				else if (tok[1].number == ARTHM_MUL) {
					if (!SymbolTable::search_record(Compiler::record_table, tok[0].string)) {
						Compiler::lex->put_back(tok[1]);
						Compiler::lex->put_back(tok[0]);
						_tree = Tree::get_tree_node_mem();
						_tree->statement = Tree::get_stmt_mem();
						_tree->statement->type = StatementType::EXPR;
						_tree->statement->expression_statement = Tree::get_expr_stmt_mem();
						_tree->statement->expression_statement->expression = expression(terminator);
						if (peek_token(SEMICOLON))
							consume_next();
						else if (is_expr_terminator_consumed) {
						}
						else if (peek_token(END))
							return tree_head;
						else {
							if (!is_expr_terminator_consumed)
								expect(SEMICOLON, true);
							else
								return tree_head;
						}
						Tree::add_tree_node(&tree_head, &_tree);
					}
					else {
						Compiler::lex->put_back(tok[1]);
						simple_declaration(tok[0], types, true, &Compiler::symtab);
						
						if (peek_token(PARENTH_OPEN)) {
							SymbolTable::remove_symbol(&Compiler::symtab, funcname.string);
							symtab = SymbolTable::get_node_mem();
							funcinfo = SymbolTable::get_func_info_mem();
							func_head(&funcinfo, funcname, tok[0], types, true);
							funcinfo->ptr_oprtr_count = ptr_oprtr_count;
							symtab->func_info = funcinfo;
							
							funcit = Compiler::func_table->find(funcname.string);
							if (funcit == Compiler::func_table->end()) {
								Compiler::func_table->insert(std::pair<std::string, FunctionInfo *>(funcname.string, funcinfo));
								expect(CURLY_OPEN, true);
								_tree = Tree::get_tree_node_mem();
								_tree->symtab = symtab;
								get_func_info(&funcinfo, funcname, NodeType::SIMPLE, types, false, false);
								_tree->symtab->func_info = funcinfo;
								_stmt = statement(&symtab);
								_tree->statement = _stmt;
								_tree->symtab = symtab;
								Tree::add_tree_node(&tree_head, &_tree);
								expect(CURLY_CLOSE, true);
							}
							else {
								Log::error_at(funcname.loc, "redeclaration of function " + funcname.string);
								SymbolTable::delete_func_info(&funcinfo);
								return tree_head;
							}
						}
					}
					ptr_oprtr_count = 0;
					funcname = nulltoken;
					types.clear();
				}
				else if (assignment_operator(tok[1].number) || tok[1].number == SQUARE_OPEN) {
					Compiler::lex->put_back(tok[1]);
					Compiler::lex->put_back(tok[0]);
					_tree = Tree::get_tree_node_mem();
					SymbolTable::delete_node(&(_tree->symtab));
					_tree->statement = Tree::get_stmt_mem();
					_tree->statement->type = StatementType::EXPR;
					_tree->statement->expression_statement = Tree::get_expr_stmt_mem();
					_tree->statement->expression_statement->expression = expression(terminator);
					if (peek_token(SEMICOLON))
						consume_next();
					else if (is_expr_terminator_consumed && consumed_terminator.number == SEMICOLON) {
					}
					else {
						expect(SEMICOLON, true);
					}
					Tree::add_tree_node(&tree_head, &_tree);
				}
				else if (binary_operator(tok[1].number) || tok[1].number == INCR_OP || tok[1].number == DECR_OP) {
					Compiler::lex->put_back(tok[1]);
					Compiler::lex->put_back(tok[0]);
					_tree = Tree::get_tree_node_mem();
					_tree->statement = Tree::get_stmt_mem();
					_tree->statement->type = StatementType::EXPR;
					_tree->statement->expression_statement = Tree::get_expr_stmt_mem();
					_tree->statement->expression_statement->expression = expression(terminator);

					if (peek_token(SEMICOLON))
						consume_next();
					else if (is_expr_terminator_consumed) {
					}
					else if (peek_token(END))
						return tree_head;
					else {
						if (!is_expr_terminator_consumed)
							expect(SEMICOLON, true);
						else
							return tree_head;
					}
					Tree::add_tree_node(&tree_head, &_tree);
				}
				else if (tok[1].number == PARENTH_OPEN) {
					Compiler::lex->put_back(tok[1]);
					Compiler::lex->put_back(tok[0]);
					_tree = Tree::get_tree_node_mem();
					_tree->statement = Tree::get_stmt_mem();
					_tree->statement->type = StatementType::EXPR;
					_tree->statement->expression_statement = Tree::get_expr_stmt_mem();
					_tree->statement->expression_statement->expression = expression(terminator);
					Tree::add_tree_node(&tree_head, &_tree);
				}
				else {
					Log::error_at(tok[1].loc, "invalid Token found while parsing '" + tok[1].string + "'");
					return tree_head;
				}
			}
			else if (tok[0].number == KEY_RECORD) {
				Compiler::lex->put_back(tok[0]);
				record_specifier();
			}
			else if (expression_token(tok[0].number)) {
				Compiler::lex->put_back(tok[0]);
				_tree = Tree::get_tree_node_mem();
				SymbolTable::delete_node(&(_tree->symtab));
				_tree->statement = Tree::get_stmt_mem();
				_tree->statement->type = StatementType::EXPR;
				_tree->statement->expression_statement = Tree::get_expr_stmt_mem();
				_tree->statement->expression_statement->expression = expression(terminator);
				if (peek_token(SEMICOLON))
					consume_next();
				else if (is_expr_terminator_consumed) {
				}
				else if (peek_token(END))
					return tree_head;
				else {
					if (!is_expr_terminator_consumed)
						expect(SEMICOLON, true);
					else
						return tree_head;
				}
				Tree::add_tree_node(&tree_head, &_tree);
			}
			else if (tok[0].number == KEY_ASM) {
				Compiler::lex->put_back(tok[0]);
				_tree = Tree::get_tree_node_mem();
				SymbolTable::delete_node(&(_tree->symtab));
				_tree->statement = Tree::get_stmt_mem();
				_tree->statement->type = StatementType::ASM;
				_tree->statement->asm_statement = asm_statement();
				Tree::add_tree_node(&tree_head, &_tree);
			}
			else if (tok[0].number == SEMICOLON) {
				consume_next();
			}
			else {
				Log::error_at(tok[0].loc, "invalid Token found while parsing '" + tok[0].string + "'");
				return tree_head;
			}
		}
		return tree_head;
	}
}
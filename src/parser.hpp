/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


// data/operations used by class parser.

#pragma once

#include <vector>
#include <stack>
#include <map>
#include "token.hpp"
#include "lex.hpp"
#include "tree.hpp"

namespace xlang {
	
	typedef std::vector<token_t> terminator_t;
	
	class parser {
		public:
		
		parser();
		
		tree_node *parse();
		
		friend std::ostream &operator<<(std::ostream &, const std::vector<token> &);
		
		friend std::ostream &operator<<(std::ostream &, const std::list<token> &);
		
		private:
		
		bool is_expr_terminator_got = false;
		bool is_expr_terminator_consumed = false;
		int ptr_oprtr_count = 0;
		
		token funcname;
		token consumed_terminator;
		token nulltoken;
		
		std::map<token_t, std::string> token_lexeme_table;
		std::stack<token> parenth_stack;
		std::vector<token> expr_list;
		
		std::string s_quotestring(std::string str) {
			return "'" + str + "'";
		}
		
		std::string d_quotestring(std::string str) {
			return "\"" + str + "\"";
		}
		
		bool peek_token(token_t);
		
		bool peek_token(const char *format...);
		
		bool peek_token(std::vector<token_t> &);
		
		bool peek_nth_token(token_t, int);
		
		token_t get_peek_token();
		
		token_t get_nth_token(int);
		
		bool expr_literal(token_t);
		
		bool peek_expr_literal();
		
		bool expect(token_t);
		
		bool expect(token_t, bool);
		
		bool expect(token_t, bool, std::string);
		
		bool expect(token_t, bool, std::string, std::string);
		
		bool expect(const char *format...);
		
		void consume_next();
		
		void consume_n(int);
		
		void consume_till(terminator_t &);
		
		bool expect_binary_operator();
		
		bool expect_literal();
		
		bool check_parenth();
		
		bool matches_terminator(terminator_t &, token_t);
		
		std::string get_terminator(terminator_t &);
		
		bool unary_operator(token_t);
		
		bool binary_operator(token_t);
		
		bool arithmetic_operator(token_t);
		
		bool logical_operator(token_t);
		
		bool comparison_operator(token_t);
		
		bool bitwise_operator(token_t);
		
		bool assignment_operator(token_t);
		
		bool peek_binary_operator();
		
		bool peek_literal();
		
		bool peek_literal_string();
		
		bool peek_unary_operator();
		
		bool integer_literal(token_t);
		
		bool character_literal(token_t);
		
		bool constant_expr(token_t);
		
		bool peek_constant_expr();
		
		bool peek_assignment_operator();
		
		bool expect_assignment_operator();
		
		bool expression_token(token_t);
		
		bool peek_expr_token();
		
		expr *expression(terminator_t &);
		
		void primary_expr(terminator_t &);
		
		void sub_primary_expr(terminator_t &);
		
		int precedence(token_t);
		
		void postfix_expression(std::list<token> &);
		
		primary_expr_t *get_primary_expr_tree();
		
		id_expr_t *get_id_expr_tree();
		
		bool peek_identifier();
		
		void id_expr(terminator_t &);
		
		void subscript_id_access(terminator_t &);
		
		void pointer_indirection_access(terminator_t &);
		
		void pointer_operator_sequence();
		
		int get_pointer_operator_sequence();
		
		//void incr_decr_expr(terminator_t &);
		id_expr_t *prefix_incr_expr(terminator_t &);
		
		void postfix_incr_expr(terminator_t &);
		
		id_expr_t *prefix_decr_expr(terminator_t &);
		
		void postfix_decr_expr(terminator_t &);
		
		bool member_access_operator(token_t);
		
		bool peek_member_access_operator();
		
		id_expr_t *address_of_expr(terminator_t &);
		
		bool peek_type_specifier(std::vector<token> &);
		
		bool type_specifier(token_t);
		
		bool peek_type_specifier();
		
		bool peek_type_specifier_from(int);
		
		void get_type_specifier(std::vector<token> &);
		
		sizeof_expr_t *sizeof_expr(terminator_t &);
		
		cast_expr_t *cast_expr(terminator_t &);
		
		void cast_type_specifier(cast_expr_t **);
		
		assgn_expr_t *assignment_expr(terminator_t &, bool);
		
		call_expr_t *call_expr(terminator_t &);
		
		void func_call_expr_list(std::list<expr *> &, terminator_t &);
		
		void record_specifier();
		
		bool record_head(token *, bool *, bool *);
		
		void record_member_definition(st_record_node **);
		
		void rec_id_list(st_record_node **, st_type_info **);
		
		void rec_subscript_member(std::list<token> &);
		
		void rec_func_pointer_member(st_record_node **, int *, st_type_info **);
		
		void rec_func_pointer_params(st_symbol_info **);
		
		void simple_declaration(token, std::vector<token> &, bool, st_node **);
		
		void simple_declarator_list(st_node **, st_type_info **);
		
		void subscript_declarator(st_symbol_info **);
		
		void subscript_initializer(std::vector<std::vector<token>> &);
		
		void literal_list(std::vector<token> &);
		
		void func_head(st_func_info **, token, token, std::vector<token> &, bool);
		
		void func_params(std::list<st_func_param_info *> &);
		
		labled_stmt *labled_statement();
		
		expr_stmt *expression_statement();
		
		select_stmt *selection_statement(st_node **);
		
		iter_stmt *iteration_statement(st_node **);
		
		jump_stmt *jump_statement();
		
		stmt *statement(st_node **);
		
		asm_stmt *asm_statement();
		
		void asm_statement_sequence(asm_stmt **);
		
		void asm_operand(std::vector<st_asm_operand *> &);
		
		void get_func_info(st_func_info **, token, int, std::vector<token> &, bool, bool);
	};
	
}
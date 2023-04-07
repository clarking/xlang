/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <vector>
#include <stack>
#include <map>
#include "token.hpp"
#include "lex.hpp"
#include "tree.hpp"

namespace xlang {
	
	typedef std::vector<TokenId> terminator_t;
	
	class Parser {
		public:
		
		Parser();
		
		TreeNode *parse();
		
		friend std::ostream &operator<<(std::ostream &, const std::vector<Token> &);
		
		friend std::ostream &operator<<(std::ostream &, const std::list<Token> &);
		
		private:
		
		bool is_expr_terminator_got{false};
		bool is_expr_terminator_consumed{false};
		int ptr_oprtr_count{0};
		
		Token funcname;
		Token consumed_terminator;
		Token nulltoken;

		std::stack<Token> parenth_stack;
		std::vector<Token> expr_list;

        //token_lexeme_table used for string of special symbols
		std::map<TokenId, std::string> token_lexeme_table = {
            {PTR_OP,        "*"},
            {LOG_NOT,       "!"},
		    {ADDROF_OP,     "&"},
		    {ARROW_OP,      "->"},
		    {DOT_OP,        "."},
		    {COMMA_OP,      ","},
		    {COLON_OP,      ":"},
		    {CURLY_OPEN,    "{"},
		    {CURLY_CLOSE,   "}"},
		    {PARENTH_OPEN,  "("},
		    {PARENTH_CLOSE, ")"},
		    {SQUARE_OPEN,   "["},
		    {SQUARE_CLOSE,  "]"},
		    {SEMICOLON,     ";"}
        };
		
		std::string s_quotestring(std::string str) {
			return "'" + str + "'";
		}
		
		std::string d_quotestring(std::string str) {
			return "\"" + str + "\"";
		}
		
		bool peek_token(TokenId);
		
		bool peek_token(const char *format...);
		
		bool peek_token(std::vector<TokenId> &);
		
		bool peek_nth_token(TokenId, int);
		
		TokenId get_peek_token();
		
		TokenId get_nth_token(int);
		
		bool expr_literal(TokenId);
		
		bool peek_expr_literal();
		
		bool expect(TokenId);
		
		bool expect(TokenId, bool);
		
		bool expect(TokenId, bool, std::string);
		
		bool expect(TokenId, bool, std::string, std::string);
		
		bool expect(const char *format...);
		
		void consume_next();
		
		void consume_n(int);
		
		void consume_till(terminator_t &);
		
		bool expect_binary_operator();
		
		bool expect_literal();
		
		bool check_parenth();
		
		bool matches_terminator(terminator_t &, TokenId);
		
		std::string get_terminator(terminator_t &);
		
		bool unary_operator(TokenId);
		
		bool binary_operator(TokenId);
		
		bool arithmetic_operator(TokenId);
		
		bool logical_operator(TokenId);
		
		bool comparison_operator(TokenId);
		
		bool bitwise_operator(TokenId);
		
		bool assignment_operator(TokenId);
		
		bool peek_binary_operator();
		
		bool peek_literal();
		
		bool peek_literal_string();
		
		bool peek_unary_operator();
		
		bool integer_literal(TokenId);
		
		bool character_literal(TokenId);
		
		bool constant_expr(TokenId);
		
		bool peek_constant_expr();
		
		bool peek_assignment_operator();
		
		bool expect_assignment_operator();
		
		bool expression_token(TokenId);
		
		bool peek_expr_token();
		
		Expression *expression(terminator_t &);
		
		void primary_expr(terminator_t &);
		
		void sub_primary_expr(terminator_t &);
		
		int precedence(TokenId);
		
		void postfix_expression(std::list<Token> &);
		
		PrimaryExpression *get_primary_expr_tree();
		
		IdentifierExpression *get_id_expr_tree();
		
		bool peek_identifier();
		
		void id_expr(terminator_t &);
		
		void subscript_id_access(terminator_t &);
		
		void pointer_indirection_access(terminator_t &);
		
		void pointer_operator_sequence();
		
		int get_pointer_operator_sequence();
		
		IdentifierExpression *prefix_incr_expr(terminator_t &);
		
		void postfix_incr_expr(terminator_t &);
		
		IdentifierExpression *prefix_decr_expr(terminator_t &);
		
		void postfix_decr_expr(terminator_t &);
		
		bool member_access_operator(TokenId);
		
		bool peek_member_access_operator();
		
		IdentifierExpression *address_of_expr(terminator_t &);
		
		bool peek_type_specifier(std::vector<Token> &);
		
		bool type_specifier(TokenId);
		
		bool peek_type_specifier();
		
		bool peek_type_specifier_from(int);
		
		void get_type_specifier(std::vector<Token> &);
		
		SizeOfExpression *sizeof_expr(terminator_t &);
		
		CastExpression *cast_expr(terminator_t &);
		
		void cast_type_specifier(CastExpression **);
		
		AssignmentExpression *assignment_expr(terminator_t &, bool);
		
		CallExpression *call_expr(terminator_t &);
		
		void func_call_expr_list(std::list<Expression *> &, terminator_t &);
		
		void record_specifier();
		
		bool record_head(Token *, bool *, bool *);
		
		void record_member_definition(RecordNode **);
		
		void rec_id_list(RecordNode **, TypeInfo **);
		
		void rec_subscript_member(std::list<Token> &);
		
		void rec_func_pointer_member(RecordNode **, int *, TypeInfo **);
		
		void rec_func_pointer_params(SymbolInfo **);
		
		void simple_declaration(Token, std::vector<Token> &, bool, Node **);
		
		void simple_declarator_list(Node **, TypeInfo **);
		
		void subscript_declarator(SymbolInfo **);
		
		void subscript_initializer(std::vector<std::vector<Token>> &);
		
		void literal_list(std::vector<Token> &);
		
		void func_head(FunctionInfo **, Token, Token, std::vector<Token> &, bool);
		
		void func_params(std::list<FuncParamInfo *> &);
		
		LabelStatement *labled_statement();
		
		ExpressionStatement *expression_statement();
		
		SelectStatement *selection_statement(Node **);
		
		IterationStatement *iteration_statement(Node **);
		
		JumpStatement *jump_statement();
		
		Statement *statement(Node **);
		
		AsmStatement *asm_statement();
		
		void asm_statement_sequence(AsmStatement **);
		
		void asm_operand(std::vector<AsmOperand *> &);
		
		void get_func_info(FunctionInfo **, Token, NodeType, std::vector<Token> &, bool, bool);
	};
}
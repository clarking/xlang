/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2023, Aaron Clark Diaz.
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
#include "symtab.hpp"

namespace xlang {
	
	class Analyzer {
    public:
		void analyze(TreeNode **);
		
    private:
		TreeNode *parse_tree = nullptr;

		Node *func_symtab = nullptr;

		FunctionInfo *func_info = nullptr;
		std::stack<PrimaryExpression *> prim_expr_stack;
		std::map<std::string, Token> labels;
		int break_inloop = 0, continue_inloop = 0;
		std::list<Token> goto_list; // for forward reference of labels
		PrimaryExpression *factor_1 = nullptr, *factor_2 = nullptr, *primoprtr = nullptr;
		
		static std::string boolean(bool b) {
			return b ? "true" : "false";
		}
		
		SymbolInfo *search_func_params(Token);
		
		SymbolInfo *search_id(Token);
		
		int tree_height(ExpressionType, PrimaryExpression *, IdentifierExpression *);
		
		void analyze_statement(Statement **);
		
		void analyze_expr(Expression **);
		
		void analyze_primary_expr(PrimaryExpression **);
		
		void analyze_id_expr(IdentifierExpression **);
		
		void analyze_sizeof_expr(SizeOfExpression **);
		
		void analyze_cast_expr(CastExpression **);
		
		void analyze_funccall_expr(CallExpression **);
		
		void analyze_assgn_expr(AssignmentExpression **);
		
		void analyze_label_statement(LabelStatement **);
		
		void analyze_selection_statement(SelectStatement **);
		
		void analyze_iteration_statement(IterationStatement **);
		
		void analyze_return_jmpstmt(JumpStatement **);
		
		void analyze_jump_statement(JumpStatement **);
		
		void analyze_goto_jmpstmt();
		
		void analyze_func_param_info(FunctionInfo **);
		
		void analyze_asm_template(AsmStatement **);
		
		void analyze_asm_output_operand(AsmOperand **);
		
		void analyze_asm_input_operand(AsmOperand **);
		
		void analyze_asm_operand_expr(Expression **);
		
		void analyze_asm_statement(AsmStatement **);
		
		void analyze_global_assignment(TreeNode **);
		
		void analyze_func_params(FunctionInfo *);
		
		void analyze_local_declaration(TreeNode **);
		
		void check_invalid_type_declaration(Node *);
		
		bool check_pointer_arithmetic(PrimaryExpression *, PrimaryExpression *, PrimaryExpression *);
		
		bool check_primexp_type_argument(PrimaryExpression *, PrimaryExpression *, PrimaryExpression *);
		
		bool check_unary_primexp_type_argument(PrimaryExpression *);
		
		bool check_array_subscript(IdentifierExpression *);
		
		bool check_unary_idexp_type_argument(IdentifierExpression *);
		
		bool check_assignment_type_argument(AssignmentExpression *, ExpressionType, IdentifierExpression *, PrimaryExpression *);
		
		void get_idexpr_idinfo(IdentifierExpression *, SymbolInfo **);
		
		IdentifierExpression *get_idexpr_attrbute_node(IdentifierExpression **);
		
		IdentifierExpression *get_assgnexpr_idexpr_attribute(IdentifierExpression *);
		
		std::string get_template_token(std::string);
		
		std::vector<int> get_asm_template_tokens_vector(Token);
		
		bool is_digit(char);
		
		void simplify_assgn_primary_expr(AssignmentExpression **);
		
		bool has_constant_member(PrimaryExpression *);
		
		bool has_constant_array_subscript(IdentifierExpression *);
		
		template<typename type>
		void clear_stack(std::stack<type> &stk) {
			while (!stk.empty())
				stk.pop();
		}
	};
}

/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <stack>
#include <limits>
#include "token.hpp"
#include "lex.hpp"
#include "tree.hpp"
#include "symtab.hpp"

namespace xlang {
	
	constexpr unsigned int maxint = std::numeric_limits<int>::max();
	
	class Optimizer {
    public:
		void optimize(TreeNode **);
		
    private:
		bool evaluate(Token &, Token &, Token &, std::string &, bool);
		
		std::stack<PrimaryExpression *> pexpr_stack;
		
		void clear_primary_expr_stack();
		
		void get_inorder_primary_expr(PrimaryExpression **);
		
		bool has_float_type(PrimaryExpression *);
		
		bool has_id(PrimaryExpression *);
		
		void id_constant_folding(PrimaryExpression **);
		
		void constant_folding(PrimaryExpression **);
		
		bool equals(std::stack<PrimaryExpression *>, std::stack<PrimaryExpression *>);
		
		PrimaryExpression *get_cmnexpr1_node(PrimaryExpression **, PrimaryExpression **);
		
		void change_subexpr_pointers(PrimaryExpression **, PrimaryExpression **, PrimaryExpression **);
		
		void common_subexpression_elimination(PrimaryExpression **);
		
		bool is_powerof_2(int, int *);
		
		void strength_reduction(PrimaryExpression **);
		
		void optimize_primary_expr(PrimaryExpression **);
		
		void optimize_assignment_expr(AssignmentExpression **);
		
		void optimize_expr(Expression **);
		
		std::unordered_map<std::string, int> local_members;
		std::unordered_map<std::string, int> global_members;
		Node *func_symtab = nullptr;
		
		void update_count(std::string);
		
		void search_id_in_primary_expr(PrimaryExpression *);
		
		void search_id_in_id_expr(IdentifierExpression *);
		
		void search_id_in_expr(Expression **);
		
		void search_id_in_statement(Statement **);
		
		void dead_code_elimination(TreeNode **);
		
		void optimize_statement(Statement **);
		
		template<typename type>
		void clear_stack(std::stack<type> &stk) {
			while (!stk.empty())
				stk.pop();
		}
	};
}
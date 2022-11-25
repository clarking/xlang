/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


// data/operations used in optimize.cpp file by class optimizer.

#pragma once

#include <stack>
#include <limits>
#include "token.hpp"
#include "lex.hpp"
#include "tree.hpp"
#include "symtab.hpp"

namespace xlang {
	
	constexpr unsigned int maxint = std::numeric_limits<int>::max();
	
	class optimizer {
		public:
		void optimize(tree_node **);
		
		private:
		bool evaluate(token &, token &, token &, std::string &, bool);
		
		std::stack<primary_expr_t *> pexpr_stack;
		
		void clear_primary_expr_stack();
		
		void get_inorder_primary_expr(primary_expr_t **);
		
		bool has_float_type(primary_expr_t *);
		
		bool has_id(primary_expr_t *);
		
		void id_constant_folding(primary_expr_t **);
		
		void constant_folding(primary_expr_t **);
		
		bool equals(std::stack<primary_expr_t *>, std::stack<primary_expr_t *>);
		
		primary_expr_t *get_cmnexpr1_node(primary_expr_t **, primary_expr_t **);
		
		void change_subexpr_pointers(primary_expr_t **, primary_expr_t **, primary_expr_t **);
		
		void common_subexpression_elimination(primary_expr_t **);
		
		bool is_powerof_2(int, int *);
		
		void strength_reduction(primary_expr_t **);
		
		void optimize_primary_expr(primary_expr_t **);
		
		void optimize_assignment_expr(assgn_expr_t **);
		
		void optimize_expr(expr **);
		
		std::unordered_map<std::string, int> local_members;
		std::unordered_map<std::string, int> global_members;
		st_node *func_symtab = nullptr;
		
		void update_count(std::string);
		
		void search_id_in_primary_expr(primary_expr_t *);
		
		void search_id_in_id_expr(id_expr_t *);
		
		void search_id_in_expr(expr **);
		
		void search_id_in_statement(stmt **);
		
		void dead_code_elimination(tree_node **);
		
		void optimize_statement(stmt **);
		
		template<typename type>
		void clear_stack(std::stack<type> &stk) {
			while (!stk.empty())
				stk.pop();
		}
	};
}
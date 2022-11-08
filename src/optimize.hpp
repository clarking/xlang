/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*
* Contains data/operations used in optimize.cpp file by class optimizer.
*/

#ifndef OPTIMIZE_HPP
#define OPTIMIZE_HPP

#include <stack>
#include <limits>
#include "token.hpp"
#include "types.hpp"
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
		std::stack<primary_expr *> pexpr_stack;
		void clear_primary_expr_stack();
		void get_inorder_primary_expr(primary_expr **);
		bool has_float_type(primary_expr *);
		bool has_id(primary_expr *);
		void id_constant_folding(primary_expr **);
		void constant_folding(primary_expr **);
		bool equals(std::stack<primary_expr *>, std::stack<primary_expr *>);
		primary_expr *get_cmnexpr1_node(primary_expr **, primary_expr **);
		void change_subexpr_pointers(primary_expr **, primary_expr **, primary_expr **);
		void common_subexpression_elimination(primary_expr **);
		bool is_powerof_2(int, int *);
		void strength_reduction(primary_expr **);
		void optimize_primary_expression(primary_expr **);
		void optimize_assignment_expression(assgn_expr **);
		void optimize_expression(expr **);

		std::unordered_map<std::string, int> local_members;
		std::unordered_map<std::string, int> global_members;
		st_node *func_symtab = nullptr;

		void update_count(std::string);
		void search_id_in_primary_expr(primary_expr *);
		void search_id_in_id_expr(id_expr *);
		void search_id_in_expression(expr **);
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

#endif


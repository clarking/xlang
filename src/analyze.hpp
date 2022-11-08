/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*
* Contains data/operations used in analyze.cpp file by class analyzer.
*/

#ifndef ANALYZE_H
#define ANALYZE_H

#include <vector>
#include <stack>
#include <map>
#include "token.hpp"
#include "types.hpp"
#include "lex.hpp"
#include "tree.hpp"
#include "symtab.hpp"

namespace xlang {

	class analyzer {
	public:
		void analyze(tree_node **);

	private:
		tree_node *parse_tree = nullptr;
		st_node *func_symtab = nullptr;
		st_func_info *func_info = nullptr;
		std::stack<primary_expr *> prim_expr_stack;

		std::map<std::string, token> labels;

		int break_inloop = 0, continue_inloop = 0;

		std::list<token> goto_list; // for forward reference of labels

		primary_expr *factor_1 = nullptr, *factor_2 = nullptr, *primoprtr = nullptr;

		static std::string boolean(bool b) {
			return b ? "true" : "false";
		}

		st_symbol_info *search_func_params(token);
		st_symbol_info *search_id(token);

		int tree_height(int, primary_expr *, id_expr *);

		void analyze_statement(stmt **);
		void analyze_expression(expr **);
		void analyze_primary_expr(primary_expr **);
		void analyze_id_expr(id_expr **);
		void analyze_sizeof_expr(sizeof_expr **);
		void analyze_cast_expr(cast_expr **);
		void analyze_funccall_expr(func_call_expr **);
		void analyze_assgn_expr(assgn_expr **);
		void analyze_label_statement(labled_stmt **);
		void analyze_selection_statement(select_stmt **);
		void analyze_iteration_statement(iter_stmt **);
		void analyze_return_jmpstmt(jump_stmt **);
		void analyze_jump_statement(jump_stmt **);
		void analyze_goto_jmpstmt();
		void analyze_func_param_info(st_func_info **);
		void analyze_asm_template(asm_stmt **);
		void analyze_asm_output_operand(st_asm_operand **);
		void analyze_asm_input_operand(st_asm_operand **);
		void analyze_asm_operand_expression(expr **);
		void analyze_asm_statement(asm_stmt **);
		void analyze_global_assignment(tree_node **);
		void analyze_func_params(st_func_info *);
		void analyze_local_declaration(tree_node **);

		void check_invalid_type_declaration(st_node *);
		bool check_pointer_arithmetic(primary_expr *, primary_expr *, primary_expr *);
		bool check_primexp_type_argument(primary_expr *, primary_expr *, primary_expr *);
		bool check_unary_primexp_type_argument(primary_expr *);
		bool check_array_subscript(id_expr *);
		bool check_unary_idexp_type_argument(id_expr *);
		bool check_assignment_type_argument(assgn_expr *, int, id_expr *, primary_expr *);

		void get_idexpr_idinfo(id_expr *, st_symbol_info **);
		id_expr *get_idexpr_attrbute_node(id_expr **);
		id_expr *get_assgnexpr_idexpr_attribute(id_expr *);
		std::string get_template_token(std::string);
		std::vector<int> get_asm_template_tokens_vector(token);

		bool is_digit(char);
		void simplify_assgn_primary_expression(assgn_expr **);
		bool has_constant_member(primary_expr *);
		bool has_constant_array_subscript(id_expr *);

		template<typename type>
		void clear_stack(std::stack<type> &stk) {
			while (!stk.empty())
				stk.pop();
		}
	};
}

#endif



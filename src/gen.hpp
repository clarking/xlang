/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


// data/operations used in x86_gen.cpp file by class x86_gen.

#pragma once

#include <vector>
#include <stack>
#include <map>
#include <unordered_map>
#include <memory>
#include "token.hpp"
#include "lex.hpp"
#include "tree.hpp"
#include "symtab.hpp"
#include "regs.hpp"
#include "insn.hpp"
#include "optimize.hpp"

namespace xlang {
	
	class gen {
		public:
		
		gen() : reg{new xlang::regs}, insncls{new xlang::insn_class} {}
		~gen() {
			for (auto x: data_section)
				insncls->delete_data(&x);
			for (auto x: resv_section)
				insncls->delete_resv(&x);
			for (auto x: text_section)
				insncls->delete_text(&x);
			for (auto x: instructions)
				insncls->delete_insn(&x);
			delete reg;
			delete insncls;
		}
		
		void get_code(tree_node **);
		
		private:
		
		xlang::regs *reg;
		xlang::insn_class *insncls;
		
		st_node *func_symtab = nullptr;
		st_func_info *func_params = nullptr;
		
		size_t float_data_count = 1, string_data_count = 1;
		size_t if_label_count = 1, else_label_count = 1, exit_if_count = 1;
		size_t while_loop_count = 1, dowhile_loop_count = 1, for_loop_count = 1;
		size_t exit_loop_label_count = 1;
		
		iter_stmt_t current_loop = WHILE_STMT;
		std::stack<int> for_loop_stack, while_loop_stack, dowhile_loop_stack;
		std::unordered_map<std::string, st_symbol_info *> initialized_data;
		
		//vectors for data, resv, text sections and instructions
		std::vector<data *> data_section;
		std::vector<resv *> resv_section;
		std::vector<text *> text_section;
		std::vector<insn *> instructions;
		
		//function member, member type size and frame-pointer displacement(fp)
		struct func_member {
			int insize;
			int fp_disp;
		};
		
		//total size of local members, and hash table for each member location
		struct func_local_members {
			size_t total_size;
			std::unordered_map<std::string, func_member> members;
		};
		
		//hash table for each function and its local members
		std::unordered_map<std::string, func_local_members> func_members;
		
		using funcmem_iterator = std::unordered_map<std::string, func_local_members>::iterator;
		using memb_iterator = std::unordered_map<std::string, func_member>::iterator;
		
		template<typename type>
		void clear_stack(std::stack<type> &stk) {
			while (!stk.empty())
				stk.pop();
		}
		
		int data_type_size(token);
		
		int data_decl_size(declspace_t);
		
		int resv_decl_size(resspace_t);
		
		declspace_t declspace_type_size(token);
		
		resspace_t resvspace_type_size(token);
		
		bool has_float(primary_expr_t *);
		
		void max_datatype_size(primary_expr_t *, int *);
		
		void get_func_local_members();
		
		st_symbol_info *search_func_params(std::string);
		
		st_symbol_info *search_id(std::string);
		
		insnsize_t get_insn_size_type(int);
		
		std::stack<primary_expr_t *> get_post_order_prim_expr(primary_expr_t *);
		
		insn_t get_arthm_op(std::string);
		
		insn_t get_farthm_op(std::string, bool);
		
		insn *get_insn(insn_t instype, int oprcount);
		
		void insert_comment(std::string);
		
		data *search_data(std::string);
		
		data *search_string_data(std::string);
		
		std::string hex_escape_sequence(char);
		
		std::string get_hex_string(std::string);
		
		bool get_function_local_member(func_member *, token);
		
		regs_t gen_int_primexp_single_assgn(primary_expr_t *, int);
		
		bool gen_int_primexp_compl(primary_expr_t *, int);
		
		data *create_string_data(std::string);
		
		regs_t gen_string_literal_primary_expr(primary_expr_t *);
		
		fregs_t gen_float_primexp_single_assgn(primary_expr_t *, declspace_t);
		
		regs_t gen_int_primary_expr(primary_expr_t *);
		
		data *create_float_data(declspace_t, std::string);
		
		void gen_float_primary_expr(primary_expr_t *);
		
		std::pair<int, int> gen_primary_expr(primary_expr_t **);
		
		void gen_assgn_primary_expr(assgn_expr_t **);
		
		void gen_sizeof_expr(sizeof_expr_t **);
		
		void gen_assgn_sizeof_expr(assgn_expr_t **);
		
		void gen_assgn_cast_expr(assgn_expr_t **);
		
		void gen_id_expr(id_expr_t **);
		
		void gen_assgn_id_expr(assgn_expr_t **);
		
		void gen_assgn_funccall_expr(assgn_expr_t **);
		
		void gen_assignment_expr(assgn_expr_t **);
		
		void gen_funccall_expr(call_expr_t **);
		
		void gen_cast_expr(cast_expr_t **);
		
		void gen_expr(expr **);
		
		void save_frame_pointer();
		
		void restore_frame_pointer();
		
		void func_return();
		
		void gen_function();
		
		void gen_uninitialized_data();
		
		void gen_array_init_declaration(st_node *);
		
		void gen_global_declarations(tree_node **trnode);
		
		void gen_label_statement(labled_stmt **);
		
		void gen_jump_statement(jump_stmt **);
		
		regs_t get_reg_type_by_char(char);
		
		std::string get_asm_output_operand(st_asm_operand **);
		
		std::string get_asm_input_operand(st_asm_operand **);
		
		void get_nonescaped_string(std::string &);
		
		void gen_asm_statement(asm_stmt **);
		
		bool is_literal(token);
		
		bool gen_float_type_condition(primary_expr_t **, primary_expr_t **, primary_expr_t **opr);
		
		token_t gen_select_stmt_condition(expr *);
		
		void gen_selection_statement(select_stmt **);
		
		void gen_iteration_statement(iter_stmt **);
		
		void gen_statement(stmt **);
		
		void write_record_member_to_asm_file(record_data_type &, std::ofstream &);
		
		void write_record_data_to_asm_file(resv **, std::ofstream &);
		
		void write_data_to_asm_file(std::ofstream &);
		
		void write_resv_to_asm_file(std::ofstream &);
		
		void write_text_to_asm_file(std::ofstream &);
		
		void write_instructions_to_asm_file(std::ofstream &);
		
		void write_asm_file();
		
		bool search_text(text *);
		
		void gen_record();
		
		std::unordered_map<std::string, int> record_sizes;
	};
}
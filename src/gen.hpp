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
	
	class CodeGen {
    public:
		
		CodeGen() : reg{new Registers}, insncls{new InstructionClass} {}

		~CodeGen() {

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
		
		void get_code(TreeNode **);
		
    private:
		
		Registers *reg;
		InstructionClass *insncls;
		
		Node *func_symtab = nullptr;
		FunctionInfo *func_params = nullptr;
		
		size_t float_data_count = 1, string_data_count = 1;
		size_t if_label_count = 1, else_label_count = 1, exit_if_count = 1;
		size_t while_loop_count = 1, dowhile_loop_count = 1, for_loop_count = 1;
		size_t exit_loop_label_count = 1;
		
		IterationType current_loop = IterationType::WHILE;
		std::stack<int> for_loop_stack, while_loop_stack, dowhile_loop_stack;
		std::unordered_map<std::string, SymbolInfo *> initialized_data;
		
		//vectors for data, bss, text sections and instructions
		std::vector<Member *> data_section;
		std::vector<ReserveSection *> resv_section;
		std::vector<TextSection *> text_section;
		std::vector<Instruction *> instructions;
		
		struct FunctionMember {
			int insize;  // member type size 
			int fp_disp; // frame-pointer displacement(fp)
		};
		
		struct LocalMembers {
			size_t total_size;
			std::unordered_map<std::string, FunctionMember> members;
		};
		
		
		std::unordered_map<std::string, LocalMembers> func_members;
		
		using funcmem_iterator = std::unordered_map<std::string, LocalMembers>::iterator;
		using memb_iterator = std::unordered_map<std::string, FunctionMember>::iterator;
		
		template<typename type>
		void clear_stack(std::stack<type> &stk) {
			while (!stk.empty())
				stk.pop();
		}
		
		int data_type_size(Token);
		
		int data_decl_size(DeclarationType);
		
		int resv_decl_size(ReservationType);
		
		DeclarationType declspace_type_size(Token);
		
		ReservationType resvspace_type_size(Token);
		
		bool has_float(PrimaryExpression *);
		
		void max_datatype_size(PrimaryExpression *, int *);
		
		void get_func_local_members();
		
		SymbolInfo *search_func_params(std::string);
		
		SymbolInfo *search_id(std::string);
		
		InstructionSize get_insn_size_type(int);
		
		std::stack<PrimaryExpression *> get_post_order_prim_expr(PrimaryExpression *);
		
		InstructionType get_arthm_op(std::string);
		
		InstructionType get_farthm_op(std::string, bool);
		
		Instruction *get_insn(InstructionType instype, int oprcount);
		
		void insert_comment(std::string);
		
		Member *search_data(std::string);
		
		Member *search_string_data(std::string);
		
		std::string hex_escape_sequence(char);
		
		std::string get_hex_string(std::string);
		
		bool get_function_local_member(FunctionMember *, Token);
		
		RegisterType gen_int_primexp_single_assgn(PrimaryExpression *, int);
		
		bool gen_int_primexp_compl(PrimaryExpression *, int);
		
		Member *create_string_data(std::string);
		
		RegisterType gen_string_literal_primary_expr(PrimaryExpression *);
		
		FloatRegisterType gen_float_primexp_single_assgn(PrimaryExpression *, DeclarationType);
		
		RegisterType gen_int_primary_expr(PrimaryExpression *);
		
		Member *create_float_data(DeclarationType, std::string);
		
		void gen_float_primary_expr(PrimaryExpression *);
		
		std::pair<int, int> gen_primary_expr(PrimaryExpression **);
		
		void gen_assgn_primary_expr(AssignmentExpression **);
		
		void gen_sizeof_expr(SizeOfExpression **);
		
		void gen_assgn_sizeof_expr(AssignmentExpression **);
		
		void gen_assgn_cast_expr(AssignmentExpression **);
		
		void gen_id_expr(IdentifierExpression **);
		
		void gen_assgn_id_expr(AssignmentExpression **);
		
		void gen_assgn_funccall_expr(AssignmentExpression **);
		
		void gen_assignment_expr(AssignmentExpression **);
		
		void gen_funccall_expr(CallExpression **);
		
		void gen_cast_expr(CastExpression **);
		
		void gen_expr(Expression **);
		
		void save_frame_pointer();
		
		void restore_frame_pointer();
		
		void func_return();
		
		void gen_function();
		
		void gen_uninitialized_data();
		
		void gen_array_init_declaration(Node *);
		
		void gen_global_declarations(TreeNode **trnode);
		
		void gen_label_statement(LabelStatement **);
		
		void gen_jump_statement(JumpStatement **);
		
		RegisterType get_reg_type_by_char(char);
		
		std::string get_asm_output_operand(AsmOperand **);
		
		std::string get_asm_input_operand(AsmOperand **);
		
		void get_nonescaped_string(std::string &);
		
		void gen_asm_statement(AsmStatement **);
		
		bool is_literal(Token);
		
		bool gen_float_type_condition(PrimaryExpression **, PrimaryExpression **, PrimaryExpression **opr);
		
		TokenId gen_select_stmt_condition(Expression *);
		
		void gen_selection_statement(SelectStatement **);
		
		void gen_iteration_statement(IterationStatement **);
		
		void gen_statement(Statement **);
		
		void write_record_member_to_asm_file(RecordDataType &, std::ofstream &);
		
		void write_record_data_to_asm_file(ReserveSection **, std::ofstream &);
		
		void write_data_to_asm_file(std::ofstream &);
		
		void write_resv_to_asm_file(std::ofstream &);
		
		void write_text_to_asm_file(std::ofstream &);
		
		void write_instructions_to_asm_file(std::ofstream &);
		
		void write_asm_file();
		
		bool search_text(TextSection *);
		
		void gen_record();
		
		std::unordered_map<std::string, int> record_sizes;
	};
}
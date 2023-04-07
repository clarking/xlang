/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <stack>
#include "symtab.hpp"
#include "types.hpp"

namespace xlang {
	
	class Tree {
    public:
		static SizeOfExpression *get_sizeof_expr_mem();
		
		static void delete_sizeof_expr(SizeOfExpression **);
		
		static CastExpression *get_cast_expr_mem();
		
		static void delete_cast_expr(CastExpression **);
		
		static PrimaryExpression *get_primary_expr_mem();
		
		static void delete_primary_expr(PrimaryExpression **);
		
		static IdentifierExpression *get_id_expr_mem();
		
		static void delete_id_expr(IdentifierExpression **);
		
		static Expression *get_expr_mem();
		
		static void delete_expr(Expression **);
		
		static AssignmentExpression *get_assgn_expr_mem();
		
		static void delete_assgn_expr(AssignmentExpression **);
		
		static CallExpression *get_func_call_expr_mem();
		
		static void delete_func_call_expr(CallExpression **);
		
		static AsmOperand *get_asm_operand_mem();
		
		static void delete_asm_operand(AsmOperand **);
		
		static LabelStatement *get_label_stmt_mem();
		
		static ExpressionStatement *get_expr_stmt_mem();
		
		static SelectStatement *get_select_stmt_mem();
		
		static IterationStatement *get_iter_stmt_mem();
		
		static JumpStatement *get_jump_stmt_mem();
		
		static AsmStatement *get_asm_stmt_mem();
		
		static Statement *get_stmt_mem();
		
		static TreeNode *get_tree_node_mem();
		
		static void delete_label_stmt(LabelStatement **);
		
		static void delete_expr_stmt(ExpressionStatement **);
		
		static void delete_select_stmt(SelectStatement **);
		
		static void delete_iter_stmt(IterationStatement **);
		
		static void delete_jump_stmt(JumpStatement **);
		
		static void delete_asm_stmt(AsmStatement **);
		
		static void delete_stmt(Statement **);
		
		static void delete_tree(TreeNode **);
		
		static void delete_tree_node(TreeNode **);
		
		static void get_inorder_primary_expr(PrimaryExpression **);
		
		static void add_asm_statement(AsmStatement **, AsmStatement **);
		
		static void add_statement(Statement **, Statement **);
		
		static void add_tree_node(TreeNode **, TreeNode **);
		
    private:
		static std::stack<PrimaryExpression *> pexpr_stack;
	};
}

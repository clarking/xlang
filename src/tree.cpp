/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


#include <list>
#include "tree.hpp"

namespace xlang {
	
	SizeOfExpression *Tree::get_sizeof_expr_mem() {
		SizeOfExpression *newexpr = new SizeOfExpression();
		return newexpr;
	}
	
	void Tree::delete_sizeof_expr(SizeOfExpression **soexpr) {
		if (*soexpr == nullptr)
			return;
		delete *soexpr;
		*soexpr = nullptr;
	}
	
	CastExpression *Tree::get_cast_expr_mem() {
		CastExpression *newexpr = new CastExpression();
		return newexpr;
	}
	
	void Tree::delete_cast_expr(CastExpression **cexpr) {
		if (*cexpr == nullptr)
			return;
		delete *cexpr;
		*cexpr = nullptr;
	}
	
	PrimaryExpression *Tree::get_primary_expr_mem() {
		PrimaryExpression *newexpr = new PrimaryExpression();
		newexpr->id_info = nullptr;
		newexpr->left = nullptr;
		newexpr->right = nullptr;
		newexpr->unary_node = nullptr;
		return newexpr;
	}
	
	std::stack<PrimaryExpression *> Tree::pexpr_stack;
	
	void Tree::get_inorder_primary_expr(PrimaryExpression **pexpr) {

		PrimaryExpression *pexp = *pexpr;
		if (pexp == nullptr)
			return;
		
		pexpr_stack.push(pexp);
		get_inorder_primary_expr(&pexp->left);
		get_inorder_primary_expr(&pexp->right);
	}
	
	void Tree::delete_primary_expr(PrimaryExpression **pexpr) {
		if (*pexpr == nullptr)
			return;
		
		get_inorder_primary_expr(&(*pexpr));
		
		while (!pexpr_stack.empty()) {
			if (pexpr_stack.top() != nullptr) {
				delete pexpr_stack.top();
				
			}
			pexpr_stack.pop();
		}
	}
	
	IdentifierExpression *Tree::get_id_expr_mem() {
		IdentifierExpression *newexpr = new IdentifierExpression();
		newexpr->id_info = nullptr;
		newexpr->left = nullptr;
		newexpr->right = nullptr;
		newexpr->unary = nullptr;
		return newexpr;
	}
	
	void Tree::delete_id_expr(IdentifierExpression **idexpr) {
		if (*idexpr == nullptr)
			return;
		delete_id_expr(&((*idexpr)->left));
		delete_id_expr(&((*idexpr)->right));
		delete_id_expr(&((*idexpr)->unary));
		
		delete *idexpr;
		*idexpr = nullptr;
	}
	
	Expression *Tree::get_expr_mem() {
		Expression *newexpr = new Expression;
		newexpr->primary_expr = nullptr;
		newexpr->sizeof_expr = nullptr;
		newexpr->cast_expr = nullptr;
		newexpr->id_expr = nullptr;
		newexpr->assgn_expr = nullptr;
		newexpr->call_expr = nullptr;
		return newexpr;
	}
	
	void Tree::delete_expr(Expression **exp) {
		Expression *exp2 = *exp;
		if (*exp == nullptr)
			return;
		switch (exp2->expr_kind) {
			case ExpressionType::PRIMARY_EXPR :
				delete_primary_expr(&exp2->primary_expr);
				break;
			case ExpressionType::ASSGN_EXPR :
				delete_assgn_expr(&exp2->assgn_expr);
				break;
			case ExpressionType::SIZEOF_EXPR :
				delete_sizeof_expr(&exp2->sizeof_expr);
				break;
			case ExpressionType::CAST_EXPR :
				delete_cast_expr(&exp2->cast_expr);
				break;
			case ExpressionType::ID_EXPR :
				delete_id_expr(&exp2->id_expr);
				break;
			case ExpressionType::FUNC_CALL_EXPR :
				delete_func_call_expr(&exp2->call_expr);
				break;
		}
		delete *exp;
		*exp = nullptr;
	}
	
	AssignmentExpression *Tree::get_assgn_expr_mem() {
		AssignmentExpression *newexpr = new AssignmentExpression();
		newexpr->id_expr = nullptr;
		newexpr->expression = nullptr;
		return newexpr;
	}
	
	void Tree::delete_assgn_expr(AssignmentExpression **exp) {
		if (*exp == nullptr)
			return;
		delete_id_expr(&(*exp)->id_expr);
		delete_expr(&(*exp)->expression);
		*exp = nullptr;
	}
	
	CallExpression *Tree::get_func_call_expr_mem() {
		CallExpression *newexpr = new CallExpression();
		newexpr->function = nullptr;
		return newexpr;
	}
	
	void Tree::delete_func_call_expr(CallExpression **exp) {
		CallExpression *exp2 = *exp;
		if (exp2 == nullptr)
			return;
		delete_id_expr(&exp2->function);
		std::list<Expression *>::iterator lst = (exp2->expression_list).begin();
		while (lst != (exp2->expression_list).end()) {
			delete *lst;
			lst++;
		}
		delete *exp;
		*exp = nullptr;
	}
	
	AsmOperand *Tree::get_asm_operand_mem() {
		AsmOperand *asmop = new AsmOperand();
		asmop->expression = nullptr;
		return asmop;
	}
	
	void Tree::delete_asm_operand(AsmOperand **asmop) {
		if (*asmop == nullptr)
			return;
		delete_expr(&((*asmop)->expression));
		delete *asmop;
		*asmop = nullptr;
	}
	
	LabelStatement *Tree::get_label_stmt_mem() {
		LabelStatement *newstmt = new LabelStatement();
		return newstmt;
	}
	
	ExpressionStatement *Tree::get_expr_stmt_mem() {
		ExpressionStatement *newstmt = new ExpressionStatement();
		newstmt->expression = nullptr;
		return newstmt;
	}
	
	SelectStatement *Tree::get_select_stmt_mem() {
		SelectStatement *newstmt = new SelectStatement();
		newstmt->condition = nullptr;
		newstmt->else_statement = nullptr;
		newstmt->if_statement = nullptr;
		return newstmt;
	}
	
	IterationStatement *Tree::get_iter_stmt_mem() {
		IterationStatement *newstmt = new IterationStatement();
		newstmt->_while.condition = nullptr;
		newstmt->_while.statement = nullptr;
		newstmt->_dowhile.condition = nullptr;
		newstmt->_dowhile.statement = nullptr;
		newstmt->_for.init_expr = nullptr;
		newstmt->_for.condition = nullptr;
		newstmt->_for.update_expr = nullptr;
		newstmt->_for.statement = nullptr;
		return newstmt;
	}
	
	JumpStatement *Tree::get_jump_stmt_mem() {
		JumpStatement *newstmt = new JumpStatement();
		newstmt->expression = nullptr;
		return newstmt;
	}
	
	AsmStatement *Tree::get_asm_stmt_mem() {
		AsmStatement *asmstmt = new AsmStatement;
		asmstmt->p_next = nullptr;
		return asmstmt;
	}
	
	Statement *Tree::get_stmt_mem() {
		Statement *newstmt = new Statement();
		newstmt->labled_statement = nullptr;
		newstmt->expression_statement = nullptr;
		newstmt->selection_statement = nullptr;
		newstmt->iteration_statement = nullptr;
		newstmt->jump_statement = nullptr;
		newstmt->asm_statement = nullptr;
		newstmt->p_next = nullptr;
		newstmt->p_prev = nullptr;
		return newstmt;
	}
	
	TreeNode *Tree::get_tree_node_mem() {
		TreeNode *newtr = new TreeNode();
		newtr->symtab = SymbolTable::get_node_mem();
		newtr->statement = nullptr;
		newtr->p_next = nullptr;
		newtr->p_prev = nullptr;
		return newtr;
	}
	
	void Tree::delete_label_stmt(LabelStatement **lbstmt) {
		if (*lbstmt == nullptr)
			return;
		delete *lbstmt;
		*lbstmt = nullptr;
	}
	
	void Tree::delete_asm_stmt(AsmStatement **asmstmt) {
		std::vector<AsmOperand *>::iterator it;
		AsmStatement *temp = *asmstmt;
		if (*asmstmt == nullptr)
			return;
		while (temp != nullptr) {
			it = temp->output_operand.begin();
			while (it != temp->output_operand.end()) {
				delete_asm_operand(&(*it));
				it++;
			}
			it = temp->input_operand.begin();
			while (it != temp->output_operand.end()) {
				delete_asm_operand(&(*it));
				it++;
			}
			temp = temp->p_next;
		}
		delete *asmstmt;
		*asmstmt = nullptr;
	}
	
	void Tree::delete_expr_stmt(ExpressionStatement **expstmt) {
		if (*expstmt == nullptr)
			return;
		delete_expr(&((*expstmt)->expression));
		delete *expstmt;
		*expstmt = nullptr;
	}
	
	void Tree::delete_select_stmt(SelectStatement **selstmt) {
		if (*selstmt == nullptr)
			return;
		delete_expr(&((*selstmt)->condition));
		delete_stmt(&((*selstmt)->if_statement));
		delete_stmt(&((*selstmt)->else_statement));
		delete *selstmt;
		*selstmt = nullptr;
	}
	
	void Tree::delete_iter_stmt(IterationStatement **itstmt) {
		if (*itstmt == nullptr)
			return;
		switch ((*itstmt)->type) {
			case IterationType::WHILE :
				delete_expr(&((*itstmt)->_while.condition));
				delete_stmt(&((*itstmt)->_while.statement));
				break;
			case IterationType::DOWHILE :
				delete_expr(&((*itstmt)->_dowhile.condition));
				delete_stmt(&((*itstmt)->_dowhile.statement));
				break;
			case IterationType::FOR :
				delete_expr(&((*itstmt)->_for.init_expr));
				delete_expr(&((*itstmt)->_for.condition));
				delete_expr(&((*itstmt)->_for.update_expr));
				delete_stmt(&((*itstmt)->_for.statement));
				break;
		}
		delete *itstmt;
		*itstmt = nullptr;
	}
	
	void Tree::delete_jump_stmt(JumpStatement **jmpstmt) {
		if (*jmpstmt == nullptr)
			return;
		delete_expr(&((*jmpstmt)->expression));
		delete *jmpstmt;
		*jmpstmt = nullptr;
	}
	
	void Tree::delete_stmt(Statement **stm) {
		Statement *curr = *stm;
		Statement *temp = nullptr;
		if (*stm == nullptr)
			return;
		while (curr != nullptr) {
			temp = curr->p_next;
			delete_label_stmt(&curr->labled_statement);
			delete_expr_stmt(&curr->expression_statement);
			delete_select_stmt(&curr->selection_statement);
			delete_iter_stmt(&curr->iteration_statement);
			delete_jump_stmt(&curr->jump_statement);
			curr->p_prev = nullptr;
			curr->p_next = nullptr;
			delete curr;
			curr = temp;
		}
	}
	
	void Tree::delete_tree(TreeNode **tr) {
		TreeNode *curr = *tr;
		TreeNode *temp = nullptr;
		while (curr != nullptr) {
			temp = curr->p_next;
			delete_stmt(&curr->statement);
			if (curr->symtab != nullptr) {
				SymbolTable::delete_node(&curr->symtab);
			}
			curr->p_prev = nullptr;
			curr->p_next = nullptr;
			delete curr;
			curr = temp;
		}
	}
	
	void Tree::delete_tree_node(TreeNode **trn) {
		if (*trn == nullptr)
			return;
		delete_stmt(&((*trn)->statement));
		(*trn)->p_next = nullptr;
		(*trn)->p_prev = nullptr;
		delete *trn;
		*trn = nullptr;
	}
	
	void Tree::add_asm_statement(AsmStatement **ststart, AsmStatement **asmstmt) {
		AsmStatement *temp = *ststart;
		
		if (temp == nullptr) {
			*ststart = *asmstmt;
			return;
		}
		
		while (temp->p_next != nullptr)
			temp = temp->p_next;
		
		temp->p_next = *asmstmt;
		
	}
	
	void Tree::add_statement(Statement **ststart, Statement **_stmt) {
		Statement *temp = *ststart;
		
		if (temp == nullptr) {
			*ststart = *_stmt;
			return;
		}
		
		while (temp->p_next != nullptr)
			temp = temp->p_next;
		
		(*_stmt)->p_prev = temp;
		temp->p_next = *_stmt;
		
	}
	
	void Tree::add_tree_node(TreeNode **trstart, TreeNode **_trn) {
		TreeNode *temp = *trstart;
		
		if (temp == nullptr) {
			*trstart = *_trn;
			return;
		}
		
		while (temp->p_next != nullptr)
			temp = temp->p_next;
		
		(*_trn)->p_prev = temp;
		temp->p_next = *_trn;
	}
	
}
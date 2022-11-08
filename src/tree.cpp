/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*
* Contains Abstract Syntax Tree(AST) related functions
* such as memory allocation/deallocation, insert/delete etc.
*/

#include <list>
#include "tree.hpp"
#include "log.hpp"

using namespace xlang;

struct sizeof_expr *xlang::tree::get_sizeof_expr_mem() {
	sizeof_expr *newexpr = new sizeof_expr();
	return newexpr;
}

void xlang::tree::delete_sizeof_expr(struct sizeof_expr **soexpr) {
	if (*soexpr == nullptr)
		return;
	delete *soexpr;
	*soexpr = nullptr;
}

struct cast_expr *xlang::tree::get_cast_expr_mem() {
	cast_expr *newexpr = new cast_expr();
	return newexpr;
}

void xlang::tree::delete_cast_expr(struct cast_expr **cexpr) {
	if (*cexpr == nullptr)
		return;
	delete *cexpr;
	*cexpr = nullptr;
}

struct primary_expr *xlang::tree::get_primary_expr_mem() {
	primary_expr *newexpr = new primary_expr();
	newexpr->id_info = nullptr;
	newexpr->left = nullptr;
	newexpr->right = nullptr;
	newexpr->unary_node = nullptr;
	return newexpr;
}

std::stack<primary_expr *> tree::pexpr_stack;

void xlang::tree::get_inorder_primary_expr(primary_expr **pexpr) {
	primary_expr *pexp = *pexpr;
	if (pexp == nullptr)
		return;

	pexpr_stack.push(pexp);
	get_inorder_primary_expr(&pexp->left);
	get_inorder_primary_expr(&pexp->right);
}

void xlang::tree::delete_primary_expr(primary_expr **pexpr) {
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

struct id_expr *xlang::tree::get_id_expr_mem() {
	id_expr *newexpr = new id_expr();
	newexpr->id_info = nullptr;
	newexpr->left = nullptr;
	newexpr->right = nullptr;
	newexpr->unary = nullptr;
	return newexpr;
}

void xlang::tree::delete_id_expr(id_expr **idexpr) {
	if (*idexpr == nullptr)
		return;
	delete_id_expr(&((*idexpr)->left));
	delete_id_expr(&((*idexpr)->right));
	delete_id_expr(&((*idexpr)->unary));

	delete *idexpr;
	*idexpr = nullptr;
}

struct expr *xlang::tree::get_expr_mem() {
	expr *newexpr = new struct expr;
	newexpr->primary_expression = nullptr;
	newexpr->sizeof_expression = nullptr;
	newexpr->cast_expression = nullptr;
	newexpr->id_expression = nullptr;
	newexpr->assgn_expression = nullptr;
	newexpr->func_call_expression = nullptr;
	return newexpr;
}

void xlang::tree::delete_expr(expr **exp) {
	expr *exp2 = *exp;
	if (*exp == nullptr)
		return;
	switch (exp2->expr_kind) {
		case PRIMARY_EXPR :
			delete_primary_expr(&exp2->primary_expression);
			break;
		case ASSGN_EXPR :
			delete_assgn_expr(&exp2->assgn_expression);
			break;
		case SIZEOF_EXPR :
			delete_sizeof_expr(&exp2->sizeof_expression);
			break;
		case CAST_EXPR :
			delete_cast_expr(&exp2->cast_expression);
			break;
		case ID_EXPR :
			delete_id_expr(&exp2->id_expression);
			break;
		case FUNC_CALL_EXPR :
			delete_func_call_expr(&exp2->func_call_expression);
			break;
	}
	delete *exp;
	*exp = nullptr;
}

struct assgn_expr *xlang::tree::get_assgn_expr_mem() {
	assgn_expr *newexpr = new assgn_expr();
	newexpr->id_expression = nullptr;
	newexpr->expression = nullptr;
	return newexpr;
}

void xlang::tree::delete_assgn_expr(struct assgn_expr **exp) {
	if (*exp == nullptr)
		return;
	delete_id_expr(&(*exp)->id_expression);
	delete_expr(&(*exp)->expression);
	*exp = nullptr;
}

struct func_call_expr *xlang::tree::get_func_call_expr_mem() {
	func_call_expr *newexpr = new func_call_expr();
	newexpr->function = nullptr;
	return newexpr;
}

void xlang::tree::delete_func_call_expr(struct func_call_expr **exp) {
	func_call_expr *exp2 = *exp;
	if (exp2 == nullptr)
		return;
	delete_id_expr(&exp2->function);
	std::list<expr *>::iterator lst = (exp2->expression_list).begin();
	while (lst != (exp2->expression_list).end()) {
		delete *lst;
		lst++;
	}
	delete *exp;
	*exp = nullptr;
}

struct st_asm_operand *xlang::tree::get_asm_operand_mem() {
	st_asm_operand *asmop = new st_asm_operand();
	asmop->expression = nullptr;
	return asmop;
}

void xlang::tree::delete_asm_operand(st_asm_operand **asmop) {
	if (*asmop == nullptr)
		return;
	delete_expr(&((*asmop)->expression));
	delete *asmop;
	*asmop = nullptr;
}

struct labled_stmt *xlang::tree::get_label_stmt_mem() {
	labled_stmt *newstmt = new labled_stmt();
	return newstmt;
}

struct expr_stmt *xlang::tree::get_expr_stmt_mem() {
	expr_stmt *newstmt = new expr_stmt();
	newstmt->expression = nullptr;
	return newstmt;
}

struct select_stmt *xlang::tree::get_select_stmt_mem() {
	select_stmt *newstmt = new select_stmt();
	newstmt->condition = nullptr;
	newstmt->else_statement = nullptr;
	newstmt->if_statement = nullptr;
	return newstmt;
}

struct iter_stmt *xlang::tree::get_iter_stmt_mem() {
	iter_stmt *newstmt = new iter_stmt();
	newstmt->_while.condition = nullptr;
	newstmt->_while.statement = nullptr;
	newstmt->_dowhile.condition = nullptr;
	newstmt->_dowhile.statement = nullptr;
	newstmt->_for.init_expression = nullptr;
	newstmt->_for.condition = nullptr;
	newstmt->_for.update_expression = nullptr;
	newstmt->_for.statement = nullptr;
	return newstmt;
}

struct jump_stmt *xlang::tree::get_jump_stmt_mem() {
	jump_stmt *newstmt = new jump_stmt();
	newstmt->expression = nullptr;
	return newstmt;
}

struct asm_stmt *xlang::tree::get_asm_stmt_mem() {
	asm_stmt *asmstmt = new asm_stmt;
	asmstmt->p_next = nullptr;
	return asmstmt;
}

struct stmt *xlang::tree::get_stmt_mem() {
	stmt *newstmt = new stmt();
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

struct tree_node *xlang::tree::get_tree_node_mem() {
	tree_node *newtr = new tree_node();
	newtr->symtab = xlang::symtable::get_node_mem();
	newtr->statement = nullptr;
	newtr->p_next = nullptr;
	newtr->p_prev = nullptr;
	return newtr;
}

void xlang::tree::delete_label_stmt(labled_stmt **lbstmt) {
	if (*lbstmt == nullptr)
		return;
	delete *lbstmt;
	*lbstmt = nullptr;
}

void xlang::tree::delete_asm_stmt(asm_stmt **asmstmt) {
	std::vector<st_asm_operand *>::iterator it;
	asm_stmt *temp = *asmstmt;
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

void xlang::tree::delete_expr_stmt(expr_stmt **expstmt) {
	if (*expstmt == nullptr)
		return;
	delete_expr(&((*expstmt)->expression));
	delete *expstmt;
	*expstmt = nullptr;
}

void xlang::tree::delete_select_stmt(select_stmt **selstmt) {
	if (*selstmt == nullptr)
		return;
	delete_expr(&((*selstmt)->condition));
	delete_stmt(&((*selstmt)->if_statement));
	delete_stmt(&((*selstmt)->else_statement));
	delete *selstmt;
	*selstmt = nullptr;
}

void xlang::tree::delete_iter_stmt(iter_stmt **itstmt) {
	if (*itstmt == nullptr)
		return;
	switch ((*itstmt)->type) {
		case WHILE_STMT :
			delete_expr(&((*itstmt)->_while.condition));
			delete_stmt(&((*itstmt)->_while.statement));
			break;
		case DOWHILE_STMT :
			delete_expr(&((*itstmt)->_dowhile.condition));
			delete_stmt(&((*itstmt)->_dowhile.statement));
			break;
		case FOR_STMT :
			delete_expr(&((*itstmt)->_for.init_expression));
			delete_expr(&((*itstmt)->_for.condition));
			delete_expr(&((*itstmt)->_for.update_expression));
			delete_stmt(&((*itstmt)->_for.statement));
			break;
	}
	delete *itstmt;
	*itstmt = nullptr;
}

void xlang::tree::delete_jump_stmt(jump_stmt **jmpstmt) {
	if (*jmpstmt == nullptr)
		return;
	delete_expr(&((*jmpstmt)->expression));
	delete *jmpstmt;
	*jmpstmt = nullptr;
}

void xlang::tree::delete_stmt(stmt **stm) {
	stmt *curr = *stm;
	stmt *temp = nullptr;
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

void xlang::tree::delete_tree(tree_node **tr) {
	tree_node *curr = *tr;
	tree_node *temp = nullptr;
	while (curr != nullptr) {
		temp = curr->p_next;
		delete_stmt(&curr->statement);
		if (curr->symtab != nullptr) {
			xlang::symtable::delete_node(&curr->symtab);
		}
		curr->p_prev = nullptr;
		curr->p_next = nullptr;
		delete curr;
		curr = temp;
	}
}

void xlang::tree::delete_tree_node(tree_node **trn) {
	if (*trn == nullptr)
		return;
	delete_stmt(&((*trn)->statement));
	(*trn)->p_next = nullptr;
	(*trn)->p_prev = nullptr;
	delete *trn;
	*trn = nullptr;
}

void xlang::tree::add_asm_statement(asm_stmt **ststart, asm_stmt **asmstmt) {
	asm_stmt *temp = *ststart;

	if (temp == nullptr) {
		*ststart = *asmstmt;
		return;
	}

	while (temp->p_next != nullptr)
		temp = temp->p_next;

	temp->p_next = *asmstmt;

}

void xlang::tree::add_statement(stmt **ststart, stmt **_stmt) {
	stmt *temp = *ststart;

	if (temp == nullptr) {
		*ststart = *_stmt;
		return;
	}

	while (temp->p_next != nullptr)
		temp = temp->p_next;

	(*_stmt)->p_prev = temp;
	temp->p_next = *_stmt;

}

void xlang::tree::add_tree_node(tree_node **trstart, tree_node **_trn) {
	tree_node *temp = *trstart;

	if (temp == nullptr) {
		*trstart = *_trn;
		return;
	}

	while (temp->p_next != nullptr)
		temp = temp->p_next;

	(*_trn)->p_prev = temp;
	temp->p_next = *_trn;

}





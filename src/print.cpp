/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


#include "tree.hpp"

namespace xlang {

	void labled_stmt::print() {
		std::cout << "------------ labled statement -----------------" << std::endl;
		std::cout << "ptr : " << this << std::endl;
		std::cout << "label : " << label.string << std::endl;
		std::cout << "-----------------------------------------------" << std::endl;
	}

	void id_expr::print() {
		std::cout << "{\n";
		std::cout << "  string : " << tok.string << std::endl;
		std::cout << "  is_oprtr : " << (is_oprtr ? "true" : "false") << std::endl;
		std::cout << "  is_id : " << (is_id ? "true" : "false") << std::endl;
		std::cout << "  id_info : " << id_info << std::endl;
		std::cout << "  is_subscript : " << (is_subscript ? "true" : "false");

		for (const auto &it: subscript) {
			std::cout << " [" << it.string << "]";
		}

		std::cout << std::endl;
		std::cout << "  is_ptr : " << (is_ptr ? "true" : "false") << std::endl;
		std::cout << "  ptr_oprtr_count : " << ptr_oprtr_count << std::endl;
		std::cout << "  this : " << this << std::endl;
		std::cout << "  left : " << left << std::endl;
		std::cout << "  right : " << right << std::endl;
		std::cout << "  unary : " << unary << std::endl;
		std::cout << "}\n";
//
//		left->print();
//		right->print();
//		unary->print();
	}

	void sizeof_expr::print() {
		//size_t i;
		std::cout << "(sizeof expression : " << this << ")" << std::endl;
		std::cout << "  is_simple_type : " << (is_simple_type) << std::endl;
		std::cout << "  simple_type : ";
		for (const auto &st: simple_type) {
			std::cout << st.string << " ";
		}
		std::cout << std::endl;
		std::cout << "  identifier : " << identifier.string << std::endl;
		std::cout << "  is_ptr : " << is_ptr << std::endl;
		std::cout << "  ptr_oprtr_count : " << ptr_oprtr_count << std::endl;
	}

	void cast_expr::print() {
		//size_t i;
		std::cout << "(cast expression : " << this << ")" << std::endl;
		std::cout << "  is_simple_type : " << (is_simple_type ? "true" : "false") << std::endl;
		std::cout << "  simple_type : ";
		for (const auto &st: simple_type)
			std::cout << st.string << " ";
		std::cout << std::endl;
		std::cout << "  identifier = " << identifier.string << std::endl;
		std::cout << "  ptr_oprtr_count = " << ptr_oprtr_count << std::endl;
		std::cout << "  target : " << std::endl;
		target->print();
	}

	void assgn_expr::print() {
		std::cout << "(assgn expression : " << this << ")" << std::endl;
		std::cout << "{\n";
		std::cout << "  tok : " << tok.string << std::endl;
		std::cout << "  id_expression : " << id_expression << std::endl;
		std::cout << "  expression : " << expression << std::endl;
		std::cout << "}\n";
		if (id_expression == nullptr)
			return;
		if (expression == nullptr)
			return;
		id_expression->print();
		expression->print();
	}

	void expr_stmt::print() {
		std::cout << "------------ expression statement -----------------" << std::endl;
		std::cout << "ptr : " << this << std::endl;
		std::cout << "expression : " << expression << std::endl;
		expression->print();
		std::cout << "---------------------------------------------------" << std::endl;
	}

	void primary_expr::print() {
		std::cout << "{\n";
		std::cout << "  string : " << tok.string << std::endl;
		std::cout << "  token : " << tok.number << std::endl;
		std::cout << "  is_oprtr : " << (is_oprtr ? "true" : "false") << std::endl;
		std::cout << "  oprtr_kind : " << oprtr_kind << std::endl;
		std::cout << "  is_id : " << (is_id) << std::endl;
		std::cout << "  this : " << this << std::endl;
		std::cout << "  left : " << left << std::endl;
		std::cout << "  right : " << right << std::endl;
		std::cout << "  unary_node : " << unary_node << std::endl;
		std::cout << "}\n";
	}

	void stmt::print() {
		stmt *curr = this;
		while (curr != nullptr) {
			std::cout << "||||||||||||||||||||||| statement ||||||||||||||||||||" << std::endl;
			std::cout << "ptr : " << curr << std::endl;
			std::cout << "type : " << curr->type << std::endl;
			std::cout << "labled_statement : " << curr->labled_statement << std::endl;
			std::cout << "expression_statement : " << curr->expression_statement << std::endl;
			std::cout << "selection_statement : " << curr->selection_statement << std::endl;
			std::cout << "iteration_statement : " << curr->iteration_statement << std::endl;
			std::cout << "jump_statement : " << curr->jump_statement << std::endl;
			std::cout << "asm statement : " << curr->asm_statement << std::endl;
			std::cout << "p_next : " << curr->p_next << std::endl;
			std::cout << "p_prev : " << curr->p_prev << std::endl;
			switch (curr->type) {
				case LABEL_STMT :
					curr->labled_statement->print();
					break;
				case EXPR_STMT :
					curr->expression_statement->print();
					break;
				case SELECT_STMT :
					curr->selection_statement->print();
					break;
				case ITER_STMT :
					curr->iteration_statement->print();
					break;
				case JUMP_STMT :
					curr->jump_statement->print();
					break;
				case ASM_STMT :
					curr->asm_statement->print();
					break;

				default:
					break;
			}

			std::cout << "||||||||||||||||||||||||||||||||||||||||||||||||||||||" << std::endl;
			curr = curr->p_next;
		}
	}

	void func_call_expr::print() {
		std::cout << "(func call expression : " << this << ")" << std::endl;
		std::cout << "{\n";
		std::cout << "  function : " << function << std::endl;
		for (auto &e: expression_list) {
			e->print();
		}

		std::cout << "}\n";
		function->print();
		for (auto &e: expression_list) {
			e->print();
		}
	}

	void select_stmt::print() {
		std::cout << "------------- selection statement -----------------" << std::endl;
		std::cout << "ptr : " << this << std::endl;
		std::cout << "iftok : " << iftok.string << std::endl;
		std::cout << "elsetok : " << elsetok.string << std::endl;
		std::cout << "condition : " << condition << std::endl;
		std::cout << "if_statement : " << if_statement << std::endl;
		std::cout << "else_statement : " << else_statement << std::endl;
		condition->print();
		if_statement->print();
		else_statement->print();
		std::cout << "---------------------------------------------------" << std::endl;
	}

	void iter_stmt::print() {
		std::cout << "------------ iteration statement -----------------" << std::endl;
		std::cout << "ptr : " << this << std::endl;
		std::cout << "type : " << type << std::endl;
		switch (type) {
			case WHILE_STMT :
				std::cout << "whiletok : " << _while.whiletok.string << std::endl;
				std::cout << "condition : " << _while.condition << std::endl;
				std::cout << "statement : " << _while.statement << std::endl;
				_while.condition->print();
				_while.statement->print();
				break;
			case DOWHILE_STMT :
				std::cout << "dotok : " << _dowhile.dotok.string << std::endl;
				std::cout << "whiletok : " << _dowhile.whiletok.string << std::endl;
				std::cout << "condition : " << _dowhile.condition << std::endl;
				std::cout << "statement : " << _dowhile.statement << std::endl;
				_dowhile.condition->print();
				_dowhile.statement->print();
				break;
			case FOR_STMT :
				std::cout << "fortok : " << _for.fortok.string << std::endl;
				std::cout << "init_expression : " << _for.init_expression << std::endl;
				std::cout << "condition : " << _for.condition << std::endl;
				std::cout << "update_expression : " << _for.update_expression << std::endl;
				std::cout << "statement : " << _for.statement << std::endl;
				_for.init_expression->print();
				_for.condition->print();
				_for.update_expression->print();
				_for.statement->print();
				break;
		}
		std::cout << "---------------------------------------------------" << std::endl;
	}

	void expr::print() {

		std::cout << "(expression : " << this << ")" << std::endl;
		switch (expr_kind) {
			case PRIMARY_EXPR :
				std::cout << "  [primary expression : " << primary_expression << "]" << std::endl;
				primary_expression->print();
				break;
			case ASSGN_EXPR :
				std::cout << "  [assignment expression : " << assgn_expression << "]" << std::endl;
				assgn_expression->print();
				break;
			case SIZEOF_EXPR :
				std::cout << "  [sizeof expression : " << sizeof_expression << "]" << std::endl;
				sizeof_expression->print();
				break;
			case CAST_EXPR :
				std::cout << "  [cast expression : " << cast_expression << "]" << std::endl;
				cast_expression->print();
				break;
			case ID_EXPR :
				std::cout << "  [id expression : " << id_expression << "]" << std::endl;
				id_expression->print();
				break;
			case FUNC_CALL_EXPR :
				std::cout << "funccall expression : " << func_call_expression << std::endl;
				func_call_expression->print();
				break;
			default:
				std::cout << "  [invalid expression to print]" << std::endl;
				break;
		}
	}

	void asm_stmt::print() {
		std::vector<st_asm_operand *>::iterator it;
		asm_stmt *temp = this;
		while (temp != nullptr) {
			std::cout << "--------------- asm statement ------------------" << std::endl;
			std::cout << "ptr : " << temp << std::endl;
			std::cout << "p_next : " << temp->p_next << std::endl;
			std::cout << "template : " << temp->asm_template.string << std::endl;
			std::cout << "~~~~~~~~~ output operand ~~~~~~~~~~" << std::endl;
			for (const auto &t: temp->output_operand) {
				t->print();
			}
			std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
			std::cout << "~~~~~~~~~ input operand ~~~~~~~~~~" << std::endl;
			it = temp->input_operand.begin();
			for (const auto &t: temp->input_operand) {
				t->print();
			}
			std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
			std::cout << "------------------------------------------------" << std::endl;
			temp = temp->p_next;
		}
	}

	void jump_stmt::print() {
		std::cout << "------------ jump statement -----------------" << std::endl;
		std::cout << "ptr : " << this << std::endl;
		std::cout << "type : " << type << std::endl;
		std::cout << "tok : " << tok.string << std::endl;
		std::cout << "expression : " << expression << std::endl;
		std::cout << "goto_id : " << goto_id.string << std::endl;
		expression->print();
		std::cout << "-----------------------------------------------" << std::endl;
	}

	void st_asm_operand::print() {
		std::cout << "constraint : " << constraint.string << std::endl;
		std::cout << "expression : " << expression << std::endl;
		expression->print();
	}
}
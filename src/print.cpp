/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


#include "tree.hpp"
#include "log.hpp"

namespace xlang {
	
	
	void st_node::print() {
		// TODO:: not implemented yet
	}
	
	void st_record_symtab::print() {
		// TODO:: not implemented yet
	}
	
	void labled_stmt::print() {
		
		log::line("------------ labled statement -----------------");
		log::line("ptr : ", this);
		log::line("label : ", label.string);
		log::line("-----------------------------------------------");
	}
	
	void id_expr_t::print() {
		log::line("{\n");
		log::line("  string : ", tok.string);
		log::line("  is_oprtr : ", (is_oprtr ? "true" : "false"));
		log::line("  is_id : ", (is_id ? "true" : "false"));
		log::line("  id_info : ", id_info);
		log::line("  is_subscript : ", (is_subscript ? "true" : "false"));
		
		for (const auto &it: subscript) {
			log::line(" [", it.string, "]");
		}
		
		log::line("");
		log::line("  is_ptr : ", (is_ptr ? "true" : "false"));
		log::line("  ptr_oprtr_count : ", ptr_oprtr_count);
		log::line("  this : ", this);
		log::line("  left : ", left);
		log::line("  right : ", right);
		log::line("  unary : ", unary);
		log::line("}");
		//
		//		left->print();
		//		right->print();
		//		unary->print();
	}
	
	void sizeof_expr_t::print() {
		//size_t i;
		log::line("(sizeof expression : ", this, ")");
		log::line("  is_simple_type : ", (is_simple_type));
		log::line("  simple_type : ");
		for (const auto &st: simple_type) {
			log::line(st.string, " ");
		}
		log::line("");
		log::line("  identifier : ", identifier.string);
		log::line("  is_ptr : ", is_ptr);
		log::line("  ptr_oprtr_count : ", ptr_oprtr_count);
	}
	
	void cast_expr_t::print() {
		//size_t i;
		log::line("(cast expression : ", this, ")");
		log::line("  is_simple_type : ", (is_simple_type ? "true" : "false"));
		log::line("  simple_type : ");
		for (const auto &st: simple_type)
			log::line(st.string, " ");
		log::line("");
		log::line("  identifier = ", identifier.string);
		log::line("  ptr_oprtr_count = ", ptr_oprtr_count);
		log::line("  target : ");
		target->print();
	}
	
	void assgn_expr_t::print() {
		log::line("(assgn expression : ", this, ")");
		log::line("{\n");
		log::line("  tok : ", tok.string);
		log::line("  id_expr : ", id_expr);
		log::line("  expression : ", expression);
		log::line("}\n");
		if (id_expr == nullptr)
			return;
		if (expression == nullptr)
			return;
		id_expr->print();
		expression->print();
	}
	
	void expr_stmt::print() {
		log::line("------------ expression statement -----------------");
		log::line("ptr : ", this);
		log::line("expression : ", expression);
		expression->print();
		log::line("---------------------------------------------------");
	}
	
	void primary_expr_t::print() {
		log::line("{");
		log::line("  string : ", tok.string);
		log::line("  token : ", tok.number);
		log::line("  is_oprtr : ", (is_oprtr ? "true" : "false"));
		log::line("  oprtr_kind : ", oprtr_kind);
		log::line("  is_id : ", (is_id));
		log::line("  this : ", this);
		log::line("  left : ", left);
		log::line("  right : ", right);
		log::line("  unary_node : ", unary_node);
		log::line("}");
	}
	
	void stmt::print() {
		stmt *curr = this;
		while (curr != nullptr) {
			log::line("||||||||||||||||||||||| statement ||||||||||||||||||||");
			log::line("ptr : ", curr);
			log::line("type : ", curr->type);
			log::line("labled_statement : ", curr->labled_statement);
			log::line("expression_statement : ", curr->expression_statement);
			log::line("selection_statement : ", curr->selection_statement);
			log::line("iteration_statement : ", curr->iteration_statement);
			log::line("jump_statement : ", curr->jump_statement);
			log::line("asm statement : ", curr->asm_statement);
			log::line("p_next : ", curr->p_next);
			log::line("p_prev : ", curr->p_prev);
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
			
			log::line("||||||||||||||||||||||||||||||||||||||||||||||||||||||");
			curr = curr->p_next;
		}
	}
	
	void call_expr_t::print() {
		log::line("(func call expression : ", this, ")");
		log::line("{");
		log::line("  function : ", function);
		for (auto &e: expression_list) {
			e->print();
		}
		
		log::line("}");
		function->print();
		for (auto &e: expression_list) {
			e->print();
		}
	}
	
	void select_stmt::print() {
		log::line("------------- selection statement -----------------");
		log::line("ptr : ", this);
		log::line("iftok : ", iftok.string);
		log::line("elsetok : ", elsetok.string);
		log::line("condition : ", condition);
		log::line("if_statement : ", if_statement);
		log::line("else_statement : ", else_statement);
		condition->print();
		if_statement->print();
		else_statement->print();
		log::line("---------------------------------------------------");
	}
	
	void iter_stmt::print() {
		log::line("------------ iteration statement -----------------");
		log::line("ptr : ", this);
		log::line("type : ", type);
		switch (type) {
			case WHILE_STMT :
				log::line("whiletok : ", _while.whiletok.string);
				log::line("condition : ", _while.condition);
				log::line("statement : ", _while.statement);
				_while.condition->print();
				_while.statement->print();
				break;
			case DOWHILE_STMT :
				log::line("dotok : ", _dowhile.dotok.string);
				log::line("whiletok : ", _dowhile.whiletok.string);
				log::line("condition : ", _dowhile.condition);
				log::line("statement : ", _dowhile.statement);
				_dowhile.condition->print();
				_dowhile.statement->print();
				break;
			case FOR_STMT :
				log::line("fortok : ", _for.fortok.string);
				log::line("init_expr : ", _for.init_expr);
				log::line("condition : ", _for.condition);
				log::line("update_expr : ", _for.update_expr);
				log::line("statement : ", _for.statement);
				_for.init_expr->print();
				_for.condition->print();
				_for.update_expr->print();
				_for.statement->print();
				break;
		}
		log::line("---------------------------------------------------");
	}
	
	void expr::print() {
		
		log::line("(expression : ", this, ")");
		switch (expr_kind) {
			case PRIMARY_EXPR :
				log::line("  [primary expression : ", primary_expr, "]");
				primary_expr->print();
				break;
			case ASSGN_EXPR :
				log::line("  [assignment expression : ", assgn_expr, "]");
				assgn_expr->print();
				break;
			case SIZEOF_EXPR :
				log::line("  [sizeof expression : ", sizeof_expr, "]");
				sizeof_expr->print();
				break;
			case CAST_EXPR :
				log::line("  [cast expression : ", cast_expr, "]");
				cast_expr->print();
				break;
			case ID_EXPR :
				log::line("  [id expression : ", id_expr, "]");
				id_expr->print();
				break;
			case FUNC_CALL_EXPR :
				log::line("funccall expression : ", call_expr);
				call_expr->print();
				break;
			default:
				log::line("  [invalid expression to print]");
				break;
		}
	}
	
	void asm_stmt::print() {
		std::vector<st_asm_operand *>::iterator it;
		asm_stmt *temp = this;
		while (temp != nullptr) {
			log::line("--------------- asm statement ------------------");
			log::line("ptr : ", temp);
			log::line("p_next : ", temp->p_next);
			log::line("template : ", temp->asm_template.string);
			log::line("~~~~~~~~~ output operand ~~~~~~~~~~");
			for (const auto &t: temp->output_operand) {
				t->print();
			}
			log::line("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
			log::line("~~~~~~~~~ input operand ~~~~~~~~~~");
			it = temp->input_operand.begin();
			for (const auto &t: temp->input_operand) {
				t->print();
			}
			log::line("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
			log::line("------------------------------------------------");
			temp = temp->p_next;
		}
	}
	
	void jump_stmt::print() {
		log::line("------------ jump statement -----------------");
		log::line("ptr : ", this);
		log::line("type : ", type);
		log::line("tok : ", tok.string);
		log::line("expression : ", expression);
		log::line("goto_id : ", goto_id.string);
		expression->print();
		log::line("-----------------------------------------------");
	}
	
	void st_asm_operand::print() {
		log::line("constraint : ", constraint.string);
		log::line("expression : ", expression);
		expression->print();
	}
}
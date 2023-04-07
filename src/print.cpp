/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


#include "tree.hpp"
#include "log.hpp"

namespace xlang {
	
	
	void Node::print() {
		// TODO:: not implemented yet
	}
	
	void RecordSymtab::print() {
		// TODO:: not implemented yet
	}
	
	void LabelStatement::print() {
		
		Log::line("------------ labled statement -----------------");
		Log::line("ptr : ", this);
		Log::line("label : ", label.string);
		Log::line("-----------------------------------------------");
	}
	
	void IdentifierExpression::print() {
		Log::line("{\n");
		Log::line("  string : ", tok.string);
		Log::line("  is_oprtr : ", (is_oprtr ? "true" : "false"));
		Log::line("  is_id : ", (is_id ? "true" : "false"));
		Log::line("  id_info : ", id_info);
		Log::line("  is_subscript : ", (is_subscript ? "true" : "false"));
		
		for (const auto &it: subscript) {
			Log::line(" [", it.string, "]");
		}
		
		Log::line("");
		Log::line("  is_ptr : ", (is_ptr ? "true" : "false"));
		Log::line("  ptr_oprtr_count : ", ptr_oprtr_count);
		Log::line("  this : ", this);
		Log::line("  left : ", left);
		Log::line("  right : ", right);
		Log::line("  unary : ", unary);
		Log::line("}");
		//
		//		left->print();
		//		right->print();
		//		unary->print();
	}
	
	void SizeOfExpression::print() {
		//size_t i;
		Log::line("(sizeof expression : ", this, ")");
		Log::line("  is_simple_type : ", (is_simple_type));
		Log::line("  simple_type : ");
		for (const auto &st: simple_type) {
			Log::line(st.string, " ");
		}
		Log::line("");
		Log::line("  identifier : ", identifier.string);
		Log::line("  is_ptr : ", is_ptr);
		Log::line("  ptr_oprtr_count : ", ptr_oprtr_count);
	}
	
	void CastExpression::print() {
		//size_t i;
		Log::line("(cast expression : ", this, ")");
		Log::line("  is_simple_type : ", (is_simple_type ? "true" : "false"));
		Log::line("  simple_type : ");
		for (const auto &st: simple_type)
			Log::line(st.string, " ");
		Log::line("");
		Log::line("  identifier = ", identifier.string);
		Log::line("  ptr_oprtr_count = ", ptr_oprtr_count);
		Log::line("  target : ");
		target->print();
	}
	
	void AssignmentExpression::print() {
		Log::line("(assgn expression : ", this, ")");
		Log::line("{\n");
		Log::line("  tok : ", tok.string);
		Log::line("  id_expr : ", id_expr);
		Log::line("  expression : ", expression);
		Log::line("}\n");
		if (id_expr == nullptr)
			return;
		if (expression == nullptr)
			return;
		id_expr->print();
		expression->print();
	}
	
	void ExpressionStatement::print() {
		Log::line("------------ expression statement -----------------");
		Log::line("ptr : ", this);
		Log::line("expression : ", expression);
		expression->print();
		Log::line("---------------------------------------------------");
	}
	
	void PrimaryExpression::print() {
		Log::line("{");
		Log::line("  string : ", tok.string);
		Log::line("  Token : ", tok.number);
		Log::line("  is_oprtr : ", (is_oprtr ? "true" : "false"));
		Log::line("  oprtr_kind : ", (uint8_t)oprtr_kind);
		Log::line("  is_id : ", (is_id));
		Log::line("  this : ", this);
		Log::line("  left : ", left);
		Log::line("  right : ", right);
		Log::line("  unary_node : ", unary_node);
		Log::line("}");
	}
	
	void Statement::print() {
		Statement *curr = this;
		while (curr != nullptr) {
			Log::line("||||||||||||||||||||||| statement ||||||||||||||||||||");
			Log::line("ptr : ", curr);
			Log::line("type : ",(uint8_t) curr->type);
			Log::line("labled_statement : ", curr->labled_statement);
			Log::line("expression_statement : ", curr->expression_statement);
			Log::line("selection_statement : ", curr->selection_statement);
			Log::line("iteration_statement : ", curr->iteration_statement);
			Log::line("jump_statement : ", curr->jump_statement);
			Log::line("asm statement : ", curr->asm_statement);
			Log::line("p_next : ", curr->p_next);
			Log::line("p_prev : ", curr->p_prev);
			switch (curr->type) {
				case StatementType::LABEL :
					curr->labled_statement->print();
					break;
				case StatementType::EXPR :
					curr->expression_statement->print();
					break;
				case StatementType::SELECT :
					curr->selection_statement->print();
					break;
				case StatementType::ITER :
					curr->iteration_statement->print();
					break;
				case StatementType::JUMP :
					curr->jump_statement->print();
					break;
				case StatementType::ASM :
					curr->asm_statement->print();
					break;
				
				default:
					break;
			}
			
			Log::line("||||||||||||||||||||||||||||||||||||||||||||||||||||||");
			curr = curr->p_next;
		}
	}
	
	void CallExpression::print() {
		Log::line("(func call expression : ", this, ")");
		Log::line("{");
		Log::line("  function : ", function);
		for (auto &e: expression_list) {
			e->print();
		}
		
		Log::line("}");
		function->print();
		for (auto &e: expression_list) {
			e->print();
		}
	}
	
	void SelectStatement::print() {
		Log::line("------------- selection statement -----------------");
		Log::line("ptr : ", this);
		Log::line("iftok : ", iftok.string);
		Log::line("elsetok : ", elsetok.string);
		Log::line("condition : ", condition);
		Log::line("if_statement : ", if_statement);
		Log::line("else_statement : ", else_statement);
		condition->print();
		if_statement->print();
		else_statement->print();
		Log::line("---------------------------------------------------");
	}
	
	void IterationStatement::print() {
		Log::line("------------ iteration statement -----------------");
		Log::line("ptr : ", this);
		Log::line("type : ", (uint8_t)type);
		switch (type) {
			case IterationType::WHILE :
				Log::line("whiletok : ", _while.whiletok.string);
				Log::line("condition : ", _while.condition);
				Log::line("statement : ", _while.statement);
				_while.condition->print();
				_while.statement->print();
				break;
			case IterationType::DOWHILE :
				Log::line("dotok : ", _dowhile.dotok.string);
				Log::line("whiletok : ", _dowhile.whiletok.string);
				Log::line("condition : ", _dowhile.condition);
				Log::line("statement : ", _dowhile.statement);
				_dowhile.condition->print();
				_dowhile.statement->print();
				break;
			case IterationType::FOR :
				Log::line("fortok : ", _for.fortok.string);
				Log::line("init_expr : ", _for.init_expr);
				Log::line("condition : ", _for.condition);
				Log::line("update_expr : ", _for.update_expr);
				Log::line("statement : ", _for.statement);
				_for.init_expr->print();
				_for.condition->print();
				_for.update_expr->print();
				_for.statement->print();
				break;
		}
		Log::line("---------------------------------------------------");
	}
	
	void Expression::print() {
		
		Log::line("(expression : ", this, ")");
		switch (expr_kind) {
			case ExpressionType::PRIMARY_EXPR :
				Log::line("  [primary expression : ", primary_expr, "]");
				primary_expr->print();
				break;
			case ExpressionType::ASSGN_EXPR :
				Log::line("  [assignment expression : ", assgn_expr, "]");
				assgn_expr->print();
				break;
			case ExpressionType::SIZEOF_EXPR :
				Log::line("  [sizeof expression : ", sizeof_expr, "]");
				sizeof_expr->print();
				break;
			case ExpressionType::CAST_EXPR :
				Log::line("  [cast expression : ", cast_expr, "]");
				cast_expr->print();
				break;
			case ExpressionType::ID_EXPR :
				Log::line("  [id expression : ", id_expr, "]");
				id_expr->print();
				break;
			case ExpressionType::FUNC_CALL_EXPR :
				Log::line("funccall expression : ", call_expr);
				call_expr->print();
				break;
			default:
				Log::line("  [invalid expression to print]");
				break;
		}
	}
	
	void AsmStatement::print() {
		std::vector<AsmOperand *>::iterator it;
		AsmStatement *temp = this;
		while (temp != nullptr) {
			Log::line("--------------- asm statement ------------------");
			Log::line("ptr : ", temp);
			Log::line("p_next : ", temp->p_next);
			Log::line("template : ", temp->asm_template.string);
			Log::line("~~~~~~~~~ output Operand ~~~~~~~~~~");
			for (const auto &t: temp->output_operand) {
				t->print();
			}
			Log::line("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
			Log::line("~~~~~~~~~ input Operand ~~~~~~~~~~");
			it = temp->input_operand.begin();
			for (const auto &t: temp->input_operand) {
				t->print();
			}
			Log::line("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
			Log::line("------------------------------------------------");
			temp = temp->p_next;
		}
	}
	
	void JumpStatement::print() {
		Log::line("------------ jump statement -----------------");
		Log::line("ptr : ", this);
		Log::line("type : ", (uint8_t)type);
		Log::line("tok : ", tok.string);
		Log::line("expression : ", expression);
		Log::line("goto_id : ", goto_id.string);
		expression->print();
		Log::line("-----------------------------------------------");
	}
	
	void AsmOperand::print() {
		Log::line("constraint : ", constraint.string);
		Log::line("expression : ", expression);
		expression->print();
	}
}
/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

// optimizing compiler functions

#include <vector>
#include <stack>
#include <math.h>
#include "token.hpp"
#include "lex.hpp"
#include "tree.hpp"
#include "log.hpp"
#include "convert.hpp"
#include "symtab.hpp"
#include "parser.hpp"
#include "optimize.hpp"
#include "compiler.hpp"

namespace xlang {
	
	/*
	evaluate an expression with two factors(f1,f2) and operator op
	evaluation result is always a double, depends on has_float parameter
	*/
	bool optimizer::evaluate(token &f1, token &f2, token &op, std::string &stresult, bool has_float) {
		double d1 = 0.0, d2 = 0.0, result = 0.0;
		bool bres = false;
		
		if (has_float) {
			d1 = std::stod(f1.string);
			d2 = std::stod(f2.string);
		} else {
			d1 = static_cast<double>(xlang::get_decimal(f1));
			d2 = static_cast<double>(xlang::get_decimal(f2));
		}
		switch (op.number) {
			case ARTHM_ADD :
				result = d1 + d2;
				bres = true;
				break;
			case ARTHM_SUB :
				result = d1 - d2;
				bres = true;
				break;
			case ARTHM_MUL :
				result = d1 * d2;
				bres = true;
				break;
			case ARTHM_DIV :
				if (d2 == 0) {
					log::error("divide by zero found in optimization");
					bres = false;
				} else {
					result = d1 / d2;
					bres = true;
				}
				break;
			case ARTHM_MOD :
				result = static_cast<int>(d1) % static_cast<int>(d2);
				bres = true;
				break;
			default:
				log::error("invalid operator found in optimization '" + op.string + "'");
				bres = false;
				break;
		}
		
		stresult = std::to_string(result);
		if (bres)
			return true;
		else
			return false;
	}
	
	void optimizer::clear_primary_expr_stack() {
		clear_stack(pexpr_stack);
	}
	
	/*
	returns true if any node of primary expression
	has float literal or float/double data type
	*/
	bool optimizer::has_float_type(primary_expr_t *pexpr) {
		token type;
		if (pexpr == nullptr)
			return false;
		if (pexpr->is_id) {
			if (pexpr->id_info->type_info->type == SIMPLE_TYPE) {
				type = pexpr->id_info->type_info->type_specifier.simple_type[0];
				if (type.number == KEY_FLOAT || type.number == KEY_DOUBLE) {
					return true;
				} else {
					return (has_float_type(pexpr->left) || has_float_type(pexpr->right));
				}
			} else {
				return (has_float_type(pexpr->left) || has_float_type(pexpr->right));
			}
		} else if (pexpr->is_oprtr) {
			return (has_float_type(pexpr->left) || has_float_type(pexpr->right));
		} else {
			if (pexpr->tok.number == LIT_FLOAT) {
				return true;
			} else {
				return (has_float_type(pexpr->left) || has_float_type(pexpr->right));
			}
		}
		return false;
	}
	
	//returns true if any identifier found in primary expression
	bool optimizer::has_id(primary_expr_t *pexpr) {
		if (pexpr == nullptr)
			return false;
		if (pexpr->is_id) {
			return true;
		} else if (pexpr->is_oprtr) {
			return (has_id(pexpr->left) || has_id(pexpr->right));
		} else {
			return (has_id(pexpr->left) || has_id(pexpr->right));
		}
		return false;
	}
	
	void optimizer::get_inorder_primary_expr(primary_expr_t **pexpr) {
		primary_expr_t *pexp = *pexpr;
		if (pexp == nullptr)
			return;
		
		pexpr_stack.push(pexp);
		get_inorder_primary_expr(&pexp->left);
		get_inorder_primary_expr(&pexp->right);
	}
	
	/*
	traverse primary expr tree for constant expressions,
	if found any then fold it
	*/
	void optimizer::id_constant_folding(primary_expr_t **pexpr) {
		if (*pexpr == nullptr)
			return;
		if (!has_id(*pexpr)) {
			constant_folding(&(*pexpr));
		}
		id_constant_folding(&((*pexpr)->left));
		id_constant_folding(&((*pexpr)->right));
	}
	
	/*
	constant folding optimization on primary expression
	*/
	void optimizer::constant_folding(primary_expr_t **pexpr) {
		primary_expr_t *pexp = *pexpr;
		token fact1, fact2, opr, restok;
		primary_expr_t *temp = nullptr;
		std::string stresult;
		int result = 0;
		std::stack<token> pexp_eval;
		bool has_float = has_float_type(pexp);
		if (pexp == nullptr)
			return;
		
		if (has_id(pexp)) {
			id_constant_folding(&(*pexpr));
			return;
		}
		
		//get inorder primary expr into pexpr_stack
		get_inorder_primary_expr(&pexp);
		
		while (!pexpr_stack.empty()) {
			temp = pexpr_stack.top();
			opr = temp->tok;
			if (temp->is_oprtr) {
				if (pexp_eval.size() > 1) {
					fact1 = pexp_eval.top();
					pexp_eval.pop();
					fact2 = pexp_eval.top();
					pexp_eval.pop();
					
					if (evaluate(fact1, fact2, opr, stresult, has_float)) {
						restok.loc = opr.loc;
						if (has_float) {
							restok.number = LIT_FLOAT;
							restok.string = stresult;
						} else {
							result = std::stoi(stresult);
							if (result < 0) {
								stresult = xlang::decimal_to_hex(result);
								restok.number = LIT_HEX;
								restok.string = "0x" + stresult;
							} else {
								restok.number = LIT_DECIMAL;
								restok.string = std::to_string(result);
							}
						}
						pexp_eval.push(restok);
					}
				}
			} else {
				pexp_eval.push(opr);
			}
			
			pexpr_stack.pop();
		}
		
		//delete whole sub-expression tree, add new node with evaluated result
		if (pexp_eval.size() > 0) {
			restok = pexp_eval.top();
			xlang::tree::delete_primary_expr(&(*pexpr));
			*pexpr = nullptr;
			*pexpr = xlang::tree::get_primary_expr_mem();
			(*pexpr)->is_id = false;
			(*pexpr)->is_oprtr = false;
			(*pexpr)->tok = restok;
			pexp_eval.pop();
		}
		
		clear_primary_expr_stack();
	}
	
	//returns true of both stacks are equals by primary expr lexeme values
	bool optimizer::equals(std::stack<primary_expr_t *> st1, std::stack<primary_expr_t *> st2) {
		int result = 1;
		if (st1.size() != st2.size())
			return false;
		while (!st1.empty()) {
			if (st1.top()->tok.string == st2.top()->tok.string)
				result &= 1;
			else
				result &= 0;
			
			if (st1.empty())
				break;
			st1.pop();
			if (st2.empty())
				break;
			st2.pop();
		}
		
		if (result == 1)
			return true;
		else
			return false;
	}
	
	/*
	search cmn1 expr node in primary expr tree
	if found then return its address from tree
	*/
	primary_expr_t *optimizer::get_cmnexpr1_node(primary_expr_t **root, primary_expr_t **cmn1) {
		if (*root == nullptr)
			return nullptr;
		if ((*root)->left != nullptr) {
			if ((*root)->left == *cmn1) {
				return (*root)->left;
			} else {
				get_cmnexpr1_node(&(*root)->left, &(*cmn1));
				get_cmnexpr1_node(&(*root)->left->right, &(*cmn1));
			}
		}
		return nullptr;
	}
	
	/*
	cmn1 is common subexpression from right side of a tree
	cmn2 is common subexpression from left side of a tree
	traverse primary expr tree, search for common expression
	in tree by calling get_cmnexpr1_node(), delete its right subtree,
	and set its right side pointer to received common subexpr node
	by function get_cmnexpr1_node()
	*/
	void optimizer::change_subexpr_pointers(primary_expr_t **root, primary_expr_t **cmn1, primary_expr_t **cmn2) {
		primary_expr_t *node1 = nullptr;
		if (*root == nullptr)
			return;
		if ((*root)->right != nullptr) {
			node1 = get_cmnexpr1_node(&(*root), &(*cmn2));
			if ((*root)->right == *cmn1) {
				xlang::tree::delete_primary_expr(&(*root)->right);
				(*root)->right = node1;
				return;
			} else {
				change_subexpr_pointers(&(*root)->left, &(*cmn1), &(*cmn2));
				change_subexpr_pointers(&(*root)->right, &(*cmn1), &(*cmn2));
			}
		}
	}
	
	/*
	common subexpression elimination optimization
	traverse tree, putting each node on stack, and then stack are compared
	it can only optimize simple two factors expression(e.g: (a+b)*(a+b)
	more than this are not handled yet(change in loop).
	*/
	void optimizer::common_subexpression_elimination(primary_expr_t **pexpr) {
		primary_expr_t *pexp = *pexpr;
		primary_expr_t *cmnexpr1 = nullptr;
		primary_expr_t *cmnexpr2 = nullptr;
		primary_expr_t *temp = nullptr;
		std::stack<primary_expr_t *> st;
		std::stack<primary_expr_t *> prev_stack;
		
		if (pexp == nullptr)
			return;
		
		get_inorder_primary_expr(&pexp);
		
		while (!pexpr_stack.empty()) {
			temp = pexpr_stack.top();
			prev_stack.push(temp);
			pexpr_stack.pop();
			if (temp->is_oprtr) {
				break;
			}
		}
		
		while (!pexpr_stack.empty()) {
			temp = pexpr_stack.top();
			st.push(temp);
			pexpr_stack.pop();
			if (temp->is_oprtr) {
				if (equals(st, prev_stack)) {
					if (pexpr_stack.empty())
						break;
					pexpr_stack.pop();
					if (prev_stack.size() > 0) {
						cmnexpr1 = prev_stack.top();
						cmnexpr2 = st.top();
						break;
					}
				} else {
					if (pexpr_stack.empty())
						break;
					st.pop();
				}
			}
		}
		
		clear_stack(prev_stack);
		clear_stack(st);
		clear_primary_expr_stack();
		
		if (cmnexpr1 != nullptr && cmnexpr2 != nullptr)
			change_subexpr_pointers(&(*pexpr), &cmnexpr1, &cmnexpr2);
	}
	
	//returns true if n is power of 2
	bool optimizer::is_powerof_2(int n, int *iter) {
		
		unsigned int pow = 0;
		unsigned int result = 0;
		while (result < maxint) {
			result = std::pow(2, pow);
			if (result == static_cast<unsigned int>(n)) {
				*iter = pow;
				return true;
			}
			pow++;
		}
		return false;
	}
	
	/*
	strength reduction optimization
	converting multiplication by 2^n left-shift operator(<<)
	division by 2^n right-shift operator(>>)
	modulus by (2^n)-1 bitwise-and operator(&)
	*/
	
	void xlang::optimizer::strength_reduction(primary_expr_t **pexpr) {
		primary_expr_t *root = *pexpr, *left = nullptr, *right = nullptr;
		int iter = 0, decm = 0;
		
		if (root == nullptr)
			return;
		if (root->left != nullptr && root->right != nullptr) {
			left = root->left;
			right = root->right;
			
			if (root->is_oprtr) {
				if (!left->is_oprtr && !right->is_oprtr) {
					if (!right->is_id) {
						if (right->tok.number != LIT_FLOAT) {
							decm = xlang::get_decimal(right->tok);
							switch (root->tok.number) {
								case ARTHM_MUL :
									if (is_powerof_2(decm, &iter)) {
										root->tok.string = BIT_LSHIFT;
										root->tok.string = "<<";
										right->tok.string = std::to_string(iter);
									}
									break;
								case ARTHM_DIV :
									if (is_powerof_2(decm, &iter)) {
										root->tok.string = BIT_RSHIFT;
										root->tok.string = ">>";
										right->tok.string = std::to_string(iter);
									}
									break;
								case ARTHM_MOD :
									if (is_powerof_2(decm, &iter)) {
										root->tok.string = BIT_AND;
										root->tok.string = "&";
										right->tok.string = std::to_string(decm - 1);
									}
									break;
								default:
									break;
							}
						}
					}
				}
			}
			
			strength_reduction(&root->left);
			strength_reduction(&root->right);
		}
	}
	
	void optimizer::optimize_primary_expr(primary_expr_t **pexpr) {
		primary_expr_t *pexp = *pexpr;
		if (pexp == nullptr)
			return;
		
		constant_folding(&(*pexpr));
		common_subexpression_elimination(&(*pexpr));
		strength_reduction(&(*pexpr));
	}
	
	void optimizer::optimize_assignment_expr(assgn_expr_t **assexpr) {
		assgn_expr_t *asexp = *assexpr;
		if (asexp == nullptr)
			return;
		optimize_expr(&asexp->expression);
	}
	
	void optimizer::optimize_expr(expr **exp) {
		expr *exp2 = *exp;
		if (exp2 == nullptr)
			return;
		switch (exp2->expr_kind) {
			case PRIMARY_EXPR :
				optimize_primary_expr(&exp2->primary_expr);
				break;
			case ASSGN_EXPR :
				optimize_assignment_expr(&exp2->assgn_expr);
				break;
			default:
				break;
		}
	}
	
	void optimizer::optimize_statement(stmt **stm) {
		stmt *stm2 = *stm;
		if (stm2 == nullptr)
			return;
		
		while (stm2 != nullptr) {
			switch (stm2->type) {
				case EXPR_STMT :
					optimize_expr(&stm2->expression_statement->expression);
					break;
				default:
					break;
			}
			stm2 = stm2->p_next;
		}
	}
	
	// search symbol in global_members/local_members and update its count
	void optimizer::update_count(std::string symbol) {
		
		std::unordered_map<std::string, int>::iterator it;
		it = local_members.find(symbol);
		
		if (it == local_members.end()) {
			it = global_members.find(symbol);
			if (it != global_members.end())
				global_members[symbol] = it->second + 1;
			
			return;
		}
		
		local_members[symbol] = it->second + 1;
		
	}
	
	//search id symbol in primary expression for dead-code elimination
	void optimizer::search_id_in_primary_expr(primary_expr_t *pexpr) {
		
		if (pexpr == nullptr)
			return;
		
		if (pexpr->unary_node != nullptr)
			pexpr = pexpr->unary_node;
		
		if (pexpr->is_id)
			update_count(pexpr->tok.string);
		
		search_id_in_primary_expr(pexpr->left);
		search_id_in_primary_expr(pexpr->right);
	}
	
	//search id symbol in id expression for dead-code elimination
	void optimizer::search_id_in_id_expr(id_expr_t *idexpr) {
		
		if (idexpr == nullptr)
			return;
		
		if (idexpr->is_id)
			update_count(idexpr->tok.string);
		
		search_id_in_id_expr(idexpr->left);
		search_id_in_id_expr(idexpr->right);
	}
	
	//search id symbol in expression for dead-code elimination
	void optimizer::search_id_in_expr(expr **exp) {
		
		expr *exp2 = *exp;
		if (exp2 == nullptr)
			return;
		
		switch (exp2->expr_kind) {
			case PRIMARY_EXPR :
				search_id_in_primary_expr(exp2->primary_expr);
				break;
			case ASSGN_EXPR :
				if (exp2->assgn_expr->id_expr->unary != nullptr)
					search_id_in_id_expr(exp2->assgn_expr->id_expr->unary);
				else
					search_id_in_id_expr(exp2->assgn_expr->id_expr);
				
				search_id_in_expr(&exp2->assgn_expr->expression);
				break;
			case CAST_EXPR :
				search_id_in_id_expr(exp2->cast_expr->target);
				break;
			case ID_EXPR :
				if (exp2->id_expr->unary != nullptr)
					search_id_in_id_expr(exp2->id_expr->unary);
				else
					search_id_in_id_expr(exp2->id_expr);
				break;
			case FUNC_CALL_EXPR :
				search_id_in_id_expr(exp2->call_expr->function);
				for (auto e: exp2->call_expr->expression_list)
					search_id_in_expr(&e);
				break;
			default:
				break;
		}
	}
	
	//search id symbol in statement for dead-code elimination
	void optimizer::search_id_in_statement(stmt **stm) {
		stmt *stm2 = *stm;
		if (stm2 == nullptr)
			return;
		
		while (stm2 != nullptr) {
			switch (stm2->type) {
				case EXPR_STMT :
					search_id_in_expr(&stm2->expression_statement->expression);
					break;
				case SELECT_STMT :
					search_id_in_expr(&stm2->selection_statement->condition);
					search_id_in_statement(&stm2->selection_statement->if_statement);
					search_id_in_statement(&stm2->selection_statement->else_statement);
					break;
				case ITER_STMT :
					switch (stm2->iteration_statement->type) {
						case WHILE_STMT :
							search_id_in_expr(&stm2->iteration_statement->_while.condition);
							search_id_in_statement(&stm2->iteration_statement->_while.statement);
							break;
						case FOR_STMT :
							search_id_in_expr(&stm2->iteration_statement->_for.init_expr);
							search_id_in_expr(&stm2->iteration_statement->_for.condition);
							search_id_in_expr(&stm2->iteration_statement->_for.update_expr);
							search_id_in_statement(&stm2->iteration_statement->_for.statement);
							break;
						case DOWHILE_STMT :
							search_id_in_expr(&stm2->iteration_statement->_dowhile.condition);
							search_id_in_statement(&stm2->iteration_statement->_dowhile.statement);
							break;
					}
					break;
				case JUMP_STMT :
					switch (stm2->jump_statement->type) {
						case RETURN_JMP :
							search_id_in_expr(&stm2->jump_statement->expression);
							break;
						default:
							break;
					}
					break;
				case ASM_STMT :
					for (auto e: stm2->asm_statement->output_operand)
						search_id_in_expr(&e->expression);
					for (auto e: stm2->asm_statement->input_operand)
						search_id_in_expr(&e->expression);
					break;
				default:
					break;
			}
			stm2 = stm2->p_next;
		}
	}
	
	/*
	dead code elimination optimization
	two tables are maintained global_members and local_members
	having entries of symbol name and it used count in expressions
	*/
	void optimizer::dead_code_elimination(tree_node **tr) {
		tree_node *trhead = *tr;
		st_symbol_info *syminfo = nullptr;
		stmt *stmthead = nullptr;
		std::unordered_map<std::string, int>::iterator it;
		if (trhead == nullptr)
			return;
		
		//copy each symbol from global symbol table into global_members hashmap
		for (int i = 0; i < ST_SIZE; i++) {
			syminfo = compiler::symtab->symbol_info[i];
			if (syminfo != nullptr)
				global_members.insert(std::pair<std::string, int>(syminfo->symbol, 0));
		}
		
		while (trhead != nullptr) {
			if (trhead->symtab != nullptr) {
				func_symtab = trhead->symtab;
				//copy each symbol from function local symbol table into local_members hashmap
				for (int i = 0; i < ST_SIZE; i++) {
					syminfo = func_symtab->symbol_info[i];
					if (syminfo != nullptr)
						local_members.insert(std::pair<std::string, int>(syminfo->symbol, 0));
				}
				//search symbol in statement list
				search_id_in_statement(&trhead->statement);
				it = local_members.begin();
				//check for used symbol count for 0
				//if found then remove that symbol
				while (it != local_members.end()) {
					if (it->second == 0)
						xlang::symtable::remove_symbol(&func_symtab, it->first);
					it++;
				}
				local_members.clear();
			} else {
				stmthead = trhead->statement;
				if (stmthead != nullptr) {
					if (stmthead->type == EXPR_STMT)
						search_id_in_expr(&stmthead->expression_statement->expression);
				}
			}
			
			trhead = trhead->p_next;
		}
		
		//check for used symbol count 0 for globally defined symbols
		it = global_members.begin();
		while (it != global_members.end()) {
			if (it->second == 0)
				xlang::symtable::remove_symbol(&compiler::symtab, it->first);
			it++;
		}
	}
	
	void optimizer::optimize(tree_node **tr) {
		struct tree_node *trhead = *tr;
		if (trhead == nullptr)
			return;
		
		dead_code_elimination(&trhead);
		trhead = *tr;
		
		while (trhead != nullptr) {
			optimize_statement(&trhead->statement);
			trhead = trhead->p_next;
		}
	}
	
}
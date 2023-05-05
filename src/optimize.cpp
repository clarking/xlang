/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

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
	

	bool Optimizer::evaluate(Token &f1, Token &f2, Token &op, std::string &stresult, bool has_float) {
	    
        // evaluate an expression with two factors(f1,f2) and operator op
	    // evaluation result is always a double, depends on has_float parameter

		double d1 = 0.0; 
        double d2 = 0.0; 
        double result = 0.0;
		bool bres = false;
		
		if (has_float) {
			d1 = std::stod(f1.string);
			d2 = std::stod(f2.string);
		} else {
			d1 = static_cast<double>(Convert::tok_to_decimal(f1));
			d2 = static_cast<double>(Convert::tok_to_decimal(f2));
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
					Log::error("divide by zero found in optimization");
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
				Log::error("invalid operator found in optimization '" + op.string + "'");
				bres = false;
				break;
		}
		
		stresult = std::to_string(result);
		if (bres)
			return true;
		else
			return false;
	}
	
	void Optimizer::clear_primary_expr_stack() {
		clear_stack(pexpr_stack);
	}
	
	bool Optimizer::has_float_type(PrimaryExpression *pexpr) {
        
        // returns true if any node of primary expression
        // has float literal or float/double Member type

		Token type;
		if (pexpr == nullptr)
			return false;
		if (pexpr->is_id) {
			if (pexpr->id_info->type_info->type == NodeType::SIMPLE) {
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
	
	bool Optimizer::has_id(PrimaryExpression *pexpr) {
	    
        //returns true if any identifier found in primary expression

		if (pexpr == nullptr)
			return false;

		if (pexpr->is_id) 
			return true;
		else if (pexpr->is_oprtr) 
			return (has_id(pexpr->left) || has_id(pexpr->right));
		else 
			return (has_id(pexpr->left) || has_id(pexpr->right));
		
		return false;
	}
	
	void Optimizer::get_inorder_primary_expr(PrimaryExpression **pexpr) {
		PrimaryExpression *pexp = *pexpr;
		if (pexp == nullptr)
			return;
		
		pexpr_stack.push(pexp);
		get_inorder_primary_expr(&pexp->left);
		get_inorder_primary_expr(&pexp->right);
	}
	
	void Optimizer::id_constant_folding(PrimaryExpression **pexpr) {

        // traverse primary Expression tree for constant expressions,
        // if found any then fold it

		if (*pexpr == nullptr)
			return;

		if (!has_id(*pexpr)) 
			constant_folding(&(*pexpr));

		id_constant_folding(&((*pexpr)->left));
		id_constant_folding(&((*pexpr)->right));
	}
	
	void Optimizer::constant_folding(PrimaryExpression **pexpr) {
	    
        // constant folding optimization on primary expression

		PrimaryExpression *pexp = *pexpr;
		Token fact1, fact2, opr, restok;
		PrimaryExpression *temp = nullptr;
		std::string stresult;
		int result = 0;
		std::stack<Token> pexp_eval;
		bool has_float = has_float_type(pexp);
		if (pexp == nullptr)
			return;
		
		if (has_id(pexp)) {
			id_constant_folding(&(*pexpr));
			return;
		}
		
		// get inorder primary Expression into pexpr_stack
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
								stresult = Convert::dec_to_hex(result);
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
		
		// delete whole sub-expression tree 
        // and add new node with evaluated result

		if (pexp_eval.size() > 0) {
			restok = pexp_eval.top();
			Tree::delete_primary_expr(&(*pexpr));
			*pexpr = nullptr;
			*pexpr = Tree::get_primary_expr_mem();
			(*pexpr)->is_id = false;
			(*pexpr)->is_oprtr = false;
			(*pexpr)->tok = restok;
			pexp_eval.pop();
		}
		
		clear_primary_expr_stack();
	}
	
	bool Optimizer::equals(std::stack<PrimaryExpression *> st1, std::stack<PrimaryExpression *> st2) {
	    
        //returns true of both stacks are equals 
        // by primary Expression lexeme values

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
	
	PrimaryExpression *Optimizer::get_cmnexpr1_node(PrimaryExpression **root, PrimaryExpression **cmn1) {

        // search cmn1 Expression node in primary Expression tree
        // if found then return its address from tree

		if (*root == nullptr)
			return nullptr;

		if ((*root)->left != nullptr) {
			if ((*root)->left == *cmn1) 
				return (*root)->left;
			else {
				get_cmnexpr1_node(&(*root)->left, &(*cmn1));
				get_cmnexpr1_node(&(*root)->left->right, &(*cmn1));
			}
		}

		return nullptr;
	}

	void Optimizer::change_subexpr_pointers(PrimaryExpression **root, PrimaryExpression **cmn1, PrimaryExpression **cmn2) {
	
        // cmn1 is common subexpression from right side of a tree
        // cmn2 is common subexpression from left side of a tree
        //
        // traverse primary Expression tree, search for common expression
        // in tree by calling get_cmnexpr1_node(), delete its right subtree,
        // and set its right side pointer to received common subexpr node
        // by function get_cmnexpr1_node()
        
		PrimaryExpression *node1 = nullptr;
		if (*root == nullptr)
			return;
		if ((*root)->right != nullptr) {
			node1 = get_cmnexpr1_node(&(*root), &(*cmn2));
			if ((*root)->right == *cmn1) {
				Tree::delete_primary_expr(&(*root)->right);
				(*root)->right = node1;
				return;
			} else {
				change_subexpr_pointers(&(*root)->left, &(*cmn1), &(*cmn2));
				change_subexpr_pointers(&(*root)->right, &(*cmn1), &(*cmn2));
			}
		}
	}
	
	void Optimizer::common_subexpression_elimination(PrimaryExpression **pexpr) {
 
        // common subexpression elimination optimization
        // traverse tree, putting each node on stack, and then stack are compared
        // it can only optimize simple two factors expression(e.g: (a+b)*(a+b)
        // more than this are not handled yet(change in loop).
        
		PrimaryExpression *pexp = *pexpr;
		PrimaryExpression *cmnexpr1 = nullptr;
		PrimaryExpression *cmnexpr2 = nullptr;
		PrimaryExpression *temp = nullptr;
		std::stack<PrimaryExpression *> st;
		std::stack<PrimaryExpression *> prev_stack;
		
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
	
	bool Optimizer::is_powerof_2(int n, int *iter) {
		
	    //true if n is power of 2

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
	
	void Optimizer::strength_reduction(PrimaryExpression **pexpr) {

        // strength reduction optimization
        // converting multiplication by 2^n left-shift operator(<<)
        // division by 2^n right-shift operator(>>)
        // modulus by (2^n)-1 bitwise-and operator(&)

		PrimaryExpression *root = *pexpr;
        PrimaryExpression *left = nullptr;
        PrimaryExpression *right = nullptr;

		int iter = 0;
        int decm = 0;
		
		if (root == nullptr)
			return;

        if (root->left == nullptr && root->right == nullptr) 
            return;

        left = root->left;
        right = root->right;

        if (!root->is_oprtr || (left->is_oprtr && right->is_oprtr)) {
            strength_reduction(&root->left);
            strength_reduction(&root->right);
            return;
        }

        if (!right->is_id) {
            if (right->tok.number != LIT_FLOAT) {
                decm = Convert::tok_to_decimal(right->tok);
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
        
        strength_reduction(&root->left);
        strength_reduction(&root->right);
    
	}
	
	void Optimizer::optimize_primary_expr(PrimaryExpression **pexpr) {
		PrimaryExpression *pexp = *pexpr;
		if (pexp == nullptr)
			return;
		
		constant_folding(&(*pexpr));
		common_subexpression_elimination(&(*pexpr));
		strength_reduction(&(*pexpr));
	}
	
	void Optimizer::optimize_assignment_expr(AssignmentExpression **assexpr) {
		AssignmentExpression *asexp = *assexpr;
		if (asexp == nullptr)
			return;
		optimize_expr(&asexp->expression);
	}
	
	void Optimizer::optimize_expr(Expression **exp) {
		Expression *exp2 = *exp;
		if (exp2 == nullptr)
			return;
		switch (exp2->expr_kind) {
			case ExpressionType::PRIMARY_EXPR :
				optimize_primary_expr(&exp2->primary_expr);
				break;
			case ExpressionType::ASSGN_EXPR :
				optimize_assignment_expr(&exp2->assgn_expr);
				break;
			default:
				break;
		}
	}
	
	void Optimizer::optimize_statement(Statement **stm) {
		Statement *stm2 = *stm;
		if (stm2 == nullptr)
			return;
		
		while (stm2 != nullptr) {
			switch (stm2->type) {
				case StatementType::EXPR :
					optimize_expr(&stm2->expression_statement->expression);
					break;
				default:
					break;
			}
			stm2 = stm2->p_next;
		}
	}
	
	void Optimizer::update_count(std::string symbol) {
		
	    // search symbol in global_members/local_members and update its count

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
	
	void Optimizer::search_id_in_primary_expr(PrimaryExpression *pexpr) {
	    
        //search id symbol in primary expression for dead-code elimination
		
		if (pexpr == nullptr)
			return;
		
		if (pexpr->unary_node != nullptr)
			pexpr = pexpr->unary_node;
		
		if (pexpr->is_id)
			update_count(pexpr->tok.string);
		
		search_id_in_primary_expr(pexpr->left);
		search_id_in_primary_expr(pexpr->right);
	}
	
	void Optimizer::search_id_in_id_expr(IdentifierExpression *idexpr) {
	    
        //search id symbol in id expression for dead-code elimination
		
		if (idexpr == nullptr)
			return;
		
		if (idexpr->is_id)
			update_count(idexpr->tok.string);
		
		search_id_in_id_expr(idexpr->left);
		search_id_in_id_expr(idexpr->right);
	}
	
	void Optimizer::search_id_in_expr(Expression **exp) {
	    
        //search id symbol in expression for dead-code elimination
		
		Expression *exp2 = *exp;
		if (exp2 == nullptr)
			return;
		
		switch (exp2->expr_kind) {
			case ExpressionType::PRIMARY_EXPR :
				search_id_in_primary_expr(exp2->primary_expr);
				break;
			case ExpressionType::ASSGN_EXPR :
				if (exp2->assgn_expr->id_expr->unary != nullptr)
					search_id_in_id_expr(exp2->assgn_expr->id_expr->unary);
				else
					search_id_in_id_expr(exp2->assgn_expr->id_expr);
				
				search_id_in_expr(&exp2->assgn_expr->expression);
				break;
			case ExpressionType::CAST_EXPR :
				search_id_in_id_expr(exp2->cast_expr->target);
				break;
			case ExpressionType::ID_EXPR :
				if (exp2->id_expr->unary != nullptr)
					search_id_in_id_expr(exp2->id_expr->unary);
				else
					search_id_in_id_expr(exp2->id_expr);
				break;
			case ExpressionType::FUNC_CALL_EXPR :
				search_id_in_id_expr(exp2->call_expr->function);
				for (auto e: exp2->call_expr->expression_list)
					search_id_in_expr(&e);
				break;
			default:
				break;
		}
	}
	
	void Optimizer::search_id_in_statement(Statement **stm) {
	    
        //search id symbol in statement for dead-code elimination

		Statement *stm2 = *stm;
		if (stm2 == nullptr)
			return;
		
		while (stm2 != nullptr) {
			switch (stm2->type) {
				case StatementType::EXPR :
					search_id_in_expr(&stm2->expression_statement->expression);
					break;
				case StatementType::SELECT :
					search_id_in_expr(&stm2->selection_statement->condition);
					search_id_in_statement(&stm2->selection_statement->if_statement);
					search_id_in_statement(&stm2->selection_statement->else_statement);
					break;
				case StatementType::ITER :
					switch (stm2->iteration_statement->type) {
						case IterationType::WHILE :
							search_id_in_expr(&stm2->iteration_statement->_while.condition);
							search_id_in_statement(&stm2->iteration_statement->_while.statement);
							break;
						case IterationType::FOR :
							search_id_in_expr(&stm2->iteration_statement->_for.init_expr);
							search_id_in_expr(&stm2->iteration_statement->_for.condition);
							search_id_in_expr(&stm2->iteration_statement->_for.update_expr);
							search_id_in_statement(&stm2->iteration_statement->_for.statement);
							break;
						case IterationType::DOWHILE :
							search_id_in_expr(&stm2->iteration_statement->_dowhile.condition);
							search_id_in_statement(&stm2->iteration_statement->_dowhile.statement);
							break;
					}
					break;
				case StatementType::JUMP :
					switch (stm2->jump_statement->type) {
						case JumpType::RETURN :
							search_id_in_expr(&stm2->jump_statement->expression);
							break;
						default:
							break;
					}
					break;
				case StatementType::ASM :
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

	void Optimizer::dead_code_elimination(TreeNode **tr) {

        // dead code elimination optimization
        // two tables are maintained global_members and local_members
        // having entries of symbol name and it used count in expressions

		TreeNode *trhead = *tr;
		SymbolInfo *syminfo = nullptr;
		Statement *stmthead = nullptr;
		std::unordered_map<std::string, int>::iterator it;
		if (trhead == nullptr)
			return;
		
		//copy each symbol from global symbol table into global_members hashmap
		for (int i = 0; i < ST_SIZE; i++) {
			syminfo = Compiler::symtab->symbol_info[i];
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

				//if found, remove used symbol count for 0
				while (it != local_members.end()) {
					if (it->second == 0)
						SymbolTable::remove_symbol(&func_symtab, it->first);
					it++;
				}
				local_members.clear();
			} else {
				stmthead = trhead->statement;
				if (stmthead != nullptr) {
					if (stmthead->type == StatementType::EXPR)
						search_id_in_expr(&stmthead->expression_statement->expression);
				}
			}
			
			trhead = trhead->p_next;
		}
		
		//check for used symbol count 0 for globally defined symbols
		it = global_members.begin();
		while (it != global_members.end()) {
			if (it->second == 0)
				SymbolTable::remove_symbol(&Compiler::symtab, it->first);
			it++;
		}
	}
	
	void Optimizer::optimize(TreeNode **tr) {
		struct TreeNode *trhead = *tr;
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
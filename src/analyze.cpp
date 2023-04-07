/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <algorithm>
#include "log.hpp"
#include "analyze.hpp"
#include "parser.hpp"
#include "compiler.hpp"

namespace xlang {
	
	SymbolInfo *Analyzer::search_func_params(Token tok) {

		if (func_info == nullptr)
			return nullptr;
		
		if (func_info->param_list.size() > 0) {
			for (auto syminf: func_info->param_list) {
				if (syminf->symbol_info != nullptr) {
					if (syminf->symbol_info->symbol == tok.string)
						return syminf->symbol_info;
				}
			}
		}
		return nullptr;
	}
	
	SymbolInfo *Analyzer::search_id(Token tok) {

		SymbolInfo *syminf = nullptr;

		if (func_symtab != nullptr) {
			syminf = SymbolTable::search_symbol_node(func_symtab, tok.string);
			if (syminf == nullptr) {
				//if null, then search in function parameters
				syminf = search_func_params(tok);
				if (syminf == nullptr) {
					//if null, then search in global symbol table
					syminf = SymbolTable::search_symbol_node(Compiler::symtab, tok.string);
				}
			}
		} else 
			//if function symbol table null, then search in global symbol table
			syminf = SymbolTable::search_symbol_node(Compiler::symtab, tok.string);
		
		return syminf;
	}
	
	void Analyzer::check_invalid_type_declaration(Node *symtab) {

		unsigned i;

		if (symtab == nullptr)
			return;
		
		for (i = 0; i < ST_SIZE; i++) {
		
			if (symtab->symbol_info[i] != nullptr
			    && symtab->symbol_info[i]->type_info != nullptr
			    && symtab->symbol_info[i]->type_info->type == NodeType::SIMPLE
			    && symtab->symbol_info[i]->type_info->type_specifier.simple_type[0].number == KEY_VOID
			    && !symtab->symbol_info[i]->is_ptr) {
				Log::error_at(symtab->symbol_info[i]->tok.loc, "variable " + symtab->symbol_info[i]->symbol + " is declared as void");
			}
		}
	}
	
	bool Analyzer::check_pointer_arithmetic(PrimaryExpression *opr, PrimaryExpression *fact_1, PrimaryExpression *fact_2) {
		
		if (opr == nullptr || fact_1 == nullptr || fact_2 == nullptr)
			return true;
		
		if (!fact_1->is_id && !fact_2->is_id)
			return true;
		
		//if fact_1 is id & fact_2 is not and fact_1 id is pointer
		if (fact_1->is_id && !fact_2->is_id && fact_1->id_info != nullptr && fact_1->id_info->is_ptr) {

			if (opr->tok.number == ARTHM_ADD || opr->tok.number == ARTHM_SUB) {
				if (fact_2->tok.number == LIT_FLOAT || fact_2->tok.number == LIT_STRING) {
					Log::error_at(opr->tok.loc, "invalid Operand to binary " + opr->tok.string
					                            + " (have " + fact_2->tok.string + ")");
					return false;
				}
			} else {
				Log::error_at(opr->tok.loc, "invalid Operand to binary " + opr->tok.string + " (have " + fact_1->tok.string + ")");
				return false;
			}
		} else if (fact_2->is_id && !fact_1->is_id && fact_2->id_info != nullptr && fact_2->id_info->is_ptr) {
			
			if (opr->tok.number == ARTHM_ADD || opr->tok.number == ARTHM_SUB) {
				if (fact_1->tok.number == LIT_FLOAT || fact_1->tok.number == LIT_STRING) {
					Log::error_at(opr->tok.loc, "invalid Operand to binary " + opr->tok.string + " (have " + fact_2->tok.string + ")");
					return false;
				}
			} else {
				Log::error_at(opr->tok.loc, "invalid Operand to binary " + opr->tok.string + " (have " + fact_2->tok.string + ")");
				return false;
			}

		} else if (fact_1->is_id && fact_2->is_id && fact_1->id_info != nullptr && fact_2->id_info != nullptr) {
			
            //if both are pointers
			if (fact_1->id_info->is_ptr && fact_2->id_info->is_ptr) {
				Log::error_at(opr->tok.loc, "invalid Operand to binary " + opr->tok.string);
				return false;
			} else if (fact_1->id_info->is_ptr && !fact_2->id_info->is_ptr) {
				if (opr->tok.number == ARTHM_ADD || opr->tok.number == ARTHM_SUB) {
				} else {
					Log::error_at(opr->tok.loc, "invalid Operand to binary " + opr->tok.string);
					return false;
				}
			} else if (fact_2->id_info->is_ptr && !fact_1->id_info->is_ptr) {
				if (opr->tok.number == ARTHM_ADD || opr->tok.number == ARTHM_SUB) {
				} else {
					Log::error_at(opr->tok.loc, "invalid Operand to binary " + opr->tok.string);
					return false;
				}
			}
		}
		return true;
	}
	
	/*
	check type argument of primary expression
	when operators are %, &, |, ^, <<, >>
	allow only integer types not floating types
	*/
	bool Analyzer::check_primexp_type_argument(
			PrimaryExpression *opr,
			PrimaryExpression *fact_1,
			PrimaryExpression *fact_2) {
		if (opr == nullptr)
			return true;
		
		switch (opr->tok.number) {
			case ARTHM_MOD :
			case BIT_AND :
			case BIT_OR :
			case BIT_EXOR :
			case BIT_LSHIFT :
			case BIT_RSHIFT : {

				if (opr->tok.number == BIT_LSHIFT || opr->tok.number == BIT_RSHIFT) {
					if (fact_2->is_id) {
						Log::error_at(opr->tok.loc, "only literals expected to <<, >> at right hand side");
						return false;
					}
				}

				//if fact_1 is pointer id then error
				if (fact_1 != nullptr && fact_1->is_id && fact_1->id_info != nullptr && fact_1->id_info->is_ptr) {
					Log::error_at(opr->tok.loc, "invalid Operand to binary " + opr->tok.string + " (have " + fact_1->tok.string + ")");
					return false;
				}
				
				//if fact_2 is pointer id then error
				if (fact_2 != nullptr && fact_2->is_id && fact_2->id_info != nullptr && fact_2->id_info->is_ptr) {
					Log::error_at(opr->tok.loc, "invalid Operand to binary " + opr->tok.string + " (have " + fact_1->tok.string + ")");
					return false;
				}
				
				//if fact_1 is id but type is float/double then error
				if (fact_1 != nullptr && fact_1->is_id && fact_1->id_info != nullptr && !fact_1->id_info->is_ptr) {
					if (fact_1->id_info->type_info->type == NodeType::SIMPLE) {
						if (fact_1->id_info->type_info->type_specifier.simple_type[0].number == KEY_DOUBLE
						    || fact_1->id_info->type_info->type_specifier.simple_type[0].number == KEY_FLOAT) {
							
							Log::error_at(opr->tok.loc, "invalid Operand to binary " + opr->tok.string + " (have " + fact_1->tok.string + ")");
							return false;
						}
					}
				}
				
				//if fact_2 is id but type is float/double then error
				if (fact_2 != nullptr && fact_2->is_id && fact_2->id_info != nullptr && !fact_2->id_info->is_ptr) {
					if (fact_2->id_info->type_info->type == NodeType::SIMPLE) {
						if (fact_2->id_info->type_info->type_specifier.simple_type[0].number == KEY_DOUBLE
						    || fact_2->id_info->type_info->type_specifier.simple_type[0].number == KEY_FLOAT) {
							Log::error_at(opr->tok.loc, "invalid Operand to binary " + opr->tok.string + " (have " + fact_2->tok.string + ")");
							return false;
						}
					}
				}
				
				//if fact_1 is not id and Token is float literal then error
				if (fact_1 != nullptr && !fact_1->is_id && fact_1->tok.number == LIT_FLOAT) {
					Log::error_at(opr->tok.loc, "invalid Operand to binary " + opr->tok.string + " (have " + fact_1->tok.string + ")");
					return false;
				}

				//if fact_2 is not id and Token is float literal then error
				if (fact_2 != nullptr && !fact_2->is_id && fact_2->tok.number == LIT_FLOAT) {
					Log::error_at(opr->tok.loc, "invalid Operand to binary " + opr->tok.string + " (have " + fact_2->tok.string + ")");
					return false;
				}
			}
				break;
			default:
				break;
		}
		return true;
	}
	
	bool Analyzer::check_unary_primexp_type_argument(PrimaryExpression *pexpr) {

		bool result = true;
		SymbolInfo *syminf = nullptr;
		
		if (pexpr == nullptr)
			return result;
		
		if (pexpr->is_id) {
			syminf = search_id(pexpr->tok);
			if (syminf == nullptr) {
				Log::error_at(pexpr->tok.loc, "undeclared '" + pexpr->tok.string + "'");
				return false;
			} else {
				pexpr->id_info = syminf;
			}
		}
		
		//check for type as double or float
		//because these types are not allowed for unary operation ~, & |, ^
		//return false if found
		if (pexpr->is_id && pexpr->id_info != nullptr) {
			if (pexpr->id_info->type_info->type == NodeType::SIMPLE &&
			   (pexpr->id_info->type_info->type_specifier.simple_type[0].number == KEY_DOUBLE ||
                pexpr->id_info->type_info->type_specifier.simple_type[0].number == KEY_FLOAT)) {
				result = false;
			} else if (pexpr->id_info->is_ptr)
				result = false;
		}
		
		if (!pexpr->is_id && pexpr->tok.number == LIT_FLOAT)
			result = false;
		
		return (result && 
                check_unary_primexp_type_argument(pexpr->left) &&
                check_unary_primexp_type_argument(pexpr->right));
	}
	
	bool Analyzer::check_unary_idexp_type_argument(IdentifierExpression *idexpr) {

		bool result = true;
		SymbolInfo *syminf = nullptr;
		
		if (idexpr == nullptr)
			return result;
		
		if (idexpr->is_id) {
			syminf = search_id(idexpr->tok);
			if (syminf == nullptr) {
				Log::error_at(idexpr->tok.loc, "undeclared '" + idexpr->tok.string + "'");
				return false;
			}
			
			idexpr->id_info = syminf;
		}
		
		if (idexpr->is_id && idexpr->id_info != nullptr) {

			if (idexpr->id_info->type_info->type_specifier.simple_type[0].number == KEY_DOUBLE || 
                idexpr->id_info->type_info->type_specifier.simple_type[0].number == KEY_FLOAT) {
				result = false;
			} else if (idexpr->id_info->is_ptr)
				result = false;
		}
		
		if (!idexpr->is_id && !idexpr->is_oprtr && idexpr->tok.number == LIT_FLOAT)
			result = false;
		
		return (result && check_unary_idexp_type_argument(idexpr->left)
		        && check_unary_idexp_type_argument(idexpr->right));
	}
	
	bool Analyzer::check_array_subscript(IdentifierExpression *idexpr) {

		bool result = true;
		SymbolInfo *syminf = nullptr;
		
		if (idexpr == nullptr)
			return result;
		
		if (idexpr->is_id) {
			syminf = search_id(idexpr->tok);
			if (syminf == nullptr) {
				Log::error_at(idexpr->tok.loc, "undeclared '" + idexpr->tok.string + "'");
				return false;
			}
			
			idexpr->id_info = syminf;
		}
		
		if (idexpr->is_id && idexpr->id_info != nullptr) {
			
			if (!idexpr->id_info->is_array) {
				result = false;
				if (!idexpr->id_info->is_ptr)
					result = false;
				else {
					if (idexpr->subscript.size() <= (size_t) idexpr->id_info->ptr_oprtr_count)
						result = true;
					else
						result = false;
				}
			} else {
				if (idexpr->subscript.size() <= idexpr->id_info->arr_dimension_list.size())
					result = true;
				else
					result = false;
			}
			
			if (!result) {
				Log::error_at(idexpr->tok.loc, "subscript is neither array nor pointer '" + idexpr->tok.string + "'");
				Log::error_at(idexpr->tok.loc, "array dimension is different at declaration '" + idexpr->tok.string + "'");
			}
		}
		
		return (result && 
                check_array_subscript(idexpr->left) &&
                check_array_subscript(idexpr->right));
	}
	
	void Analyzer::analyze_primary_expr(PrimaryExpression **_pexpr) {

		std::stack<PrimaryExpression *> pexp_stack;
		std::stack<PrimaryExpression *> pexp_out_stack;
		PrimaryExpression *pexp_root = *_pexpr;
		PrimaryExpression *pexp = nullptr;
		SymbolInfo *syminf = nullptr;
		
		if (pexp_root == nullptr)
			return;

		if (pexp_root->unary_node != nullptr) {
			if (pexp_root->is_oprtr && pexp_root->tok.number == BIT_COMPL) {
				if (!check_unary_primexp_type_argument(pexp_root->unary_node)) {
					Log::error_at(pexp_root->tok.loc, "wrong type argument to bit-complement ");
					return;
				}
			}
		}
		
		//traverse tree post-orderly
		//and get post order into pexp_out_stack
		pexp_stack.push(pexp_root);
		
		while (!pexp_stack.empty()) {

			pexp = pexp_stack.top();
			pexp_stack.pop();
			pexp_out_stack.push(pexp);
			
			if (pexp->left != nullptr)
				pexp_stack.push(pexp->left);
			
			if (pexp->right != nullptr)
				pexp_stack.push(pexp->right);
		}
		
		clear_stack(pexp_stack);
		
		//get three addess code, two factors and one operator
		//check factor types according to operator between them

		while (!pexp_out_stack.empty()) {

			pexp = pexp_out_stack.top();

			if (pexp->is_oprtr) {
				if (factor_1 != nullptr && factor_2 != nullptr) {
					check_pointer_arithmetic(pexp, factor_1, factor_2);
					check_primexp_type_argument(pexp, factor_1, factor_2);
					factor_1 = factor_2 = nullptr;
				} else if (factor_1 != nullptr && factor_2 == nullptr) {
					check_primexp_type_argument(pexp, factor_1, factor_2);
					factor_1 = nullptr;
				} else if (factor_1 == nullptr && factor_2 != nullptr) {
					check_primexp_type_argument(pexp, factor_1, factor_2);
					factor_2 = nullptr;
				}
			} else {
				if (pexp->is_id) {
					syminf = search_id(pexp->tok); //search symbol
					if (syminf == nullptr) {
						Log::error_at(pexp->tok.loc, "undeclared '" + pexp->tok.string + "'");
						pexp_out_stack.pop();
						continue;
					}
					
					//assign symbol info to Expression node
					pexp->id_info = syminf;
				}
				//get each arthmetic factors
				if (factor_1 == nullptr && factor_2 == nullptr)
					factor_1 = pexp;
				else if (factor_1 != nullptr && factor_2 != nullptr) {
					factor_1 = factor_2;
					factor_2 = pexp;
				} else if (factor_1 != nullptr && factor_2 == nullptr)
					factor_2 = pexp;
			}
			pexp_out_stack.pop();
		}
	}
	
	void Analyzer::analyze_id_expr(IdentifierExpression **_idexpr) {

		std::stack<IdentifierExpression *> idexp_stack;
		std::vector<IdentifierExpression *> idexp_vec;
		IdentifierExpression *idexp_root = *_idexpr;
		IdentifierExpression *idexp = nullptr;
		IdentifierExpression *idobj = nullptr, *idmember = nullptr;
		SymbolInfo *syminf = nullptr;
		RecordNode *record = nullptr;
		std::string recordname;
		size_t i;
		
		if (idexp_root == nullptr)
			return;
		
		if (idexp_root->unary != nullptr) {
			//if first operator is bit compl(~), then check rest of the expression
			if (idexp_root->is_oprtr && idexp_root->tok.number == BIT_COMPL) {
				if (!check_unary_idexp_type_argument(idexp_root->unary)) {
					Log::error_at(idexp_root->tok.loc, "wrong type argument to bit-complement ");
					return;
				}
			}
		}
		
		//if ++, --, &(addressof)
		if (idexp != nullptr && idexp_root->unary != nullptr) {
			if (idexp_root->is_oprtr &&
			    (idexp_root->tok.number == INCR_OP || idexp->tok.number == DECR_OP
			     || idexp_root->tok.number == ADDROF_OP)) {
				analyze_id_expr(&idexp_root->unary);
			}
		}
		
		//traverse tree in-orderly
		//and get expression in vector
		idexp = idexp_root;
		
		while (!idexp_stack.empty() || idexp != nullptr) {
			
			if (idexp != nullptr) {
				idexp_stack.push(idexp);
				idexp = idexp->left;
			} else {
				idexp = idexp_stack.top();
				idexp_vec.push_back(idexp);
				idexp_stack.pop();
				idexp = idexp->right;
			}
		}
		
		clear_stack(idexp_stack);
		
		//get left mode node of id-Expression
		idobj = idexp_vec[0];
		if (idobj == nullptr)
			return;
		
		if (idobj->unary != nullptr)
			idobj = idobj->unary;
		
		if (idobj->is_id) {
			//search symbol
			syminf = search_id(idobj->tok);
			if (syminf == nullptr) {
				Log::error_at(idobj->tok.loc, "undeclared '" + idobj->tok.string + "'");
				return;
			} else {
				idobj->id_info = syminf;
				//if symbol has simple type and it is array
				if (idobj->id_info->type_info->type != NodeType::RECORD) {
					if (idobj->id_info->is_array || idobj->id_info->is_ptr || idobj->is_subscript) {
						check_array_subscript(idobj);
						return;
					} else
						return;
				}
				recordname = idobj->id_info->type_info->type_specifier.record_type.string;
			}
		}
		
		i = 0;
		idobj = nullptr;

		//check each sub id expression by searching its entry
		//in record table members (idobj ./-> symbol ./-> symbol ....)
		while (i + 1 < idexp_vec.size()) {
			if (i < idexp_vec.size() &&
			    i + 1 < idexp_vec.size() &&
			    i + 2 < idexp_vec.size()) {
				
				idobj = idexp_vec[i];
				idmember = idexp_vec[i + 2];
				
				if (idobj->is_id) {
					record = SymbolTable::search_record_node(Compiler::record_table, recordname);
					if (record != nullptr) {
						syminf = SymbolTable::search_symbol_node(record->symtab, idobj->tok.string);
						if (syminf != nullptr) {
							idobj->id_info = syminf;
							recordname = idobj->id_info->type_info->type_specifier.record_type.string;
						}
					}
				}
				
				switch (idexp_vec[i + 1]->tok.number) {
					case ARROW_OP :
						if (idobj->id_info != nullptr && !idobj->id_info->is_ptr)
							Log::error_at(idobj->tok.loc, " dot(.) expected instead of ->");
						break;
					case DOT_OP :
						if (idobj->id_info != nullptr && idobj->id_info->is_ptr)
							Log::error_at(idobj->tok.loc, " arrow(->) expected instead of dot(.)");
						break;
					default:
						break;
				}
				
				if (idobj->id_info != nullptr) {
					switch (idobj->id_info->type_info->type) {
						case NodeType::RECORD :
							record = SymbolTable::search_record_node(Compiler::record_table, recordname);
							if (record != nullptr && idmember != nullptr) {
								if (!SymbolTable::search_symbol(record->symtab, idmember->tok.string)) {
									Log::error_at(idmember->tok.loc, "record '" + record->recordname + "' has no member '" + idmember->tok.string + "'");
								}
							}
							
							break;
						case NodeType::SIMPLE :
							Log::error_at(idobj->tok.loc, "'" + idobj->tok.string + "' is not a record type");
							break;
						default:
							break;
					}
				}
				
				i += 2;
				idobj = nullptr;
				idmember = nullptr;
			}
		}
	}
	
	void Analyzer::analyze_sizeof_expr(SizeOfExpression **_szofexpr) {
		SizeOfExpression *sizeexpr = *_szofexpr;
		SymbolInfo *sminf = nullptr;
		RecordNode *record = nullptr;
		
		if (sizeexpr == nullptr)
			return;
		
		if (!sizeexpr->is_simple_type) {
			record = SymbolTable::search_record_node(Compiler::record_table, sizeexpr->identifier.string);
			if (record == nullptr) {
				sminf = search_id(sizeexpr->identifier);
				if (sminf == nullptr)
					Log::error_at(sizeexpr->identifier.loc, "undeclared '" + sizeexpr->identifier.string + "'");
			}
		}
	}
	
	void Analyzer::analyze_cast_expr(CastExpression **_cstexpr) {
		CastExpression *cast_expr = *_cstexpr;
		
		if (cast_expr == nullptr)
			return;
		
		analyze_id_expr(&cast_expr->target);
	}
	
	void Analyzer::get_idexpr_idinfo(IdentifierExpression *idexpr, SymbolInfo **idinfo) {
		if (idexpr == nullptr)
			return;
		
		if (idexpr->left == nullptr && idexpr->right == nullptr)
			*idinfo = idexpr->id_info;
		
		get_idexpr_idinfo(idexpr->right, &(*idinfo));
	}
	
	IdentifierExpression *Analyzer::get_idexpr_attrbute_node(IdentifierExpression **_idexpr) {

		std::stack<IdentifierExpression *> idexp_stack;
		std::vector<IdentifierExpression *> idexp_vec;
		IdentifierExpression *idexp_root = *_idexpr;
		IdentifierExpression *idexp = nullptr;
		IdentifierExpression *idobj = nullptr, *idmember = nullptr;
		SymbolInfo *syminf = nullptr;
		RecordNode *record = nullptr;
		std::string recordname;
		size_t i;
		IdentifierExpression *result = nullptr;
		
		if (idexp_root == nullptr)
			return nullptr;
		
		if (idexp_root->unary != nullptr) {
			Log::error_at(idexp_root->tok.loc, "unary operator to assignement ");
			return nullptr;
		}
		
		//traverse tree in-orderly
		idexp = idexp_root;
		
		while (!idexp_stack.empty() || idexp != nullptr) {
			
			if (idexp != nullptr) {
				idexp_stack.push(idexp);
				idexp = idexp->left;
			} else {
				idexp = idexp_stack.top();
				idexp_vec.push_back(idexp);
				idexp_stack.pop();
				idexp = idexp->right;
			}
		}

		clear_stack(idexp_stack);
		
		idobj = idexp_vec[0];
		if (idobj == nullptr)
			return nullptr;
		
		if (idobj->is_id) {
			syminf = search_id(idobj->tok);
			if (syminf == nullptr) {
				Log::error_at(idobj->tok.loc, "undeclared '" + idobj->tok.string + "'");
				return nullptr;
			} else {
				idobj->id_info = syminf;
				if (idobj->id_info->type_info->type != NodeType::RECORD) {
					if (idobj->id_info->is_array || idobj->id_info->is_ptr) {
						check_array_subscript(idobj);
						return idobj;
					} else
						return idobj;
				}
				recordname = idobj->id_info->type_info->type_specifier.record_type.string;
			}
		}
		
		i = 0;
		idobj = nullptr;
		while (i + 1 < idexp_vec.size()) {
			if (i < idexp_vec.size() &&
			    i + 1 < idexp_vec.size() &&
			    i + 2 < idexp_vec.size()) {
				
				idobj = idexp_vec[i];
				idmember = idexp_vec[i + 2];
				
				//search id in record table and assign to node
				if (idobj->is_id) {
					record = SymbolTable::search_record_node(Compiler::record_table, recordname);
					if (record != nullptr) {
						syminf = SymbolTable::search_symbol_node(record->symtab, idmember->tok.string);
						if (syminf != nullptr) {
							idmember->id_info = syminf;
							recordname = idmember->id_info->type_info->type_specifier.record_type.string;
						}
					}
				}
				
				i += 2;
				result = idmember;
				idobj = nullptr;
				idmember = nullptr;
			}
		}
		return result;
	}
	
	int Analyzer::tree_height(ExpressionType exprtype, PrimaryExpression *pexpr, IdentifierExpression *idexpr) {
		int left, right;
		switch (exprtype) {
			case ExpressionType::PRIMARY_EXPR :
				if (pexpr == nullptr)
					return 0;
				else {
					left = tree_height(exprtype, pexpr->left, idexpr);
					right = tree_height(exprtype, pexpr->right, idexpr);
					if (left > right)
						return left + 1;
					else
						return right + 1;
				}
				break;
			
			case ExpressionType::ID_EXPR :
				if (idexpr == nullptr)
					return 0;
				else {
					left = tree_height(exprtype, pexpr, idexpr->left);
					right = tree_height(exprtype, pexpr, idexpr->right);
					if (left > right)
						return left + 1;
					else
						return right + 1;
				}
				break;
            default:
                break;
		}
		return 0;
	}
	
	IdentifierExpression *Analyzer::get_assgnexpr_idexpr_attribute(IdentifierExpression *_idexp) {
		IdentifierExpression *idexpr = nullptr;
		SymbolInfo *syminf = nullptr;
		
		if (_idexp != nullptr &&
		    tree_height(ExpressionType::ID_EXPR, nullptr, _idexp) > 1) {
			idexpr = get_idexpr_attrbute_node(&_idexp);
		} else {
			if (_idexp->is_id) {
				syminf = search_id(_idexp->tok);
				if (syminf == nullptr) {
					Log::error_at(_idexp->tok.loc, "undeclared '" + _idexp->tok.string + "'");
					return nullptr;
				} else {
					_idexp->id_info = syminf;
					if (_idexp->id_info->is_array || _idexp->id_info->is_ptr) {
						check_array_subscript(_idexp);
					}
					idexpr = _idexp;
				}
			}
		}
		
		if (idexpr == nullptr)
			return nullptr;
		
		return idexpr;
	}
	
	bool Analyzer::check_assignment_type_argument(AssignmentExpression *assgnexpr, ExpressionType type, IdentifierExpression *idexpr, PrimaryExpression *pexpr) {

		//if bitwise assignment operator, %=, &=, |=, ^=, <<=, >>=
		if (assgnexpr->tok.number == ASSGN_MOD       ||
		    assgnexpr->tok.number == ASSGN_BIT_AND   ||
		    assgnexpr->tok.number == ASSGN_BIT_OR    ||
		    assgnexpr->tok.number == ASSGN_BIT_EX_OR ||
		    assgnexpr->tok.number == ASSGN_LSHIFT    ||
		    assgnexpr->tok.number == ASSGN_RSHIFT) {
			
			switch (type) {
				case ExpressionType::PRIMARY_EXPR :
					if (!check_unary_primexp_type_argument(pexpr)) {
						Log::error_at(assgnexpr->tok.loc, "expected only simple type argument to '" + assgnexpr->tok.string + "'");
						return false;
					}
					break;

				case ExpressionType::ID_EXPR :
					if (idexpr->id_info != nullptr) {
						if (idexpr->id_info->type_info->type == NodeType::SIMPLE) {
							if (idexpr->id_info->type_info->type_specifier.simple_type[0].number == KEY_FLOAT ||
							    idexpr->id_info->type_info->type_specifier.simple_type[0].number == KEY_FLOAT) {
								Log::error_at(assgnexpr->tok.loc, "wrong type argument to '" + assgnexpr->tok.string + "'");
								return false;
							}
						} else {
							Log::error_at(assgnexpr->tok.loc, "expected only simple type argument to '" + assgnexpr->tok.string + "'");
							return false;
						}
					}
					break;

                default:
                    break;
			}
		}
		return true;
	}
	
	void Analyzer::simplify_assgn_primary_expr(AssignmentExpression **asexpr) {

		AssignmentExpression *assgnexp = *asexpr;
		Token tok;
		PrimaryExpression *left = nullptr, *opr = nullptr;
		
		if (*asexpr == nullptr)
			return;
		
		if (assgnexp->id_expr->left != nullptr && assgnexp->id_expr->right != nullptr)
			return;
		
		tok = assgnexp->tok;
		
		(*asexpr)->tok.number = ASSGN;
		(*asexpr)->tok.string = "=";
		
		left = Tree::get_primary_expr_mem();
		left->is_id = true;
		left->tok = assgnexp->id_expr->tok;
		left->is_oprtr = false;
		left->id_info = search_id(left->tok);
		
		opr = Tree::get_primary_expr_mem();
		opr->is_oprtr = true;
		opr->oprtr_kind = OperatorType::BINARY;
		opr->left = left;
		
		switch (tok.number) {
			case ASSGN_ADD :
				opr->tok.string = "+";
				opr->tok.number = ARTHM_ADD;
				break;
			case ASSGN_SUB :
				opr->tok.string = "-";
				opr->tok.number = ARTHM_SUB;
				break;
			case ASSGN_MUL :
				opr->tok.string = "*";
				opr->tok.number = ARTHM_MUL;
				break;
			case ASSGN_DIV :
				opr->tok.string = "/";
				opr->tok.number = ARTHM_DIV;
				break;
			case ASSGN_MOD :
				opr->tok.string = "%";
				opr->tok.number = ARTHM_MOD;
				break;
			case ASSGN_LSHIFT :
				opr->tok.string = "<<";
				opr->tok.number = BIT_LSHIFT;
				break;
			case ASSGN_RSHIFT :
				opr->tok.string = ">>";
				opr->tok.number = BIT_RSHIFT;
				break;
			case ASSGN_BIT_AND :
				opr->tok.string = "&";
				opr->tok.number = BIT_AND;
				break;
			case ASSGN_BIT_OR :
				opr->tok.string = "|";
				opr->tok.number = BIT_OR;
				break;
			case ASSGN_BIT_EX_OR :
				opr->tok.string = "^";
				opr->tok.number = BIT_EXOR;
				break;
			default:
				break;
		}
		
		opr->right = (*asexpr)->expression->primary_expr;
		(*asexpr)->expression->primary_expr = opr;
	}
	
	void Analyzer::analyze_assgn_expr(AssignmentExpression **_assgnexpr) {

		AssignmentExpression *assgnexpr = *_assgnexpr;
		TypeInfo *typeinf = nullptr;
		IdentifierExpression *assgnleft = nullptr;
		IdentifierExpression *idright = nullptr;
		FunctionInfo *funcinfo = nullptr;
		std::map<std::string, FunctionInfo *>::iterator findit;
		
		if (assgnexpr == nullptr)
			return;
		
		analyze_id_expr(&assgnexpr->id_expr);
		if (assgnexpr->tok.number != ASSGN)
			simplify_assgn_primary_expr(&(*_assgnexpr));
		
		analyze_expr(&assgnexpr->expression);
		assgnleft = get_assgnexpr_idexpr_attribute(assgnexpr->id_expr);
		if (assgnleft == nullptr)
			return;

		if (assgnleft->id_info == nullptr)
			return;

		typeinf = assgnleft->id_info->type_info;
		if (typeinf == nullptr)
			return;
		
		switch (assgnexpr->expression->expr_kind) {

			case ExpressionType::PRIMARY_EXPR : {
				PrimaryExpression *prim_exp = assgnexpr->expression->primary_expr;
				
				if (!check_assignment_type_argument(assgnexpr, ExpressionType::PRIMARY_EXPR, nullptr, prim_exp))
					return;
				
				if (assgnleft->id_info->is_ptr && prim_exp->is_id && prim_exp->id_info->is_ptr) {
					if (typeinf->type != prim_exp->id_info->type_info->type)
						Log::error_at(assgnexpr->tok.loc, "incompatible types for assignment to '" + assgnleft->tok.string + "'");
				} else {

					if (assgnleft->id_info->is_ptr && !prim_exp->is_id) {
						if (!check_unary_primexp_type_argument(prim_exp))
							Log::error_at(assgnexpr->tok.loc, "incompatible types for assignment to '" + assgnleft->tok.string + "'");
					}

					if (!assgnleft->id_info->is_ptr && !prim_exp->is_id && assgnleft->id_info->type_info->type == NodeType::RECORD) {
						if (!check_unary_primexp_type_argument(prim_exp))
							Log::error_at(assgnexpr->tok.loc, "incompatible types for assignment to '" + assgnleft->tok.string + "'");
					}
					
					if (typeinf->type == NodeType::SIMPLE && typeinf->type_specifier.simple_type[0].number == KEY_CHAR) {
						if (!assgnleft->id_info->is_array && !assgnleft->id_info->is_ptr) {
							if (prim_exp->tok.number == LIT_STRING) {
								Log::error_at(assgnexpr->tok.loc, "incompatible types for string assignment to '" + assgnleft->tok.string + "'");
								return;
							}
						}
					}
					
					if (!prim_exp->is_id)
						return;
					
					switch (typeinf->type) {
						case NodeType::SIMPLE :
							switch (prim_exp->id_info->type_info->type) {
								case NodeType::SIMPLE :
									if ((typeinf->type_specifier.simple_type[0].number == KEY_VOID) &&
									    (prim_exp->id_info->type_info->type_specifier.simple_type[0].number == KEY_FLOAT ||
									     prim_exp->id_info->type_info->type_specifier.simple_type[0].number == KEY_DOUBLE)) {
										Log::error_at(assgnexpr->tok.loc, "incompatible types for assignment to '" + assgnleft->tok.string + "'");
									}
									break;
								case NodeType::RECORD :
									if (typeinf->type_specifier.simple_type[0].number != KEY_INT && typeinf->type_specifier.simple_type[0].number != KEY_VOID)
										Log::error_at(assgnexpr->tok.loc, "incompatible types for assignment to '" + assgnleft->tok.string + "'");
									break;
                                default:
                                    break;                                    
							}
							break;
						case NodeType::RECORD :
							switch (prim_exp->id_info->type_info->type) {
								case NodeType::SIMPLE :
									if (prim_exp->id_info->type_info->type_specifier.simple_type[0].number == KEY_INT ||
									    prim_exp->id_info->type_info->type_specifier.simple_type[0].number == KEY_VOID) {
										Log::error_at(assgnexpr->tok.loc, "incompatible types for assignment to45 '" + assgnleft->tok.string + "'");
										return;
									}
									break;
								case NodeType::RECORD :
									break;
                                default:
                                    break;                                    
							}
							break;
                        default:
                            break;
					}
				}
				
				if (typeinf->type == NodeType::RECORD && prim_exp->is_id) {
					if (typeinf->type_specifier.record_type.string != prim_exp->id_info->type_info->type_specifier.record_type.string) {
						Log::error_at(assgnexpr->tok.loc, "incompatible types for assignment to '" + assgnleft->tok.string + "'");
					}
					
					if (typeinf->type_specifier.record_type.string == prim_exp->id_info->type_info->type_specifier.record_type.string &&
					    assgnleft->id_info->is_ptr != prim_exp->id_info->is_ptr &&
					    assgnleft->id_info->ptr_oprtr_count != prim_exp->id_info->ptr_oprtr_count) {
						Log::error_at(assgnexpr->tok.loc, "incompatible types for assignment to '" + assgnleft->tok.string + "'");
					}
				}
				
				break;
			}
			case ExpressionType::CAST_EXPR : {
				CastExpression *cast_exp = assgnexpr->expression->cast_expr;

				if (typeinf->type == NodeType::SIMPLE && cast_exp->is_simple_type) {
					if ((typeinf->type_specifier.simple_type[0].number == KEY_FLOAT ||
					     typeinf->type_specifier.simple_type[0].number == KEY_DOUBLE) &&
					    (cast_exp->simple_type[0].number == KEY_FLOAT ||
					     cast_exp->simple_type[0].number == KEY_DOUBLE)) {
						
						idright = get_assgnexpr_idexpr_attribute(cast_exp->target);
						if (idright == nullptr)
							return;
						if (idright->id_info->is_ptr)
							Log::error_at(assgnexpr->tok.loc, "incompatible types for assignment by casting to '" + assgnleft->tok.string + "'");
					}
					//record type = record type
				} else if (typeinf->type == NodeType::RECORD && !cast_exp->is_simple_type) {
					if (typeinf->type_specifier.record_type.string != cast_exp->identifier.string)
						Log::error_at(assgnexpr->tok.loc, "incompatible types for assignment by casting to '" + assgnleft->tok.string + "'");
				} else {
					idright = get_assgnexpr_idexpr_attribute(cast_exp->target);
					if (idright == nullptr)
						return;
				}
				break;
			}

			case ExpressionType::ID_EXPR :
				if (assgnexpr->expression->id_expr->tok.number == ADDROF_OP) {
					
					analyze_id_expr(&assgnexpr->expression->id_expr->unary);
					idright = get_assgnexpr_idexpr_attribute(assgnexpr->expression->id_expr->unary);
					if (idright == nullptr)
						return;
					
					if (!assgnleft->id_info->is_ptr) {
						Log::error_at(assgnexpr->tok.loc, "pointer type expected to the left hand side '" + assgnleft->tok.string + "'");
						return;
					}
					
					if (idright->id_info != nullptr && assgnleft->id_info->is_ptr && idright->id_info->is_ptr &&
					    assgnleft->id_info->ptr_oprtr_count <= idright->id_info->ptr_oprtr_count) {
						Log::error_at(assgnexpr->tok.loc, "invalid pointer type assignment ");
						return;
					}
					
					if (assgnleft && typeinf->type == NodeType::RECORD && idright->id_info->type_info->type != NodeType::RECORD) {
						if (idright->id_info->type_info->type == NodeType::SIMPLE &&
						    idright->id_info->type_info->type_specifier.simple_type[0].number != KEY_INT) {
							Log::error_at(assgnexpr->tok.loc, "invalid pointer type assignment ");
							return;
						}
					}
				} else {
					idright = get_assgnexpr_idexpr_attribute(assgnexpr->expression->id_expr);
					if (idright == nullptr)
						return;
					if (!check_assignment_type_argument(assgnexpr, ExpressionType::ID_EXPR, idright, nullptr)) {
						return;
					}
					if (idright->id_info != nullptr && assgnleft->id_info->is_ptr && idright->id_info->is_ptr &&
					    assgnleft->id_info->ptr_oprtr_count != idright->id_info->ptr_oprtr_count) {
						Log::error_at(assgnexpr->tok.loc, "invalid pointer type assignment ");
					} else if (idright->id_info != nullptr && assgnleft->id_info->is_ptr && !idright->id_info->is_ptr) {
						if (idright->id_info->type_info->type_specifier.simple_type[0].number != KEY_INT) {
							Log::error_at(assgnexpr->tok.loc, "invalid type assignment4 '" + idright->id_info->tok.string + "' to '" + assgnleft->id_info->tok.string + "'");
							return;
						}
					}
					if (assgnleft && idright->id_info && typeinf->type == NodeType::RECORD && idright->id_info->type_info->type != NodeType::RECORD) {
						Log::error_at(assgnexpr->tok.loc, "invalid type assignment '" + idright->id_info->tok.string + "' to '" + assgnleft->id_info->tok.string + "'");
						return;
					} else if (assgnleft && idright->id_info && typeinf->type == NodeType::SIMPLE && idright->id_info->type_info->type != NodeType::SIMPLE) {
						return;
					} else if (assgnleft && idright->id_info && assgnleft->id_info->is_ptr && typeinf->type == NodeType::RECORD && idright->id_info->type_info->type != NodeType::RECORD) {
						Log::error_at(assgnexpr->tok.loc, "invalid type assignment '" + idright->id_info->tok.string + "' to '" + assgnleft->id_info->tok.string + "'");
						return;
					} else if (assgnleft && idright->id_info && assgnleft->id_info->is_ptr && typeinf->type == NodeType::RECORD && idright->id_info->type_info->type == NodeType::SIMPLE) {
						if (idright->id_info->type_info->type_specifier.simple_type[0].number != KEY_INT) {
							Log::error_at(assgnexpr->tok.loc, "invalid type assignment '" + idright->id_info->tok.string + "' to '" + assgnleft->id_info->tok.string + "'");
							return;
						}
					}
				}
				break;
			
			case ExpressionType::FUNC_CALL_EXPR : {
				if (assgnexpr->expression->call_expr == nullptr)
					return;
				
				findit = Compiler::func_table->find(assgnexpr->expression->call_expr->function->tok.string);
				if (findit == Compiler::func_table->end())
					return;
				
				funcinfo = findit->second;
				
				if (funcinfo != nullptr) {
					if (typeinf->type != funcinfo->return_type->type) {
						Log::error_at(assgnexpr->tok.loc, "mismatched type assignment of function-call '" + funcinfo->func_name + "' to '" + assgnleft->id_info->tok.string + "'");
						return;
					}
					
					if (funcinfo->return_type == nullptr)
						return;
					if (typeinf == nullptr)
						return;
					
					switch (typeinf->type) {
						case NodeType::SIMPLE :
							if (typeinf->type_specifier.simple_type[0].number != funcinfo->return_type->type_specifier.simple_type[0].number) {
								Log::error_at(assgnexpr->tok.loc,
										"mismatched type assignment of function-call '" + funcinfo->func_name + "' to '"
										+ assgnleft->id_info->tok.string + "'");
								return;
							}
							if (assgnleft->id_info->ptr_oprtr_count != funcinfo->ptr_oprtr_count) {
								Log::error_at(assgnexpr->tok.loc,
										"mismatched pointer type assignment of function-call '" + funcinfo->func_name + "' to '" + assgnleft->id_info->tok.string + "'");
								
								return;
							}
							break;
						
						case NodeType::RECORD :
							if (typeinf->type_specifier.record_type.string !=
							    funcinfo->return_type->type_specifier.record_type.string) {
								Log::error_at(assgnexpr->tok.loc,
										"mismatched type assignment of function-call '" + funcinfo->func_name + "' to '" + assgnleft->id_info->tok.string + "'");
								return;
							}
							if (assgnleft->id_info->ptr_oprtr_count != funcinfo->ptr_oprtr_count) {
								Log::error_at(assgnexpr->tok.loc,
										"mismatched pointer type assignment of function-call '" + funcinfo->func_name + "' to '" + assgnleft->id_info->tok.string + "'");
								return;
							}
							break;
                        default:
                            break;
					}
				}
			}
				break;
			
			default:
				break;
		}
	}
	
	void Analyzer::analyze_funccall_expr(CallExpression **_funcallexpr) {

		CallExpression *funcexpr = *_funcallexpr;
		FunctionInfo *funcinfo = nullptr;
		std::map<std::string, FunctionInfo *>::iterator findit;
		std::list<Expression *>::iterator func_exprit;
		
		if (funcexpr == nullptr)
			return;
		
		findit = Compiler::func_table->find(funcexpr->function->tok.string);
		if (findit == Compiler::func_table->end()) {
			Log::error_at(funcexpr->function->tok.loc, "undeclared function called '" + funcexpr->function->tok.string + "'");
			return;
		}
		
		funcinfo = findit->second;
		if (funcinfo != nullptr) {
			if (funcinfo->param_list.size() != funcexpr->expression_list.size()) {
				Log::error_at(funcexpr->function->tok.loc,
						"In function call '" + funcexpr->function->tok.string + "', require " + std::to_string(funcinfo->param_list.size()) + " arguments");
				return;
			}
		}
		
		for (auto exp: funcexpr->expression_list)
			analyze_expr(&exp);
	}
	
	void Analyzer::analyze_expr(Expression **__expr) {
		Expression *_expr = *__expr;
		if (_expr == nullptr)
			return;
		
		switch (_expr->expr_kind) {
			case ExpressionType::PRIMARY_EXPR :
				analyze_primary_expr(&(_expr->primary_expr));
				break;
			case ExpressionType::ASSGN_EXPR :
				analyze_assgn_expr(&(_expr->assgn_expr));
				break;
			case ExpressionType::SIZEOF_EXPR :
				analyze_sizeof_expr(&(_expr->sizeof_expr));
				break;
			case ExpressionType::CAST_EXPR :
				analyze_cast_expr(&(_expr->cast_expr));
				break;
			case ExpressionType::ID_EXPR :
				analyze_id_expr(&(_expr->id_expr));
				break;
			case ExpressionType::FUNC_CALL_EXPR :
				analyze_funccall_expr(&(_expr->call_expr));
				break;
		}
	}
	
	void Analyzer::analyze_label_statement(LabelStatement **labelstmt) {

		std::map<std::string, Token>::iterator labels_it;
		if (*labelstmt == nullptr)
			return;
		
		labels_it = labels.find((*labelstmt)->label.string);
		if (labels_it != labels.end()) {
			Log::error_at((*labelstmt)->label.loc, "duplicate label '" + (*labelstmt)->label.string + "'");
			return;
		} else
			labels.insert(std::pair<std::string, Token>((*labelstmt)->label.string, (*labelstmt)->label));
	}
	
	void Analyzer::analyze_selection_statement(SelectStatement **selstmt) {

		if (*selstmt == nullptr)
			return;
		
		analyze_expr(&((*selstmt)->condition));
		analyze_statement(&((*selstmt)->if_statement));
		analyze_statement(&((*selstmt)->else_statement));
	}
	
	void Analyzer::analyze_iteration_statement(IterationStatement **iterstmt) {
		if (*iterstmt == nullptr)
			return;
		
		break_inloop++;
		continue_inloop++;
		
		switch ((*iterstmt)->type) {
			case IterationType::WHILE :
				analyze_expr(&((*iterstmt)->_while.condition));
				analyze_statement(&((*iterstmt)->_while.statement));
				break;
			
			case IterationType::DOWHILE :
				analyze_expr(&((*iterstmt)->_dowhile.condition));
				analyze_statement(&((*iterstmt)->_dowhile.statement));
				break;
			
			case IterationType::FOR :
				analyze_expr(&((*iterstmt)->_for.init_expr));
				analyze_expr(&((*iterstmt)->_for.condition));
				analyze_expr(&((*iterstmt)->_for.update_expr));
				analyze_statement(&((*iterstmt)->_for.statement));
				break;
		}
	}
	
	void Analyzer::analyze_return_jmpstmt(JumpStatement **jmpstmt) {

		TypeInfo *returntype = nullptr;
		analyze_expr(&((*jmpstmt)->expression));
		
		if (func_symtab != nullptr) {
			if (func_symtab->func_info != nullptr) {
				returntype = func_symtab->func_info->return_type;
			} else
				return;
			
		} else
			return;
		
		switch (returntype->type) {
			case NodeType::SIMPLE :
				if (returntype->type_specifier.simple_type[0].number == KEY_VOID && (*jmpstmt)->expression != nullptr) {
					Log::error_at((*jmpstmt)->tok.loc, "return with value having 'void' function return type ");
					return;
				}
				break;
			case NodeType::RECORD :
				break;
            default:
                break;
		}
	}
	
	void Analyzer::analyze_jump_statement(JumpStatement **jmpstmt) {

		if (*jmpstmt == nullptr)
			return;
		
		switch ((*jmpstmt)->type) {
			case JumpType::BREAK :
				if (break_inloop > 0)
					break_inloop--;
				else {
					Log::error_at((*jmpstmt)->tok.loc, "not in loop/redeclared in loop, break");
					return;
				}
				break;
			case JumpType::CONTINUE :
				if (continue_inloop > 0)
					continue_inloop--;
				else {
					Log::error_at((*jmpstmt)->tok.loc, "not in loop/redeclared in loop, continue");
					return;
				}
				break;
			case JumpType::RETURN :
				analyze_return_jmpstmt(&(*jmpstmt));
				break;
			case JumpType::GOTO :
				goto_list.push_back((*jmpstmt)->goto_id);
				break;
		}
	}
	
	void Analyzer::analyze_goto_jmpstmt() {

		std::list<Token>::iterator it;
		std::map<std::string, Token>::iterator labels_it;
		
		for (it = goto_list.begin(); it != goto_list.end(); it++) {
			labels_it = labels.find(it->string);
			if (labels_it == labels.end()) {
				Log::error_at(it->loc, "label '" + it->string + "' does not exists");
				return;
			}
		}
		goto_list.clear();
	}
	
	bool Analyzer::is_digit(char ch) {
		return ((ch - '0' >= 0) && (ch - '0' <= 9));
	}
	
	std::string Analyzer::get_template_token(std::string asmtemplate) {

		std::string asmtoken;
		unsigned i;
		
		for (i = 0; i < asmtemplate.length(); i++) {
			if (is_digit(asmtemplate.at(i))) {
				asmtoken.push_back(asmtemplate.at(i));
                continue;
			} 
            break;
		}

		return asmtoken;
	}
	
	std::vector<int> Analyzer::get_asm_template_tokens_vector(Token tok) {

		size_t loc;
		std::vector<int> v;
		std::string asmtoken;
		std::string asmtemplate = tok.string;
		
		loc = asmtemplate.find_first_of("%");
		while (loc != std::string::npos) {

			if (loc + 1 >= asmtemplate.size())
				break;

			asmtemplate = asmtemplate.substr(loc + 1, asmtemplate.length() - loc);
			asmtoken = get_template_token(asmtemplate);

			if (!asmtoken.empty())
				v.push_back(std::stoi(asmtoken));

			loc = asmtemplate.find_first_of("%");
		}
		return v;
	}
	
	void Analyzer::analyze_asm_template(AsmStatement **asmstmt) {
		
        AsmStatement *asmstmt2 = *asmstmt;
		std::vector<int> v;
		size_t maxelem;
		size_t operandsize;
		
		if (asmstmt2->output_operand.empty())
			return;
		
        if (asmstmt2->input_operand.empty())
			return;
		
		v = get_asm_template_tokens_vector(asmstmt2->asm_template);
		operandsize = asmstmt2->output_operand.size() + asmstmt2->input_operand.size();
		
        if (v.size() > 1) {
			maxelem = *(std::max_element(std::begin(v), std::end(v)));
			if (maxelem > operandsize - 1)
				Log::error_at(asmstmt2->asm_template.loc, "asm Operand number out of range '%" + std::to_string(maxelem) + "'");
		}
	}
	
	void Analyzer::analyze_asm_output_operand(AsmOperand **Operand) {

		if (*Operand == nullptr)
			return;

		Token constrainttok = (*Operand)->constraint;
		std::string constraint = constrainttok.string;
		size_t len = constraint.length();
		char ch;
		
		if (constraint.empty()) {
			Log::error_at(constrainttok.loc, "asm output Operand constraint lacks '='");
			return;
		}

		if (len == 1) {
			if (constraint.at(0) == '=')
				Log::error_at(constrainttok.loc, "asm impossible constraint '='");
			else
				Log::error_at(constrainttok.loc, "asm output Operand constraint lacks '='");
			return;
		} else if (len > 1) {
			if (constraint.at(0) == '=') {
				ch = constraint.at(1);
				if (ch == 'a' || ch == 'b' || ch == 'c' || ch == 'd' || ch == 'S' || ch == 'D' || ch == 'm') {
					if (ch == 'm') {
						if ((*Operand)->expression == nullptr) 
							Log::error_at(constrainttok.loc, "asm constraint '=m' requires memory location id");
						else 
							analyze_expr(&(*Operand)->expression);
					}
				} else {
					Log::error_at(constrainttok.loc, "asm inconsistent Operand constraints '" + constraint + "'");
				}
			} else {
				Log::error_at(constrainttok.loc, "asm output Operand constraint lacks '='");
			}
		}
	}
	
	void Analyzer::analyze_asm_input_operand(AsmOperand **Operand) {

		if (*Operand == nullptr)
			return;

		Token constrainttok = (*Operand)->constraint;
		std::string constraint = constrainttok.string;
		size_t len = constraint.length();
		char ch;
		
		if (len > 0) {
			ch = constraint.at(0);
			if (ch == 'a' || ch == 'b' || ch == 'c' || ch == 'd' || ch == 'S' || ch == 'D' || ch == 'm' || ch == 'i') {
				if (ch == 'm') {
					if ((*Operand)->expression == nullptr)
						Log::error_at(constrainttok.loc, "asm constraint 'm' requires memory location id");
					else
						analyze_expr(&(*Operand)->expression);
				}
			} else
				Log::error_at(constrainttok.loc, "asm inconsistent Operand constraints '" + constraint + "'");
		}
	}
	
	void Analyzer::analyze_asm_operand_expr(Expression **_expr) {

		Expression *expr2 = *_expr;
		if (expr2 == nullptr)
			return;

		switch (expr2->expr_kind) {
			case ExpressionType::PRIMARY_EXPR :
				if (expr2->primary_expr == nullptr)
					return;

				if (expr2->primary_expr->left != nullptr ||
				    expr2->primary_expr->right != nullptr ||
				    expr2->primary_expr->unary_node != nullptr) {
					Log::error_at(expr2->primary_expr->tok.loc, "only single node primary expression expected in asm Operand");
				}

				break;
			default:
				Log::error("only single node primary expression expected in asm Operand");
				return;
		}
	}
	
	void Analyzer::analyze_asm_statement(AsmStatement **asmstmt) {

		AsmStatement *asmstmt2 = *asmstmt;
		std::vector<AsmOperand *>::iterator it;

		if (asmstmt2 == nullptr)
			return;
		
		while (asmstmt2 != nullptr) {
			//amalyze asm template
			analyze_asm_template(&asmstmt2);
			//analyze asm output operands
			it = asmstmt2->output_operand.begin();
			while (it != asmstmt2->output_operand.end()) {
				analyze_asm_output_operand(&(*it));
				analyze_asm_operand_expr(&((*it)->expression));
				it++;
			}
			//analyze input operands
			it = asmstmt2->input_operand.begin();
			while (it != asmstmt2->input_operand.end()) {
				analyze_asm_input_operand(&(*it));
				analyze_asm_operand_expr(&((*it)->expression));
				it++;
			}
			asmstmt2 = asmstmt2->p_next;
		}
	}
	
	void Analyzer::analyze_statement(Statement **_stmt) {

		Statement *_stmt2 = *_stmt;
		if (_stmt2 == nullptr)
			return;
		
		while (_stmt2 != nullptr) {
			switch (_stmt2->type) {
				case StatementType::LABEL :
					analyze_label_statement(&_stmt2->labled_statement);
					break;
				case StatementType::EXPR :
					analyze_expr(&(_stmt2->expression_statement->expression));
					break;
				case StatementType::SELECT :
					analyze_selection_statement(&(_stmt2->selection_statement));
					break;
				case StatementType::ITER :
					analyze_iteration_statement(&(_stmt2->iteration_statement));
					break;
				case StatementType::JUMP :
					analyze_jump_statement(&(_stmt2->jump_statement));
					break;
				case StatementType::DECL :
					break;
				case StatementType::ASM :
					analyze_asm_statement(&(_stmt2->asm_statement));
					break;
			}
			_stmt2 = _stmt2->p_next;
		}
	}
	
	void Analyzer::analyze_func_param_info(FunctionInfo **funcinfo) {

		std::list<FuncParamInfo *>::iterator it;
		if (*funcinfo == nullptr)
			return;
		
		if ((*funcinfo)->is_extern)
			return;
		
		if ((*funcinfo)->param_list.size() > 0) {
			it = (*funcinfo)->param_list.begin();
			while (it != (*funcinfo)->param_list.end()) {
				if ((*it)->type_info != nullptr) {
					if ((*it)->symbol_info == nullptr) {
						Log::error_at((*funcinfo)->tok.loc, "identifier expected in function parameter '" + (*funcinfo)->func_name + "'");
						return;
					} else if ((*it)->symbol_info->symbol.empty()) {
						Log::error_at((*funcinfo)->tok.loc, "identifier expected in function parameter '" + (*funcinfo)->func_name + "'");
						return;
					}
				}
				it++;
			}
		}
	}
	
	bool Analyzer::has_constant_member(PrimaryExpression *pexpr) {
		if (pexpr == nullptr)
			return true;
		
		if (pexpr->is_id)
			return (has_constant_member(pexpr->left) && has_constant_member(pexpr->right));
		else
			return (has_constant_member(pexpr->left) && has_constant_member(pexpr->right));
		
		return true;
	}
	
	bool Analyzer::has_constant_array_subscript(IdentifierExpression *idexpr) {
		bool b = true;
		if (idexpr == nullptr)
			return true;
		if (idexpr->is_subscript) {
			for (auto x: idexpr->subscript) {
				if (x.number == LIT_BIN     ||
                    x.number == LIT_DECIMAL || 
                    x.number == LIT_HEX     || 
                    x.number == LIT_OCTAL) {
					b &= true;
				} else
					b &= false;
			}
		}
		return b;
	}
	
	void Analyzer::analyze_global_assignment(TreeNode **trnode) {

		TreeNode *trhead = *trnode;
		Statement *stmthead = nullptr;
		Expression *_expr = nullptr;

		if (trhead == nullptr)
			return;
		
		while (trhead != nullptr) {
			if (trhead->symtab != nullptr) {
				if (trhead->symtab->func_info != nullptr) {
					trhead = trhead->p_next;
					continue;
				}
			}
			
			stmthead = trhead->statement;
			if (stmthead == nullptr)
				return;
			
			while (stmthead != nullptr) {
				if (stmthead->type == StatementType::EXPR) {
					_expr = stmthead->expression_statement->expression;
					if (_expr != nullptr) {
						switch (_expr->expr_kind) {
							case ExpressionType::ASSGN_EXPR :
								if (_expr->assgn_expr->expression == nullptr)
									return;
								if (!has_constant_array_subscript(_expr->assgn_expr->id_expr))
									Log::error_at(_expr->assgn_expr->tok.loc, "constant expression expected in array subscript");
								if (_expr->assgn_expr->expression->expr_kind == ExpressionType::PRIMARY_EXPR) {
									PrimaryExpression *pexpr = _expr->assgn_expr->expression->primary_expr;
									if (pexpr->left != nullptr || pexpr->right != nullptr)
										Log::error_at(_expr->assgn_expr->tok.loc, "constant expression expected ");
								} else
									Log::error_at(_expr->assgn_expr->tok.loc, "expected constant primary expression ");
								break;
							case ExpressionType::PRIMARY_EXPR :
								Log::error_at(_expr->primary_expr->tok.loc, "expected assignment expression ");
								break;
							case ExpressionType::SIZEOF_EXPR :
								Log::error_at(_expr->sizeof_expr->identifier.loc, "expected assignment expression ");
								break;
							case ExpressionType::CAST_EXPR :
								Log::error_at(_expr->cast_expr->identifier.loc, "expected assignment expression ");
								break;
							case ExpressionType::ID_EXPR :
								Log::error_at(_expr->id_expr->tok.loc, "expected assignment expression ");
								break;
							case ExpressionType::FUNC_CALL_EXPR :
								Log::error("unexpected function call expression ");
								break;
						}
					}
				}
				stmthead = stmthead->p_next;
			}
			trhead = trhead->p_next;
		}
	}
	
	void Analyzer::analyze_func_params(FunctionInfo *func_params) {
		if (func_params == nullptr)
			return;
		if (func_params->param_list.size() == 1)
			return;
		if (func_params->is_extern)
			return;
		for (FuncParamInfo *param: func_params->param_list) {
			if (param == nullptr)
				return;
			for (FuncParamInfo *param2: func_params->param_list) {
				if (param2 == nullptr)
					return;
				if (param != param2 && (param->symbol_info->symbol == param2->symbol_info->symbol)) {
					Log::error_at(param2->symbol_info->tok.loc, "same name used in function parameter '" + param2->symbol_info->symbol + "'");
					return;
				}
			}
		}
	}
	
	void Analyzer::analyze_local_declaration(TreeNode **trnode) {

		TreeNode *trhead = *trnode;
		if (trhead == nullptr)
			return;
		
		while (trhead != nullptr) {
			if (trhead->symtab != nullptr) {
				func_symtab = trhead->symtab;
				func_info = trhead->symtab->func_info;
				
				if (func_symtab != nullptr && func_info != nullptr)
					analyze_func_params(func_info);
				
				for (FuncParamInfo *param: func_info->param_list) {
					if (param != nullptr && param->symbol_info != nullptr) {
						if (SymbolTable::search_symbol(func_symtab, param->symbol_info->symbol)) {
							Log::error_at(param->symbol_info->tok.loc, "redeclaration of '" + param->symbol_info->symbol + "', same name used for function parameter");
						}
					}
				}
			}
			trhead = trhead->p_next;
		}
	}
	
	void Analyzer::analyze(TreeNode **trnode) {
		TreeNode *trhead = nullptr;
		parse_tree = trhead = *trnode;
		
		if (trhead == nullptr)
			return;
		
		check_invalid_type_declaration(Compiler::symtab);
		while (trhead != nullptr) {
			if (trhead->symtab != nullptr) {
				analyze_func_param_info(&trhead->symtab->func_info);
				func_info = trhead->symtab->func_info;
			}

			func_symtab = trhead->symtab;
			check_invalid_type_declaration(func_symtab);
			analyze_statement(&trhead->statement);
			analyze_goto_jmpstmt();
			labels.clear();
			trhead = trhead->p_next;
		}
		
		//one pass for localy declared variables
		analyze_local_declaration(&(*trnode));
		
		//one pass for globally declared expressions
		analyze_global_assignment(&(*trnode));
	}
}
/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

// NASM code generation

#include "log.hpp"
#include "parser.hpp"
#include "convert.hpp"
#include "gen.hpp"
#include "compiler.hpp"

namespace xlang {

	int CodeGen::data_type_size(Token tok) {
		switch (tok.number) {
			case KEY_VOID:
			case KEY_CHAR:
				return 1;
			case KEY_SHORT:
				return 2;
			case KEY_INT:
			case KEY_LONG:
			case KEY_FLOAT:
				return 4;
			case KEY_DOUBLE:
				return 8;
			default:
				return 0;
		}
	}

	int CodeGen::data_decl_size(DeclarationType ds) {
		switch (ds) {
			case DB :
				return 1;
			case DW :
				return 2;
			case DD :
				return 4;
			case DQ :
				return 8;
			default:
				return 0;
		}
	}

	int CodeGen::resv_decl_size(ReservationType rs) {
		switch (rs) {
			case RESB:
				return 1;
			case RESW:
				return 2;
			case RESD:
				return 4;
			case RESQ:
				return 8;
			default:
				return 0;
		}
	}

	DeclarationType CodeGen::declspace_type_size(Token tok) {
		int sz = data_type_size(tok);
		switch (sz) {
			case 1:
				return DB;
			case 2:
				return DW;
			case 4:
				return DD;
			case 8:
				return DQ;
			default:
				return DSPNONE;
		}
	}

	ReservationType CodeGen::resvspace_type_size(Token tok) {
		int sz = data_type_size(tok);
		switch (sz) {
			case 1:
				return RESB;
			case 2:
				return RESW;
			case 4:
				return RESD;
			case 8:
				return RESQ;
			default:
				return RESPNONE;
		}
	}

	bool CodeGen::has_float(PrimaryExpression *pexpr) {

		// returns true if any node of primary expression
		// has float literal or float/double data type

		Token type;
		if (pexpr == nullptr)
			return false;
		if (pexpr->is_id) {

			if (pexpr->id_info == nullptr)
				return false;

			if (pexpr->id_info->type_info->type == NodeType::SIMPLE) {
				type = pexpr->id_info->type_info->type_specifier.simple_type[0];

				if (type.number == KEY_FLOAT || type.number == KEY_DOUBLE)
					return true;
				else
					return (has_float(pexpr->left) || has_float(pexpr->right));
			}
			else
				return (has_float(pexpr->left) || has_float(pexpr->right));
		}
		else if (pexpr->is_oprtr)
			return (has_float(pexpr->left) || has_float(pexpr->right));
		else {
			if (pexpr->tok.number == LIT_FLOAT)
				return true;
			else
				return (has_float(pexpr->left) || has_float(pexpr->right));
		}
		return false;
	}

	void CodeGen::max_datatype_size(PrimaryExpression *pexpr, int *dsize) {

		// returns maximum data type size used in primary expression
		// by checking each node data type size

		Token type;
		int dsize2 = 0;
		if (pexpr == nullptr)
			return;
		if (pexpr->is_id) {
			if (pexpr->id_info == nullptr) {
				*dsize = 0;
				return;
			}
			if (pexpr->id_info->type_info->type == NodeType::SIMPLE) {
				type = pexpr->id_info->type_info->type_specifier.simple_type[0];
				dsize2 = data_type_size(type);
				if (*dsize < dsize2) {
					*dsize = dsize2;
				}
			}
			else {
				max_datatype_size(pexpr->left, &(*dsize));
				max_datatype_size(pexpr->right, &(*dsize));
			}
		}
		else if (pexpr->is_oprtr) {
			max_datatype_size(pexpr->left, &(*dsize));
			max_datatype_size(pexpr->right, &(*dsize));
		}
		else {
			switch (pexpr->tok.number) {
				case LIT_CHAR :
					if (*dsize < 1)
						*dsize = 1;
					break;

				case LIT_BIN :
				case LIT_DECIMAL :
				case LIT_HEX :
				case LIT_OCTAL :
				case LIT_FLOAT :
					if (*dsize < 4)
						*dsize = 4;
					break;
				default:
					max_datatype_size(pexpr->left, &(*dsize));
					max_datatype_size(pexpr->right, &(*dsize));
					break;
			}
		}
	}

	void CodeGen::get_func_local_members() {

		// generate function local members on stack

		LocalMembers flm;
		FunctionMember fm;
		size_t index;
		int fp = 0;
		int total = 0;
		SymbolInfo *syminf = nullptr;
		if (func_symtab == nullptr)
			return;

		// allocate members from function symbol table
		// according to its data type sizes
		// by adjusting stack frame pointer
		// and store them in FunctionMember structure 

		for (index = 0; index < ST_SIZE; ++index) {
			syminf = func_symtab->symbol_info[index];
			while (syminf != nullptr && syminf->type_info != nullptr) {
				switch (syminf->type_info->type) {
					case NodeType::SIMPLE :
						if (syminf->is_ptr) {
							fm.insize = 4;
							fp = fp - 4;
							fm.fp_disp = fp;
							total += 4;
						}
						else {
							fm.insize = data_type_size(syminf->type_info->type_specifier.simple_type[0]);
							fp = fp - fm.insize;
							fm.fp_disp = fp;
							total += fm.insize;
						}
						flm.members.insert(std::pair<std::string, FunctionMember>(syminf->symbol, fm));
						break;
					case NodeType::RECORD :
						fm.insize = 4;
						fp = fp - 4;
						fm.fp_disp = fp;
						total += 4;
						flm.members.insert(std::pair<std::string, FunctionMember>(syminf->symbol, fm));
						break;
					default:
						break;
				}
				syminf = syminf->p_next;
			}
		}

		flm.total_size = total;

		// allocate function parameters on stack
		// fp = 4(ebp) always contain return address
		// when call invoked.
		// so allocating above that

		fp = 4;
		for (FuncParamInfo *fparam: func_symtab->func_info->param_list) {

			if (fparam == nullptr)
				break;

			switch (fparam->type_info->type) {
				case NodeType::SIMPLE :
					if (fparam->symbol_info->is_ptr) {
						fm.insize = 4;
						fp = fp + 4;
						fm.fp_disp = fp;
					}
					else {
						fm.insize = data_type_size(fparam->type_info->type_specifier.simple_type[0]);
						fp = fp + 4;
						fm.fp_disp = fp;
					}

					flm.members.insert(std::pair<std::string, FunctionMember>(fparam->symbol_info->symbol, fm));
					break;

				case NodeType::RECORD :
					fm.insize = 4;
					fp = fp + 4;
					fm.fp_disp = fp;
					flm.members.insert(std::pair<std::string, FunctionMember>(fparam->symbol_info->symbol, fm));
					break;

				default:
					break;
			}
		}

		func_members.insert(std::pair<std::string, LocalMembers>(func_symtab->func_info->func_name, flm));
	}

	SymbolInfo *CodeGen::search_func_params(const std::string &str) {

		//search symbol in function parameters

		if (func_params == nullptr)
			return nullptr;

		if (func_params->param_list.size() > 0) {
			for (auto syminf: func_params->param_list) {
				if (syminf->symbol_info != nullptr) {
					if (syminf->symbol_info->symbol == str)
						return syminf->symbol_info;
				}
			}
		}
		return nullptr;
	}

	SymbolInfo *CodeGen::search_id(const std::string &str) {

		//search in symbol tables, same as in analyze.cpp

		SymbolInfo *syminf = nullptr;
		if (func_symtab != nullptr) {
			//search in function symbol table
			syminf = SymbolTable::search_symbol_node(func_symtab, str);
			if (syminf == nullptr) {
				//if null, then search in function parameters
				syminf = search_func_params(str);
				if (syminf == nullptr) {
					//if null, then search in global symbol table
					syminf = SymbolTable::search_symbol_node(Compiler::symtab, str);
				}
			}
		}
		else
			//if function symbol table null, then search in global symbol table
			syminf = SymbolTable::search_symbol_node(Compiler::symtab, str);

		return syminf;
	}

	InstructionSize CodeGen::get_insn_size_type(int sz) {

		//return Operand sizes used in instructions

		if (sz == 1)
			return BYTE;
		else if (sz == 2)
			return WORD;
		else if (sz == 4)
			return DWORD;
		else if (sz == 8)
			return QWORD;
		else
			return INSZNONE;
	}

	std::stack<PrimaryExpression *> CodeGen::get_post_order_prim_expr(PrimaryExpression *pexpr) {

		//get post order of an primary expression tree on stack

		std::stack<PrimaryExpression *> pexp_stack;
		std::stack<PrimaryExpression *> pexp_out_stack;
		PrimaryExpression *pexp_root = pexpr, *pexp = nullptr;

		//traverse tree post-orderly and get post order into pexp_out_stack
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
		return pexp_out_stack;
	}

	Instruction *CodeGen::get_insn(InstructionType instype, int oprcount) {

		//returns memory allocated instruction pointer

		Instruction *in = insncls->get_insn_mem();
		in->insn_type = instype;
		in->operand_count = oprcount;
		in->operand_1->is_array = false;
		in->operand_2->is_array = false;
		return in;
	}

	void CodeGen::insert_comment(const std::string &cmnt) {

		//add new instruction with only comment

		Instruction *in = get_insn(INSNONE, 0);
		in->comment = cmnt;
		insncls->delete_operand(&(in->operand_1));
		insncls->delete_operand(&(in->operand_2));
		instructions.push_back(in);
	}

	Member *CodeGen::search_data(const std::string &dt) {

		//search given data in data section vector

		for (Member *d: data_section)
			if (dt == d->value)
				return d;

		return nullptr;
	}

	Member *CodeGen::search_string_data(const std::string &dt) {

		//search given data in data section vector

		std::string hstr = get_hex_string(dt);
		for (Member *d: data_section)
			if (get_hex_string(dt) == d->value)
				return d;

		return nullptr;
	}

	std::string CodeGen::hex_escape_sequence(char ch) {

		//return escape sequence character hex value in 1 byte

		switch (ch) {
			case '\'':
				return "0x27";
			case '"':
				return "0x22";
			case '\\':
				return "0x5A";
			case 'a':
				return "0x07";
			case 'b':
				return "0x08";
			case 'f':
				return "0x0C";
			case 'n':
				return "0x0A";
			case 'r':
				return "0x0D";
			case 't':
				return "0x09";
			case 'v':
				return "0x0B";
			case '0':
				return "0x00";
			default:
				return "";
		}
	}

	std::string CodeGen::get_hex_string(const std::string &str) {

		//convert string into its hex representation, 1 byte each

		std::string result, esc_seq;
		size_t len = str.size();
		std::vector<std::string> vec;
		size_t index = 0;

		while (index < len) {
			if (str.at(index) == '\\') {
				if (index + 1 < len) {
					esc_seq = hex_escape_sequence(str.at(index + 1));
					if (!esc_seq.empty()) {
						result += esc_seq;
						result.push_back(',');
						index += 2;
					}
					else {
						result += "0x" + Convert::dec_to_hex(str.at(index));
						result.push_back(',');
						result += "0x" + Convert::dec_to_hex(str.at(index + 1));
						result.push_back(',');
						index += 2;
					}
				}
				else {
					result += "0x" + Convert::dec_to_hex(str.at(index));
					result.push_back(',');
					index++;
				}
			}
			else {
				result += "0x" + Convert::dec_to_hex(str.at(index));
				result.push_back(',');
				index++;
			}
		}

		result = result + "0x00";
		return result;
	}

	bool CodeGen::get_function_local_member(FunctionMember *fmemb, Token tok) {
		funcmem_iterator fmemit;
		memb_iterator memit;

		if (func_symtab == nullptr) {
			fmemb->insize = -1;
			return false;
		}

		if (tok.number != IDENTIFIER) {
			fmemb->insize = -1;
			return false;
		}

		fmemit = func_members.find(func_symtab->func_info->func_name);

		if (fmemit != func_members.end()) {

			memit = (fmemit->second.members).find(tok.string);

			if (memit != (fmemit->second.members).end()) {
				fmemb->insize = memit->second.insize;
				fmemb->fp_disp = memit->second.fp_disp;
				return true;
			}

			fmemb->insize = -1;
		}

		return false;
	}

	InstructionType CodeGen::get_arthm_op(const std::string &symbol) {
		//get arithmetic instruction type

		if (symbol == "+")
			return ADD;
		else if (symbol == "-")
			return SUB;
		else if (symbol == "*")
			return MUL;
		else if (symbol == "/")
			return DIV;
		else if (symbol == "%")
			return DIV;
		else if (symbol == "&")
			return AND;
		else if (symbol == "|")
			return OR;
		else if (symbol == "^")
			return XOR;
		else if (symbol == "<<")
			return SHL;
		else if (symbol == ">>")
			return SHR;
		return INSNONE;
	}

	RegisterType CodeGen::gen_int_primexp_single_assgn(PrimaryExpression *pexpr, int dtsize) {

		// generate x86 assembly of primary expression
		// when there is only one node in primary expression
		// e.g: x = x;

		Instruction *in = nullptr;
		RegisterType rs = RNONE;
		SymbolInfo *syminf = nullptr;

		if (pexpr == nullptr)
			return RNONE;

		FunctionMember fmem;
		if (dtsize == 1)
			rs = AL;
		else if (dtsize == 2)
			rs = AX;
		else
			rs = EAX;

		if (pexpr->left == nullptr && pexpr->right == nullptr) {

			if (pexpr->id_info != nullptr) {

				if (get_function_local_member(&fmem, pexpr->id_info->tok)) {

					in = get_insn(MOV, 2);
					in->operand_1->type = REGISTER;
					in->operand_2->type = MEMORY;
					in->operand_2->mem.mem_type = LOCAL;

					syminf = search_id(pexpr->id_info->symbol);
					if (syminf != nullptr && syminf->is_ptr) {

						if (Compiler::global.x64) {
							in->operand_1->reg = RAX;
							in->operand_2->mem.mem_size = 8;
						}
						else {
							in->operand_1->reg = EAX;
							in->operand_2->mem.mem_size = 4;
						}
					}
					else {
						in->operand_1->reg = rs;
						in->operand_2->mem.mem_size = dtsize;
					}

					in->operand_2->mem.fp_disp = fmem.fp_disp;
					in->comment = "  ; assignment " + pexpr->id_info->symbol;
					instructions.push_back(in);
				}
				else {

					in = get_insn(MOV, 2);
					in->operand_1->type = REGISTER;
					in->operand_1->reg = rs;
					in->operand_2->type = MEMORY;
					in->operand_2->mem.mem_type = GLOBAL;

					syminf = search_id(pexpr->id_info->symbol);
					if (syminf != nullptr && syminf->is_ptr) {
						if (Compiler::global.x64) {
							in->operand_1->reg = RAX;
							in->operand_2->mem.mem_size = 8;
						}
						else {
							in->operand_1->reg = EAX;
							in->operand_2->mem.mem_size = 4;
						}
					}
					else {
						in->operand_1->reg = rs;
						in->operand_2->mem.mem_size = dtsize;
					}

					in->operand_2->mem.name = pexpr->id_info->symbol;
					in->comment = "  ; assignment " + pexpr->id_info->symbol;
					instructions.push_back(in);
				}
			}
			else {
				in = get_insn(MOV, 2);
				in->operand_1->type = REGISTER;
				in->operand_1->reg = rs;
				in->operand_2->type = LITERAL;
				in->operand_2->literal = std::to_string(Convert::tok_to_decimal(pexpr->tok));
				instructions.push_back(in);
			}
			return rs;
		}
		return RNONE;
	}

	bool CodeGen::gen_int_primexp_compl(PrimaryExpression *pexpr, int dtsize) {

		// generate bit complement x86 assembly of primary expression

		Instruction *in = nullptr;
		FunctionMember fmem;
		if (pexpr == nullptr)
			return false;

		pexpr = pexpr->unary_node;
		insert_comment("; line " + std::to_string(pexpr->tok.loc.line));

		if (pexpr->left == nullptr && pexpr->right == nullptr) {
			if (pexpr->id_info != nullptr) {

				in = get_insn(NEG, 1);
				insncls->delete_operand(&in->operand_2);
				in->operand_1->type = MEMORY;

				if (get_function_local_member(&fmem, pexpr->id_info->tok)) {
					in->operand_1->mem.mem_type = LOCAL;
					in->operand_1->mem.mem_size = dtsize;
					in->operand_1->mem.fp_disp = fmem.fp_disp;
					in->comment = "  ; " + pexpr->id_info->symbol;
				}
				else {
					in->operand_1->mem.mem_type = GLOBAL;
					in->operand_1->mem.mem_size = dtsize;
					in->operand_1->mem.name = pexpr->id_info->symbol;
					in->comment = "  ; " + pexpr->id_info->symbol;
				}

				instructions.push_back(in);
			}
			return true;
		}
		return false;
	}

	Member *CodeGen::create_string_data(const std::string &value) {

		//create new data in data section with string

		Member *dt = insncls->get_data_mem();
		dt->symbol = "string_val" + std::to_string(string_data_count);
		dt->type = DB;
		dt->value = get_hex_string(value);
		dt->is_array = false;
		dt->comment = "    ; '" + value + "'";
		string_data_count++;
		return dt;
	}

	RegisterType CodeGen::gen_string_literal_primary_expr(PrimaryExpression *pexpr) {

		// generate string literal x86 assembly
		// data name is used in assignment or function call

		if (pexpr == nullptr)
			return RNONE;
		if (pexpr->left == nullptr && pexpr->right == nullptr) {
			if (pexpr->tok.number == LIT_STRING) {
				Member *dt = search_string_data(pexpr->tok.string);
				if (dt == nullptr) {
					dt = create_string_data(pexpr->tok.string);
					data_section.push_back(dt);
				}

				Instruction *in = get_insn(MOV, 2);
				in->operand_1->type = REGISTER;

				if (Compiler::global.x64)
					in->operand_1->reg = RAX;
				else
					in->operand_1->reg = EAX;

				in->operand_2->type = MEMORY;
				in->operand_2->mem.mem_type = GLOBAL;
				in->operand_2->mem.mem_size = -1;
				in->operand_2->mem.name = dt->symbol;
				instructions.push_back(in);

				if (Compiler::global.x64)
					return RAX;
				else
					return EAX;
			}
		}
		return RNONE;
	}

	RegisterType CodeGen::gen_int_primary_expr(PrimaryExpression *pexpr) {

		// generate int type x86 assembly of primary expression

		std::stack<PrimaryExpression *> pexp_stack;
		std::stack<PrimaryExpression *> pexp_out_stack;
		PrimaryExpression *pexp = nullptr, *fact1 = nullptr, *fact2 = nullptr;
		int dtsize = 0;
		size_t stsize = 0;
		RegisterType r1, r2;
		InstructionType op;
		Instruction *in;
		int push_count = 0;
		FunctionMember fmem;
		std::stack<RegisterType> result;
		std::set<PrimaryExpression *> common_node_set;

		if (pexpr == nullptr)
			return RNONE;

		//get maximum data type size
		max_datatype_size(pexpr, &dtsize);

		if (pexpr->unary_node != nullptr) {
			// check for bit complement operator
			if (pexpr->tok.number == BIT_COMPL) {
				max_datatype_size(pexpr->unary_node, &dtsize);
				if (gen_int_primexp_compl(pexpr, dtsize))
					return RNONE;
			}
		}

		//check for string type primary Expression
		r1 = gen_string_literal_primary_expr(pexpr);
		if (r1 != RNONE)
			return r1;

		if (dtsize <= 0)
			return RNONE;

		insert_comment("; line " + std::to_string(pexpr->tok.loc.line));

		//if only one node in primary Expression
		r1 = gen_int_primexp_single_assgn(pexpr, dtsize);
		if (r1 != RNONE) {
			return r1;
		}

		pexp_out_stack = get_post_order_prim_expr(pexpr);

		//cleraing out registers eax and edx for arithmetic operations
		in = get_insn(XOR, 2);

		in->operand_1->type = REGISTER;
		Compiler::global.x64 ? in->operand_1->reg = RAX : in->operand_1->reg = EAX;

		in->operand_2->type = REGISTER;
		Compiler::global.x64 ? in->operand_2->reg = RAX : in->operand_2->reg = EAX;

		instructions.push_back(in);

		in = get_insn(XOR, 2);
		in->operand_1->type = REGISTER;

		Compiler::global.x64 ? in->operand_1->reg = RDX : in->operand_1->reg = EDX;
		in->operand_2->type = REGISTER;

		Compiler::global.x64 ? in->operand_2->reg = RDX : in->operand_2->reg = EDX;
		instructions.push_back(in);

		while (!pexp_out_stack.empty()) {
			pexp = pexp_out_stack.top();
			if (pexp->is_oprtr) {
				stsize = pexp_stack.size();

				//generate code when common-subexpression node found after optimization
				if (common_node_set.find(pexp) != common_node_set.end()) {
					if (stsize >= 2) {
						pexp_stack.pop();
						pexp_stack.pop();
						if (!pexp_out_stack.empty())
							pexp_out_stack.pop();
						stsize = 0;
						push_count = 0;
						continue;
					}
				}
				else
					common_node_set.insert(pexp);

				if (stsize >= 2 && push_count > 1) {

					r1 = reg->allocate_register(dtsize);
					r2 = reg->allocate_register(dtsize);
					fact2 = pexp_stack.top();
					pexp_stack.pop();
					fact1 = pexp_stack.top();
					pexp_stack.pop();

					//store previous calculated result on stack
					if (result.size() > 0) {
						in = get_insn(PUSH, 1);
						in->operand_1->type = REGISTER;
						in->operand_1->reg = result.top();
						insncls->delete_operand(&in->operand_2);
						instructions.push_back(in);
						in = nullptr;
						reg->free_register(result.top());
						reg->free_register(r2);
						r1 = reg->allocate_register(dtsize);
					}

					in = get_insn(MOV, 2);

					//if literal
					if (!fact1->is_id) {
						in->operand_1->type = REGISTER;
						in->operand_1->reg = r1;
						in->operand_2->type = LITERAL;
						in->operand_2->literal = fact1->tok.string;
						instructions.push_back(in);
						in = nullptr;
						result.push(r1);
					}
					else {
						//if identifier
						in->operand_1->type = REGISTER;
						in->operand_1->reg = r1;
						in->operand_2->type = MEMORY;

						if (get_function_local_member(&fmem, fact1->id_info->tok)) {
							in->operand_2->mem.mem_type = LOCAL;
							in->operand_2->mem.mem_size = dtsize;
							in->operand_2->mem.fp_disp = fmem.fp_disp;
							in->comment = "  ; " + fact1->id_info->symbol;
							instructions.push_back(in);
							in = nullptr;
							result.push(r1);
						}
						else {
							in = get_insn(MOV, 2);
							in->operand_2->mem.mem_type = GLOBAL;
							in->operand_2->mem.mem_size = dtsize;
							in->operand_2->mem.name = fact1->id_info->symbol;
							in->comment = "  ; " + fact1->id_info->symbol;
							instructions.push_back(in);
							in = nullptr;
							result.push(r1);
						}
					}

					//get arithmetic operator instruction
					op = get_arthm_op(pexp->tok.string);

					if (!fact2->is_id) {

						if (op != SHL || op != SHR) {
							in = get_insn(MOV, 2);
							in->operand_1->type = REGISTER;
							in->operand_1->reg = r2;
							in->operand_2->type = LITERAL;

							if (fact1->id_info != nullptr && fact1->id_info->is_ptr)
								in->operand_2->literal = std::to_string(Convert::tok_to_decimal(fact2->tok) * 4);
							else
								in->operand_2->literal = fact2->tok.string;

							instructions.push_back(in);
							in = nullptr;
						}
					}
					else {
						in = get_insn(MOV, 2);
						in->operand_1->type = REGISTER;
						in->operand_1->reg = r2;
						in->operand_2->type = MEMORY;

						if (get_function_local_member(&fmem, fact2->id_info->tok)) {
							in->operand_2->mem.mem_type = LOCAL;
							in->operand_2->mem.mem_size = dtsize;
							in->operand_2->mem.fp_disp = fmem.fp_disp;
							in->comment = "  ; " + fact2->id_info->symbol;
							instructions.push_back(in);
							in = nullptr;
						}
						else {
							in->operand_2->mem.mem_type = GLOBAL;
							in->operand_2->mem.mem_size = dtsize;
							in->operand_2->mem.name = fact2->id_info->symbol;
							in->comment = "  ; " + fact2->id_info->symbol;
							instructions.push_back(in);
							in = nullptr;
						}
					}

					reg->free_register(r2);

					if (op == MUL || op == DIV) {
						in = get_insn(op, 1);
						in->operand_1->type = REGISTER;
						in->operand_1->reg = r2;
						insncls->delete_operand(&in->operand_2);
						instructions.push_back(in);
						in = nullptr;
						//if Token == %
						if (pexp->tok.number == ARTHM_MOD) {
							in = get_insn(MOV, 2);
							in->operand_1->type = REGISTER;
							in->operand_2->type = REGISTER;

							if (dtsize == 1) {
								in->operand_1->reg = AL;
								in->operand_2->reg = DL;
							}
							else if (dtsize == 2) {
								in->operand_1->reg = AX;
								in->operand_2->reg = DX;
							}
							else if (dtsize == 4) {
								in->operand_1->reg = EAX;
								in->operand_2->reg = EDX;
							}
							else if (dtsize == 8) {
								in->operand_1->reg = RAX;
								in->operand_2->reg = RDX;
							}

							in->comment = "  ; copy % result";
							instructions.push_back(in);
							in = nullptr;
						}
					}
					else if (op == SHL || op == SHR) {
						in = get_insn(op, 2);
						in->operand_1->type = REGISTER;
						in->operand_1->reg = r1;
						in->operand_2->type = LITERAL;
						in->operand_2->literal = fact2->tok.string;
						instructions.push_back(in);
						in = nullptr;
					}
					else {
						in = get_insn(op, 2);
						in->operand_1->type = REGISTER;
						in->operand_1->reg = r1;
						in->operand_2->type = REGISTER;
						in->operand_2->reg = r2;
						instructions.push_back(in);
						in = nullptr;
					}
				}
				else if (stsize >= 1) {
					r2 = reg->allocate_register(dtsize);
					fact1 = pexp_stack.top();
					pexp_stack.pop();
					if (!fact1->is_id) {
						in = get_insn(MOV, 2);
						in->operand_1->type = REGISTER;
						in->operand_1->reg = r2;
						in->operand_2->type = LITERAL;
						in->operand_2->literal = fact1->tok.string;
					}
					else {

						in = get_insn(MOV, 2);
						in->operand_1->type = REGISTER;
						in->operand_1->reg = r2;
						in->operand_2->type = MEMORY;

						if (get_function_local_member(&fmem, fact1->id_info->tok)) {
							in->operand_2->mem.mem_type = LOCAL;
							in->operand_2->mem.mem_size = dtsize;
							in->operand_2->mem.fp_disp = fmem.fp_disp;
						}
						else {
							in->operand_2->mem.mem_type = GLOBAL;
							in->operand_2->mem.mem_size = dtsize;
							in->operand_2->mem.name = fact1->id_info->symbol;
						}

						in->comment = "  ; " + fact1->id_info->symbol;
					}

					instructions.push_back(in);
					in = nullptr;
					reg->free_register(r2);

					op = get_arthm_op(pexp->tok.string);
					if (op == MUL || op == DIV) {

						in = get_insn(op, 1);
						in->operand_1->type = REGISTER;
						in->operand_1->reg = r2;
						insncls->delete_operand(&in->operand_2);
						instructions.push_back(in);
						in = nullptr;

						//if Token == %
						if (pexp->tok.number == ARTHM_MOD) {

							in = get_insn(MOV, 2);
							in->operand_1->type = REGISTER;
							in->operand_2->type = REGISTER;

							if (dtsize == 1) {
								in->operand_1->reg = AL;
								in->operand_2->reg = DL;
							}
							else if (dtsize == 2) {
								in->operand_1->reg = AX;
								in->operand_2->reg = DX;
							}
							else if (dtsize == 4) {
								in->operand_1->reg = EAX;
								in->operand_2->reg = EDX;
							}
							else if (dtsize == 8) {
								in->operand_1->reg = RAX;
								in->operand_2->reg = RDX;
							}

							in->comment = "  ; copy % result";
							instructions.push_back(in);
							in = nullptr;
						}
					}
					else {
						in = get_insn(op, 2);
						in->operand_1->type = REGISTER;
						in->operand_1->reg = r1;
						in->operand_2->type = REGISTER;
						in->operand_2->reg = r2;
						instructions.push_back(in);
						in = nullptr;
					}
				}
				else {
					RegisterType _tr1;
					if (!result.empty()) {
						_tr1 = result.top();
						result.pop();
					}

					in = get_insn(MOV, 2);
					in->operand_1->type = REGISTER;
					auto szreg = [=](int sz) {
						if (sz == 1)
							return BL;
						else if (sz == 2)
							return BX;
						else
							return EBX;
					};

					in->operand_1->reg = szreg(dtsize);
					in->operand_2->type = REGISTER;
					in->operand_2->reg = _tr1;
					in->comment = "   ; copy result to register";
					instructions.push_back(in);
					in = nullptr;

					if (push_count > 0) {
						in = get_insn(POP, 1);
						in->operand_1->type = REGISTER;
						in->operand_1->reg = _tr1;
						insncls->delete_operand(&in->operand_2);
						in->comment = "    ; pop previous result to register";
						instructions.push_back(in);
						in = nullptr;
						push_count--;
					}

					op = get_arthm_op(pexp->tok.string);
					if (op == MUL || op == DIV) {
						in = get_insn(op, 1);
						in->operand_1->type = REGISTER;
						in->operand_1->reg = szreg(dtsize);
						insncls->delete_operand(&in->operand_2);
						instructions.push_back(in);
						in = nullptr;
						//if Token == %
						if (pexp->tok.number == ARTHM_MOD) {
							in = get_insn(MOV, 2);
							in->operand_1->type = REGISTER;
							in->operand_2->type = REGISTER;
							if (dtsize == 1) {
								in->operand_1->reg = AL;
								in->operand_2->reg = DL;
							}
							else if (dtsize == 2) {
								in->operand_1->reg = AX;
								in->operand_2->reg = DX;
							}
							else if (dtsize == 4) {
								in->operand_1->reg = EAX;
								in->operand_2->reg = EDX;
							}
							else if (dtsize == 8) {
								in->operand_1->reg = RAX;
								in->operand_2->reg = RDX;
							}

							in->comment = "  ; copy % result";
							instructions.push_back(in);
							in = nullptr;
						}
					}
					else {
						in = get_insn(op, 2);
						in->operand_1->type = REGISTER;
						in->operand_1->reg = _tr1;
						in->operand_2->type = REGISTER;
						in->operand_2->reg = EBX;
						instructions.push_back(in);
						in = nullptr;
					}
				}
			}
			else {
				push_count++;
				pexp_stack.push(pexp);
			}

			op = INSNONE;
			pexp_out_stack.pop();
		}

		common_node_set.clear();
		return r1;
	}

	InstructionType CodeGen::get_farthm_op(const std::string &symbol, bool reverse_ins) {

		//return float arithmetic instruction types

		if (symbol == "+")
			return FADD;
		else if (symbol == "-")
			return (reverse_ins ? FSUBR : FSUB);
		else if (symbol == "*")
			return FMUL;
		else if (symbol == "/")
			return (reverse_ins ? FDIVR : FDIV);
		return INSNONE;
	}

	Member *CodeGen::create_float_data(DeclarationType ds, const std::string &value) {

		//create float data in data section

		Member *dt = search_data(value);
		if (dt != nullptr)
			return dt;

		dt = insncls->get_data_mem();
		dt->symbol = "float_val" + std::to_string(float_data_count);
		dt->type = ds;
		dt->value = value;
		data_section.push_back(dt);
		float_data_count++;
		return dt;
	}

	FloatRegisterType CodeGen::gen_float_primexp_single_assgn(PrimaryExpression *pexpr, DeclarationType decsp) {

		Instruction *in = nullptr;
		FloatRegisterType rs = FRNONE;
		Member *dt = nullptr;
		FunctionMember fmem;
		if (pexpr == nullptr)
			return FRNONE;

		if (pexpr->left == nullptr && pexpr->right == nullptr) {
			if (!pexpr->is_id) {
				dt = create_float_data(decsp, pexpr->tok.string);
				in = get_insn(FLD, 1);
				in->operand_1->type = MEMORY;
				in->operand_1->mem.mem_type = GLOBAL;
				in->operand_1->mem.mem_size = data_decl_size(decsp);
				in->operand_1->mem.name = dt->symbol;
				in->comment = "  ; " + pexpr->tok.string;
			}
			else {

				in = get_insn(FLD, 1);
				in->operand_1->type = MEMORY;

				if (get_function_local_member(&fmem, pexpr->id_info->tok)) {
					in->operand_1->mem.mem_type = LOCAL;
					in->operand_1->mem.mem_size = data_decl_size(decsp);
					in->operand_1->mem.fp_disp = fmem.fp_disp;
				}
				else {
					in->operand_1->mem.mem_type = GLOBAL;
					in->operand_1->mem.mem_size = data_decl_size(decsp);
					in->operand_1->mem.name = pexpr->id_info->symbol;
				}
			}

			insncls->delete_operand(&(in->operand_2));
			instructions.push_back(in);
			return rs;
		}

		return FRNONE;
	}

	void CodeGen::gen_float_primary_expr(PrimaryExpression *pexpr) {

		// generate float type x86 assembly of primary expression
		// generator does not store previous calculated result here

		std::stack<PrimaryExpression *> pexp_stack;
		std::stack<PrimaryExpression *> pexp_out_stack;
		PrimaryExpression *pexp = nullptr, *fact1 = nullptr, *fact2 = nullptr;
		int dtsize = 0;
		size_t stsize = 0;
		FloatRegisterType r1, r2;
		InstructionType op;
		Instruction *in;
		int push_count = 0;
		Member *dt = nullptr;
		DeclarationType decsp = DSPNONE;
		FunctionMember fmem;

		if (pexpr == nullptr)
			return;

		max_datatype_size(pexpr, &dtsize);

		if (dtsize <= 0)
			return;

		if (dtsize == 4)
			decsp = DD;
		else if (dtsize == 8)
			decsp = DQ;

		insert_comment("; line " + std::to_string(pexpr->tok.loc.line));

		r1 = gen_float_primexp_single_assgn(pexpr, decsp);
		if (r1 != FRNONE)
			return;

		pexp_out_stack = get_post_order_prim_expr(pexpr);

		while (!pexp_out_stack.empty()) {
			pexp = pexp_out_stack.top();
			if (pexp->is_oprtr) {
				stsize = pexp_stack.size();
				if (stsize >= 2 && push_count > 1) {
					r1 = reg->allocate_float_register();
					r2 = reg->allocate_float_register();
					fact2 = pexp_stack.top();
					pexp_stack.pop();
					fact1 = pexp_stack.top();
					pexp_stack.pop();

					if (!fact1->is_id) {
						dt = create_float_data(decsp, fact1->tok.string);
						in = get_insn(FLD, 1);
						in->operand_1->type = MEMORY;
						in->operand_1->mem.mem_type = GLOBAL;
						in->operand_1->mem.mem_size = dtsize;
						in->operand_1->mem.name = dt->symbol;
						in->comment = "  ; " + fact1->tok.string;
						insncls->delete_operand(&(in->operand_2));
						instructions.push_back(in);
						in = nullptr;
						dt = nullptr;
					}
					else {
						if (get_function_local_member(&fmem, fact1->id_info->tok)) {
							in = get_insn(FLD, 1);
							in->operand_1->type = MEMORY;
							in->operand_1->mem.mem_type = LOCAL;
							in->operand_1->mem.mem_size = dtsize;
							in->operand_1->mem.fp_disp = fmem.fp_disp;
							in->comment = "  ; " + fact1->id_info->symbol;
							insncls->delete_operand(&(in->operand_2));
							instructions.push_back(in);
							in = nullptr;
						}
						else {
							in = get_insn(FLD, 1);
							in->operand_1->type = MEMORY;
							in->operand_1->mem.mem_type = GLOBAL;
							in->operand_1->mem.mem_size = dtsize;
							in->operand_1->mem.name = fact1->id_info->symbol;
							in->comment = "  ; " + fact1->id_info->symbol;
							insncls->delete_operand(&(in->operand_2));
							instructions.push_back(in);
							in = nullptr;
						}
					}

					if (!fact2->is_id) {
						dt = create_float_data(decsp, fact2->tok.string);
						in = get_insn(FLD, 1);
						in->operand_1->type = MEMORY;
						in->operand_1->mem.mem_type = GLOBAL;
						in->operand_1->mem.mem_size = dtsize;
						in->operand_1->mem.name = dt->symbol;
						in->comment = "  ; " + fact2->tok.string;
						insncls->delete_operand(&(in->operand_2));
						instructions.push_back(in);
						in = nullptr;
						dt = nullptr;
					}
					else {

						in = get_insn(FLD, 1);
						in->operand_1->type = MEMORY;

						if (get_function_local_member(&fmem, fact2->id_info->tok)) {
							in->operand_1->mem.mem_type = LOCAL;
							in->operand_1->mem.mem_size = dtsize;
							in->operand_1->mem.fp_disp = fmem.fp_disp;
						}
						else {
							in->operand_1->mem.mem_type = GLOBAL;
							in->operand_1->mem.mem_size = dtsize;
							in->operand_1->mem.name = fact2->id_info->symbol;
						}

						in->comment = "  ; " + fact2->id_info->symbol;
						insncls->delete_operand(&(in->operand_2));
						instructions.push_back(in);
						in = nullptr;
					}

					reg->free_float_register(r2);

					op = get_farthm_op(pexp->tok.string, false);
					in = get_insn(op, 1);
					in->operand_1->type = FREGISTER;
					in->operand_1->freg = r2;
					insncls->delete_operand(&(in->operand_2));
					instructions.push_back(in);
					in = nullptr;
					push_count = 0;

				}
				else if (stsize >= 1) {
					r2 = reg->allocate_float_register();
					fact1 = pexp_stack.top();
					pexp_stack.pop();

					if (!fact1->is_id) {
						dt = create_float_data(decsp, fact1->tok.string);
						in = get_insn(FLD, 1);
						in->operand_1->type = MEMORY;
						in->operand_1->mem.mem_type = GLOBAL;
						in->operand_1->mem.mem_size = dtsize;
						in->operand_1->mem.name = dt->symbol;
						in->comment = "  ; " + fact1->tok.string;
						insncls->delete_operand(&(in->operand_2));
						instructions.push_back(in);
						in = nullptr;
						dt = nullptr;
					}
					else {
						if (get_function_local_member(&fmem, fact1->id_info->tok)) {
							in = get_insn(FLD, 1);
							in->operand_1->type = MEMORY;
							in->operand_1->mem.mem_type = LOCAL;
							in->operand_1->mem.mem_size = dtsize;
							in->operand_1->mem.fp_disp = fmem.fp_disp;
							in->comment = "  ; " + fact1->id_info->symbol;
							insncls->delete_operand(&(in->operand_2));
							instructions.push_back(in);
							in = nullptr;
						}
						else {
							in = get_insn(FLD, 1);
							in->operand_1->type = MEMORY;
							in->operand_1->mem.mem_type = GLOBAL;
							in->operand_1->mem.mem_size = dtsize;
							in->operand_1->mem.name = fact1->id_info->symbol;
							in->comment = "  ; " + fact1->id_info->symbol;
							insncls->delete_operand(&(in->operand_2));
							instructions.push_back(in);
							in = nullptr;
						}
					}

					op = get_farthm_op(pexp->tok.string, true);
					in = get_insn(op, 1);
					in->operand_1->type = FREGISTER;
					in->operand_1->freg = r2;
					insncls->delete_operand(&(in->operand_2));
					instructions.push_back(in);
					in = nullptr;
					push_count = 0;
					reg->free_float_register(r2);
				}
			}
			else {
				push_count++;
				pexp_stack.push(pexp);
			}
			op = INSNONE;
			pexp_out_stack.pop();
		}

		reg->free_float_register(r1);
	}

	std::pair<int, int> CodeGen::gen_primary_expr(PrimaryExpression **pexpr) {

		// return pair as result of an primary expression
		// pair(type: int,float, register: simple, float)
		// int type result is always in eax register
		// and float type result in st0 stack register

		RegisterType result;
		std::pair<int, int> pr(-1, -1);
		PrimaryExpression *pexpr2 = *pexpr;

		if (pexpr2 == nullptr)
			return pr;

		if (has_float(pexpr2)) {
			gen_float_primary_expr(pexpr2);
			pr.first = 2;
			pr.second = static_cast<int>(ST0);
		}
		else {
			result = gen_int_primary_expr(pexpr2);
			reg->free_register(result);
			pr.first = 1;
			pr.second = static_cast<int>(result);
		}
		return pr;
	}

	void CodeGen::gen_assgn_primary_expr(AssignmentExpression **asexpr) {

		//generate x86 assembly for assignment of primary expression
		//e.g: x = 1 + 2 *3 ;

		AssignmentExpression *assgnexp = *asexpr;
		std::pair<int, int> pexp_result;
		Instruction *in = nullptr;
		int dtsize = 0;
		IdentifierExpression *left = nullptr;
		FunctionMember fmem;
		Token type;

		if (assgnexp == nullptr)
			return;

		if (assgnexp->id_expr == nullptr)
			return;

		left = assgnexp->id_expr;
		if (left->unary != nullptr)
			left = left->unary;

		//generate primary expression & get its result
		pexp_result = gen_primary_expr(&(assgnexp->expression->primary_expr));

		if (pexp_result.first == -1)
			return;

		if (left->id_info == nullptr)
			return;

		if (left->id_info->type_info == nullptr)
			return;

		if (get_function_local_member(&fmem, left->id_info->tok)) {
			type = left->id_info->type_info->type_specifier.simple_type[0];
			dtsize = data_type_size(type);

			in = get_insn(MOV, 2);
			in->operand_1->type = MEMORY;
			in->operand_1->mem.mem_type = LOCAL;
			in->operand_1->mem.fp_disp = fmem.fp_disp;
			int res = pexp_result.second;
			//if simple int type
			if (pexp_result.first == 1) {
				in->operand_2->type = REGISTER;

				if (dtsize == 1)
					res = static_cast<int>(AL);
				else if (dtsize == 2)
					res = static_cast<int>(AX);

				in->operand_2->reg = static_cast<RegisterType>(res);
				in->operand_1->mem.mem_size = reg->regsize(static_cast<RegisterType>(res));
			}
			else if (pexp_result.first == 2) {
				//if floating type, result store in st0
				in->operand_count = 1;
				in->insn_type = FSTP;
				in->operand_1->mem.mem_size = dtsize;
				insncls->delete_operand(&(in->operand_2));
			}

			instructions.push_back(in);
			in = nullptr;
		}
		else {
			type = left->id_info->type_info->type_specifier.simple_type[0];
			dtsize = data_type_size(type);

			in = get_insn(MOV, 2);
			in->operand_1->type = MEMORY;
			in->operand_1->mem.mem_type = GLOBAL;
			in->operand_1->mem.mem_size = dtsize;
			in->operand_1->mem.name = left->id_info->symbol;

			if (left->is_subscript) {
				in->operand_1->is_array = true;
				Token sb = *(left->subscript.begin());
				if (is_literal(sb)) {
					in->operand_1->mem.fp_disp = Convert::tok_to_decimal(sb) * dtsize;
					in->operand_1->reg = RNONE;
				}
				else {
					FunctionMember fmem2;
					Instruction *in2 = nullptr;
					auto indexreg = [=](int sz) {
						if (sz == 1)
							return CL;
						else if (sz == 2)
							return CX;
						else if (sz == 4)
							return ECX;
						else
							return RCX;
					};

					//clearing out index register ecx
					in2 = get_insn(XOR, 2);

					in2->operand_1->type = REGISTER;
					Compiler::global.x64 ? in2->operand_1->reg = RCX : in2->operand_1->reg = ECX;

					in2->operand_2->type = REGISTER;
					Compiler::global.x64 ? in2->operand_2->reg = RCX : in2->operand_2->reg = ECX;

					instructions.push_back(in2);
					in2 = nullptr;

					in2 = get_insn(MOV, 2);
					in2->operand_1->type = REGISTER;
					in2->operand_1->reg = indexreg(dtsize);
					in2->operand_2->type = MEMORY;

					if (get_function_local_member(&fmem2, sb)) {
						in2->operand_2->mem.mem_type = LOCAL;
						in2->operand_2->mem.mem_size = dtsize;
						in2->operand_2->mem.fp_disp = fmem2.fp_disp;
						instructions.push_back(in2);
					}
					else {
						in2->operand_2->mem.mem_type = GLOBAL;
						in2->operand_2->mem.mem_size = dtsize;
						in2->operand_2->mem.name = sb.string;
						instructions.push_back(in2);
					}

					Compiler::global.x64 ? in->operand_1->reg = RCX : in->operand_1->reg = ECX;
					in->operand_1->arr_disp = dtsize;
				}
			}

			if (pexp_result.first == 1) {
				in->operand_2->type = REGISTER;
				int res = pexp_result.second;

				if (dtsize == 1)
					res = static_cast<int>(AL);
				else if (dtsize == 2)
					res = static_cast<int>(AX);

				in->operand_2->reg = static_cast<RegisterType>(res);
				in->operand_1->mem.mem_size = reg->regsize(static_cast<RegisterType>(res));
			}
			else if (pexp_result.first == 2) {
				in->operand_count = 1;
				in->insn_type = FSTP;
				in->operand_1->mem.mem_size = dtsize;
				insncls->delete_operand(&(in->operand_2));
			}

			instructions.push_back(in);
			in = nullptr;
		}
	}

	void CodeGen::gen_sizeof_expr(SizeOfExpression **sofexpr) {

		// generate sizeof expression
		// by calculating size of an type
		// and aasigning it to EAX register

		SizeOfExpression *szofnexp = *sofexpr;
		Instruction *in = nullptr;

		if (szofnexp == nullptr)
			return;

		if (szofnexp->is_simple_type) {
			insert_comment("; line " + std::to_string(szofnexp->simple_type[0].loc.line));
			in = get_insn(MOV, 2);

			in->operand_1->type = REGISTER;
			Compiler::global.x64 ? in->operand_1->reg = RAX : in->operand_1->reg = EAX;

			in->operand_2->type = LITERAL;
			in->comment = "    ;  sizeof " + szofnexp->simple_type[0].string;

			if (szofnexp->is_ptr) {
				Compiler::global.x64 ? in->operand_2->literal = "8" : in->operand_2->literal = "4";
				in->comment += " pointer";
			}
			else
				in->operand_2->literal = std::to_string(data_type_size(szofnexp->simple_type[0]));

			instructions.push_back(in);
		}
		else {
			insert_comment("; line " + std::to_string(szofnexp->identifier.loc.line));
			in = get_insn(MOV, 2);
			in->operand_1->type = REGISTER;

			Compiler::global.x64 ? in->operand_1->reg = RAX : in->operand_1->reg = EAX;
			in->operand_2->type = LITERAL;
			in->comment = "    ;  sizeof " + szofnexp->identifier.string;

			if (szofnexp->is_ptr) {
				Compiler::global.x64 ? in->operand_2->literal = "8" : in->operand_2->literal = "4";
				in->comment += " pointer";
			}
			else {
				std::unordered_map<std::string, int>::iterator it;
				it = record_sizes.find(szofnexp->identifier.string);
				if (it != record_sizes.end())
					in->operand_2->literal = std::to_string(it->second);
			}
			instructions.push_back(in);
		}
	}

	void CodeGen::gen_assgn_sizeof_expr(AssignmentExpression **asexpr) {

		AssignmentExpression *assgnexp = *asexpr;
		Instruction *in = nullptr;
		int dtsize = 0;
		IdentifierExpression *left = nullptr;
		Token type;
		FunctionMember fmem;

		if (assgnexp == nullptr)
			return;
		if (assgnexp->id_expr == nullptr)
			return;

		left = assgnexp->id_expr;
		if (left->unary != nullptr)
			left = left->unary;

		gen_sizeof_expr(&assgnexp->expression->sizeof_expr);

		if (left->id_info == nullptr)
			return;

		type = left->id_info->type_info->type_specifier.simple_type[0];
		dtsize = data_type_size(type);
		in = get_insn(MOV, 2);
		in->operand_1->type = MEMORY;

		if (get_function_local_member(&fmem, left->id_info->tok)) {
			in->operand_1->mem.mem_type = LOCAL;
			in->operand_1->mem.fp_disp = fmem.fp_disp;
			Compiler::global.x64 ? in->operand_1->mem.mem_size = 8 : in->operand_1->mem.mem_size = 4;
			in->operand_2->type = REGISTER;
			Compiler::global.x64 ? in->operand_2->reg = RAX : in->operand_2->reg = EAX;
			in->comment = "    ; line: " + std::to_string(assgnexp->tok.loc.line);
			instructions.push_back(in);
		}
		else {
			in->operand_1->mem.mem_type = GLOBAL;

			Compiler::global.x64 ? in->operand_1->mem.mem_size = 8 : in->operand_1->mem.mem_size = 4;
			in->operand_1->mem.name = left->id_info->symbol;
			in->operand_2->type = REGISTER;

			Compiler::global.x64 ? in->operand_2->reg = RAX : in->operand_2->reg = EAX;

			if (left->is_subscript) {
				Token sb = *(left->subscript.begin());
				in->operand_1->mem.fp_disp = std::stoi(sb.string) * dtsize;
			}

			in->comment = "    ; line: " + std::to_string(assgnexp->tok.loc.line);
			instructions.push_back(in);
		}
	}

	void CodeGen::gen_assgn_cast_expr(AssignmentExpression **asexpr) {

		AssignmentExpression *assgnexp = *asexpr;
		Instruction *in = nullptr;
		int dtsize = 0;
		IdentifierExpression *left = nullptr;
		Token type;
		FunctionMember fmem;

		if (assgnexp == nullptr)
			return;
		if (assgnexp->id_expr == nullptr)
			return;

		auto resreg = [=](int sz) {
			if (sz == 1)
				return AL;
			else if (sz == 2)
				return AX;
			else if (sz == 4)
				return EAX;
			else
				return RAX;
		};

		left = assgnexp->id_expr;
		if (left->unary != nullptr)
			left = left->unary;

		gen_cast_expr(&assgnexp->expression->cast_expr);

		if (left->id_info == nullptr)
			return;

		if (get_function_local_member(&fmem, left->id_info->tok)) {
			type = left->id_info->type_info->type_specifier.simple_type[0];
			dtsize = data_type_size(type);
			in = get_insn(MOV, 2);
			in->operand_1->type = MEMORY;
			in->operand_1->mem.mem_type = LOCAL;
			in->operand_1->mem.fp_disp = fmem.fp_disp;
			in->operand_1->mem.mem_size = dtsize;
			in->operand_2->type = REGISTER;
			in->operand_2->reg = resreg(dtsize);

		}
		else {
			type = left->id_info->type_info->type_specifier.simple_type[0];
			dtsize = data_type_size(type);
			in = get_insn(MOV, 2);
			in->operand_1->type = MEMORY;
			in->operand_1->mem.mem_type = GLOBAL;
			in->operand_1->mem.mem_size = dtsize;
			in->operand_1->mem.name = left->id_info->symbol;
			in->operand_2->type = REGISTER;
			in->operand_2->reg = resreg(dtsize);

			if (left->is_subscript) {
				Token sb = *(left->subscript.begin());
				in->operand_1->mem.fp_disp = std::stoi(sb.string) * dtsize;
			}
		}

		in->comment = "    ; line: " + std::to_string(assgnexp->tok.loc.line);
		instructions.push_back(in);
	}

	void CodeGen::gen_id_expr(IdentifierExpression **idexpr) {

		// generate id expresion
		// RECORD tyeps are not considered while code generation
		// only simple types are used
		// id expression, checking for addressof, ++, --

		IdentifierExpression *idexp = *idexpr;
		Instruction *in = nullptr;
		int dtsize = 0;
		Token type;
		TokenId op;
		FunctionMember fmem;

		if (idexp == nullptr)
			return;

		insert_comment("; line " + std::to_string(idexp->tok.loc.line));

		if (idexp->unary != nullptr) {
			op = idexp->tok.number;
			if (idexp->is_oprtr) {
				in = get_insn(INSNONE, 2);
				in->operand_1->type = REGISTER;

				if (Compiler::global.x64)
					in->operand_1->reg = RAX;
				else
					in->operand_1->reg = EAX;

				idexp = idexp->unary;
				if (idexp->id_info == nullptr)
					return;

				if (idexp->id_info->type_info == nullptr)
					return;

				type = idexp->id_info->type_info->type_specifier.simple_type[0];
				dtsize = data_type_size(type);
				in->operand_2->type = MEMORY;

				if (get_function_local_member(&fmem, idexp->id_info->tok)) {
					in->operand_2->mem.mem_type = LOCAL;
					in->operand_2->mem.fp_disp = fmem.fp_disp;
				}
				else {
					in->operand_2->mem.mem_type = GLOBAL;
					in->operand_2->mem.name = idexp->id_info->symbol;
				}

				in->operand_2->mem.mem_size = dtsize;
			}
			if (op == ADDROF_OP) {
				in->insn_type = LEA;
				in->operand_count = 2;
				in->operand_2->mem.mem_size = 0;
				in->comment = "    ; address of";
			}
			else if (op == INCR_OP) {
				in->insn_type = INC;
				in->operand_count = 1;
				insncls->delete_operand(&in->operand_1);
				in->operand_1 = in->operand_2;
				in->comment = "    ; ++";
				if (in->operand_1->mem.mem_size > 4)
					in->operand_1->mem.mem_size = 4;
				in->operand_2 = nullptr;
			}
			else if (op == DECR_OP) {
				in->insn_type = DEC;
				in->operand_count = 1;
				insncls->delete_operand(&in->operand_1);
				in->operand_1 = in->operand_2;
				in->comment = "    ; --";
				if (in->operand_1->mem.mem_size > 4)
					in->operand_1->mem.mem_size = 4;
				in->operand_2 = nullptr;
			}
			instructions.push_back(in);
		}
		else {

			if (idexp->id_info == nullptr)
				return;
			type = idexp->id_info->type_info->type_specifier.simple_type[0];
			dtsize = data_type_size(type);
			auto resreg = [=](int sz) {
				if (sz == 1)
					return AL;
				else if (sz == 2)
					return AX;
				else if (sz == 4)
					return EAX;
				else
					return RAX;
			};

			in = get_insn(MOV, 2);
			in->operand_1->type = REGISTER;
			in->operand_1->reg = resreg(dtsize);

			if (get_function_local_member(&fmem, idexp->id_info->tok)) {
				in->operand_2->type = MEMORY;
				in->operand_2->mem.mem_type = LOCAL;
				in->operand_2->mem.mem_size = dtsize;
				in->operand_2->mem.fp_disp = fmem.fp_disp;
			}
			else {
				in->operand_2->type = MEMORY;
				in->operand_2->mem.mem_type = GLOBAL;
				in->operand_2->mem.mem_size = dtsize;
				in->operand_2->mem.name = idexp->id_info->symbol;
				//if has array subscript
				if (idexp->is_subscript) {
					in->operand_2->is_array = true;
					Token sb = *(idexp->subscript.begin());
					if (is_literal(sb)) {
						in->operand_2->mem.fp_disp = Convert::tok_to_decimal(sb) * dtsize;
						in->operand_2->reg = RNONE;
					}
					else {
						FunctionMember fmem2;
						Instruction *in2 = nullptr;
						auto indexreg = [=](int sz) {
							if (sz == 1)
								return CL;
							else if (sz == 2)
								return CX;
							else if (sz == 4)
								return ECX;
							else
								return RAX;
						};

						in2 = get_insn(XOR, 2);
						in2->operand_1->type = REGISTER;
						if (Compiler::global.x64)
							in2->operand_1->reg = RCX;
						else
							in2->operand_1->reg = ECX;

						in2->operand_2->type = REGISTER;
						if (Compiler::global.x64)
							in2->operand_2->reg = RCX;
						else
							in2->operand_2->reg = ECX;

						instructions.push_back(in2);
						in2 = nullptr;
						in2 = get_insn(MOV, 2);
						in2->operand_1->type = REGISTER;
						in2->operand_1->reg = indexreg(dtsize);
						in2->operand_2->type = MEMORY;

						if (get_function_local_member(&fmem2, sb)) {
							in2->operand_2->mem.mem_type = LOCAL;
							in2->operand_2->mem.fp_disp = fmem2.fp_disp;
						}
						else {
							in2->operand_2->mem.mem_type = GLOBAL;
							in2->operand_2->mem.name = sb.string;
						}

						in2->operand_2->mem.mem_size = dtsize;
						instructions.push_back(in2);
						Compiler::global.x64 ? in->operand_2->reg = RCX : in->operand_2->reg = ECX;
						in->operand_2->arr_disp = dtsize;
					}
				}
			}

			instructions.push_back(in);

			//check for pointer operator count
			if (idexp->ptr_oprtr_count > 1) {
				for (int i = 1; i < idexp->ptr_oprtr_count; i++) {

					//insert instruction by dereferencing pointer
					//for dereferencing, we need size, so storing it as memory type
					//with register name eax as global variable name

					in = get_insn(MOV, 2);
					in->operand_1->type = REGISTER;

					if (Compiler::global.x64)
						in->operand_1->reg = RAX;
					else
						in->operand_1->reg = EAX;

					in->operand_2->type = MEMORY;
					in->operand_2->mem.mem_type = GLOBAL;

					if (Compiler::global.x64) {
						in->operand_2->mem.mem_size = 8;
						in->operand_2->mem.name = "rax";
					}
					else {
						in->operand_2->mem.mem_size = 4;
						in->operand_2->mem.name = "eax";
					}

					instructions.push_back(in);
				}
			}
		}
	}

	void CodeGen::gen_assgn_id_expr(AssignmentExpression **asexpr) {

		AssignmentExpression *assgnexp = *asexpr;
		Instruction *in = nullptr;
		int dtsize = 0;
		IdentifierExpression *left = nullptr;
		Token type;
		FunctionMember fmem;

		if (assgnexp == nullptr)
			return;

		if (assgnexp->id_expr == nullptr)
			return;

		left = assgnexp->id_expr;
		if (left->unary != nullptr)
			left = left->unary;

		gen_id_expr(&assgnexp->expression->id_expr);

		auto resultreg = [=](int sz) {
			if (sz == 1)
				return AL;
			else if (sz == 2)
				return AX;
			else if (sz == 4)
				return EAX;
			else
				return RAX;
		};

		if (left->id_info == nullptr)
			return;

		type = left->id_info->type_info->type_specifier.simple_type[0];
		dtsize = data_type_size(type);

		in = get_insn(MOV, 2);
		in->operand_1->type = MEMORY;

		if (get_function_local_member(&fmem, left->id_info->tok)) {
			in->operand_1->mem.mem_type = LOCAL;
			in->operand_1->mem.fp_disp = fmem.fp_disp;
		}
		else {
			in->operand_1->mem.mem_type = GLOBAL;
			in->operand_1->mem.name = left->id_info->symbol;

			if (left->is_subscript) {
				Token sb = *(left->subscript.begin());
				in->operand_1->mem.fp_disp = std::stoi(sb.string) * dtsize;
			}
		}

		in->operand_2->type = REGISTER;
		in->operand_2->reg = resultreg(dtsize);
		in->operand_1->mem.mem_size = dtsize;
		in->comment = "    ; line: " + std::to_string(assgnexp->tok.loc.line);
		instructions.push_back(in);
	}

	void CodeGen::gen_assgn_funccall_expr(AssignmentExpression **asexpr) {

		AssignmentExpression *assgnexp = *asexpr;
		Instruction *in = nullptr;
		int dtsize = 0;
		IdentifierExpression *left = nullptr;
		Token type;
		FunctionMember fmem;

		if (assgnexp == nullptr)
			return;
		if (assgnexp->id_expr == nullptr)
			return;

		left = assgnexp->id_expr;
		if (left->unary != nullptr)
			left = left->unary;

		gen_funccall_expr(&assgnexp->expression->call_expr);

		if (left->id_info == nullptr)
			return;

		if (get_function_local_member(&fmem, left->id_info->tok)) {
			type = left->id_info->type_info->type_specifier.simple_type[0];
			dtsize = data_type_size(type);
			in = get_insn(MOV, 2);
			in->operand_1->type = MEMORY;
			in->operand_1->mem.mem_type = LOCAL;
			in->operand_1->mem.fp_disp = fmem.fp_disp;
			in->operand_1->mem.mem_size = 4;
			in->operand_2->type = REGISTER;

			if (Compiler::global.x64)
				in->operand_2->reg = RAX;
			else
				in->operand_2->reg = EAX;

			in->comment = "    ; line: " + std::to_string(assgnexp->tok.loc.line) + ", assign";
			instructions.push_back(in);
		}
		else {
			type = left->id_info->type_info->type_specifier.simple_type[0];
			dtsize = data_type_size(type);
			in = get_insn(MOV, 2);
			in->operand_1->type = MEMORY;
			in->operand_1->mem.mem_type = GLOBAL;

			Compiler::global.x64 ? in->operand_1->mem.mem_size = 8 : in->operand_1->mem.mem_size = 4;
			in->operand_1->mem.name = left->id_info->symbol;
			in->operand_2->type = REGISTER;

			Compiler::global.x64 ? in->operand_2->reg = RAX : in->operand_2->reg = EAX;
			if (left->is_subscript) {
				Token sb = *(left->subscript.begin());
				in->operand_1->mem.fp_disp = std::stoi(sb.string) * dtsize;
			}

			in->comment = "    ; line: " + std::to_string(assgnexp->tok.loc.line) + " assign to " + left->id_info->symbol;
			instructions.push_back(in);
		}
	}

	void CodeGen::gen_assignment_expr(AssignmentExpression **asexpr) {
		AssignmentExpression *assgnexp = *asexpr;

		if (assgnexp == nullptr)
			return;
		if (assgnexp->id_expr == nullptr)
			return;

		switch (assgnexp->expression->expr_kind) {
			case ExpressionType::PRIMARY_EXPR :
				gen_assgn_primary_expr(&assgnexp);
				break;
			case ExpressionType::ASSGN_EXPR :
				gen_assignment_expr(&(assgnexp->expression->assgn_expr));
				break;
			case ExpressionType::SIZEOF_EXPR :
				gen_assgn_sizeof_expr(&assgnexp);
				break;
			case ExpressionType::CAST_EXPR :
				gen_assgn_cast_expr(&assgnexp);
				break;
			case ExpressionType::ID_EXPR :
				gen_assgn_id_expr(&assgnexp);
				break;
			case ExpressionType::FUNC_CALL_EXPR :
				gen_assgn_funccall_expr(&assgnexp);
				break;
		}
	}

	void CodeGen::gen_funccall_expr(CallExpression **fccallex) {

		// generate x86 function call
		// each passed parameter is 4 byte
		// even float, double is not yet considered yet to pass
		// globals can be used anywhere
		// function call parameters are pushed on stack in reverse order

		Instruction *in = nullptr;
		int pushed_count = 0;
		int param_count = 0;
		CallExpression *fcexpr = *fccallex;
		std::list<Expression *>::reverse_iterator it;
		std::pair<int, int> pr;

		if (fcexpr == nullptr)
			return;
		if (fcexpr->function == nullptr)
			return;

		insert_comment("; line: " + std::to_string(fcexpr->function->tok.loc.line) + ", func_call: " + fcexpr->function->tok.string);

		it = fcexpr->expression_list.rbegin();
		param_count = fcexpr->expression_list.size();
		while (it != fcexpr->expression_list.rend()) {
			if (*it == nullptr)
				break;
			switch ((*it)->expr_kind) {
				case ExpressionType::PRIMARY_EXPR :
					pr = gen_primary_expr(&((*it)->primary_expr));
					if (pr.first == 2) {
						in = get_insn(FSTP, 1);
						in->operand_1->type = MEMORY;

						if (Compiler::global.x64) {
							in->operand_1->reg = RAX;
							in->operand_1->mem.mem_size = 8;
						}
						else {
							in->operand_1->reg = EAX;
							in->operand_1->mem.mem_size = 4;
						}

						in->operand_1->mem.mem_type = GLOBAL;
						insncls->delete_operand(&(in->operand_2));
						in->comment = "    ; retrieve value from float stack(st0) ";
						instructions.push_back(in);

						in = nullptr;
						in = get_insn(PUSH, 1);
						in->operand_1->type = REGISTER;

						if (Compiler::global.x64)
							in->operand_1->reg = RAX;
						else
							in->operand_1->reg = EAX;

						insncls->delete_operand(&(in->operand_2));
						in->comment = "    ; param " + std::to_string(param_count);
						instructions.push_back(in);
					}
					else {
						in = get_insn(PUSH, 1);
						in->operand_1->type = REGISTER;

						if (Compiler::global.x64)
							in->operand_1->reg = RAX;
						else
							in->operand_1->reg = EAX;

						insncls->delete_operand(&(in->operand_2));
						in->comment = "    ; param " + std::to_string(param_count);
						instructions.push_back(in);
						in = nullptr;
					}
					break;

				case ExpressionType::SIZEOF_EXPR :
					gen_sizeof_expr(&((*it)->sizeof_expr));
					in = get_insn(PUSH, 1);
					in->operand_1->type = REGISTER;
					Compiler::global.x64 ? in->operand_1->reg = RAX : in->operand_1->reg = EAX;
					in->comment = "    ; param " + std::to_string(param_count);
					insncls->delete_operand(&(in->operand_2));
					instructions.push_back(in);
					in = nullptr;
					break;

				case ExpressionType::ID_EXPR:
					gen_id_expr(&((*it)->id_expr));
					in = get_insn(PUSH, 1);
					in->operand_1->type = REGISTER;
					Compiler::global.x64 ? in->operand_1->reg = RAX : in->operand_1->reg = EAX;
					in->comment = "    ; param " + std::to_string(param_count);
					insncls->delete_operand(&(in->operand_2));
					instructions.push_back(in);
					in = nullptr;
					break;

				default :
					break;
			}

			pushed_count += 4;
			param_count--;
			it++;
		}

		in = get_insn(CALL, 1);
		in->operand_1->type = LITERAL;

		if (fcexpr->function->left == nullptr && fcexpr->function->right == nullptr)
			in->operand_1->literal = fcexpr->function->tok.string;

		insncls->delete_operand(&(in->operand_2));
		instructions.push_back(in);

		if (fcexpr->expression_list.size() > 0) {
			in = get_insn(ADD, 2);
			in->operand_1->type = REGISTER;
			Compiler::global.x64 ? in->operand_1->reg = RAX : in->operand_1->reg = EAX;
			in->operand_2->type = LITERAL;
			in->operand_2->literal = std::to_string(pushed_count);
			in->comment = "    ; restore func-call params stack frame";
			instructions.push_back(in);
		}
	}

	void CodeGen::gen_cast_expr(CastExpression **cexpr) {
		CastExpression *cstexpr = *cexpr;
		Instruction *in = nullptr;
		int dtsize = -1;
		FunctionMember fmem;

		if (cstexpr == nullptr)
			return;

		auto resreg = [=](int sz) {
			if (sz == 1)
				return AL;
			else if (sz == 2)
				return AX;
			else if (sz == 4)
				return EAX;
			else if (sz == 8)
				return RAX;
			else
				return RNONE;
		};

		if (!cstexpr->is_simple_type) {
			return;
		}

		if (cstexpr->target == nullptr)
			return;

		if (cstexpr->target->tok.number != IDENTIFIER)
			return;

		if (cstexpr->target->id_info == nullptr)
			return;

		insert_comment("; cast expression, line " + std::to_string(cstexpr->simple_type[0].loc.line));
		dtsize = data_type_size(cstexpr->simple_type[0]);
		get_function_local_member(&fmem, cstexpr->target->id_info->tok);

		in = get_insn(MOV, 2);
		in->operand_1->type = REGISTER;
		in->operand_1->reg = resreg(dtsize);

		if (fmem.insize != -1) {
			in->operand_2->type = MEMORY;
			in->operand_2->mem.mem_type = LOCAL;
			in->operand_2->mem.mem_size = dtsize;
			in->operand_2->mem.fp_disp = fmem.fp_disp;
		}
		else {
			in->operand_2->type = MEMORY;
			in->operand_2->mem.name = cstexpr->target->id_info->symbol;
			in->operand_2->mem.mem_type = GLOBAL;
			in->operand_2->mem.mem_size = dtsize;
		}

		instructions.push_back(in);
	}

	void CodeGen::gen_expr(Expression **__expr) {
		Expression *_expr = *__expr;
		if (_expr == nullptr)
			return;

		reg->free_all_registers();
		reg->free_all_float_registers();

		switch (_expr->expr_kind) {
			case ExpressionType::PRIMARY_EXPR :
				gen_primary_expr(&(_expr->primary_expr));
				break;
			case ExpressionType::ASSGN_EXPR :
				gen_assignment_expr(&(_expr->assgn_expr));
				break;
			case ExpressionType::SIZEOF_EXPR :
				gen_sizeof_expr(&(_expr->sizeof_expr));
				break;
			case ExpressionType::CAST_EXPR :
				gen_cast_expr(&(_expr->cast_expr));
				break;
			case ExpressionType::ID_EXPR :
				gen_id_expr(&(_expr->id_expr));
				break;
			case ExpressionType::FUNC_CALL_EXPR :
				gen_funccall_expr(&(_expr->call_expr));
				break;
		}
	}

	void CodeGen::gen_label_statement(LabelStatement **labstmt) {
		if (*labstmt == nullptr)
			return;

		insert_comment("; line " + std::to_string((*labstmt)->label.loc.line));
		Instruction *in = get_insn(INSLABEL, 0);
		in->label = "." + (*labstmt)->label.string;
		insncls->delete_operand(&(in->operand_1));
		insncls->delete_operand(&(in->operand_2));
		instructions.push_back(in);
	}

	void CodeGen::gen_jump_statement(JumpStatement **jstmt) {
		JumpStatement *jmpstmt = *jstmt;
		Instruction *in = nullptr;
		if (jmpstmt == nullptr)
			return;

		switch (jmpstmt->type) {
			case JumpType::BREAK:
				in = get_insn(JMP, 1);
				in->operand_1->type = LITERAL;
				switch (current_loop) {
					case IterationType::WHILE:
						if (!while_loop_stack.empty())
							in->operand_1->literal = ".exit_while_loop" + std::to_string(while_loop_stack.top());
						else
							in->operand_1->literal = ".exit_while_loop" + std::to_string(while_loop_count);
						break;
					case IterationType::DOWHILE:
						if (!dowhile_loop_stack.empty())
							in->operand_1->literal = ".exit_dowhile_loop" + std::to_string(dowhile_loop_stack.top());
						else
							in->operand_1->literal = ".exit_dowhile_loop" + std::to_string(dowhile_loop_count);
						break;
					case IterationType::FOR:
						if (!for_loop_stack.empty())
							in->operand_1->literal = ".exit_for_loop" + std::to_string(for_loop_stack.top());
						else
							in->operand_1->literal = ".exit_for_loop" + std::to_string(for_loop_count);
						break;
					default:
						break;
				}

				in->comment = "    ; break loop, line " + std::to_string(jmpstmt->tok.loc.line);
				insncls->delete_operand(&(in->operand_2));
				instructions.push_back(in);
				break;

			case JumpType::CONTINUE:
				in = get_insn(JMP, 1);
				in->operand_1->type = LITERAL;
				in->operand_1->literal = ".exit_loop" + std::to_string(exit_loop_label_count);
				in->comment = "    ; continue loop, line " + std::to_string(jmpstmt->tok.loc.line);
				insncls->delete_operand(&(in->operand_2));
				switch (current_loop) {
					case IterationType::WHILE:
						in->operand_1->literal = ".while_loop" + std::to_string(while_loop_count);
						break;
					case IterationType::DOWHILE:
						in->operand_1->literal = ".for_loop" + std::to_string(dowhile_loop_count);
						break;
					case IterationType::FOR:
						in->operand_1->literal = ".for_loop" + std::to_string(for_loop_count);
						break;
					default:
						break;
				}
				instructions.push_back(in);
				break;

			case JumpType::RETURN:
				if (jmpstmt->expression != nullptr) {
					gen_expr(&(jmpstmt->expression));
				}
				in = get_insn(JMP, 1);
				in->operand_1->type = LITERAL;
				in->operand_1->literal = "._exit_" + func_symtab->func_info->func_name;
				in->comment = "    ; return, line " + std::to_string(jmpstmt->tok.loc.line);
				insncls->delete_operand(&(in->operand_2));
				instructions.push_back(in);
				break;

			case JumpType::GOTO:
				in = get_insn(JMP, 1);
				in->operand_1->type = LITERAL;
				in->operand_1->literal = "." + jmpstmt->goto_id.string;
				in->comment = "    ; goto, line " + std::to_string(jmpstmt->tok.loc.line);
				insncls->delete_operand(&(in->operand_2));
				instructions.push_back(in);
				break;
		}
	}

	RegisterType CodeGen::get_reg_type_by_char(char ch) {

		if (Compiler::global.x64) {
			switch (ch) {
				case 'a':
					return RAX;
				case 'b':
					return RBX;
				case 'c':
					return RCX;
				case 'd':
					return RDX;
				case 'S':
					return RSI;
				case 'D':
					return RDI;
				default:
					return RNONE;
			}
		}
		else {

			switch (ch) {
				case 'a':
					return EAX;
				case 'b':
					return EBX;
				case 'c':
					return ECX;
				case 'd':
					return EDX;
				case 'S':
					return ESI;
				case 'D':
					return EDI;
				default:
					return RNONE;
			}
		}
	}

	std::string CodeGen::get_asm_output_operand(AsmOperand **asmoprnd) {
		AsmOperand *asmoperand = *asmoprnd;
		std::string constraint = "";
		FunctionMember fmem;
		PrimaryExpression *pexp;

		if (asmoperand == nullptr)
			return constraint;

		constraint = asmoperand->constraint.string;

		if (Compiler::global.x64) {
			if (constraint == "=a")
				return "rax";
			else if (constraint == "=b")
				return "rbx";
			else if (constraint == "=c")
				return "rcx";
			else if (constraint == "=d")
				return "rdx";
			else if (constraint == "=S")
				return "rsi";
			else if (constraint == "=D")
				return "rdi";
		}
		else {

			if (constraint == "=a") {
				return "eax";
			}
			else if (constraint == "=b") {
				return "ebx";
			}
			else if (constraint == "=c") {
				return "ecx";
			}
			else if (constraint == "=d") {
				return "edx";
			}
			else if (constraint == "=S") {
				return "esi";
			}
			else if (constraint == "=D") {
				return "edi";
			}
		}

		if (constraint == "=m") {

			pexp = asmoperand->expression->primary_expr;
			get_function_local_member(&fmem, pexp->tok);

			if (fmem.insize != -1) {
				std::string cast = insncls->insnsize_name(get_insn_size_type(fmem.insize));
				if (fmem.fp_disp < 0) {
					if (Compiler::global.x64) {
						return cast + "[rbp - " + std::to_string(fmem.fp_disp * (-1)) + "]";
					}
					else {
						return cast + "[ebp - " + std::to_string(fmem.fp_disp * (-1)) + "]";
					}
				}
				else {
					if (Compiler::global.x64) {
						return cast + "[rbp + " + std::to_string(fmem.fp_disp) + "]";
					}
					else {
						return cast + "[ebp + " + std::to_string(fmem.fp_disp) + "]";
					}
				}
			}
			else {
				if (pexp->id_info == nullptr) {
					pexp->id_info = search_id(pexp->tok.string);
				}
				if (pexp->id_info != nullptr) {
					Token type = pexp->id_info->type_info->type_specifier.simple_type[0];
					std::string cast = insncls->insnsize_name(get_insn_size_type(data_type_size(type)));
					return cast + "[" + pexp->tok.string + "]";
				}
			}
		}
		return "";
	}

	std::string CodeGen::get_asm_input_operand(AsmOperand **asmoprnd) {
		AsmOperand *asmoperand = *asmoprnd;
		std::string constraint;
		std::string mem = "";
		FunctionMember fmem;
		TokenId t;
		Token tok;
		PrimaryExpression *pexp;
		std::string literal;
		int decm;

		if (asmoperand == nullptr)
			return constraint;

		constraint = asmoperand->constraint.string;

		if (asmoperand->expression != nullptr) {
			pexp = asmoperand->expression->primary_expr;
			tok = pexp->tok;
			t = tok.number;
			switch (t) {
				case LIT_BIN:
				case LIT_CHAR:
				case LIT_DECIMAL:
				case LIT_HEX:
				case LIT_OCTAL:
					constraint = 'i';
					decm = Convert::tok_to_decimal(tok);
					if (decm < 0) {
						literal = "0x" + Convert::dec_to_hex(decm);
					}
					else {
						literal = std::to_string(decm);
					}
					break;
				case IDENTIFIER:
					constraint = 'm';
					if (pexp->id_info == nullptr)
						pexp->id_info = search_id(tok.string);
					break;
				default:
					break;
			}
		}

		switch (constraint[0]) {
			case 'a':
				return Compiler::global.x64 ? "rax" : "eax";
			case 'b':
				return Compiler::global.x64 ? "rbx" : "ebx";
			case 'c':
				return Compiler::global.x64 ? "rcx" : "ecx";
			case 'd':
				return Compiler::global.x64 ? "rdx" : "edx";
			case 'S':
				return Compiler::global.x64 ? "rsi" : "esi";
			case 'D':
				return Compiler::global.x64 ? "rdi" : "edi";
			case 'm':
				get_function_local_member(&fmem, pexp->tok);
				if (fmem.insize != -1) {
					std::string cast = insncls->insnsize_name(get_insn_size_type(fmem.insize));
					if (fmem.fp_disp < 0) {
						if (Compiler::global.x64) {
							return cast + "[rbp - " + std::to_string(fmem.fp_disp * (-1)) + "]";
						}
						else {
							return cast + "[ebp - " + std::to_string(fmem.fp_disp * (-1)) + "]";
						}
					}
					else {
						if (Compiler::global.x64) {
							return cast + "[rbp + " + std::to_string(fmem.fp_disp) + "]";
						}
						else {
							return cast + "[ebp + " + std::to_string(fmem.fp_disp) + "]";
						}
					}
				}
				else {
					if (pexp->id_info == nullptr) {
						pexp->id_info = search_id(pexp->tok.string);
					}
					if (pexp->id_info != nullptr) {
						Token type = pexp->id_info->type_info->type_specifier.simple_type[0];
						std::string cast = insncls->insnsize_name(get_insn_size_type(data_type_size(type)));
						return cast + "[" + pexp->tok.string + "]";
					}
				}
			case 'i':
				return literal;
		}

		return "";
	}

	void CodeGen::get_nonescaped_string(std::string &str) {
		size_t fnd;
		fnd = str.find("\\t");
		while (fnd != std::string::npos) {
			str.replace(fnd, 2, "    ");
			fnd = str.find("\\t", fnd + 2);
		}
	}

	void CodeGen::gen_asm_statement(AsmStatement **_asmstm) {
		AsmStatement *asmstmt = *_asmstm;
		Instruction *in = nullptr;
		size_t fnd;
		std::string asmtemplate, asmoperand;

		if (asmstmt == nullptr)
			return;

		if (asmstmt != nullptr) {
			if (!asmstmt->asm_template.string.empty()) {
				insert_comment("; inline assembly, line " + std::to_string(asmstmt->asm_template.loc.line));
			}
		}

		while (asmstmt != nullptr) {
			asmtemplate = asmstmt->asm_template.string;
			get_nonescaped_string(asmtemplate);
			if (!asmstmt->output_operand.empty()) {
				asmoperand = get_asm_output_operand(&asmstmt->output_operand[0]);
				if (!asmoperand.empty()) {
					fnd = asmtemplate.find_first_of("%");
					if (fnd != std::string::npos) {
						if (fnd + 1 < asmtemplate.length()) {
							if (asmtemplate.at(fnd + 1) == ',') {
								asmtemplate.replace(fnd, 1, asmoperand);
							}
							else {
								asmtemplate.replace(fnd, 2, asmoperand);
							}
						}
						else {
							asmtemplate.replace(fnd, 2, asmoperand);
						}
					}
				}
			}

			if (!asmstmt->input_operand.empty()) {
				asmoperand = get_asm_input_operand(&asmstmt->input_operand[0]);
				if (!asmoperand.empty()) {
					fnd = asmtemplate.find_first_of("%");
					if (fnd != std::string::npos) {
						asmtemplate.replace(fnd, 2, asmoperand);
					}
				}
			}

			in = get_insn(INSASM, 0);
			insncls->delete_operand(&in->operand_1);
			insncls->delete_operand(&in->operand_2);
			in->inline_asm = asmtemplate;
			instructions.push_back(in);
			in = nullptr;
			asmstmt = asmstmt->p_next;
		}
	}

	bool CodeGen::is_literal(Token tok) {
		TokenId t = tok.number;
		if (t == LIT_BIN || t == LIT_CHAR || t == LIT_DECIMAL || t == LIT_HEX || t == LIT_OCTAL) {
			return true;
		}
		return false;
	}

	bool CodeGen::gen_float_type_condition(PrimaryExpression **f1, PrimaryExpression **f2, PrimaryExpression **opr) {
		PrimaryExpression *fexp1 = *f1;
		PrimaryExpression *fexp2 = *f2;
		PrimaryExpression *fexpopr = *opr;
		Token type;
		Member *dt = nullptr;
		DeclarationType decsp = DQ;
		FunctionMember fmem;
		Instruction *in = nullptr;
		int dtsize = 0;

		if (fexp1 == nullptr)
			return false;
		if (fexp2 == nullptr)
			return false;
		if (fexpopr == nullptr)
			return false;

		if (fexp1->is_id) {
			type = fexp1->id_info->type_info->type_specifier.simple_type[0];
			if (type.number != KEY_FLOAT) {
				if (type.number == KEY_DOUBLE);
				else
					return false;
			}
			else if (type.number != KEY_DOUBLE) {
				if (type.number == KEY_FLOAT);
				else
					return false;
			}
		}
		else if (fexp2->is_id) {
			type = fexp2->id_info->type_info->type_specifier.simple_type[0];
			if (type.number != KEY_FLOAT) {
				if (type.number == KEY_DOUBLE);
				else
					return false;
			}
			else if (type.number != KEY_DOUBLE) {
				if (type.number == KEY_FLOAT);
				else
					return false;
			}
		}

		if (!fexp1->is_id) {
			if (fexp1->tok.number != LIT_FLOAT) {
				if (!fexp2->is_id) {
					if (fexp2->tok.number != LIT_FLOAT)
						return false;
				}
			}
		}

		if (!fexp1->is_id) {
			dt = search_data(fexp1->tok.string);
			if (dt == nullptr) {
				dt = create_float_data(decsp, fexp1->tok.string);
			}
			in = get_insn(FLD, 1);
			in->operand_1->type = MEMORY;
			in->operand_1->mem.mem_type = GLOBAL;
			in->operand_1->mem.mem_size = 8;
			in->operand_1->mem.name = dt->symbol;
			in->comment = "  ; " + fexp1->tok.string;
			insncls->delete_operand(&(in->operand_2));
			instructions.push_back(in);

			if (!fexp2->is_id) {
				dt = search_data(fexp2->tok.string);
				if (dt == nullptr) {
					dt = create_float_data(decsp, fexp2->tok.string);
				}
				in = get_insn(FCOM, 1);
				in->operand_1->type = MEMORY;
				in->operand_1->mem.mem_type = GLOBAL;
				in->operand_1->mem.mem_size = 8;
				in->operand_1->mem.name = dt->symbol;
				in->comment = "  ; " + fexp2->tok.string;
				insncls->delete_operand(&(in->operand_2));
				instructions.push_back(in);
			}
			else {
				type = fexp2->id_info->type_info->type_specifier.simple_type[0];
				get_function_local_member(&fmem, fexp2->tok);
				dtsize = data_type_size(type);
				if (fmem.insize != -1) {
					in = get_insn(FCOM, 1);
					in->operand_1->type = MEMORY;
					in->operand_1->mem.mem_type = LOCAL;
					in->operand_1->mem.mem_size = dtsize;
					in->operand_1->mem.fp_disp = fmem.fp_disp;
					in->comment = "  ; " + fexp2->tok.string;
					insncls->delete_operand(&(in->operand_2));
					instructions.push_back(in);
				}
				else {
					in = get_insn(FCOM, 1);
					in->operand_1->type = MEMORY;
					in->operand_1->mem.mem_type = GLOBAL;
					in->operand_1->mem.mem_size = dtsize;
					in->operand_1->mem.name = fexp2->tok.string;
					in->comment = "  ; " + fexp2->tok.string;
					insncls->delete_operand(&(in->operand_2));
					instructions.push_back(in);
				}
			}
		}
		else {
			get_function_local_member(&fmem, fexp1->tok);
			type = fexp1->id_info->type_info->type_specifier.simple_type[0];
			dtsize = data_type_size(type);
			if (fmem.insize != -1) {
				in = get_insn(FLD, 1);
				in->operand_1->type = MEMORY;
				in->operand_1->mem.mem_type = LOCAL;
				in->operand_1->mem.mem_size = dtsize;
				in->operand_1->mem.fp_disp = fmem.fp_disp;
				in->comment = "  ; " + fexp1->tok.string;
				insncls->delete_operand(&(in->operand_2));
				instructions.push_back(in);
			}
			else {
				in = get_insn(FLD, 1);
				in->operand_1->type = MEMORY;
				in->operand_1->mem.mem_type = GLOBAL;
				in->operand_1->mem.mem_size = dtsize;
				in->operand_1->mem.name = fexp1->tok.string;
				in->comment = "  ; " + fexp1->tok.string;
				insncls->delete_operand(&(in->operand_2));
				instructions.push_back(in);
			}

			if (!fexp2->is_id) {
				dt = search_data(fexp2->tok.string);
				if (dt == nullptr) {
					dt = create_float_data(decsp, fexp2->tok.string);
				}
				in = get_insn(FCOM, 1);
				in->operand_1->type = MEMORY;
				in->operand_1->mem.mem_type = GLOBAL;
				in->operand_1->mem.mem_size = 8;
				in->operand_1->mem.name = dt->symbol;
				in->comment = "  ; " + fexp2->tok.string;
				insncls->delete_operand(&(in->operand_2));
				instructions.push_back(in);
			}
			else {
				type = fexp2->id_info->type_info->type_specifier.simple_type[0];
				get_function_local_member(&fmem, fexp2->tok);
				dtsize = data_type_size(type);
				if (fmem.insize != -1) {
					in = get_insn(FCOM, 1);
					in->operand_1->type = MEMORY;
					in->operand_1->mem.mem_type = LOCAL;
					in->operand_1->mem.mem_size = dtsize;
					in->operand_1->mem.fp_disp = fmem.fp_disp;
					in->comment = "  ; " + fexp2->tok.string;
					insncls->delete_operand(&(in->operand_2));
					instructions.push_back(in);
				}
				else {
					in = get_insn(FCOM, 1);
					in->operand_1->type = MEMORY;
					in->operand_1->mem.mem_type = GLOBAL;
					in->operand_1->mem.mem_size = dtsize;
					in->operand_1->mem.name = fexp2->tok.string;
					in->comment = "  ; " + fexp2->tok.string;
					insncls->delete_operand(&(in->operand_2));
					instructions.push_back(in);
				}
			}
		}

		in = get_insn(FSTSW, 1);
		in->operand_1->type = REGISTER;
		in->operand_1->reg = AX;
		insncls->delete_operand(&(in->operand_2));
		instructions.push_back(in);

		in = get_insn(SAHF, 0);
		insncls->delete_operand(&(in->operand_1));
		insncls->delete_operand(&(in->operand_2));
		instructions.push_back(in);

		return true;
	}

	TokenId CodeGen::gen_select_stmt_condition(Expression *_expr) {
		PrimaryExpression *pexpr = nullptr;
		Token tok;
		TokenId t;
		FunctionMember fmem;
		Instruction *in = nullptr;
		Token type;
		int dtsize = 0;
		if (_expr == nullptr)
			return NONE;

		auto resreg = [=](int sz) {
			if (sz == 1)
				return AL;
			else if (sz == 2)
				return AX;
			else
				return EAX;
		};

		switch (_expr->expr_kind) {
			case ExpressionType::PRIMARY_EXPR :
				pexpr = _expr->primary_expr;
				if (pexpr == nullptr)
					return NONE;
				insert_comment("; condition checking, line " + std::to_string(pexpr->tok.loc.line));
				//only 2 primary expressions are used exp1 op exp1
				//others are discaded
				if (pexpr->is_oprtr) {
					tok = pexpr->tok;
					t = tok.number;
					if (t == COMP_EQ || t == COMP_GREAT || t == COMP_GREAT_EQ || t == COMP_LESS || t == COMP_LESS_EQ || t == COMP_NOT_EQ) {
						//if any one of them is float type
						if (gen_float_type_condition(&pexpr->left, &pexpr->right, &pexpr))
							return t;

						//if both are identifiers id op id
						if (pexpr->left->tok.number == IDENTIFIER && pexpr->right->tok.number == IDENTIFIER) {
							get_function_local_member(&fmem, pexpr->right->tok);
							type = pexpr->left->id_info->type_info->type_specifier.simple_type[0];
							dtsize = data_type_size(type);
							in = get_insn(MOV, 2);
							in->operand_1->type = REGISTER;
							in->operand_1->reg = resreg(dtsize);
							if (fmem.insize != -1) {
								in->operand_2->type = MEMORY;
								in->operand_2->mem.mem_type = LOCAL;
								in->operand_2->mem.mem_size = fmem.insize;
								in->operand_2->mem.fp_disp = fmem.fp_disp;
							}
							else {
								in->operand_2->type = MEMORY;
								in->operand_2->mem.name = pexpr->right->tok.string;
								in->operand_2->mem.mem_type = GLOBAL;
								in->operand_2->mem.mem_size = data_type_size(pexpr->right->id_info->type_info->type_specifier.simple_type[0]);
							}
							instructions.push_back(in);
							in = nullptr;

							type = pexpr->right->id_info->type_info->type_specifier.simple_type[0];
							dtsize = data_type_size(type);
							get_function_local_member(&fmem, pexpr->left->tok);
							in = get_insn(CMP, 2);
							in->operand_2->type = REGISTER;
							in->operand_2->reg = resreg(dtsize);
							if (fmem.insize != -1) {
								in->operand_1->type = MEMORY;
								in->operand_1->mem.mem_type = LOCAL;
								in->operand_1->mem.mem_size = fmem.insize;
								in->operand_1->mem.fp_disp = fmem.fp_disp;
							}
							else {
								in->operand_1->type = MEMORY;
								in->operand_1->mem.name = pexpr->left->tok.string;
								in->operand_1->mem.mem_type = GLOBAL;
								in->operand_1->mem.mem_size = data_type_size(pexpr->left->id_info->type_info->type_specifier.simple_type[0]);
							}
							instructions.push_back(in);
							in = nullptr;
						}
						else if (pexpr->left->tok.number == IDENTIFIER && is_literal(pexpr->right->tok)) {
							get_function_local_member(&fmem, pexpr->left->tok);
							in = get_insn(CMP, 2);
							in->operand_2->type = LITERAL;
							in->operand_2->literal = std::to_string(Convert::tok_to_decimal(pexpr->right->tok));
							if (fmem.insize != -1) {
								in->operand_1->type = MEMORY;
								in->operand_1->mem.mem_type = LOCAL;
								in->operand_1->mem.mem_size = fmem.insize;
								in->operand_1->mem.fp_disp = fmem.fp_disp;
							}
							else {
								in->operand_1->type = MEMORY;
								in->operand_1->mem.name = pexpr->left->tok.string;
								in->operand_1->mem.mem_type = GLOBAL;
								in->operand_1->mem.mem_size = data_type_size(pexpr->left->id_info->type_info->type_specifier.simple_type[0]);
							}
							instructions.push_back(in);
						}
						else if (is_literal(pexpr->left->tok) && pexpr->right->tok.number == IDENTIFIER) {
							get_function_local_member(&fmem, pexpr->right->tok);
							in = get_insn(CMP, 2);
							in->operand_2->type = LITERAL;
							in->operand_2->literal = std::to_string(Convert::tok_to_decimal(pexpr->left->tok));
							if (fmem.insize != -1) {
								in->operand_1->type = MEMORY;
								in->operand_1->mem.mem_type = LOCAL;
								in->operand_1->mem.mem_size = fmem.insize;
								in->operand_1->mem.fp_disp = fmem.fp_disp;
							}
							else {
								in->operand_1->type = MEMORY;
								in->operand_1->mem.name = pexpr->right->tok.string;
								in->operand_1->mem.mem_type = GLOBAL;
								in->operand_1->mem.mem_size = data_type_size(pexpr->right->id_info->type_info->type_specifier.simple_type[0]);
							}
							instructions.push_back(in);
						}
						else if (is_literal(pexpr->left->tok) && is_literal(pexpr->right->tok)) {
							in = get_insn(MOV, 2);
							in->operand_1->type = REGISTER;

							if (Compiler::global.x64)
								in->operand_1->reg = RAX;
							else
								in->operand_1->reg = EAX;

							in->operand_2->type = LITERAL;
							in->operand_2->literal = std::to_string(Convert::tok_to_decimal(pexpr->left->tok));
							instructions.push_back(in);
							in = nullptr;

							in = get_insn(CMP, 2);
							in->operand_1->type = REGISTER;

							if (Compiler::global.x64)
								in->operand_1->reg = RAX;
							else
								in->operand_1->reg = EAX;

							in->operand_2->type = LITERAL;
							in->operand_2->literal = std::to_string(Convert::tok_to_decimal(pexpr->right->tok));
							instructions.push_back(in);
						}
						return t;
					}
				}
				break;

			default:
				Log::error("only primary Expression supported in code generation");
				break;
		}
		return NONE;
	}

	void CodeGen::gen_selection_statement(SelectStatement **slstmt) {
		SelectStatement *selstmt = *slstmt;
		TokenId cond;
		Instruction *in = nullptr;

		if (selstmt == nullptr)
			return;

		cond = gen_select_stmt_condition(selstmt->condition);

		in = get_insn(JMP, 1);
		in->operand_1->type = LITERAL;
		in->operand_1->literal = ".if_label" + std::to_string(if_label_count);
		insncls->delete_operand(&(in->operand_2));

		switch (cond) {
			case COMP_EQ :
				in->insn_type = JE;
				break;
			case COMP_GREAT :
				in->insn_type = JG;
				break;
			case COMP_GREAT_EQ :
				in->insn_type = JGE;
				break;
			case COMP_LESS :
				in->insn_type = JL;
				break;
			case COMP_LESS_EQ :
				in->insn_type = JLE;
				break;
			case COMP_NOT_EQ :
				in->insn_type = JNE;
				break;
			default:
				break;
		}
		instructions.push_back(in);

		in = get_insn(JMP, 1);
		in->operand_1->type = LITERAL;
		in->operand_1->literal = ".else_label" + std::to_string(if_label_count);
		insncls->delete_operand(&(in->operand_2));
		instructions.push_back(in);

		in = get_insn(INSLABEL, 0);
		in->label = ".if_label" + std::to_string(if_label_count);
		insncls->delete_operand(&(in->operand_1));
		insncls->delete_operand(&(in->operand_2));
		instructions.push_back(in);

		if (selstmt->if_statement != nullptr) {
			if_label_count++;
			gen_statement(&(selstmt->if_statement));

			in = get_insn(JMP, 1);
			in->operand_1->type = LITERAL;
			in->operand_1->literal = ".exit_if" + std::to_string(exit_if_count);
			insncls->delete_operand(&(in->operand_2));
			instructions.push_back(in);
		}

		in = get_insn(INSLABEL, 0);
		in->label = ".else_label" + std::to_string(else_label_count);
		insncls->delete_operand(&(in->operand_1));
		insncls->delete_operand(&(in->operand_2));
		instructions.push_back(in);
		else_label_count++;

		if (selstmt->else_statement != nullptr)
			gen_statement(&(selstmt->else_statement));

		in = get_insn(INSLABEL, 0);
		in->label = ".exit_if" + std::to_string(exit_if_count);
		insncls->delete_operand(&(in->operand_1));
		insncls->delete_operand(&(in->operand_2));
		instructions.push_back(in);

		exit_if_count++;
	}

	void CodeGen::gen_iteration_statement(IterationStatement **istmt) {
		IterationStatement *itstmt = *istmt;
		TokenId cond;
		Instruction *in = nullptr;
		int forcnt, whilecnt;

		if (itstmt == nullptr)
			return;

		in = get_insn(INSLABEL, 0);
		insncls->delete_operand(&(in->operand_1));
		insncls->delete_operand(&(in->operand_2));

		//create loop label, while, dowhile, for
		switch (itstmt->type) {
			case IterationType::WHILE :
				insert_comment("; while loop, line " + std::to_string(itstmt->_while.whiletok.loc.line));
				in->label = ".while_loop" + std::to_string(while_loop_count);
				current_loop = IterationType::WHILE;
				while_loop_stack.push(while_loop_count);
				while_loop_count++;
				break;
			case IterationType::DOWHILE :
				insert_comment("; do-while loop, line " + std::to_string(itstmt->_dowhile.dotok.loc.line));
				in->label = ".dowhile_loop" + std::to_string(dowhile_loop_count);
				current_loop = IterationType::DOWHILE;
				dowhile_loop_stack.push(dowhile_loop_count);
				dowhile_loop_count++;
				break;
			case IterationType::FOR :
				insert_comment("; for loop, line " + std::to_string(itstmt->_for.fortok.loc.line));
				current_loop = IterationType::FOR;
				gen_expr(&(itstmt->_for.init_expr));
				in->label = ".for_loop" + std::to_string(for_loop_count);
				for_loop_stack.push(for_loop_count);
				for_loop_count++;
				break;
			default:
				break;
		}
		instructions.push_back(in);

		switch (itstmt->type) {
			case IterationType::WHILE :
				//gen while loop condition
				cond = gen_select_stmt_condition(itstmt->_while.condition);
				in = get_insn(JMP, 1);
				in->operand_1->type = LITERAL;

				if (!while_loop_stack.empty())
					in->operand_1->literal = ".exit_while_loop" + std::to_string(while_loop_stack.top());
				else
					in->operand_1->literal = ".exit_while_loop" + std::to_string(exit_loop_label_count);

				insncls->delete_operand(&(in->operand_2));
				instructions.push_back(in);

				switch (cond) {
					case COMP_EQ :
						in->insn_type = JNE;
						break;
					case COMP_GREAT :
						in->insn_type = JLE;
						break;
					case COMP_GREAT_EQ :
						in->insn_type = JL;
						break;
					case COMP_LESS :
						in->insn_type = JGE;
						break;
					case COMP_LESS_EQ :
						in->insn_type = JG;
						break;
					case COMP_NOT_EQ :
						in->insn_type = JE;
						break;
					default:
						insncls->delete_insn(&in);
						instructions.pop_back();
						break;
				}

				gen_statement(&(itstmt->_while.statement));

				in = get_insn(JMP, 1);
				in->operand_1->type = LITERAL;

				if (!while_loop_stack.empty()) {
					whilecnt = while_loop_stack.top();
					in->operand_1->literal = ".while_loop" + std::to_string(whilecnt);
					while_loop_stack.pop();
				}
				else {
					in->operand_1->literal = ".while_loop" + std::to_string(while_loop_count);
					whilecnt = while_loop_count;
				}

				in->comment = "    ; jmp to while loop";
				insncls->delete_operand(&(in->operand_2));
				instructions.push_back(in);
				while_loop_count++;

				in = get_insn(INSLABEL, 0);
				in->label = ".exit_while_loop" + std::to_string(whilecnt);
				insncls->delete_operand(&(in->operand_1));
				insncls->delete_operand(&(in->operand_2));
				instructions.push_back(in);
				break;

			case IterationType::DOWHILE :

				gen_statement(&(itstmt->_dowhile.statement));
				cond = gen_select_stmt_condition(itstmt->_dowhile.condition);
				in = get_insn(JMP, 1);
				in->operand_1->type = LITERAL;

				if (!dowhile_loop_stack.empty()) {
					in->operand_1->literal = ".dowhile_loop" + std::to_string(dowhile_loop_stack.top());
					dowhile_loop_stack.pop();
				}
				else
					in->operand_1->literal = ".dowhile_loop" + std::to_string(exit_loop_label_count);

				insncls->delete_operand(&(in->operand_2));

				switch (cond) {
					case COMP_EQ :
						in->insn_type = JE;
						break;
					case COMP_GREAT :
						in->insn_type = JG;
						break;
					case COMP_GREAT_EQ :
						in->insn_type = JGE;
						break;
					case COMP_LESS :
						in->insn_type = JL;
						break;
					case COMP_LESS_EQ :
						in->insn_type = JLE;
						break;
					case COMP_NOT_EQ :
						in->insn_type = JNE;
						break;
					default:
						break;
				}

				instructions.push_back(in);
				dowhile_loop_count++;
				break;

			case IterationType::FOR :

				cond = gen_select_stmt_condition(itstmt->_for.condition);
				in = get_insn(JMP, 1);
				in->operand_1->type = LITERAL;

				if (!for_loop_stack.empty())
					in->operand_1->literal = ".exit_for_loop" + std::to_string(for_loop_stack.top());
				else
					in->operand_1->literal = ".exit_for_loop" + std::to_string(exit_loop_label_count);

				insncls->delete_operand(&(in->operand_2));
				instructions.push_back(in);

				switch (cond) {
					case COMP_EQ :
						in->insn_type = JNE;
						break;
					case COMP_GREAT :
						in->insn_type = JLE;
						break;
					case COMP_GREAT_EQ :
						in->insn_type = JL;
						break;
					case COMP_LESS :
						in->insn_type = JGE;
						break;
					case COMP_LESS_EQ :
						in->insn_type = JG;
						break;
					case COMP_NOT_EQ :
						in->insn_type = JE;
						break;
					default:
						insncls->delete_insn(&in);
						instructions.pop_back();
						break;
				}

				gen_statement(&(itstmt->_for.statement));
				gen_expr(&(itstmt->_for.update_expr));
				in = get_insn(JMP, 1);
				in->operand_1->type = LITERAL;

				if (!for_loop_stack.empty()) {
					forcnt = for_loop_stack.top();
					in->operand_1->literal = ".for_loop" + std::to_string(forcnt);
					for_loop_stack.pop();
				}
				else {
					in->operand_1->literal = ".for_loop" + std::to_string(for_loop_count);
					forcnt = for_loop_count;
				}

				in->comment = "    ; jmp to for loop";
				insncls->delete_operand(&(in->operand_2));
				instructions.push_back(in);
				for_loop_count++;

				in = get_insn(INSLABEL, 0);
				in->label = ".exit_for_loop" + std::to_string(forcnt);
				insncls->delete_operand(&(in->operand_1));
				insncls->delete_operand(&(in->operand_2));
				instructions.push_back(in);
				break;

			default:
				break;
		}
	}

	void CodeGen::gen_statement(Statement **_stmt) {
		Statement *_stmt2 = *_stmt;
		if (_stmt2 == nullptr)
			return;

		while (_stmt2 != nullptr) {
			switch (_stmt2->type) {
				case StatementType::LABEL :
					gen_label_statement(&(_stmt2->labled_statement));
					break;
				case StatementType::EXPR :
					gen_expr(&(_stmt2->expression_statement->expression));
					break;
				case StatementType::SELECT :
					gen_selection_statement(&(_stmt2->selection_statement));
					break;
				case StatementType::ITER :
					gen_iteration_statement(&(_stmt2->iteration_statement));
					break;
				case StatementType::JUMP :
					gen_jump_statement(&(_stmt2->jump_statement));
					break;
				case StatementType::ASM :
					gen_asm_statement(&(_stmt2->asm_statement));
					break;
				default:
					break;
			}
			_stmt2 = _stmt2->p_next;
		}
	}

	void CodeGen::save_frame_pointer() {

		// save frame pointer and create new stack for new function
		// push ebp|rbp
		// mov ebp|rpb, esp|rsp

		if (!Compiler::global.omit_frame_pointer) {
			Instruction *in = get_insn(PUSH, 1);
			in->operand_1->type = REGISTER;

			if (Compiler::global.x64)
				in->operand_1->reg = RBP;
			else
				in->operand_1->reg = EBP;

			insncls->delete_operand(&(in->operand_2));
			instructions.push_back(in);
			in = nullptr;

			in = get_insn(MOV, 2);
			in->insn_type = MOV;
			in->operand_count = 2;
			in->operand_1->type = REGISTER;

			if (Compiler::global.x64)
				in->operand_1->reg = RBP;
			else
				in->operand_1->reg = EBP;

			in->operand_2->type = REGISTER;
			if (Compiler::global.x64)
				in->operand_2->reg = RSP;
			else
				in->operand_2->reg = ESP;

			instructions.push_back(in);
		}
	}

	void CodeGen::restore_frame_pointer() {

		// local label for exiting function,for return
		// ._exit_<function-name>
		//
		// restore stack frame pointer
		// mov esp|rsp, ebp|rbp
		// pop ebp|rbp
		//
		// here leave instruction can also be used

		Instruction *in = get_insn(INSLABEL, 0);
		in->label = "._exit_" + func_symtab->func_info->func_name;
		insncls->delete_operand(&(in->operand_1));
		insncls->delete_operand(&(in->operand_2));
		instructions.push_back(in);
		in = nullptr;

		if (!Compiler::global.omit_frame_pointer) {
			in = get_insn(MOV, 2);
			in->insn_type = MOV;
			in->operand_count = 2;

			in->operand_1->type = REGISTER;
			if (Compiler::global.x64)
				in->operand_1->reg = RSP;
			else
				in->operand_1->reg = ESP;

			in->operand_2->type = REGISTER;
			if (Compiler::global.x64)
				in->operand_2->reg = RBP;
			else
				in->operand_2->reg = EBP;

			instructions.push_back(in);
			in = nullptr;

			in = get_insn(POP, 1);
			in->insn_type = POP;
			in->operand_count = 1;
			in->operand_1->type = REGISTER;

			if (Compiler::global.x64)
				in->operand_1->reg = RBP;
			else
				in->operand_1->reg = EBP;

			insncls->delete_operand(&(in->operand_2));
			instructions.push_back(in);
		}
	}

	void CodeGen::func_return() {
		Instruction *in = nullptr;
		in = insncls->get_insn_mem();
		in->insn_type = RET;
		in->operand_count = 0;
		insncls->delete_operand(&(in->operand_1));
		insncls->delete_operand(&(in->operand_2));
		instructions.push_back(in);
	}

	void CodeGen::gen_function() {

		//generate x86|x64 assembly function

		Instruction *in = nullptr;
		funcmem_iterator fmemit;
		memb_iterator memit;
		int fpdisp = 0;
		std::string comment = "; [ function: " + func_symtab->func_info->func_name;

		if (func_symtab->func_info->param_list.size() > 0) {
			comment.push_back('(');
			for (auto e: func_symtab->func_info->param_list) {
				if (e->type_info->type == NodeType::SIMPLE) {
					comment += e->type_info->type_specifier.simple_type[0].string + " ";
					comment += e->symbol_info->symbol + ", ";
				}
				else {
					comment += e->type_info->type_specifier.record_type.string + " ";
					comment += e->symbol_info->symbol + ", ";
				}
			}
			if (comment.length() > 1) {
				comment.pop_back();
				comment.pop_back();
			}
			comment.push_back(')');
		}
		else {
			comment = comment + "()";
		}
		comment += " ]";

		insert_comment(comment);

		in = get_insn(INSLABEL, 0);
		in->label = func_symtab->func_info->func_name;
		insncls->delete_operand(&(in->operand_1));
		insncls->delete_operand(&(in->operand_2));
		instructions.push_back(in);
		in = nullptr;

		get_func_local_members();

		save_frame_pointer();

		//allocate memory on stack for local variables
		fmemit = func_members.find(func_symtab->func_info->func_name);

		if (fmemit != func_members.end()) {
			if (fmemit->second.total_size > 0) {
				in = insncls->get_insn_mem();
				in->insn_type = SUB;
				in->operand_count = 2;
				in->operand_1->type = REGISTER;

				if (Compiler::global.x64)
					in->operand_1->reg = RSP;
				else
					in->operand_1->reg = ESP;

				in->operand_2->type = LITERAL;
				in->operand_2->literal = std::to_string(fmemit->second.total_size);
				in->comment = "    ; allocate space for local variables";
				instructions.push_back(in);
			}

			//emit local variables location comments
			memit = fmemit->second.members.begin();
			while (memit != fmemit->second.members.end()) {
				fpdisp = memit->second.fp_disp;
				if (fpdisp < 0) {
					if (Compiler::global.x64)
						insert_comment("    ; " + memit->first + " = [rbp - " + std::to_string(fpdisp * (-1)) + "]" + ", " + insncls->insnsize_name(get_insn_size_type(memit->second.insize)));
					else
						insert_comment("    ; " + memit->first + " = [ebp - " + std::to_string(fpdisp * (-1)) + "]" + ", " + insncls->insnsize_name(get_insn_size_type(memit->second.insize)));
				}
				else {
					if (Compiler::global.x64)
						insert_comment("    ; " + memit->first + " = [rbp + " + std::to_string(fpdisp) + "]" + ", " + insncls->insnsize_name(get_insn_size_type(memit->second.insize)));
					else
						insert_comment("    ; " + memit->first + " = [ebp + " + std::to_string(fpdisp) + "]" + ", " + insncls->insnsize_name(get_insn_size_type(memit->second.insize)));
				}
				memit++;
			}
		}
	}

	void CodeGen::gen_uninitialized_data() {

		// generate uninitialized data in bss section
		// search symbol in global symbol table which are
		// not in data section symbols and put them in bss section

		int i;
		SymbolInfo *temp = nullptr;
		std::list<Token>::iterator it;

		if (Compiler::symtab == nullptr)
			return;

		for (i = 0; i < ST_SIZE; i++) {
			temp = Compiler::symtab->symbol_info[i];
			while (temp != nullptr && temp->type_info != nullptr) {
				//check if globaly declared variable is global or extern
				//if global/extern then put them in text section
				if (temp->type_info->is_global) {
					TextSection *txt = insncls->get_text_mem();
					txt->type = TXTGLOBAL;
					txt->symbol = temp->symbol;
					text_section.push_back(txt);
				}
				else if (temp->type_info->is_extern) {
					TextSection *txt = insncls->get_text_mem();
					txt->type = TXTEXTERN;
					txt->symbol = temp->symbol;
					text_section.push_back(txt);
				}

				if (initialized_data.find(temp->symbol) == initialized_data.end()) {
					ReserveSection *rv = insncls->get_resv_mem();
					TypeInfo *typeinf = temp->type_info;
					rv->symbol = temp->symbol;

					if (typeinf->type == NodeType::SIMPLE) {
						rv->type = resvspace_type_size(typeinf->type_specifier.simple_type[0]);
						rv->res_size = 1;
					}
					else if (typeinf->type == NodeType::RECORD) {
						rv->type = RESB;
						std::unordered_map<std::string, int>::iterator it;
						it = record_sizes.find(typeinf->type_specifier.record_type.string);
						if (it != record_sizes.end()) {
							rv->res_size = it->second;
						}
					}

					if (temp->is_array) {
						if (temp->arr_dimension_list.size() > 1) {
							it = temp->arr_dimension_list.begin();
							while (it != temp->arr_dimension_list.end()) {
								rv->res_size *= Convert::tok_to_decimal(*it);
								it++;
							}
						}
						else {
							rv->res_size = Convert::tok_to_decimal(*(temp->arr_dimension_list.begin()));
						}
					}
					else {
						if (rv->res_size < 1)
							rv->res_size = 1;
					}
					resv_section.push_back(rv);
				}

				temp = temp->p_next;
			}
		}
	}

	void CodeGen::gen_array_init_declaration(Node *symtab) {
		int i;
		SymbolInfo *syminf = nullptr;
		Member *dt = nullptr;

		if (symtab == nullptr)
			return;

		for (i = 0; i < ST_SIZE; i++) {
			syminf = symtab->symbol_info[i];
			while (syminf != nullptr) {
				if (syminf->is_array && !syminf->arr_init_list.empty()) {
					dt = insncls->get_data_mem();
					dt->is_array = true;
					dt->symbol = syminf->symbol;
					dt->type = declspace_type_size(syminf->type_info->type_specifier.simple_type[0]);
					initialized_data[dt->symbol] = syminf;

					for (auto e1: syminf->arr_init_list) {
						for (auto e2: e1) {
							if (e2.number == LIT_FLOAT) {
								dt->array_data.push_back(e2.string);
							}
							else {
								dt->array_data.push_back(std::to_string(Convert::tok_to_decimal(e2)));
							}
						}
					}
					data_section.push_back(dt);
				}
				syminf = syminf->p_next;
			}
		}
	}

	void CodeGen::gen_record() {

		// traverse through record table and generate its data section entry
		// here using struc/endstruc macro provided by NASM assembler for record type
		// also calculate size of each record and insert it into record_sizes table

		RecordNode *recnode = nullptr;
		Node *recsymtab = nullptr;
		SymbolInfo *syminf = nullptr;
		TypeInfo *typeinf = nullptr;
		int record_size = 0;

		if (Compiler::record_table == nullptr)
			return;
		for (int i = 0; i < ST_RECORD_SIZE; i++) {
			recnode = Compiler::record_table->recordinfo[i];
			//iterate through each record linked list
			while (recnode != nullptr) {
				record_size = 0;
				ReserveSection *rv = insncls->get_resv_mem();
				rv->is_record = true;
				rv->record_name = recnode->recordname;
				rv->comment = "    ; record " + recnode->recordname + " { }";
				recsymtab = recnode->symtab;

				if (recsymtab == nullptr)
					break;

				//iterate through symbol table of record
				for (int j = 0; j < ST_SIZE; j++) {
					syminf = recsymtab->symbol_info[j];
					//iterate through each symbol linked list
					while (syminf != nullptr) {
						RecordDataType rectype;
						typeinf = syminf->type_info;
						rectype.symbol = syminf->symbol;

						if (syminf->is_array) {
							int arrsize = 1;
							for (auto x: syminf->arr_dimension_list)
								arrsize = arrsize * Convert::tok_to_decimal(x);
							rectype.resv_size = arrsize;
						}
						else
							rectype.resv_size = 1;

						if (typeinf->type == NodeType::SIMPLE) {
							if (syminf->is_ptr) {
								rectype.resvsp_type = RESD;
								record_size += 4;
							}
							else {
								rectype.resvsp_type = resvspace_type_size(typeinf->type_specifier.simple_type[0]);

								if (syminf->is_array)
									record_size += rectype.resv_size * resv_decl_size(rectype.resvsp_type);
								else
									record_size += resv_decl_size(rectype.resvsp_type);
							}
						}
						else if (typeinf->type == NodeType::RECORD) {
							rectype.resvsp_type = RESD;

							if (syminf->is_array)
								record_size += rectype.resv_size * (Compiler::global.x64 ? 8 : 4);
							else
								record_size += (Compiler::global.x64 ? 8 : 4);
						}

						rv->record_members.push_back(rectype);
						syminf = syminf->p_next;
					}
				}

				record_sizes.insert(std::pair<std::string, int>(rv->record_name, record_size));
				resv_section.push_back(rv);
				rv = nullptr;
				recnode = recnode->p_next;
			}
		}
	}

	void CodeGen::gen_global_declarations(TreeNode **trnode) {

		// generate global declarations/assignment expressions
		// and put them into data section
		// this is totaly separate pass

		TreeNode *trhead = *trnode;
		Statement *stmthead = nullptr;
		Expression *_expr = nullptr;
		if (trhead == nullptr)
			return;

		gen_array_init_declaration(Compiler::symtab);

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

			if (stmthead != nullptr) {
				if (stmthead->type == StatementType::EXPR) {
					_expr = stmthead->expression_statement->expression;
					if (_expr != nullptr) {

						switch (_expr->expr_kind) {
							case ExpressionType::ASSGN_EXPR : {
								if (_expr->assgn_expr->expression == nullptr)
									return;

								PrimaryExpression *pexpr = _expr->assgn_expr->expression->primary_expr;
								if (initialized_data.find(_expr->assgn_expr->id_expr->id_info->symbol) != initialized_data.end()) {
									Log::error_at(_expr->assgn_expr->tok.loc, "'" + _expr->assgn_expr->id_expr->id_info->symbol + "' assigned multiple times");
									return;

								}

								initialized_data.insert(std::pair<std::string, SymbolInfo *>(_expr->assgn_expr->id_expr->id_info->symbol, _expr->assgn_expr->id_expr->id_info));
								Member *dt = insncls->get_data_mem();
								SymbolInfo *sminf = _expr->assgn_expr->id_expr->id_info;
								dt->symbol = sminf->symbol;
								dt->type = declspace_type_size(sminf->type_info->type_specifier.simple_type[0]);
								dt->is_array = false;

								if (pexpr->tok.number == LIT_STRING) {
									dt->symbol = dt->symbol;
									dt->value = get_hex_string(pexpr->tok.string);
									dt->comment = "    ; '" + pexpr->tok.string + "'";
								}
								else
									dt->value = pexpr->tok.string;

								data_section.push_back(dt);
							}
								break;

							default:
								break;
						}
					}
				}
			}
			trhead = trhead->p_next;
		}

		gen_record();
		//generate uninitialize data(bss)
		gen_uninitialized_data();
	}

	void CodeGen::write_text_to_asm_file(std::ofstream &outfile) {
		if (!outfile.is_open())
			return;
		if (text_section.empty())
			return;
		outfile << "\nsection .text\n";
		for (TextSection *t: text_section) {
			if (t->type != TXTNONE) {
				outfile << "    " << insncls->text_type_name(t->type) << " " << t->symbol << "\n";
			}
		}
		outfile << "\n";
	}

	void CodeGen::write_record_member_to_asm_file(RecordDataType &x, std::ofstream &outfile) {
		outfile << "      ." << x.symbol << " " << insncls->resspace_name(x.resvsp_type) << " " << std::to_string(x.resv_size) << "\n";
	}

	void CodeGen::write_record_data_to_asm_file(ReserveSection **rv, std::ofstream &outfile) {
		ReserveSection *r = *rv;
		if (r == nullptr)
			return;

		outfile << "    struc " << r->record_name << " " << r->comment << "\n";

		//write all resb types
		for (auto x: r->record_members) {
			if (x.resvsp_type == RESB) {
				write_record_member_to_asm_file(x, outfile);
			}
		}

		//write all resw types
		for (auto x: r->record_members) {
			if (x.resvsp_type == RESW) {
				write_record_member_to_asm_file(x, outfile);
			}
		}

		//write all resd types
		for (auto x: r->record_members) {
			if (x.resvsp_type == RESD) {
				write_record_member_to_asm_file(x, outfile);
			}
		}

		//write all resq types
		for (auto x: r->record_members) {
			if (x.resvsp_type == RESQ) {
				write_record_member_to_asm_file(x, outfile);
			}
		}

		outfile << "    endstruc" << "\n";
	}

	void CodeGen::write_data_to_asm_file(std::ofstream &outfile) {

		if (!outfile.is_open())
			return;

		if (data_section.empty())
			return;

		outfile << "\nsection .data\n";

		for (Member *d: data_section) {
			if (d->is_array) {
				outfile << "    " << d->symbol << " " << insncls->declspace_name(d->type) << " ";

				size_t s = d->array_data.size();
				if (s > 0) {
					for (size_t i = 0; i < s - 1; i++) {
						outfile << d->array_data[i] << ", ";
					}
					outfile << d->array_data[s - 1];
				}
				outfile << "\n";
			}
			else {
				outfile << "    " << d->symbol << " " << insncls->declspace_name(d->type) << " " << d->value << d->comment << "\n";
			}
		}
		outfile << "\n";
	}

	void CodeGen::write_resv_to_asm_file(std::ofstream &outfile) {
		if (!outfile.is_open())
			return;
		if (resv_section.empty())
			return;
		outfile << "\nsection .bss\n";
		for (ReserveSection *r: resv_section) {
			if (r->is_record) {
				write_record_data_to_asm_file(&r, outfile);
				continue;
			}
			outfile << "    " << r->symbol << " " << insncls->resspace_name(r->type) << " " << r->res_size << "\n";
		}
		outfile << "\n";
	}

	void CodeGen::write_instructions_to_asm_file(std::ofstream &outfile) {
		std::string cast;
		if (!outfile.is_open())
			return;

		for (Instruction *in: instructions) {
			if (in->insn_type == INSLABEL) {
				outfile << in->label << ":\n";
				continue;
			}
			if (in->insn_type == INSASM) {
				outfile << in->inline_asm << "\n";
				continue;
			}

			if (in->insn_type != INSNONE) {
				outfile << "    " << insncls->insn_name(in->insn_type) << " ";
			}

			if (in->operand_count == 2) {

				switch (in->operand_1->type) {
					case REGISTER :
						outfile << reg->reg_name(in->operand_1->reg);
						break;
					case FREGISTER :
						outfile << reg->freg_name(in->operand_1->freg);
						break;
					case LITERAL :
						outfile << in->operand_1->literal;
						break;
					case MEMORY :
						switch (in->operand_1->mem.mem_type) {
							case GLOBAL :
								cast = insncls->insnsize_name(get_insn_size_type(in->operand_1->mem.mem_size));
								outfile << cast << "[" << in->operand_1->mem.name;
								if (in->operand_1->is_array && in->operand_1->reg != RNONE) {
									outfile << " + " + reg->reg_name(in->operand_1->reg) << " * " << std::to_string(in->operand_1->arr_disp);
								}
								if (in->operand_1->mem.fp_disp > 0) {
									outfile << " + " + std::to_string(in->operand_1->mem.fp_disp) << "]";
								}
								else {
									outfile << "]";
								}
								break;
							case LOCAL :
								cast = insncls->insnsize_name(get_insn_size_type(in->operand_1->mem.mem_size));
								outfile << cast << "[ebp";
								if (in->operand_1->mem.fp_disp > 0) {
									outfile << " + " + std::to_string(in->operand_1->mem.fp_disp) << "]";
								}
								else {
									outfile << " - " << std::to_string((in->operand_1->mem.fp_disp) * (-1)) << "]";
								}
								break;
							default:
								break;
						}
						break;

					default:
						break;
				}

				outfile << ", ";

				switch (in->operand_2->type) {
					case REGISTER :
						outfile << reg->reg_name(in->operand_2->reg);
						break;
					case FREGISTER :
						outfile << reg->freg_name(in->operand_2->freg);
					case LITERAL :
						outfile << in->operand_2->literal;
						break;
					case MEMORY :
						switch (in->operand_2->mem.mem_type) {
							case GLOBAL :
								if (in->operand_2->mem.mem_size < 0) {
									outfile << in->operand_2->mem.name;
								}
								else {
									cast = insncls->insnsize_name(get_insn_size_type(in->operand_2->mem.mem_size));
									outfile << cast << "[" << in->operand_2->mem.name;
									if (in->operand_2->is_array && in->operand_2->reg != RNONE) {
										outfile << " + " + reg->reg_name(in->operand_2->reg) << " * " << std::to_string(in->operand_2->arr_disp);
									}
									if (in->operand_2->mem.fp_disp > 0) {
										outfile << " + " + std::to_string(in->operand_2->mem.fp_disp) << "]";
									}
									else {
										outfile << "]";
									}
								}
								break;
							case LOCAL :
								if (in->operand_2->mem.mem_size <= 0) {
									cast = "";
								}
								else {
									cast = insncls->insnsize_name(get_insn_size_type(in->operand_2->mem.mem_size));
								}
								outfile << cast << "[ebp";
								if (in->operand_2->mem.fp_disp > 0) {
									outfile << " + " + std::to_string(in->operand_2->mem.fp_disp) << "]";
								}
								else {
									outfile << " - " << std::to_string((in->operand_2->mem.fp_disp) * (-1)) << "]";
								}
								break;
							default:
								break;
						}
						break;

					default:
						break;
				}

			}
			else if (in->operand_count == 1) {
				switch (in->operand_1->type) {
					case REGISTER :
						outfile << reg->reg_name(in->operand_1->reg);
						break;
					case FREGISTER :
						outfile << reg->freg_name(in->operand_1->freg);
						break;
					case LITERAL :
						outfile << in->operand_1->literal;
						break;
					case MEMORY :
						switch (in->operand_1->mem.mem_type) {
							case GLOBAL :
								cast = insncls->insnsize_name(get_insn_size_type(in->operand_1->mem.mem_size));
								if (in->operand_1->mem.name.empty()) {
									outfile << cast << "[" << reg->reg_name(in->operand_1->reg);
									if (in->operand_1->mem.fp_disp > 0) {
										outfile << " + " + std::to_string(in->operand_1->mem.fp_disp) << "]";
									}
									else {
										outfile << "]";
									}
								}
								else {
									outfile << cast << "[" << in->operand_1->mem.name;
									if (in->operand_1->mem.fp_disp > 0) {
										outfile << " + " + std::to_string(in->operand_1->mem.fp_disp) << "]";
									}
									else {
										outfile << "]";
									}
								}
								break;
							case LOCAL :
								cast = insncls->insnsize_name(get_insn_size_type(in->operand_1->mem.mem_size));
								outfile << cast << "[ebp";
								if (in->operand_1->mem.fp_disp > 0) {
									outfile << " + " + std::to_string(in->operand_1->mem.fp_disp) << "]";
								}
								else {
									outfile << " - " << std::to_string((in->operand_1->mem.fp_disp) * (-1)) << "]";
								}
								break;
							default:
								break;
						}
						break;

					default:
						break;
				}
			}
			outfile << in->comment;
			outfile << "\n";
		}
	}

	void CodeGen::write_asm_file() {
		std::ofstream outfile(Compiler::global.file.asm_name(), std::ios::out);

		write_text_to_asm_file(outfile);
		write_instructions_to_asm_file(outfile);
		write_data_to_asm_file(outfile);
		write_resv_to_asm_file(outfile);

		outfile.close();
	}

	bool CodeGen::search_text(TextSection *tx) {
		if (tx == nullptr)
			return false;
		for (auto e: text_section) {
			if (e->type == tx->type && e->symbol == tx->symbol)
				return true;
		}
		return false;
	}

	//generate final assembly code

	void CodeGen::get_code(TreeNode **ast) {

		TreeNode *trhead = *ast;
		if (trhead == nullptr)
			return;

		if (Compiler::global.optimize) {
			Optimizer *optmz = new Optimizer();
			optmz->optimize(&trhead);
			delete optmz;
			optmz = nullptr;
			if (Compiler::global.error_count > 0)
				return;
		}

		gen_global_declarations(&trhead);

		trhead = *ast;
		while (trhead != nullptr) {

			if (trhead->symtab != nullptr) {
				func_symtab = trhead->symtab;
				func_params = trhead->symtab->func_info;
			}

			if (trhead->symtab == nullptr) {
				if (trhead->statement != nullptr && trhead->statement->type == StatementType::ASM) {
					gen_asm_statement(&trhead->statement->asm_statement);
					trhead = trhead->p_next;
					continue;
				}
			}

			//global expression does not have sumbol table
			//if symbol table is found, then function definition is also found

			if (func_symtab != nullptr) {

				//generate text section types for function(scope: global, extern)
				TextSection *t = insncls->get_text_mem();
				t->symbol = func_symtab->func_info->func_name;

				if (func_symtab->func_info->is_global)
					t->type = TXTGLOBAL;
				else if (func_symtab->func_info->is_extern)
					t->type = TXTEXTERN;
				else
					t->type = TXTNONE;

				if (t->type != TXTNONE) {
					if (search_text(t))
						insncls->delete_text(&t);
					else
						text_section.push_back(t);
				}

				if (!func_symtab->func_info->is_extern) {
					get_func_local_members();
					gen_function();

					if_label_count = 1;
					else_label_count = 1;
					exit_if_count = 1;
					while_loop_count = 1;
					dowhile_loop_count = 1;
					for_loop_count = 1;
					exit_loop_label_count = 1;

					gen_statement(&trhead->statement);
					restore_frame_pointer();
					func_return();
				}
			}

			trhead = trhead->p_next;
		}

		write_asm_file();
	}
}
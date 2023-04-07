/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


#pragma once

#include <vector>
#include <string>
#include "symtab.hpp"
#include "regs.hpp"

namespace xlang {
    
    enum InstructionType {
        INSNONE = -1,
        INSLABEL = -2,
        INSASM = -3,
        MOV = 0,
        ADD,
        SUB,
        MUL,
        IMUL,
        DIV,
        IDIV,
        INC,
        DEC,
        NEG,
        CMP,
        JMP,
        JE,
        JNE,
        JA,
        JNA,
        JAE,
        JNAE,
        JB,
        JNB,
        JBE,
        JNBE,
        JG,
        JGE,
        JNG,
        JNGE,
        JL,
        JLE,
        JNL,
        JNLE,
        LOOP,
        AND,
        OR,
        XOR,
        NOT,
        TEST,
        SHL,
        SHR,
        PUSH,
        POP,
        PUSHA,
        POPA,
        CALL,
        RET,
        LEA,
        NOP,
        FLD,
        FILD,
        FST,
        FSTP,
        FIST,
        FISTP,
        FXCH,
        FFREE,
        FADD,
        FIADD,
        FSUB,
        FSUBR,
        FISUB,
        FISUBR,
        FMUL,
        FIMUL,
        FDIV,
        FDIVR,
        FIDIV,
        FIDIVR,
        FCOM,
        FCOMP,
        FCOMPP,
        FICOM,
        FICOMP,
        FCOMI,
        FCOMIP,
        FTST,
        FINIT,
        FNINIT,
        FSAVE,
        FNSAVE,
        FRSTOR,
        FSTSW,
        FNSTSW,
        SAHF,
        FNOP
    };

    enum InstructionSize {
        INSZNONE = -1,
        BYTE,
        WORD,
        DWORD,
        QWORD
    };

	enum OperandType {
		LITERAL,
		REGISTER,
		FREGISTER,
		MEMORY
	};

	enum MemoryType {
		GLOBAL,
		LOCAL
	};

	struct Operand {
		OperandType type;     //type of Operand
		bool is_array; //if is array, then consider fp_disp with name
		int arr_disp; //array displacement size, 1,2,4 bytes
		std::string literal;  //if type=LITERAL
		RegisterType reg;
		FloatRegisterType freg;
		struct {   // if type=MEMORY
			MemoryType mem_type; //memory type
			int mem_size;  //member size
			std::string name; //if mem_type=GLOBAL, variable name
			int fp_disp;    //if mem_type=LOCAL, frame-pointer displacement(factor)
		} mem;
	};

	struct Instruction {
		InstructionType insn_type;
		std::string label;
		std::string inline_asm;
		int operand_count;
		Operand *operand_1;
		Operand *operand_2;
		std::string comment;  //comment to assembly code
	};

	//types in data section
	enum DeclarationType {
		DSPNONE = -1,
		DB = 0,
		DW,
		DD,
		DQ
	};

	//types in bss section
	enum ReservationType{
		RESPNONE = -1,
		RESB = 0,
		RESW,
		RESD,
		RESQ
	} ;

	//member of data section
	struct Member {
		DeclarationType type;
		bool is_array;
		std::string symbol;
		std::string value;
		std::vector<std::string> array_data;
		std::string comment;
	};

	struct RecordDataType {
		ReservationType resvsp_type;
		std::string symbol;
		bool is_array;
		int resv_size;
	};

	//member of bss section
	struct ReserveSection {
		ReservationType type;
		std::string symbol;
		int res_size;
		std::string comment;
		bool is_record;
		std::string record_name;
		std::vector<RecordDataType> record_members;
	};

	enum TextSectionType {
		TXTNONE,
		TXTGLOBAL,
		TXTEXTERN
	};

	struct TextSection {
		TextSectionType type;
		std::string symbol;
	};

	class InstructionClass {
	public:
		std::string insn_name(InstructionType it) const {
			return insn_names[it];
		};

		std::string insnsize_name(InstructionSize is) const {
			return insnsize_names[is];
		};

		std::string declspace_name(DeclarationType dt) const {
			return declspace_names[dt];
		}

		std::string resspace_name(ReservationType rt) const {
			return resspace_names[rt];
		}

		std::string text_type_name(TextSectionType tt) const {
			return (tt == TXTEXTERN ? "extern" : "global");
		}

		Operand *get_operand_mem();
		Instruction *get_insn_mem();
		Member *get_data_mem();
		ReserveSection *get_resv_mem();
		TextSection *get_text_mem();

		void delete_operand(Operand **);
		void delete_insn(Instruction **);
		void delete_data(Member **);
		void delete_resv(ReserveSection **);
		void delete_text(TextSection **);

	private:
		std::vector<std::string> insn_names = {
				"mov",
				"add",
				"sub",
				"mul",
				"imul",
				"div",
				"idiv",
				"inc",
				"dec",
				"neg",
				"cmp",
				"jmp",
				"je",
				"jne",
				"ja",
				"jna",
				"jae",
				"jnae",
				"jb",
				"jnb",
				"jbe",
				"jnbe",
				"jg",
				"jge",
				"jng",
				"jnge",
				"jl",
				"jle",
				"jnl",
				"jnle",
				"loop",
				"and",
				"or",
				"xor",
				"not",
				"test",
				"shl",
				"shr",
				"push",
				"pop",
				"pusha",
				"popa",
				"call",
				"ret",
				"lea",
				"nop",
				"fld",
				"fild",
				"fst",
				"fstp",
				"fist",
				"fistp",
				"fxch",
				"ffree",
				"fadd",
				"fiadd",
				"fsub",
				"fsubr",
				"fisub",
				"fisubr",
				"fmul",
				"fimul",
				"fdiv",
				"fdivr",
				"fidiv",
				"fidivr",
				"fcom",
				"fcomp",
				"fcompp",
				"ficom",
				"ficomp",
				"fcomi",
				"fcomip",
				"ftst",
				"finit",
				"fninit",
				"fsave",
				"fnsave",
				"frstor",
				"fstsw",
				"fnstsw",
				"sahf",
				"fnop"
		};

		std::vector<std::string> insnsize_names = {
				"byte",
				"word",
				"dword",
				"qword"
		};

		std::vector<std::string> declspace_names = {
				"db",
				"dw",
				"dd",
				"dq"
		};

		std::vector<std::string> resspace_names = {
				"resb",
				"resw",
				"resd",
				"resq"
		};
	};
}


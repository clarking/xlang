/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <string>
#include <vector>
#include <set>

namespace xlang {


    enum RegisterType {
        RNONE = -1, 
        AL = 0, 
        AH, 
        BL, 
        BH, 
        CL, 
        CH, 
        DL, 
        DH, 
        AX, 
        BX, 
        CX, 
        DX, 
        SP, 
        BP, 
        SI, 
        DI, 
        EAX, 
        EBX, 
        ECX, 
        EDX, 
        ESP, 
        EBP, 
        ESI, 
        EDI,
        RAX, 
        RBX, 
        RCX, 
        RDX, 
        RSP, 
        RBP, 
        RSI, 
        RDI,
    };

    enum FloatRegisterType {
        FRNONE = -1,
        ST0 = 0, 
        ST1, 
        ST2, 
        ST3, 
        ST4, 
        ST5, 
        ST6, 
        ST7
    };

	class Registers {
	public:
		RegisterType allocate_register(int);
		
		FloatRegisterType allocate_float_register();
		
		void free_register(RegisterType);
		
		void free_float_register(FloatRegisterType);
		
		void free_all_registers();
		
		void free_all_float_registers();
		
		inline std::string reg_name(RegisterType t) const {
			return reg_names[t];
		}
		
		inline  std::string freg_name(FloatRegisterType t) const {
			return freg_names[t];
		}
		
		int regsize(RegisterType t) const {
			return reg_size[t];
		}
		
	private:

		bool search_register(RegisterType);
		
		bool search_fregister(FloatRegisterType);

		std::set<RegisterType> locked_registers;
		std::set<FloatRegisterType> locked_fregisters;
		std::pair<int, int> size_indexes(int);
		
		std::vector<std::string> reg_names = {
            "al", "ah", "bl", "bh", "cl", "ch", 
            "dl", "dh", "ax", "bx", "cx", "dx", 
            "sp", "bp", "si", "di", 
            "eax", "ebx", "ecx", "edx", "esp", "ebp", "esi", "edi",
            "rax", "rbx", "rcx", "rdx", "rsp", "rbp", "rsi", "rdi"
        };
		
		std::vector<int> reg_size{
            1, 1, 1, 1, 1, 1, 1, 1, 
            2, 2, 2, 2, 2, 2, 2, 2, 
            4, 4, 4, 4, 4, 4, 4, 4,
            8, 8, 8, 8, 8, 8, 8, 8
        };
		
		std::vector<std::string> freg_names = {
            "st0", "st1", "st2", "st3", "st4", "st5", "st6", "st7"
        };
		
	};
}
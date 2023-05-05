/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "insn.hpp"

namespace xlang {
	
	Operand *InstructionClass::get_operand_mem() {
		return new struct Operand;
	}
	
	TextSection *InstructionClass::get_text_mem() {
		return new TextSection();
	}
	
	Instruction *InstructionClass::get_insn_mem() {
		Instruction *innew = new Instruction();
		innew->operand_1 = get_operand_mem();
		innew->operand_2 = get_operand_mem();
		return innew;
	}
	
	Member *InstructionClass::get_data_mem() {
		Member *d = new Member();
		d->is_array = false;
		return d;
	}
	
	ReserveSection *InstructionClass::get_resv_mem() {
		ReserveSection *r = new ReserveSection;
		r->is_record = false;
		return r;
	}
	
	void InstructionClass::delete_operand(Operand **opr) {
		delete *opr;
		*opr = nullptr;
	}
	
	void InstructionClass::delete_insn(Instruction **in) {
		delete *in;
		*in = nullptr;
	}
	
	void InstructionClass::delete_data(Member **d) {
		delete *d;
	}
	
	void InstructionClass::delete_resv(ReserveSection **r) {
		delete *r;
	}
	
	void InstructionClass::delete_text(TextSection **t) {
		delete *t;
	}
}
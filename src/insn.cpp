/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "insn.hpp"

namespace xlang {

	operand *insn_class::get_operand_mem() {
		return new struct operand;
	}

	text *insn_class::get_text_mem() {
		return new struct text;
	}

	insn *insn_class::get_insn_mem() {
		insn *innew = new insn();
		innew->operand_1 = get_operand_mem();
		innew->operand_2 = get_operand_mem();
		return innew;
	}

	data *insn_class::get_data_mem() {
		data *d = new data();
		d->is_array = false;
		return d;
	}

	resv *insn_class::get_resv_mem() {
		resv *r = new resv;
		r->is_record = false;
		return r;
	}

	void insn_class::delete_operand(operand **opr) {
		delete *opr;
		*opr = nullptr;
	}

	void insn_class::delete_insn(insn **in) {
		delete *in;
		*in = nullptr;
	}

	void insn_class::delete_data(data **d) {
		delete *d;
	}

	void insn_class::delete_resv(resv **r) {
		delete *r;
	}

	void insn_class::delete_text(text **t) {
		delete *t;
	}
}
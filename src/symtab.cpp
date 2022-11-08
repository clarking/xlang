/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*
* Contains symbol table related function
* such as memory allocation/deallocation, insert/search/delete etc
*/

#include <list>
#include "symtab.hpp"
#include "murmurhash3.hpp"

using namespace xlang;

namespace xlang {
	//each inserted symbol node and record node can be accessed
	//by following variables to change or add information in the table
	st_record_node *last_rec_node = nullptr;
	st_symbol_info *last_symbol = nullptr;
}

std::ostream &operator<<(std::ostream &ostm, const std::list<std::string> &lst) {
	for (auto x: lst)
		ostm << x << " ";
	return ostm;
}

//memory allocation functions
struct st_type_info *xlang::symtable::get_type_info_mem() {
	st_type_info *newst = new st_type_info();
	return newst;
}

struct st_rec_type_info *xlang::symtable::get_rec_type_info_mem() {
	struct st_rec_type_info *newst = new st_rec_type_info;
	return newst;
}

struct st_symbol_info *xlang::symtable::get_symbol_info_mem() {
	st_symbol_info *newst = new st_symbol_info();
	newst->type_info = nullptr;
	newst->p_next = nullptr;
	newst->is_array = false;
	newst->is_func_ptr = false;
	return newst;
}

struct st_func_param_info *xlang::symtable::get_func_param_info_mem() {
	st_func_param_info *newst = new st_func_param_info();
	newst->symbol_info = get_symbol_info_mem();
	newst->symbol_info->tok.number = NONE;
	newst->type_info = get_type_info_mem();
	return newst;
}

struct st_func_info *xlang::symtable::get_func_info_mem() {
	st_func_info *newst = new st_func_info();
	newst->return_type = nullptr;
	return newst;
}

struct st_node *xlang::symtable::get_node_mem() {
	unsigned i;
	st_node *newst = new st_node();
	newst->func_info = nullptr;
	for (i = 0; i < ST_SIZE; i++)
		newst->symbol_info[i] = nullptr;
	return newst;
}

struct st_record_node *xlang::symtable::get_record_node_mem() {
	st_record_node *newrst = new st_record_node();
	newrst->p_next = nullptr;
	newrst->symtab = get_node_mem();
	return newrst;
}

struct st_record_symtab *xlang::symtable::get_record_symtab_mem() {
	unsigned i;
	st_record_symtab *recsymt = new st_record_symtab();
	for (i = 0; i < ST_RECORD_SIZE; i++)
		recsymt->recordinfo[i] = nullptr;
	return recsymt;
}

//memory deallocation functions
void xlang::symtable::delete_type_info(st_type_info **stinf) {
	if (*stinf == nullptr)
		return;
	//delete *stinf;
	*stinf = nullptr;
}

void xlang::symtable::delete_rec_type_info(st_rec_type_info **stinf) {
	if (*stinf == nullptr)
		return;
	//delete *stinf;
	*stinf = nullptr;
}

void xlang::symtable::delete_symbol_info(st_symbol_info **stinf) {
	if (*stinf == nullptr)
		return;
	st_symbol_info *temp = *stinf;
	std::list<st_rec_type_info *>::iterator it;
	while (temp != nullptr) {
		delete_type_info(&(temp->type_info));
		it = temp->func_ptr_params_list.begin();
		while (it != temp->func_ptr_params_list.end()) {
			delete_rec_type_info(&(*it));
			it++;
		}
		temp->arr_dimension_list.clear();
		temp = temp->p_next;
	}
	*stinf = nullptr;
}

void xlang::symtable::delete_func_param_info(st_func_param_info **stinf) {
	if (*stinf == nullptr)
		return;
	delete_type_info(&((*stinf)->type_info));
	delete_symbol_info(&((*stinf)->symbol_info));
	delete *stinf;
	*stinf = nullptr;
}

void xlang::symtable::delete_func_info(st_func_info **stinf) {
	if (*stinf == nullptr)
		return;
	delete_type_info(&((*stinf)->return_type));
	std::list<st_func_param_info *>::iterator it;
	it = (*stinf)->param_list.begin();
	while (it != (*stinf)->param_list.end()) {
		delete_func_param_info(&(*it));
		it++;
	}
	delete *stinf;
	*stinf = nullptr;
}

void xlang::symtable::delete_node(st_node **stinf) {
	st_node *temp = *stinf;
	unsigned i;
	if (temp == nullptr)
		return;
	delete_func_info(&(temp->func_info));
	for (i = 0; i < ST_SIZE; i++) {
		delete_symbol_info(&(temp->symbol_info[i]));
	}
	delete *stinf;
	*stinf = nullptr;
}

void xlang::symtable::delete_record_node(st_record_node **stinf) {
	st_record_node *temp = *stinf;
	if (*stinf == nullptr)
		return;
	while (temp != nullptr) {
		delete_node(&temp->symtab);
		temp = temp->p_next;
	}
	delete *stinf;
	*stinf = nullptr;
}

void xlang::symtable::delete_record_symtab(st_record_symtab **stinf) {
	st_record_symtab *temp = *stinf;
	if (temp == nullptr)
		return;
	unsigned i;
	for (i = 0; i < ST_RECORD_SIZE; i++) {
		if (temp == nullptr)
			continue;
		delete_record_node(&temp->recordinfo[i]);
	}
}

//hashing functions
unsigned int xlang::symtable::st_hash_code(lexeme_t lxt) {
	void *key = (void *) lxt.c_str();
	unsigned int murhash = MurmurHash3_x86_32(key, lxt.size(), 4);

	return murhash % ST_SIZE;
}

unsigned int xlang::symtable::st_rec_hash_code(lexeme_t lxt) {
	void *key = (void *) lxt.c_str();
	unsigned int murhash = MurmurHash3_x86_32(key, lxt.size(), 4);
	return murhash % ST_RECORD_SIZE;
}

//table operations
void xlang::symtable::add_sym_node(st_symbol_info **symnode) {
	st_symbol_info *temp = *symnode;
	if (temp == nullptr) {
		*symnode = get_symbol_info_mem();
		xlang::last_symbol = *symnode;
	}
	else {
		while (temp->p_next != nullptr) {
			temp = temp->p_next;
		}
		temp->p_next = get_symbol_info_mem();
		xlang::last_symbol = temp->p_next;
	}
}

void xlang::symtable::insert_symbol(st_node **symtab, lexeme_t symbol) {
	st_node *symtemp = *symtab;
	unsigned int hash_code = 0;
	if (symtemp == nullptr)
		return;
	hash_code = st_hash_code(symbol);
	add_sym_node(&(symtemp->symbol_info[hash_code]));
	if (last_symbol == nullptr) {
		std::cout << "error in inserting symbol into symbol table" << std::endl;
	}
}

bool xlang::symtable::search_symbol(st_node *st, lexeme_t symbol) {
	if (st == nullptr)
		return false;
	struct st_symbol_info *temp = nullptr;
	temp = st->symbol_info[st_hash_code(symbol)];
	while (temp != nullptr) {
		if (temp->symbol == symbol)
			return true;
		else
			temp = temp->p_next;
	}
	return false;
}

struct st_symbol_info *xlang::symtable::search_symbol_node(st_node *st, lexeme_t symbol) {
	if (st == nullptr)
		return nullptr;
	st_symbol_info *temp = nullptr;
	temp = st->symbol_info[st_hash_code(symbol)];
	while (temp != nullptr) {
		if (temp->symbol == symbol)
			return temp;
		else
			temp = temp->p_next;
	}
	return nullptr;
}

void xlang::symtable::insert_symbol_node(st_node **symtab, st_symbol_info **syminf) {
	st_symbol_info *temp = nullptr;
	if (*symtab == nullptr)
		return;
	if (*syminf == nullptr)
		return;

	temp = xlang::symtable::search_symbol_node(*symtab, (*syminf)->symbol);
	if (temp != nullptr)
		temp = *syminf;
}

bool xlang::symtable::remove_symbol(st_node **symtab, lexeme_t symbol) {
	st_symbol_info *temp = nullptr;
	st_symbol_info *curr = nullptr;
	if (*symtab == nullptr)
		return false;
	curr = (*symtab)->symbol_info[st_hash_code(symbol)];
	if (curr == nullptr)
		return false;
	if (curr->symbol == symbol) {
		temp = curr->p_next;
		delete_symbol_info(&curr);
		curr = nullptr;
		curr = temp;
	}
	else {
		temp = curr;
		while (curr->p_next != nullptr) {
			if (curr->symbol == symbol) {
				temp->p_next = curr->p_next;
				delete_symbol_info(&curr);
				curr = nullptr;
				return true;
			}
			else {
				temp = curr;
				curr = curr->p_next;
			}
		}
	}
	return false;
}

void xlang::symtable::add_rec_node(st_record_node **recnode) {
	st_record_node *temp = *recnode;
	if (temp == nullptr) {
		*recnode = get_record_node_mem();
		xlang::last_rec_node = *recnode;
	}
	else {
		while (temp->p_next != nullptr)
			temp = temp->p_next;

		temp->p_next = get_record_node_mem();
		xlang::last_rec_node = temp;
	}
}

void xlang::symtable::insert_record(st_record_symtab **recsymtab,
                                    record_t recordname) {
	st_record_symtab *rectemp = *recsymtab;
	if (rectemp == nullptr)
		return;
	add_rec_node(&(rectemp->recordinfo[st_rec_hash_code(recordname)]));
	if (last_rec_node == nullptr) {
		std::cout << "error in inserting record into record table" << std::endl;
	}
}

bool xlang::symtable::search_record(st_record_symtab *rec, record_t recordname) {
	if (rec == nullptr)
		return false;
	st_record_node *temp = nullptr;
	temp = rec->recordinfo[st_rec_hash_code(recordname)];
	while (temp != nullptr) {
		if (temp->recordname == recordname)
			return true;
		else
			temp = temp->p_next;
	}
	return false;
}

struct st_record_node *xlang::symtable::search_record_node(st_record_symtab *rec, record_t recordname) {
	if (rec == nullptr)
		return nullptr;
	st_record_node *temp = nullptr;
	temp = rec->recordinfo[st_rec_hash_code(recordname)];
	while (temp != nullptr) {
		if (temp->recordname == recordname)
			return temp;
		else
			temp = temp->p_next;
	}
	return nullptr;
}


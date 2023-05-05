/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

// Contains symbol table related function

#include <list>
#include "symtab.hpp"
#include "compiler.hpp"
#include "murmurhash3.hpp"

namespace xlang {
	//each inserted symbol node and record node can be accessed
	//by following variables to change or add information in the table
//	RecordNode *last_rec_node = nullptr;
//	SymbolInfo *last_symbol = nullptr;
	
	std::ostream &operator<<(std::ostream &ostm, const std::list<std::string> &lst) {
		for (auto x: lst)
			ostm << x << " ";
		return ostm;
	}
	
	//memory allocation functions
	TypeInfo *SymbolTable::get_type_info_mem() {
		TypeInfo *newst = new TypeInfo();
		return newst;
	}
	
	RecordTypeInfo *SymbolTable::get_rec_type_info_mem() {
		RecordTypeInfo *newst = new RecordTypeInfo;
		return newst;
	}
	
	SymbolInfo *SymbolTable::get_symbol_info_mem() {
		SymbolInfo *newst = new SymbolInfo();
		newst->type_info = nullptr;
		newst->p_next = nullptr;
		newst->is_array = false;
		newst->is_func_ptr = false;
		return newst;
	}
	
	FuncParamInfo *SymbolTable::get_func_param_info_mem() {
		FuncParamInfo *newst = new FuncParamInfo();
		newst->symbol_info = get_symbol_info_mem();
		newst->symbol_info->tok.number = NONE;
		newst->type_info = get_type_info_mem();
		return newst;
	}
	
	FunctionInfo *SymbolTable::get_func_info_mem() {
		FunctionInfo *newst = new FunctionInfo();
		newst->return_type = nullptr;
		return newst;
	}
	
	std::map<std::string, FunctionInfo *> *SymbolTable::get_func_table_mem() {
		std::map<std::string, FunctionInfo *> *newst = new std::map<std::string, FunctionInfo *>();
		return newst;
	}
	
	Node *SymbolTable::get_node_mem() {
		unsigned i;
		Node *newst = new Node();
		newst->func_info = nullptr;
		for (i = 0; i < ST_SIZE; i++)
			newst->symbol_info[i] = nullptr;
		return newst;
	}
	
	RecordNode *SymbolTable::get_record_node_mem() {
		RecordNode *newrst = new RecordNode();
		newrst->p_next = nullptr;
		newrst->symtab = get_node_mem();
		return newrst;
	}
	
	RecordSymtab *SymbolTable::get_record_symtab_mem() {
		unsigned i;
		RecordSymtab *recsymt = new RecordSymtab();
		for (i = 0; i < ST_RECORD_SIZE; i++)
			recsymt->recordinfo[i] = nullptr;
		return recsymt;
	}
	
	//memory deallocation functions
	
	void SymbolTable::delete_type_info(TypeInfo **stinf) {
		if (*stinf == nullptr)
			return;
		//delete *stinf;
		*stinf = nullptr;
	}
	
	void SymbolTable::delete_rec_type_info(RecordTypeInfo **stinf) {
		if (*stinf == nullptr)
			return;
		//delete *stinf;
		*stinf = nullptr;
	}
	
	void SymbolTable::delete_symbol_info(SymbolInfo **stinf) {
		if (*stinf == nullptr)
			return;
		SymbolInfo *temp = *stinf;
		std::list<RecordTypeInfo *>::iterator it;
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
	
	void SymbolTable::delete_func_param_info(FuncParamInfo **stinf) {
		if (*stinf == nullptr)
			return;
		delete_type_info(&((*stinf)->type_info));
		delete_symbol_info(&((*stinf)->symbol_info));
		delete *stinf;
		*stinf = nullptr;
	}
	
	void SymbolTable::delete_func_info(FunctionInfo **stinf) {
		if (*stinf == nullptr)
			return;
		delete_type_info(&((*stinf)->return_type));
		std::list<FuncParamInfo *>::iterator it;
		it = (*stinf)->param_list.begin();
		while (it != (*stinf)->param_list.end()) {
			delete_func_param_info(&(*it));
			it++;
		}
		delete *stinf;
		*stinf = nullptr;
	}
	
	void SymbolTable::delete_node(Node **stinf) {
		Node *temp = *stinf;
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
	
	void SymbolTable::delete_record_node(RecordNode **stinf) {
		RecordNode *temp = *stinf;
		if (*stinf == nullptr)
			return;
		while (temp != nullptr) {
			delete_node(&temp->symtab);
			temp = temp->p_next;
		}
		delete *stinf;
		*stinf = nullptr;
	}
	
	void SymbolTable::delete_record_symtab(RecordSymtab **stinf) {
		RecordSymtab *temp = *stinf;
		if (temp == nullptr)
			return;
		unsigned i;
		for (i = 0; i < ST_RECORD_SIZE; i++) {
			delete_record_node(&temp->recordinfo[i]);
		}
	}
	
	void SymbolTable::delete_func_symtab(std::map<std::string, FunctionInfo *> **stinf) {
		std::map<std::string, FunctionInfo *> *temp = *stinf;
		
		if (temp == nullptr)
			return;
		
		for (const auto &f: *temp) {
			if (f.second->return_type != nullptr)
				delete f.second->return_type;
			
			for (const auto &p: f.second->param_list)
				delete p;
			
			delete f.second;
		}
		
	}
	
	
	//hashing functions
	unsigned int SymbolTable::st_hash_code(std::string lxt) {
		void *key = (void *) lxt.c_str();
		unsigned int murhash = MurmurHash3_x86_32(key, lxt.size(), 4);
		
		return murhash % ST_SIZE;
	}
	
	unsigned int SymbolTable::st_rec_hash_code(std::string lxt) {
		void *key = (void *) lxt.c_str();
		unsigned int murhash = MurmurHash3_x86_32(key, lxt.size(), 4);
		return murhash % ST_RECORD_SIZE;
	}
	
	//table operations
	void SymbolTable::add_sym_node(SymbolInfo **symnode) {
		SymbolInfo *temp = *symnode;
		if (temp == nullptr) {
			*symnode = get_symbol_info_mem();
			Compiler::last_symbol = *symnode;
		}
		else {
			while (temp->p_next != nullptr) {
				temp = temp->p_next;
			}
			temp->p_next = get_symbol_info_mem();
			Compiler::last_symbol = temp->p_next;
		}
	}
	
	void SymbolTable::insert_symbol(Node **symtab, std::string symbol) {
		Node *symtemp = *symtab;
		if (symtemp == nullptr)
			return;
	
		add_sym_node(&(symtemp->symbol_info[st_hash_code(symbol)]));
		if (Compiler::last_symbol == nullptr) {
			std::cout << "error in inserting symbol into symbol table" << std::endl;
		}
	}
	
	bool SymbolTable::search_symbol(Node *st, std::string symbol) {
		if (st == nullptr)
			return false;
		SymbolInfo *temp = nullptr;
		temp = st->symbol_info[st_hash_code(symbol)];
		while (temp != nullptr) {
			if (temp->symbol == symbol)
				return true;
			else
				temp = temp->p_next;
		}
		return false;
	}
	
	SymbolInfo *SymbolTable::search_symbol_node(Node *st, std::string symbol) {
		if (st == nullptr)
			return nullptr;
		SymbolInfo *temp = nullptr;
		temp = st->symbol_info[st_hash_code(symbol)];
		while (temp != nullptr) {
			if (temp->symbol == symbol)
				return temp;
			else
				temp = temp->p_next;
		}
		return nullptr;
	}
	
	void SymbolTable::insert_symbol_node(Node **symtab, SymbolInfo **syminf) {

		if (*symtab == nullptr || *syminf == nullptr)
			return;
		
		SymbolInfo *temp = SymbolTable::search_symbol_node(*symtab, (*syminf)->symbol);
		if (temp != nullptr)
			temp = *syminf;
	}
	
	bool SymbolTable::remove_symbol(Node **symtab, std::string symbol) {
		SymbolInfo *temp = nullptr;
		SymbolInfo *curr = nullptr;
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
	
	void SymbolTable::add_rec_node(RecordNode **recnode) {
		RecordNode *temp = *recnode;
		if (temp == nullptr) {
			*recnode = get_record_node_mem();
			Compiler::last_rec_node = *recnode;
		}
		else {
			while (temp->p_next != nullptr)
				temp = temp->p_next;
			
			temp->p_next = get_record_node_mem();
			Compiler::last_rec_node = temp;
		}
	}
	
	void SymbolTable::insert_record(RecordSymtab **recsymtab, std::string recordname) {
		RecordSymtab *rectemp = *recsymtab;
		if (rectemp == nullptr)
			return;
		add_rec_node(&(rectemp->recordinfo[st_rec_hash_code(recordname)]));
		if (Compiler::last_rec_node == nullptr) {
			std::cout << "error in inserting record into record table" << std::endl;
		}
	}
	
	bool SymbolTable::search_record(RecordSymtab *rec, std::string recordname) {
		if (rec == nullptr)
			return false;
		RecordNode *temp = nullptr;
		temp = rec->recordinfo[st_rec_hash_code(recordname)];
		while (temp != nullptr) {
			if (temp->recordname == recordname)
				return true;
			else
				temp = temp->p_next;
		}
		return false;
	}
	
	RecordNode *SymbolTable::search_record_node(RecordSymtab *rec, std::string recordname) {
		if (rec == nullptr)
			return nullptr;
		RecordNode *temp = nullptr;
		temp = rec->recordinfo[st_rec_hash_code(recordname)];
		while (temp != nullptr) {
			if (temp->recordname == recordname)
				return temp;
			else
				temp = temp->p_next;
		}
		return nullptr;
	}
	
}
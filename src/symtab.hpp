/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <map>
#include "token.hpp"

//symbol table and record table size

#define ST_SIZE 31
#define ST_RECORD_SIZE 31

namespace xlang {
	
	enum class NodeType {
		SIMPLE = 1,  // for primitive types
		RECORD,      // for non-primitive types
		FUNC_PTR,
		FUNC_NODE,
		BLOCK_NODE
	};
	
	struct TypeSpecifier {
		std::vector<Token> simple_type;
		Token record_type;
	};
	
	struct TypeInfo {
		NodeType type;
		TypeSpecifier type_specifier;
		bool is_const;
		bool is_global;
		bool is_extern;
		bool is_static;
	};
	
	struct RecordTypeInfo {
		NodeType type;
		TypeSpecifier type_specifier;
		bool is_const;
		bool is_ptr; 
		int ptr_oprtr_count;
	};
	
	struct SymbolInfo {
		std::string symbol;  // symbol name
		Token tok;           // original Token
		TypeInfo *type_info; // type specifier of symbol
		bool is_ptr;         // is symbol a pointer, means declared with *
		int ptr_oprtr_count; // * count
		bool is_array;       // is symbol an array
		std::list<Token> arr_dimension_list;  //list of array dimensions
		std::vector<std::vector<Token>> arr_init_list;
		bool is_func_ptr;    // is symbol a function pointer
		int ret_ptr_count;   // return type pointer count of function pointer of symbol
		std::list<RecordTypeInfo *> func_ptr_params_list;  //list of function pointer parameters
		SymbolInfo *p_next;  // next symbol info in queue
	};
	
	struct FuncParamInfo {
		TypeInfo *type_info;     //function parameter type info
		SymbolInfo *symbol_info; // function parameter symbol info
	};
	
	struct FunctionInfo {
		std::string func_name;  // function name
		Token tok;              // original name Token
		bool is_global;         // is function defined global
		bool is_extern;         // is function defined extern
		int ptr_oprtr_count;    // if return type of function is pointer, then set pointer operator count
		TypeInfo *return_type;  // function return type info
		std::list<FuncParamInfo *> param_list; //list of function parameters
	};
	
	struct Node {
		int node_type;           // table type, which is not considered yet
		FunctionInfo *func_info; // function info in which function does this table belong
		SymbolInfo *symbol_info[ST_SIZE];  //buckets of symbol info
		void print();
	};
	
	struct RecordNode {
		std::string recordname; //record name
		Token recordtok;        //original Token
		bool is_global;
		bool is_extern;
		Node *symtab;       //synbol table of record members
		RecordNode *p_next; //next record definition
	};
	
	struct RecordSymtab { 
		RecordNode *recordinfo[ST_RECORD_SIZE]; //buckets of record info
		void print();
	};
	
	class SymbolTable {
	public:
		
		friend std::ostream &operator<<(std::ostream &, const std::list<std::string> &);
		
		static TypeInfo *get_type_info_mem();
		
		static RecordTypeInfo *get_rec_type_info_mem();
		
		static std::map<std::string, FunctionInfo *> *get_func_table_mem();
		
		static SymbolInfo *get_symbol_info_mem();
		
		static FuncParamInfo *get_func_param_info_mem();
		
		static FunctionInfo *get_func_info_mem();
		
		static Node *get_node_mem();
		
		static RecordNode *get_record_node_mem();
		
		static RecordSymtab *get_record_symtab_mem();
		
		static void delete_type_info(TypeInfo **);
		
		static void delete_rec_type_info(RecordTypeInfo **);
		
		static void delete_symbol_info(SymbolInfo **);
		
		static void delete_func_param_info(FuncParamInfo **);
		
		static void delete_func_info(FunctionInfo **);
		
		static void delete_node(Node **);
		
		static void delete_record_node(RecordNode **);
		
		static void delete_record_symtab(RecordSymtab **);
		
		static void delete_func_symtab(std::map<std::string, FunctionInfo *> **stinf);

		static void insert_symbol(Node **, std::string);
		
		static bool search_symbol(Node *, std::string);
		
		static SymbolInfo *search_symbol_node(Node *, std::string);
		
		static void insert_symbol_node(Node **, SymbolInfo **);
		
		static bool remove_symbol(Node **, std::string);
		
		static void insert_record(RecordSymtab **, std::string);
		
		static bool search_record(RecordSymtab *, std::string);
		
		static RecordNode *search_record_node(RecordSymtab *, std::string);
		
    private:
		static unsigned int st_hash_code(std::string);
		
		static unsigned int st_rec_hash_code(std::string);
		
		static void add_sym_node(SymbolInfo **);
		
		static void add_rec_node(RecordNode **);
	};
}

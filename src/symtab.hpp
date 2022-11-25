/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*
* Contains data/operations, data structures for symbol table entries
* used in symtab.cpp file by class symtab.
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
	
	//types for type specifier
	enum {
		SIMPLE_TYPE = 1,  // for primitive types
		RECORD_TYPE,      // for non-primitive types
		FUNC_PTR_TYPE,
		
		FUNC_NODE_TYPE = 0x0A,  //type of symbol table node
		BLOCK_NODE_TYPE
		
	};
	
	struct st_type_specifier {
		std::vector<token> simple_type;
		token record_type;
	};
	
	//type specifier info
	struct st_type_info {
		// type of type specifier(simple,record)
		int type;
		
		st_type_specifier type_specifier;
		
		bool is_const;
		bool is_global;
		bool is_extern;
		bool is_static;
	};
	
	//symbol table for type specifier in record definition
	struct st_rec_type_info {
		int type;
		st_type_specifier type_specifier;
		bool is_const;
		bool is_ptr; //if type specifier is of pointer type set is_ptr to true and pointer operator count
		int ptr_oprtr_count;
	};
	
	//hash queue of symbol info
	struct st_symbol_info {
		std::string symbol;            // symbol name
		token tok;                  // original token
		st_type_info *type_info;    // type specifier of symbol
		bool is_ptr;                // is symbol a pointer, means declared with *
		int ptr_oprtr_count;        // * count
		bool is_array;              // is symbol an array
		std::list<token> arr_dimension_list;  //list of array dimensions
		std::vector<std::vector<token>> arr_init_list;
		bool is_func_ptr;           // is symbol a function pointer
		int ret_ptr_count;          // return type pointer count of function pointer of symbol
		std::list<st_rec_type_info *> func_ptr_params_list;  //list of function pointer parameters
		st_symbol_info *p_next;     // next symbol info in queue
	};
	
	//function parameter info
	struct st_func_param_info {
		st_type_info *type_info;     //function parameter type info
		st_symbol_info *symbol_info; // function parameter symbol info
	};
	
	//function info
	struct st_func_info {
		std::string func_name;     // function name
		token tok;              // original name token
		bool is_global;         // is function defined global
		bool is_extern;         // is function defined extern
		int ptr_oprtr_count;    // if return type of function is pointer, then set pointer operator count
		st_type_info *return_type; // function return type info
		std::list<st_func_param_info *> param_list; //list of function parameters
	};
	
	//symbol table ( only one symbol table per function allowed)
	struct st_node {
		int node_type;           // table type, which is not considered yet
		st_func_info *func_info; // function info in which function does this table belong
		st_symbol_info *symbol_info[ST_SIZE];  //buckets of symbol info
		void print();
	};
	
	//record definition node
	struct st_record_node {
		std::string recordname;    //record name
		token recordtok;        //original token
		bool is_global;
		bool is_extern;
		st_node *symtab;        //synbol table of record members
		st_record_node *p_next; //next record definition
	};
	
	//record hash table where each record is identified by recordname
	
	struct st_record_symtab {
		st_record_node *recordinfo[ST_RECORD_SIZE]; //buckets of record info
		
		void print();
	};
	
	//symbol table class
	class symtable {
		public :
		
		friend std::ostream &operator<<(std::ostream &, const std::list<std::string> &);
		
		//following functions returns memory block from heap
		
		static st_type_info *get_type_info_mem();
		
		static st_rec_type_info *get_rec_type_info_mem();
		
		static std::map<std::string, st_func_info *> *get_func_table_mem();
		
		static st_symbol_info *get_symbol_info_mem();
		
		static st_func_param_info *get_func_param_info_mem();
		
		static st_func_info *get_func_info_mem();
		
		static st_node *get_node_mem();
		
		static st_record_node *get_record_node_mem();
		
		static st_record_symtab *get_record_symtab_mem();
		
		//following function releases memory block from heap
		
		static void delete_type_info(st_type_info **);
		
		static void delete_rec_type_info(st_rec_type_info **);
		
		static void delete_symbol_info(st_symbol_info **);
		
		static void delete_func_param_info(st_func_param_info **);
		
		static void delete_func_info(st_func_info **);
		
		static void delete_node(st_node **);
		
		static void delete_record_node(st_record_node **);
		
		static void delete_record_symtab(st_record_symtab **);
		
		static void delete_func_symtab(std::map<std::string, st_func_info *> **stinf);
		//symbol table operations,insert,search,delete
		
		static void insert_symbol(st_node **, std::string);
		
		static bool search_symbol(st_node *, std::string);
		
		static st_symbol_info *search_symbol_node(st_node *, std::string);
		
		static void insert_symbol_node(st_node **, st_symbol_info **);
		
		static bool remove_symbol(st_node **, std::string);
		
		//record table operations,insert,search,delete
		
		static void insert_record(st_record_symtab **, std::string);
		
		static bool search_record(st_record_symtab *, std::string);
		
		static st_record_node *search_record_node(st_record_symtab *, std::string);
		
		private :
		// a murmurhash3 hash function is used for hashing defined in murmurhash2.cpp file
		static unsigned int st_hash_code(std::string);
		
		static unsigned int st_rec_hash_code(std::string);
		
		static void add_sym_node(st_symbol_info **);
		
		static void add_rec_node(st_record_node **);
	};
	
	extern st_record_node *last_rec_node;
	extern st_symbol_info *last_symbol;
}

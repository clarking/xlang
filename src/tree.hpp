/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*
* Contains data/operations, data structures for expressions & statements
* used in tree.cpp file by class tree.
*/

#ifndef TREE_H
#define TREE_H

#include <stack>
#include "types.hpp"
#include "symtab.hpp"

namespace xlang {

	struct expression;

	//expression type
	//primary, assignment, sizeof, casting, identifier, function call
	typedef enum {
		PRIMARY_EXPR,
		ASSGN_EXPR,
		SIZEOF_EXPR,
		CAST_EXPR,
		ID_EXPR,
		FUNC_CALL_EXPR,
	} expr_t;

	//operator types
	typedef enum {
		UNARY_OP,
		BINARY_OP
	} oprtr_t;

	//primary expression
	struct primary_expr {
		token tok;    //expression token(could be literal, operator, identifier)
		bool is_oprtr;      //is operator
		oprtr_t oprtr_kind; //operator type
		bool is_id;   //is identifier
		st_symbol_info *id_info; //if id, then pointer in symbol table
		//left & right nodes of parse tree
		//if operator is binary
		primary_expr *left;
		primary_expr *right;
		//unary node of parse tree
		//if operator is unary
		primary_expr *unary_node;
		void print();
	};

	//identifier expression
	struct id_expr {
		token tok;
		bool is_oprtr;
		bool is_id;
		st_symbol_info *id_info;
		bool is_subscript;    //is array
		std::list<token> subscript; //list of array subscripts(could be literals or identifiers)
		bool is_ptr;      //is pointer operator defined
		int ptr_oprtr_count;  //pointer operator count
		//left and right sides of parse tree if not then unary
		id_expr *left;
		id_expr *right;
		id_expr *unary;
		void print();
	};

	//sizeof expression
	struct sizeof_expr {
		bool is_simple_type;  //if simple type, then set simple_type otherwise identifier
		std::vector<token> simple_type; //pimitive type
		token identifier;   //non-primitive type(record-type)
		bool is_ptr;
		int ptr_oprtr_count;
		void print();
	};

	//cast expression
	//same as sizeof expression
	struct cast_expr {
		bool is_simple_type;
		std::vector<token> simple_type;
		token identifier;
		int ptr_oprtr_count;    //if pointer operator is defined then its count
		id_expr *target; //identifier expression that need to be cast
		void print();
	};

	struct expr;

	//assignment expression
	struct assgn_expr {
		token tok;    //assignment operator token
		id_expr *id_expression;  //left side
		expr *expression;  //right side
		void print();
	};

	//function call expresison
	struct func_call_expr {
		id_expr *function;                  //function name(could be simple or from record member(e.g x.y->func()))
		std::list<expr *> expression_list;  //fuction expression list
		void print();
	};

	/*
	expression :
	  primary-expression
	  assignment-expression
	  sizeof-expression
	  cast-expression
	  id-expression
	  function-call-expression
	*/
	struct expr {
		expr_t expr_kind;   //expression type
		primary_expr *primary_expression;
		assgn_expr *assgn_expression;
		sizeof_expr *sizeof_expression;
		cast_expr *cast_expression;
		id_expr *id_expression;
		func_call_expr *func_call_expression;
		void print();
	};

	//iteration statement types
	typedef enum {
		WHILE_STMT,
		DOWHILE_STMT,
		FOR_STMT
	} iter_stmt_t;

	struct labled_stmt {
		token label;  //label token
		void print();
	};

	struct expr_stmt {
		expr *expression;
		void print();
	};

	struct stmt;

	//where statement-list is the doubly linked list of statements
	/*
	selection-statement :
	  if ( condition ) { statement-list }
	  if ( condition ) { statement-list } else { statement-list }
	*/
	struct select_stmt {
		token iftok;    //if keyword token
		token elsetok;  //else keyword token
		expr *condition; //if condition
		stmt *if_statement;  //if statement
		stmt *else_statement;  //else statement
		void print();
	};

	/*
	iteration-statement :
	  while ( condition ) { statement-list }
	  do { statement-list } while ( condition ) ;
	  for ( init-expression ; condition ; update-expression ) { statement-list }
	*/
	struct iter_stmt {
		iter_stmt_t type; //type of iteration(while,do-while,for)

		struct {
			token whiletok;
			expr *condition;
			stmt *statement;
		} _while;

		struct {
			token dotok;
			token whiletok;
			expr *condition;
			stmt *statement;
		} _dowhile;

		struct {
			token fortok;
			expr *init_expression;
			expr *condition;
			expr *update_expression;
			stmt *statement;
		} _for;
		void print();
	};

//jump statement types
	typedef enum {
		BREAK_JMP,
		CONTINUE_JMP,
		RETURN_JMP,
		GOTO_JMP
	} jmp_stmt_t;

/*
jump-statement :
  break ;
  continue ;
  return expression
  goto identifier
*/
	struct jump_stmt {
		jmp_stmt_t type;    //type
		token tok;    //token for break,continue,return and goto
		expr *expression;  //expression for return
		token goto_id;  //token for goto
		void print();
	};

//statement types
	typedef enum {
		LABEL_STMT,
		EXPR_STMT,
		SELECT_STMT,
		ITER_STMT,
		JUMP_STMT,
		DECL_STMT,
		ASM_STMT
	} stmt_t;

//assembly operand
	struct st_asm_operand {
		token constraint;
		expr *expression;
		void print();
	};

//assembly statement
	struct asm_stmt {
		token asm_template;
		std::vector<st_asm_operand *> output_operand;
		std::vector<st_asm_operand *> input_operand;
		asm_stmt *p_next;
		void print();
	};


	/*
	statement :
	  labled-statement
	  expression-statement
	  selection-statement
	  iteration-statement
	  jump-statement
	  simple-declaration
	*/
	struct stmt {
		stmt_t type;
		labled_stmt *labled_statement;
		expr_stmt *expression_statement;
		select_stmt *selection_statement;
		iter_stmt *iteration_statement;
		jump_stmt *jump_statement;
		asm_stmt *asm_statement;

		stmt *p_next;
		stmt *p_prev;
		void print();
	};

	//tree node of parse tree
	//which is actually a doubly linked list
	struct tree_node {
		st_node *symtab; //symbol table per function
		stmt *statement; //statement-list in that function
		tree_node *p_next;
		tree_node *p_prev;

		void print() {
		}
	};

	class tree {

	public :

		//memory allocation/deallocation functions for each tree node
		static sizeof_expr *get_sizeof_expr_mem();
		static void delete_sizeof_expr(sizeof_expr **);
		static cast_expr *get_cast_expr_mem();
		static void delete_cast_expr(cast_expr **);
		static primary_expr *get_primary_expr_mem();
		static void delete_primary_expr(primary_expr **);
		static id_expr *get_id_expr_mem();
		static void delete_id_expr(id_expr **);
		static expr *get_expr_mem();
		static void delete_expr(expr **);
		static assgn_expr *get_assgn_expr_mem();
		static void delete_assgn_expr(assgn_expr **);
		static func_call_expr *get_func_call_expr_mem();
		static void delete_func_call_expr(func_call_expr **);
		static st_asm_operand *get_asm_operand_mem();
		static void delete_asm_operand(st_asm_operand **);

		static labled_stmt *get_label_stmt_mem();
		static expr_stmt *get_expr_stmt_mem();
		static select_stmt *get_select_stmt_mem();
		static iter_stmt *get_iter_stmt_mem();
		static jump_stmt *get_jump_stmt_mem();
		static asm_stmt *get_asm_stmt_mem();
		static stmt *get_stmt_mem();
		static tree_node *get_tree_node_mem();

		//memory deallocation functions
		static void delete_label_stmt(labled_stmt **);
		static void delete_expr_stmt(expr_stmt **);
		static void delete_select_stmt(select_stmt **);
		static void delete_iter_stmt(iter_stmt **);
		static void delete_jump_stmt(jump_stmt **);
		static void delete_asm_stmt(asm_stmt **);
		static void delete_stmt(stmt **);
		static void delete_tree(tree_node **);
		static void delete_tree_node(tree_node **);

		static void get_inorder_primary_expr(primary_expr **);

		//tree node adding functions such as statement or treenode
		static void add_asm_statement(asm_stmt **, asm_stmt **);
		static void add_statement(stmt **, stmt **);
		static void add_tree_node(tree_node **, tree_node **);

	private:
		static std::stack<primary_expr *> pexpr_stack;
	};
}

#endif





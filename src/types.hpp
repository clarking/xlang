
#pragma once

#include <string>
#include <vector>
#include <map>
#include <list>
#include "token.hpp"

namespace xlang {
	
	//symbol table and record table size
    #define ST_SIZE 31
    #define ST_RECORD_SIZE 31

	
	enum class ExpressionType {
		PRIMARY_EXPR, 
        ASSGN_EXPR, 
        SIZEOF_EXPR, 
        CAST_EXPR, 
        ID_EXPR, 
        FUNC_CALL_EXPR,
	};
	
	//operator types
	enum class OperatorType {
		UNARY,
        BINARY
	};
	
	//primary expression
	struct PrimaryExpression {
		Token tok;    //expression Token(could be literal, operator, identifier)
		bool is_oprtr;      //is operator
		OperatorType oprtr_kind; //operator type
		bool is_id;   //is identifier
		SymbolInfo *id_info; //if id, then pointer in symbol table
		//left & right nodes of parse tree
		//if operator is binary
		PrimaryExpression *left;
		PrimaryExpression *right;
		//unary node of parse tree
		//if operator is unary
		PrimaryExpression *unary_node;
		
		void print();
	};
	
	struct IdentifierExpression {
		Token tok;
		bool is_oprtr;
		bool is_id;
		SymbolInfo *id_info;
		bool is_subscript;    //is array
		std::list<Token> subscript; //list of array subscripts(could be literals or identifiers)
		bool is_ptr;         //is pointer operator defined
		int ptr_oprtr_count;  //pointer operator count

		//left and right sides of parse tree if not then unary
		IdentifierExpression *left;
		IdentifierExpression *right;
		IdentifierExpression *unary;
		
		void print();
	};
	
	struct SizeOfExpression {
		bool is_simple_type;  //if simple type, then set simple_type otherwise identifier
		std::vector<Token> simple_type; //pimitive type
		Token identifier;   //non-primitive type(record-type)
		bool is_ptr;
		int ptr_oprtr_count;
		
		void print();
	};
	
	struct CastExpression {
		bool is_simple_type;
		std::vector<Token> simple_type;
		Token identifier;
		int ptr_oprtr_count;    //if pointer operator is defined then its count
		IdentifierExpression *target; //identifier expression that need to be cast
		void print();
	};
	
	struct Expression;
	
	struct AssignmentExpression {
		Token tok;    //assignment operator Token
		IdentifierExpression *id_expr;  //left side
		Expression *expression;  //right side
		void print();
	};
	
	struct CallExpression {
		IdentifierExpression *function;     //function name(could be simple or from record member(e.g x.y->func()))
		std::list<Expression *> expression_list;  //fuction expression list
		void print();
	};
	
	struct Expression {
		ExpressionType expr_kind;   //expression type
		PrimaryExpression *primary_expr;
		AssignmentExpression *assgn_expr;
		SizeOfExpression *sizeof_expr;
		CastExpression *cast_expr;
		IdentifierExpression *id_expr;
		CallExpression *call_expr;
		
		void print();
	};
	
	enum class IterationType {
		WHILE,
        DOWHILE,
        FOR
	};
	
	struct LabelStatement {
		Token label;  
		void print();
	};
	
	struct ExpressionStatement {
		Expression *expression;
		void print();
	};
	
	struct Statement;
	
	//where statement-list is the doubly linked list of statements
	/*
	selection-statement :
	  if ( condition ) { statement-list }
	  if ( condition ) { statement-list } else { statement-list }
	*/
	struct SelectStatement {
		Token iftok;    //if keyword Token
		Token elsetok;  //else keyword Token
		Expression *condition; //if condition
		Statement *if_statement;  //if statement
		Statement *else_statement;  //else statement
		void print();
	};
	
	/*
	iteration-statement :
	  while ( condition ) { statement-list }
	  do { statement-list } while ( condition ) ;
	  for ( init-expression ; condition ; update-expression ) { statement-list }
	*/
	struct IterationStatement {
		IterationType type; //type of iteration(while,do-while,for)
		
		struct {
			Token whiletok;
			Expression *condition;
			Statement *statement;
		} _while;
		
		struct {
			Token dotok;
			Token whiletok;
			Expression *condition;
			Statement *statement;
		} _dowhile;
		
		struct {
			Token fortok;
			Expression *init_expr;
			Expression *condition;
			Expression *update_expr;
			Statement *statement;
		} _for;
		
		void print();
	};
	
	//jump statement types
	enum class JumpType {
		BREAK,
        CONTINUE,
        RETURN,
        GOTO
    };
	
	/*
	jump-statement :
	  break ;
	  continue ;
	  return expression
	  goto identifier
	*/
	struct JumpStatement {
		JumpType type;    //type
		Token tok;          //Token for break,continue,return and goto
		Expression *expression;   //expression for return
		Token goto_id;      //Token for goto
		void print();
	};
	
	//statement types
	enum class StatementType {
		LABEL,
        EXPR,
        SELECT,
        ITER,
        JUMP,
        DECL,
        ASM
	};
	
	struct AsmOperand {
		Token constraint;
		Expression *expression;
		void print();
	};
	
	//assembly statement
	struct AsmStatement {
		Token asm_template;
		std::vector<AsmOperand *> output_operand;
		std::vector<AsmOperand *> input_operand;
		AsmStatement *p_next;
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
	struct Statement {
		StatementType type;
		LabelStatement *labled_statement;
		ExpressionStatement *expression_statement;
		SelectStatement *selection_statement;
		IterationStatement *iteration_statement;
		JumpStatement *jump_statement;
		AsmStatement *asm_statement;
		
		Statement *p_next;
		Statement *p_prev;
		
		void print();
	};
	
	//tree node of parse tree (actually a doubly linked list)
	struct TreeNode {
		Node *symtab;          //symbol table per function
		Statement *statement;  //statement-list in that function
		TreeNode *p_next;
		TreeNode *p_prev;
		
		void print() {
		}
	};

    typedef std::map<std::string, FunctionInfo *> FunctionMap;
}


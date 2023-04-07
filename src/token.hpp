/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


#pragma once

#include <string>

namespace xlang {
	
	/*
	keyword : one of
	  asm
	  break
	  char
	  const
	  continue
	  do
	  double
	  else
	  extern
	  float
	  for
	  global
	  goto
	  if
	  int
	  long
	  record
	  return
	  short
	  sizeof
	  static
	  void
	  while
	
	symbol : one of
	  ! % ^ ~ & * ( ) - + = [ ] { } | : ; < > , . / \ ' " $
	*/
	
	
#define	NONE         0
#define	END          1
#define	IDENTIFIER   2

//keywords
#define	KEY_ASM         3
#define	KEY_BREAK       4
#define	KEY_CHAR        5
#define	KEY_CONST       6
#define	KEY_CONTINUE    7
#define	KEY_DO          8
#define	KEY_DOUBLE      9
#define	KEY_ELSE        10
#define	KEY_EXTERN      11
#define	KEY_FLOAT       12
#define	KEY_FOR         13
#define	KEY_GLOBAL      14
#define	KEY_GOTO        15
#define	KEY_IF          16
#define	KEY_INT         17
#define	KEY_LONG        18
#define	KEY_RECORD      19
#define	KEY_RETURN      20
#define	KEY_SHORT       21
#define	KEY_SIZEOF      22
#define	KEY_STATIC      23
#define	KEY_VOID        24
#define	KEY_WHILE       25

//literals
#define	LIT_DECIMAL     26
#define	LIT_OCTAL       27
#define	LIT_HEX         28
#define	LIT_BIN         29
#define	LIT_FLOAT       30
#define	LIT_CHAR        31
#define	LIT_STRING      32

//assignment operators
#define	ASSGN           33
#define	ASSGN_ADD       34
#define	ASSGN_SUB       35
#define	ASSGN_MUL       36
#define	ASSGN_DIV       37
#define	ASSGN_MOD       38
#define	ASSGN_BIT_OR    39
#define	ASSGN_BIT_AND   40
#define	ASSGN_BIT_EX_OR 41
#define	ASSGN_LSHIFT    42
#define	ASSGN_RSHIFT    43

//arithmetic operators
#define	ARTHM_ADD       44
#define	ARTHM_SUB       45
#define	ARTHM_MUL       46
#define	ARTHM_DIV       47
#define	ARTHM_MOD       48

//comparison operators
#define	COMP_LESS       49
#define	COMP_LESS_EQ    50
#define	COMP_GREAT      51
#define	COMP_GREAT_EQ   52
#define	COMP_EQ         53
#define	COMP_NOT_EQ     54

//logical opertors
#define	LOG_AND         55
#define	LOG_OR          56
#define	LOG_NOT         57

//bitwise operators
#define	BIT_AND         58
#define	BIT_OR          59
#define	BIT_EXOR        60
#define	BIT_COMPL       61
#define	BIT_LSHIFT      62
#define	BIT_RSHIFT      63

//pointer operator
#define	PTR_OP          64

//address of operator
#define	ADDROF_OP       65

//arrow operator ->
#define	ARROW_OP        66

//increment/decrement operators
#define	INCR_OP         67
#define	DECR_OP         68

//miscellaneous operators
#define	DOT_OP          69
#define	COMMA_OP        70
#define	COLON_OP        71
#define	CURLY_OPEN      72
#define	CURLY_CLOSE     73
#define	PARENTH_OPEN    74
#define	PARENTH_CLOSE   75
#define	SQUARE_OPEN     76
#define	SQUARE_CLOSE    77
#define	SEMICOLON       78
	

    typedef uint8_t TokenId;
	
	struct TokenLocation {
		int line{0};
		int col{0};
	};
	
	struct Token {
		TokenId number;   //Token number
		TokenLocation loc;        // location of Token/lexeme
		std::string string;  //original string
	};
	
}
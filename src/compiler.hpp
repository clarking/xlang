/*
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


#pragma once

#include "global.hpp"
#include "lex.hpp"
#include "parser.hpp"
#include "analyze.hpp"
#include "gen.hpp"
#include "types.hpp"

#include <string>

namespace xlang {

	// this class tries to hold everything to keep a somewhat organized mess
	
	class Compiler {
	public:
		
		~Compiler() = default;
		
		static GlobalConfig global;
		
		static Lexer *lex;
		static Parser *parser;
		static Analyzer *an;
		static CodeGen *generator;
		static TreeNode *ast;
		static Node *symtab;
		static RecordSymtab *record_table;
		static FunctionMap *func_table;
        static RecordNode *last_rec_node;
	    static SymbolInfo *last_symbol;

		static int run();
		
		static bool assemble();
		
		static bool link();
		
		static bool compile();
		
		static bool error_count();

		static bool execute(std::string cmd);
		
	};
}

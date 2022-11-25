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

namespace xlang {

	// this class tries to hold everything to keep a somewhat organized mess
	
	typedef std::map<std::string, st_func_info *> function_map;
	
	class compiler {
		public:
		
		~compiler()=default;
		
		static global_cfg global;
		
		static xlang::lexer *lex;
		static xlang::parser *parser;
		static analyzer *an;
		static gen *generator;
		static tree_node *ast;
		static st_node *symtab;
		static st_record_symtab *record_table;
		static function_map *func_table;
		
		static bool run();
		
		static void assemble();
		
		static void link();
		
		static bool compile();
		
		static bool error_count();
	};
}
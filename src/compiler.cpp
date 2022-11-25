/*
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


#include "compiler.hpp"
#include <sys/wait.h>
#include <unistd.h>

namespace xlang {
	
	lexer *compiler::lex = nullptr;
	parser *compiler::parser = nullptr;
	analyzer *compiler::an = nullptr;
	gen *compiler::generator = nullptr;
	tree_node *compiler::ast = nullptr;
	st_node *compiler::symtab = nullptr;
	st_record_symtab *compiler::record_table = nullptr;
	function_map *compiler::func_table = nullptr;
	
	bool compiler::run() {
		
		if (global.compile)
			compile();
		
		if (global.assemble)
			assemble();
		
		link();
		
		if(global.remove_asmfile)
			remove(global.file.asm_name().c_str());
		
		if(global.remove_objfile);
		remove(global.file.object_name().c_str());
		
		xlang::tree::delete_tree(&ast);
		xlang::symtable::delete_node(&symtab);
		xlang::symtable::delete_record_symtab(&record_table);
		return true;
	}
	
	void compiler::assemble() {
		
		std::string assembler = "/usr/bin/nasm";
		std::string options = "-felf32";
		int status;
		char *ps_argv[3];
		ps_argv[0] = const_cast<char *>("nasm");
		ps_argv[1] = const_cast<char *>(options.c_str());
		ps_argv[2] = const_cast<char *>(global.file.asm_name().c_str());
		ps_argv[3] = 0;
		
		pid_t pid = fork();
		
		switch (pid) {
			case -1:
				std::cout << "fork() failed\n";
				return;
			case 0:
				execvp(assembler.c_str(), ps_argv);
				return;
			default:
				waitpid(pid, &status, 0);
				
				// TODO: FIX RENAME FILE
				//			if (status == 0) {
				//				rename(global.file.object_name().c_str(), get_object_filename(filename).c_str());
				//			}
		}
	}
	
	//	link the compiled and assembled object file with GCC.
	//	To link with LD, you need to insert some program starting
	//	instructions in x86 generation phase with _start() function
	
	void compiler::link() {
		
		std::string link = "/usr/bin/gcc";
		std::string option1 = "-m32";
		std::string option2 = "-nostdlib";
		std::string option3 = "-no-pie";
		std::string option4 = "-o";
		
		std::string outputfile = global.file.name;
		int status;
		char *ps_argv[7];
		
		size_t fnd = global.file.object_name().find_last_of('/');
		if (fnd != std::string::npos) {
			outputfile = global.file.object_name().substr(0, fnd) + "/" + outputfile;
		}
		
		ps_argv[0] = const_cast<char *>("gcc");
		ps_argv[1] = const_cast<char *>(option1.c_str());
		ps_argv[2] = const_cast<char *>(global.file.object_name().c_str());
		ps_argv[3] = const_cast<char *>(option3.c_str());
		ps_argv[4] = const_cast<char *>(option4.c_str());
		ps_argv[5] = const_cast<char *>(outputfile.c_str());
		ps_argv[6] = 0;
		
		if (!global.use_cstdlib) {
			ps_argv[1] = const_cast<char *>(option1.c_str());
			ps_argv[2] = const_cast<char *>(option2.c_str());
			ps_argv[3] = const_cast<char *>(global.file.object_name().c_str());
			ps_argv[4] = 0;
		}
		
		pid_t pid = fork();
		
		switch (pid) {
			case -1:
				std::cout << "fork() failed\n";
				return;
			case 0:
				execvp(link.c_str(), ps_argv);
				return;
			default:
				waitpid(pid, &status, 0);
		}
	}
	
	
	// compile the program
	// lexer -> parser -> semantic analyzer -> optimizer  -> code generation
	bool compiler::compile() {
		
		lex = new xlang::lexer(global.file);
		lex->init();
		parser = new xlang::parser();
		
		ast = parser->parse();
		
		if (global.error_count > 0) {
			return false;
		}
		
		//create sematic analyzer object
		an = new xlang::analyzer();
		
		an->analyze(&ast);  //analyze whole program by traversing AST
		
		//check error count from analyzer
		if (!error_count()) {
			delete an;
			delete parser;
			delete lex;
			return false;
		}
		
		generator = new gen();
		generator->get_code(&ast);
		delete generator;
		
		if (error_count() == 0) {
			if (global.print_tree) {
				std::cout << "file: " << global.file.name << std::endl;
				ast->print();
			}
			if (global.print_symtab) {
				std::cout << "file: " << global.file.name << std::endl;
				symtab->print();
			}
			if (global.print_record_symtab) {
				std::cout << "file: " << global.file.name << std::endl;
				record_table->print();
			}
		}
		
		return true;
	}
	
	bool compiler::error_count() {
		if (global.error_count > 0) {
			tree::delete_tree(&ast);
			symtable::delete_node(&symtab);
			symtable::delete_record_symtab(&record_table);
			return false;
		}
		
		return true;
	}
}
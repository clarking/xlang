
/*
 * Copyright (c) 2022, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "compiler.hpp"
#include "log.hpp"

#include <sys/wait.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unistd.h>
#include <array>

namespace xlang {
	
	Lexer *Compiler::lex{nullptr};
	Parser *Compiler::parser{nullptr};
	Analyzer *Compiler::an{nullptr};
	CodeGen *Compiler::generator{nullptr};
	TreeNode *Compiler::ast{nullptr};
	Node *Compiler::symtab{nullptr};
	RecordSymtab *Compiler::record_table{nullptr};
	FunctionMap *Compiler::func_table{nullptr};
    RecordNode *Compiler::last_rec_node{nullptr};
	SymbolInfo *Compiler::last_symbol{nullptr};

	int Compiler::run() {

        bool res = false;
		if (global.compile)
			res = compile();
		
		if (res && global.assemble)
			res = assemble();
		
        if (res && global.link)
		    res =link();
		
        if(!res)
            return 1;

		if(global.remove_asmfile)
			remove(global.file.asm_name().c_str());
		
		if(global.remove_objfile)
		    remove(global.file.object_name().c_str());
		
		Tree::delete_tree(&ast);
		SymbolTable::delete_node(&symtab);
		SymbolTable::delete_record_symtab(&record_table);
		return 0;
	}
	
	bool Compiler::assemble() {
		
		std::string asm_cmd = "nasm ";
        
        if(global.x64)
            asm_cmd += "-f elf64 ";
        else 
            asm_cmd += "-f elf32 ";

        asm_cmd += global.file.asm_name();

        //std::cout << "asm: " << assembler <<"\n";
        return execute(asm_cmd);
	}
	
	
	bool Compiler::link() {
		
        //	link the compiled and assembled object file with GCC.

        std::vector<std::string> opts;
		std::string outputfile = global.file.name;

        size_t fnd = global.file.object_name().find_last_of('/');
		if (fnd != std::string::npos) 
			outputfile = global.file.object_name().substr(0, fnd) + "/" + outputfile;

        std::string link_cmd = "gcc ";

        if(!global.x64 && X64_HOST)
            link_cmd+= "-m32 ";

        if (!global.use_cstdlib) 
            link_cmd += "-nostdlib ";

        link_cmd += "-no-pie ";
        link_cmd += global.file.object_name(); 
        link_cmd += " -o ";
        link_cmd += outputfile.c_str();

        //std::cout << "link: " << link_cmd <<"\n";
        return execute(link_cmd);
	}

	bool Compiler::compile() {
		
        // compile the program
        //
        // lexer -> 
        //    parser ->
        //       analyzer -> 
        //          optimizer  -> 
        //              code generation!

		lex = new Lexer(global.file);
		lex->init();

		parser = new Parser();	
		ast = parser->parse();
		
		if (global.error_count > 0) 
			return false;
		
		an = new Analyzer();
		an->analyze(&ast); 
		
		if (!error_count()) {
			delete an;
			delete parser;
			delete lex;
			return false;
		}
		
		generator = new CodeGen();
		generator->get_code(&ast);
		delete generator;
		
		if (error_count() != 0) {
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
	
	bool Compiler::error_count() {
		if (global.error_count > 0) {
			Tree::delete_tree(&ast);
			SymbolTable::delete_node(&symtab);
			SymbolTable::delete_record_symtab(&record_table);
			return false;
		}
		
		return true;
	}

    bool Compiler::execute(std::string cmd)
    {
        int status = 0;
        auto pp = ::popen(cmd.c_str(), "r");

        if(pp == nullptr) {
           std::cerr << "couldn't execute, failed to open pipe\n";
           return false;
        }

        std::array<char, 256> buffer;
        std::string result;

        while(not std::feof(pp)) {
            auto bytes = std::fread(buffer.data(), 1, buffer.size(), pp);
            result.append(buffer.data(), bytes);
        }

        if(!result.empty())
            std::cout << result;

        auto rc = ::pclose(pp);
        if(WIFEXITED(rc))
            status = WEXITSTATUS(rc);

        return (status == 0 ? true:false);
    }

}

/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <iostream>
#include <filesystem>
#include "compiler.hpp"
#include "log.hpp"

using namespace xlang;

const std::string VERSION = "0.0.1";

static void Version() {
	Log::line("xlang ", VERSION);
	exit(0);
}

static void Help() {
	
	std::vector<std::string> lines = {
			"  usage: ./xlang [options] <file>",
			"    -h  or --help (this message)",
			"    -t  or --print-tree (print symbol table)",
			"    -r  or --print-record-symtab (print record symnol table)",
			"    -a  or --assemble (assemble only)",
			"    -l  or --link     (link  only)",
			"    -c  or --compile  (compile includes assembly and link passes)",
			"    -o  or --optimize  (apply optimizations)",
			"    -f  or --filename  (specity output filename)",
			"    -no-stdlib (don't incude stdsib)",
			"    -no-frameptr (omits frame pointer)",
			"    -m32 (only applies for x86_64 hosts to output 32 bit code)",
			"    -v  or --version (show version)"
	};
	
	Log::print_lines(lines);
	exit(0);
}

static void process_args(GlobalConfig &global, int argc, char **argv) {
	
	for (int i = 1; i < argc; ++i) {
		std::string str = argv[i];
		if (str == "--print-tree" || str == "-t") 
			global.print_tree = true;
		else if (str == "--print-symtab" || str == "-s") 
			global.print_symtab = true;
		else if (str == "--print-record-symtab" || str == "-r") 
			global.print_record_symtab = true;
		else if (str == "--compile" || str == "-c") 
			global.compile = true;
		else if (str == "--assemble" || str == "-a") 
			global.assemble = true;
		else if (str == "--optimize" || str == "-o") 
			global.optimize = true;
		else if (str == "--link" || str == "-l") 
			global.optimize = true;
		else if (str == "--no-stdlib") 
			global.use_cstdlib = false;
		else if (str == "--no-frameptr") 
			global.omit_frame_pointer = true;
		else if (str == "ak" || str == "--keep-asm-file") 
			global.remove_asmfile = false;
		else if (str == "ok" || str == "--keep-obj-file") 
			global.remove_objfile = false;
		else if (str == "-v" || str == "--version") 
			Version();
		else if (str == "-m32") {
            global.x64 = false;
			// TODO
		}
		else if (str == "-h" || str == "--help") 
			Help();
		else {
			std::filesystem::path path(str);
			global.file.name = path.filename();
			global.file.path = absolute(path);
			
			if (path.has_extension())
				global.file.extension = path.extension();
		}
	}
}

GlobalConfig Compiler::global;

int main(int argc, char **argv) {
	
	if (argc < 2) {
		Log::error("No input file provided");
		Help();
		return 0;
	}
	
	process_args(Compiler::global, argc, argv);
	if (Compiler::global.file.name.empty())
		Log::error("No files provided");

	Compiler comp;
	return comp.run();
}
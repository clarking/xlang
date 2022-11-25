/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


// main compiler driver program

#include <iostream>
#include <filesystem>
#include "compiler.hpp"
#include "log.hpp"

using namespace xlang;

const std::string VERSION = "0.0.1";

static void version() {
	log::line("xlang ", VERSION);
	exit(0);
}

static void help() {
	
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
	
	log::print_lines(lines);
	exit(0);
}

void process_args(global_cfg &global, int argc, char **argv) {
	
	for (int i = 1; i < argc; ++i) {
		std::string str = argv[i];
		if (str == "--print-tree" || str == "-t") {
			global.print_tree = true;
		}
		else if (str == "--print-symtab" || str == "-s") {
			global.print_symtab = true;
		}
		else if (str == "--print-record-symtab" || str == "-r") {
			global.print_record_symtab = true;
		}
		else if (str == "--compile" || str == "-c") {
			global.compile = true;
		}
		else if (str == "--assemble" || str == "-a") {
			global.assemble = true;
		}
		else if (str == "--optimize" || str == "-o") {
			global.optimize = true;
		}
		else if (str == "--link" || str == "-l") {
			global.optimize = true;
		}
		else if (str == "--no-stdlib") {
			global.use_cstdlib = false;
		}
		else if (str == "--no-frameptr") {
			global.omit_frame_pointer = true;
		}
		else if (str == "ak" || str == "--keep-asm-file") {
			global.remove_asmfile = false;
		}
		else if (str == "ok" || str == "--keep-obj-file") {
			global.remove_objfile = false;
		}
		else if (str == "-v" || str == "--version") {
			version();
		}
		else if (str == "-m32") {
			// TODO
		}
		else if (str == "-h" || str == "--help") {
			help();
		}
		else {
			
			std::filesystem::path path(str);
			global.file.name = path.filename();
			global.file.path = absolute(path);
			
			if (path.has_extension())
				global.file.extension = path.extension();
			
		}
	}
}

global_cfg compiler::global = global_cfg();

int main(int argc, char **argv) {
	
	if (argc < 2) {
		log::error("No input file provided");
		help();
		return 0;
	}
	
	process_args(compiler::global, argc, argv);
	if (compiler::global.file.name.empty())
		log::error("No files provided");
	else {
		compiler comp;
		if (comp.run())
			return 0;
	}
	
	return 1;
}
/*
 * Copyright (c) 2022, Aaron Clark Diaz
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <iomanip>
#include <cstdarg>
#include <iostream>
#include <vector>
#include "compiler.hpp"
#include "token.hpp"

namespace xlang {
	
	#define    LOG_DISABLE  0
	#define    LOG_MINIMAL  1
	#define    LOG_VERBOSE  2
	#define    LOG_ANNOYING 3
	
	class Log {
	public:
		
		template<typename ...Args>
		static void error(Args &&...args) {
			(std::cout << ... << args);
			std::cout << '\n';
			exit(-1);
		}
		
		template<typename ...Args>
		static void error_at(TokenLocation loc, Args &&...args) {
			//std::cout << cfg.file.name << ": [" << loc.line << ":" << loc.col << "] ";
			// TODO: log file path and name here, absolute if possible
			std::cout << "[" << loc.line << ":" << loc.col << "] ";
			(std::cout << ... << args);
            std::cout << "\n";
			exit(-1);
		}
		
		template<typename ...Args>
		static void info(Args &&...args) {
			if (Compiler::global.log_level != LOG_DISABLE) {
				(std::cout << ... << args);
			}
		}
		
		template<typename ...Args>
		static void warn(Args &&...args) {
			if (Compiler::global.log_level != LOG_DISABLE) {
				(std::cout << ... << args);
			}
		}
		
		template<typename ...Args>
		static void debug(Args &&...args) {
			if (Compiler::global.log_level != LOG_DISABLE) {
				(std::cout << ... << args);
			}
		}
		
		template<typename ...Args>
		static void line(Args &&...args) {
			if (Compiler::global.log_level != LOG_DISABLE) {
				(std::cout << ... << args);
				std::cout << '\n';
			}
		}
		
		static void print_tokens(std::vector<Token> toks) {
			for (const auto &tok: toks) {
				line(tok.string);
			}
		}
		
		static void print_lines(std::vector<std::string> lines) {
			for (const auto &l: lines) {
				std::cout << l << "\n";
			}
		}
	};
	
}
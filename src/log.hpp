/*
 * Copyright (c) 2022, Aaron Clark Diaz
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef LOG_HPP
#define LOG_HPP

#include "types.hpp"
#include <iomanip>
#include <cstdarg>


extern int log_level;

namespace xlang {

	#define    LOG_DISABLE  0
	#define    LOG_MINIMAL  1
	#define    LOG_VERBOSE  2
	#define    LOG_ANNOYING 3

	template<typename ...Args>
	void log_error(Args &&...args) {
		(std::cout << ... << args);
		exit(-1);
	}

	template<typename ...Args>
	void log_error_at(loc_t loc, Args &&...args) {
		std::cout << "[" << loc.line << ":" << loc.col << "] ";
		(std::cout << ... << args);
		exit(-1);
	}

	template<typename ...Args>
	void log_error_at(std::string filename, loc_t loc, Args &&...args) {
		std::cout << filename << ": [" << loc.line << ":" << loc.col << "]\n";
		(std::cout << ... << args);
		exit(-1);
	}

	template<typename ...Args>
	void log_info(Args &&...args) {

		if (log_level != LOG_DISABLE) {
			(std::cout << ... << args);
		}
	}

	template<typename ...Args>
	void log_warn(Args &&...args) {
		if (log_level != LOG_DISABLE) {
			(std::cout << ... << args);
		}
	}

	template<typename ...Args>
	void log_debug(Args &&...args) {
		if (log_level != LOG_DISABLE) {
			(std::cout << ... << args);
		}
	}
}

#endif
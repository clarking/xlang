/*
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


#pragma once

#include <string>

namespace xlang {
	
	struct src_file {
		int id = 0;
		std::string path;
		std::string name;
		std::string extension;
		std::string content;
		bool loaded = false;
		
		// remove .x from end of filename and attach .asm as suffix
		std::string asm_name() {
			size_t fnd = name.find_last_of('.');
			if (fnd != std::string::npos) {
				name = name.substr(0, fnd);
			}
			return name + ".asm";
		}
		
		// remove .x from end of filename and attach .asm as suffix
		std::string object_name() {
			size_t fnd = name.find_last_of('.');
			if (fnd != std::string::npos) {
				name = name.substr(0, fnd);
			}
			return name + ".o";
		}
	};
}
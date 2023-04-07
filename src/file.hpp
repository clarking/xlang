/*
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


#pragma once

#include <string>

namespace xlang {
	
	struct SourceFile {
		
        unsigned id{0};
		std::string path;
		std::string name;
		std::string extension;
		std::string content;
		bool loaded = false;
		
		std::string asm_name() {

		    // remove .x from end of filename and attach .asm as suffix

			size_t fnd = name.find_last_of('.');
			if (fnd != std::string::npos) 
				name = name.substr(0, fnd);

			return name + ".asm";
		}
		
		std::string object_name() {

		    // remove .x from end of filename and attach .asm as suffix

			size_t fnd = name.find_last_of('.');
            if (fnd != std::string::npos) 
				name = name.substr(0, fnd);
			
			return name + ".o";
		}
	};
}
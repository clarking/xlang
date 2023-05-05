/*
 * Copyright (c) 2023, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


#pragma once

#include "tree.hpp"
#include "file.hpp"
#include <string>
#include <cstdint>

namespace xlang {

#if INTPTR_MAX == INT32_MAX
    #define X64_HOST 0
#elif INTPTR_MAX == INT64_MAX
    #define X64_HOST 1
#else
    #error "Host not supported."
#endif

	struct GlobalConfig {
		SourceFile file;
		int log_level{1};
		int error_count{0};
		bool print_tree{false};
		bool print_symtab{false};
		bool print_record_symtab{false};
		bool use_cstdlib{true};
		bool omit_frame_pointer{false};
		bool compile{true};
		bool assemble{true};
		bool link{true};
		bool optimize{false};
		bool remove_asmfile{true};
		bool remove_objfile{true};
        bool x64{false};
	};
}
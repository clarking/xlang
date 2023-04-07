/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


#pragma once

#include "token.hpp"
#include <string>

namespace xlang {
	

    class Convert {
    public:
	    static int tok_to_decimal(Token&);
	    static int octal_to_decimal(std::string&);
	    static int hex_to_decimal(std::string&);
	    static int bin_to_decimal(std::string&);
	    static int char_to_decimal(std::string&);
	    static std::string dec_to_hex(unsigned int);
    };
}
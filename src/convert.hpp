/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef CONVERT_HPP
#define CONVERT_HPP

#include "token.hpp"

namespace xlang {

	extern int get_decimal(token);
	extern int convert_octal_to_decimal(std::string);
	extern int convert_hex_to_decimal(std::string);
	extern int convert_bin_to_decimal(std::string);
	extern int convert_char_to_decimal(std::string);
	extern std::string decimal_to_hex(unsigned int);

}

#endif


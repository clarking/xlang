/*
 * Copyright (c) 2019  Pritam Zope
 * Copyright (c) 2021, Aaron Clark Diaz.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


/*
* Contains structures/types of general types
*/

#ifndef TYPES_HPP
#define TYPES_HPP

#include <iostream>

//buffer size which lexer will use to read block from a file
#define BUFFER_SIZE 512

typedef std::string lexeme_t;
typedef std::string record_t;
typedef std::string identifier_t;

//location of token
typedef struct {
	int line;
	int col;
} loc_t;

#endif

//------------------------------------------------------------------------------
//
// Indentation output stream class implementation.
//
// The indent_ostream class purpose is to provide simple interface
// for pretty printing program-like data. Mainly, it introduces
// new manipulators that can increase and descrease indentation level.
// This level then used to indent output each time newline character
// is printed. Thus, usage model is quite simple:
//
// indent_ostream(std::cout, 2 /* level spaces */) ios;
// ios << increase_indent << "Hello\nWorld\n" << decrease_indent << "!";
//
// This example will produce the following output:
// Hello
//   World
// !
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include "utils/indent_ostream.h"

#include <ios>

const int utils::indent_ostream_t::x_stream_id_ = std::ios_base::xalloc();

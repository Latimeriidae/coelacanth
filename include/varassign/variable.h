//------------------------------------------------------------------------------
//
// Varassign: variable type
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

namespace va {

struct variable_t {
  int id;
  int type_id;
  variable_t(int i = -1, int ti = -1) : id(i), type_id(ti) {}
};

} // namespace va
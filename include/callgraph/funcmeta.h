//------------------------------------------------------------------------------
//
// Metastructure for functions
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#pragma once

#include "config/configs.h"
#include "typegraph/typecats.h"

namespace ms {

struct metanode_t {
  unsigned usesigned : 1;
  unsigned usefloat : 1;
  unsigned usecomplex : 1;
  unsigned usepointers : 1;
};

// create random metainfo node
metanode_t random_meta(const cfg::config &config);

// check if type conform to metastructure
bool check_type(metanode_t m, tg::vertexprop_t vpt);

} // namespace ms
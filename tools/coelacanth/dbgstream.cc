//------------------------------------------------------------------------------
//
// multi-thread debug output
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

#include "dbgstream.h"

// global mutex for debug stream
std::mutex dbgs::mut_dbgs;

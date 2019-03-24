//------------------------------------------------------------------------------
//
// Options
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

// TODO: autogenerate this with ruby from pretty looking description

#if defined(OPTENUM)

enum { NCONSUMERS, NMODULES, NVAR, NSPLITS, NLOCS, NARITH };

#endif

#if defined(OPREGISTRY)

#define OPTSINGLE(N, V, T) register_option(N, #N, single{V}, T)
#define OPTDIAP(N, V1, V2, T) register_option(N, #N, diap{V1, V2}, T)

// programm-level
OPTSINGLE(NCONSUMERS, 5, "Number of consumer threads");
OPTSINGLE(NVAR, 2, "Number of varassign randomizations");
OPTSINGLE(NSPLITS, 5, "Number of controlgraph randomizations");
OPTSINGLE(NLOCS, 5, "Number of LocIR randomizations");
OPTSINGLE(NARITH, 10, "Number of ExprIR randomizations");

// callgraph-level
OPTDIAP(NMODULES, 2, 6, "Numer of programm modules");

#undef OPTDIAP
#undef OPTSINGLE

#endif

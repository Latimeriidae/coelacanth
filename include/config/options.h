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

enum { NCONSUMERS = 0, NVAR, NSPLITS, NLOCS, NARITH };

enum class TG { SEEDS = 100, SPLITS, CONTTYPE, NFIELDS, LONGT, FPT };

enum class CG { MODULES = 200 };

// for TG::CONTTYPE
enum { TGC_ARRAY = 0, TGC_STRUCT = 1 };

#endif

#if defined(OPREGISTRY)

#define OPTSINGLE(N, V, T)                                                     \
  register_option(static_cast<int>(N), #N, single{V}, T)
#define OPTDIAP(N, V1, V2, T)                                                  \
  register_option(static_cast<int>(N), #N, diap{V1, V2}, T)
#define OPTPROBF(N, V, T) register_option(static_cast<int>(N), #N, probf{V}, T)

// programm-level
OPTSINGLE(NCONSUMERS, 5, "Number of consumer threads");
OPTSINGLE(NVAR, 2, "Number of varassign randomizations");
OPTSINGLE(NSPLITS, 5, "Number of controlgraph randomizations");
OPTSINGLE(NLOCS, 5, "Number of LocIR randomizations");
OPTSINGLE(NARITH, 10, "Number of ExprIR randomizations");

// typegraph-level
OPTSINGLE(TG::SEEDS, 12, "Number of typegraph seed nodes");
OPTSINGLE(TG::SPLITS, 20, "Number of typegraph splits to perform");
OPTPROBF(TG::CONTTYPE, (probf_t{50, 100}),
         "Probability function for type containers");
OPTDIAP(TG::NFIELDS, 2, 6, "Number of structure fields");
OPTSINGLE(TG::LONGT, 0, "Allow long types");
OPTSINGLE(TG::FPT, 0, "Allow FP types");

// callgraph-level
OPTDIAP(CG::MODULES, 2, 6, "Numer of programm modules");

#undef OPTPROBF
#undef OPTDIAP
#undef OPTSINGLE

#endif

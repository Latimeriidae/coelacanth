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

// programm level (from 0)
enum class PG { CONSUMERS = 0, VAR, SPLITS, LOCS, ARITH, MAX };

// typegraph level (from PG::MAX)
enum class TG {
  START = int(PG::MAX),
  SEEDS,
  SPLITS,
  CONTTYPE,
  TYPEPROB,
  NFIELDS,
  ARRSIZE,
  MAXARRPREDS,
  MAXSTRUCTPREDS,
  MAXPREDS,
  BFPROB,
  BFSIZE,
  MAX
};

// callgraph level
enum class CG { START = int(TG::MAX), MODULES, VERTICES, EDGES, ADDLEAFS, MAX };

// varassign level
enum class VA { START = int(CG::MAX), VARS, MAX };

// controlgraph level
enum class CN { START = int(VA::MAX), MAX };

// locir level
enum class LI { START = int(CN::MAX), MAX };

// exprir level
enum class EI { START = int(LI::MAX), MAX };

// probability distribution structures

// for TG::CONTTYPE
enum { TGC_ARRAY = 0, TGC_STRUCT };

// for TG::TYPEPROB
enum {
  TGP_UCHAR = 0,
  TGP_SCHAR,
  TGP_USHORT,
  TGP_SSHORT,
  TGP_UINT,
  TGP_SINT,
  TGP_ULONG,
  TGP_SLONG,
  TGP_ULLONG,
  TGP_SLLONG,
  TGP_FLOAT,
  TGP_DOUBLE
};

#endif

#if defined(OPREGISTRY)

#define OPTSINGLE(N, V, T)                                                     \
  register_option(static_cast<int>(N), #N, single{V}, T)
#define OPTDIAP(N, V1, V2, T)                                                  \
  register_option(static_cast<int>(N), #N, diap{V1, V2}, T)
#define OPTPROBF(N, V, T) register_option(static_cast<int>(N), #N, probf{V}, T)

// programm-level
OPTSINGLE(PG::CONSUMERS, 5, "Number of consumer threads");
OPTSINGLE(PG::VAR, 2, "Number of varassign randomizations");
OPTSINGLE(PG::SPLITS, 5, "Number of controlgraph randomizations");
OPTSINGLE(PG::LOCS, 5, "Number of LocIR randomizations");
OPTSINGLE(PG::ARITH, 10, "Number of ExprIR randomizations");

// typegraph-level
OPTSINGLE(TG::SEEDS, 20, "Number of typegraph seed nodes");
OPTSINGLE(TG::SPLITS, 30, "Number of typegraph splits to perform");
OPTPROBF(TG::CONTTYPE, (probf_t{50, 100}),
         "Probability function for type containers");
OPTPROBF(TG::TYPEPROB,
         (probf_t{8, 17, 25, 33, 42, 50, 58, 67, 75, 83, 92, 100}),
         "Probability function for scalar types");
OPTDIAP(TG::NFIELDS, 2, 6, "Number of structure fields");
OPTDIAP(TG::ARRSIZE, 2, 10, "Size of array");
OPTSINGLE(TG::MAXARRPREDS, 3, "Maximum number of nested arrays");
OPTSINGLE(TG::MAXSTRUCTPREDS, 3, "Maximum number of nested structures");
OPTSINGLE(TG::MAXPREDS, 5, "Maximum number of nested types");
OPTSINGLE(TG::BFPROB, 10, "Percentage of bitfield probability");
OPTDIAP(TG::BFSIZE, 1, 31, "Bitfiled size diap");

// callgraph-level
OPTDIAP(CG::MODULES, 2, 6, "Number of programm modules");
OPTDIAP(CG::VERTICES, 10, 20, "Number of initial leaf and non-leaf functions");
OPTDIAP(CG::EDGES, 20, 30, "Number of initial CG edges");
OPTDIAP(CG::ADDLEAFS, 10, 15, "Number of additional leaf functions");

#undef OPTPROBF
#undef OPTDIAP
#undef OPTSINGLE

#endif

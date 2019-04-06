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
  SCALTYPE,
  TYPEPROB,
  NFIELDS,
  ARRSIZE,
  MAXARRPREDS,
  MAXSTRUCTPREDS,
  MAXPREDS,
  BFPROB,
  BFSIZE,
  MORESCALARS,
  MAX
};

// callgraph level
enum class CG {
  START = int(TG::MAX),
  MODULES,
  VERTICES,
  EDGESET,
  ADDLEAFS,
  SELFLOOP,
  INDSETCNT,
  ARTIFICIAL_CONNS,
  MAX
};

// metastructure level
enum class MS {
  START = int(CG::MAX),
  USEFLOAT,
  USESIGNED,
  USECOMPLEX,
  USEPOINTERS,
  SPLITS,
  NVARS,
  MAX
};

// varassign level
enum class VA { START = int(MS::MAX), VARS, MAX };

// controlgraph level
enum class CN { START = int(VA::MAX), MAX };

// locir level
enum class LI { START = int(CN::MAX), MAX };

// exprir level
enum class EI { START = int(LI::MAX), MAX };

// probability distribution structures

// for TG::CONTTYPE
enum { TGC_ARRAY = 0, TGC_STRUCT, TGC_MAX };

// for TG::SCALTYPE
enum { TGS_SCALAR = 0, TGS_POINTER, TGS_MAX };

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
  TGP_DOUBLE,
  TGP_MAX
};

#endif

#if defined(OPREGISTRY)

#define OPTSINGLE(N, V, T)                                                     \
  register_option(static_cast<int>(N), #N, single{V}, T)
#define OPTDIAP(N, V1, V2, T)                                                  \
  register_option(static_cast<int>(N), #N, diap{V1, V2}, T)
#define OPTPROBF(N, V, M, T)                                                   \
  register_option(static_cast<int>(N), #N, probf{V}, T, M)
#define OPTPFLAG(N, V, M, T)                                                   \
  register_option(static_cast<int>(N), #N, pflag{V, M}, T, M)

// programm-level
OPTSINGLE(PG::CONSUMERS, 5, "Number of consumer threads");
OPTSINGLE(PG::VAR, 2, "Number of varassign randomizations");
OPTSINGLE(PG::SPLITS, 5, "Number of controlgraph randomizations");
OPTSINGLE(PG::LOCS, 5, "Number of LocIR randomizations");
OPTSINGLE(PG::ARITH, 10, "Number of ExprIR randomizations");

// typegraph-level
OPTSINGLE(TG::SEEDS, 20, "Number of typegraph seed nodes");
OPTSINGLE(TG::SPLITS, 50, "Number of typegraph splits to perform");
OPTPROBF(TG::CONTTYPE, (probf_t{50, 100}), TGC_MAX,
         "Probability function for type containers");
OPTPROBF(TG::SCALTYPE, (probf_t{90, 100}), TGS_MAX,
         "Probability function for scalar types");
OPTPROBF(TG::TYPEPROB,
         (probf_t{8, 17, 25, 33, 42, 50, 58, 67, 75, 83, 92, 100}), TGP_MAX,
         "Probability function for scalar types");
OPTDIAP(TG::NFIELDS, 2, 6, "Number of structure fields");
OPTDIAP(TG::ARRSIZE, 2, 10, "Size of array");
OPTSINGLE(TG::MAXARRPREDS, 3, "Maximum number of nested arrays");
OPTSINGLE(TG::MAXSTRUCTPREDS, 3, "Maximum number of nested structures");
OPTSINGLE(TG::MAXPREDS, 5, "Maximum number of nested types");
OPTPFLAG(TG::BFPROB, 10, 100, "Probability of generating bitfield");
OPTDIAP(TG::BFSIZE, 1, 31, "Bitfiled size diap");
OPTSINGLE(
    TG::MORESCALARS, 0,
    "Add more top-level scalars (additional scalar for every type split)");

// callgraph-level
OPTDIAP(CG::MODULES, 2, 6, "Number of programm modules");
OPTDIAP(CG::VERTICES, 15, 25, "Number of initial leaf and non-leaf functions");
OPTPFLAG(CG::EDGESET, 6, 100, "Probability to set edge");
OPTDIAP(CG::ADDLEAFS, 10, 15, "Number of additional leaf functions");
OPTPFLAG(CG::SELFLOOP, 6, 100, "Probability to create self-loop");
OPTDIAP(CG::INDSETCNT, 6, 9, "Number of vertices to allow indirect calls");
OPTSINGLE(CG::ARTIFICIAL_CONNS, 5,
          "Artificial connections in case of no zero in-degree edges");

// function-wise metastructure
OPTPFLAG(MS::USEFLOAT, 10, 100,
         "Probability of floating-point arithmetics usage per function");
OPTPFLAG(MS::USESIGNED, 20, 100,
         "Probability of signed data types usage per function");
OPTPFLAG(MS::USECOMPLEX, 60, 100,
         "Probability of complex data types usage per function");
OPTPFLAG(MS::USEPOINTERS, 60, 100,
         "Probability of pointers usage per function");
OPTDIAP(
    MS::SPLITS, 5, 90,
    "Number of splits in (roughly: cyclomatic complexity of) every function");
OPTDIAP(MS::NVARS, 5, 20,
        "Local variables added pressure in functions (whatever it be)");

#undef OPTPFLAG
#undef OPTPROBF
#undef OPTDIAP
#undef OPTSINGLE

#endif

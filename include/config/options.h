//------------------------------------------------------------------------------
//
// Options
//
// coelacanth supports different modes of work
//
// programm-config:
//   one can use it as random code generator (this is called default mode)
//   but it can produce only call-graphs
//   or use given pack of call-graphs
//   this is called programm-config level options
//
// programm:
//   modulo program-config whole programm options (like number of splits, etc)
//   are here
//
// TODO: comment on other sections
//
//------------------------------------------------------------------------------
//
// This file is licensed after LGPL v3
// Look at: https://www.gnu.org/licenses/lgpl-3.0.en.html for details
//
//------------------------------------------------------------------------------

// TODO: autogenerate this with ruby from pretty looking description

#if defined(OPTENUM)

// program-config level
enum class PGC {
  STOP_ON_TG = 0,
  USETG,
  TGNAME,
  STOP_ON_CG,
  USECG,
  CGNAME,
  STOP_ON_VA,
  USEVA,
  VANAME,
  STOP_ON_CN,
  USECN,
  CNNAME,
  MAX
};

// programm level
enum class PG { CONSUMERS = int(PGC::MAX), VAR, SPLITS, LOCS, ARITH, MAX };

// typegraph level
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
  TYPEATTEMPTS,
  NARGS,
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
enum class VA {
  START = int(MS::MAX),
  NGLOBALS,
  NIDX,
  NVATTS,
  USEPERM,
  MAXPERM,
  MAX
};

// controlgraph level
enum class CN {
  START = int(VA::MAX),
  ADDBLOCKS,
  EXPANDCONT,
  CONTPROB,
  NBRANCHES_IF,
  NBRANCHES_SWITCH,
  NBRANCHES_RGN,
  FOR_START,
  FOR_SIZE,
  FOR_STEP,
  BLOCKPROB,
  BREAKTYPE,
  DEFS,
  USES,
  MAX
};

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

// for CN::CONTPROB
enum { CNC_IF, CNC_FOR, CNC_SWITCH, CNC_REGION, CNC_MAX };

// for CN::BLOCKPROB
enum { CNB_BREAK, CNB_CCALL, CNB_ICALL, CNB_MAX };

// for CN::BREAKTYPE
enum { CNBR_BREAK, CNBR_CONT, CNBR_RET, CNBR_MAX };

#endif

#if defined(OPREGISTRY)

#define OPTBOOL(N, T)                                                          \
  register_option(static_cast<int>(N), #N, single_bool{false}, T)
#define OPTSTRING(N, V, T)                                                     \
  register_option(static_cast<int>(N), #N, single_string{V}, T)
#define OPTSINGLE(N, V, T)                                                     \
  register_option(static_cast<int>(N), #N, single{V}, T)
#define OPTDIAP(N, V1, V2, T)                                                  \
  register_option(static_cast<int>(N), #N, diap{V1, V2}, T)
#define OPTPROBF(N, V, M, T)                                                   \
  register_option(static_cast<int>(N), #N, probf{V}, T, M)
#define OPTPFLAG(N, V, M, T)                                                   \
  register_option(static_cast<int>(N), #N, pflag{V, M}, T, M)

// programm-config level
OPTBOOL(PGC::STOP_ON_TG, "Stop after type graph is ready");
OPTBOOL(PGC::USETG, "Do not generate type graph, use existing");
OPTSTRING(PGC::TGNAME, "default.cf", "Specify type graph name to use");
OPTBOOL(PGC::STOP_ON_CG, "Stop after call graph is ready");
OPTBOOL(PGC::USECG, "Do not generate call graph, use existing");
OPTSTRING(PGC::CGNAME, "default.cf", "Specify call graph name to use");
OPTBOOL(PGC::STOP_ON_VA, "Stop after varassign is ready");
OPTBOOL(PGC::USEVA, "Do not generate varassign, use existing");
OPTSTRING(PGC::VANAME, "default.cf", "Specify varassign name to use");
OPTBOOL(PGC::STOP_ON_CN, "Stop after control flow graph is ready");
OPTBOOL(PGC::USECN, "Do not generate control flow graph, use existing");
OPTSTRING(PGC::CNNAME, "default.cf", "Specify control flow graph name to use");

// programm level
OPTSINGLE(PG::CONSUMERS, 5, "Number of consumer threads");
OPTSINGLE(PG::VAR, 2, "Number of varassign randomizations");
OPTSINGLE(PG::SPLITS, 5, "Number of controlgraph randomizations");
OPTSINGLE(PG::LOCS, 5, "Number of LocIR randomizations");
OPTSINGLE(PG::ARITH, 10, "Number of ExprIR randomizations");

// typegraph level
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

// callgraph level
OPTDIAP(CG::MODULES, 2, 6, "Number of programm modules");
OPTDIAP(CG::VERTICES, 15, 25, "Number of initial leaf and non-leaf functions");
OPTPFLAG(CG::EDGESET, 6, 100, "Probability to set edge");
OPTDIAP(CG::ADDLEAFS, 10, 15, "Number of additional leaf functions");
OPTPFLAG(CG::SELFLOOP, 6, 100, "Probability to create self-loop");
OPTDIAP(CG::INDSETCNT, 6, 9, "Number of vertices to allow indirect calls");
OPTSINGLE(CG::ARTIFICIAL_CONNS, 5,
          "Artificial connections in case of no zero in-degree edges");
OPTSINGLE(CG::TYPEATTEMPTS, 10, "# of attempts to pick random type");
OPTDIAP(CG::NARGS, 0, 5, "# of function arguments");

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

// varassign level
OPTSINGLE(VA::NGLOBALS, 10, "Number of globals out of starting");
OPTSINGLE(VA::NIDX, 5, "Number of free indexes for function");
OPTSINGLE(VA::NVATTS, 50, "Number of attemps to choose locals");
OPTPFLAG(VA::USEPERM, 10, 100, "Probability to add permutator to array");
OPTSINGLE(VA::MAXPERM, 6, "Maximum number of index permutations");

// controlgraph level
OPTSINGLE(CN::ADDBLOCKS, 2, "Number of blocks to add on cf split");
OPTPFLAG(CN::EXPANDCONT, 80, 100, "Probability to get container cf split");
OPTPROBF(CN::CONTPROB, (probf_t{40, 80, 90, 100}), CNC_MAX,
         "Probability function for containers");
OPTDIAP(CN::NBRANCHES_IF, 2, 6, "Amount of branches inside if");
OPTDIAP(CN::NBRANCHES_SWITCH, 6, 10, "Amount of branches inside switch");
OPTDIAP(CN::NBRANCHES_RGN, 4, 7, "Amount of branches inside region");
OPTPROBF(CN::BLOCKPROB, (probf_t{40, 70, 100}), CNB_MAX,
         "Probability function for special blocks (calls, breaks)");
OPTDIAP(CN::FOR_START, 0, 20, "Starting value of loops");
OPTDIAP(CN::FOR_SIZE, 10, 50, "Number of iterations from start");
OPTDIAP(CN::FOR_STEP, 1, 3, "Step size of loops");
OPTPROBF(CN::BREAKTYPE, (probf_t{40, 80, 100}), CNBR_MAX,
         "Probability function for breaktypes");
OPTDIAP(CN::DEFS, 2, 4, "Number of defs");
OPTDIAP(CN::USES, 4, 6, "Number of uses");

#undef OPTPFLAG
#undef OPTPROBF
#undef OPTDIAP
#undef OPTSINGLE

#endif

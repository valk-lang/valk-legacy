
#ifndef _H_DEFS
#define _H_DEFS

#define usize unsigned long

// Core
typedef struct Allocator Allocator;
typedef struct AllocatorBlock AllocatorBlock;
// Utils
typedef struct Array Array;
typedef struct Map Map;
typedef struct Str Str;
// Build
typedef struct Build Build;
typedef struct Fc Fc;
typedef struct Nsc Nsc;
typedef struct Pkc Pkc;
typedef struct PkgConfig PkgConfig;
typedef struct Chunk Chunk;
typedef struct Stage Stage;
typedef struct CompileData CompileData;
// IR
typedef struct IR IR;
typedef struct IRFunc IRFunc;
typedef struct IRBlock IRBlock;
// Parse
typedef struct Func Func;
typedef struct FuncArg FuncArg;
typedef struct Scope Scope;
typedef struct Idf Idf;
typedef struct Type Type;
typedef struct Decl Decl;
typedef struct Global Global;
typedef struct Class Class;
typedef struct ClassProp ClassProp;
// Tokens
typedef struct Token Token;
typedef struct TDeclare TDeclare;
typedef struct TIf TIf;
typedef struct TWhile TWhile;
// Values
typedef struct Value Value;
typedef struct VPair VPair;
typedef struct VFuncPtr VFuncPtr;
typedef struct VFuncCall VFuncCall;
typedef struct VInt VInt;
typedef struct VClassPA VClassPA;
typedef struct VPtrv VPtrv;
typedef struct VOp VOp;
typedef struct VIncr VIncr;


#endif

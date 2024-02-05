
#ifndef H_IR
#define H_IR

#include "typedefs.h"

IR* ir_make(Fc* fc);
void ir_gen_globals(IR* ir);
void ir_gen_functions(IR* ir);
void ir_gen_final(IR* ir);

struct IR {
    Fc *fc;
    Build *b;
    //
    char* char_buf;
    Str* str_buf;
    //
    Str* code_final;
    Str* code_struct;
    Str* code_global;
    Str* code_extern;
    Str* code_attr;
    //
    Func* func;
    Array* funcs;
    Array* attrs;
    Array* declared_funcs;
    Array* declared_classes;
    Map* globals;
    // DI
    char *di_cu;
    char *di_file;
    char *di_retained_nodes;
    char *di_type_ptr;
    //
    int c_string;
    int c_attr;
    //
    bool use_stack_save;
    bool debug;
};
struct IRFunc {
    Str* code;
};

#endif


#ifndef H_IR
#define H_IR

#include "typedefs.h"

IR* ir_make(Fc* fc);
void ir_gen_globals(IR* ir);
void ir_gen_functions(IR* ir);
void ir_gen_final(IR* ir);

char *ir_type(IR *ir, Type *type);
char *ir_type_int(IR *ir, int bytes);
void ir_define_struct(IR *ir, Class* class);
char *ir_var(IRFunc* func);

IRBlock *ir_block_make(IR *ir, IRFunc* func);

struct IR {
    Fc *fc;
    Build *b;
    Allocator* alc;
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
    IR* ir;
    Func* func;
    //
    Array* blocks;
    IRBlock* block;
    IRBlock* block_start;
    IRBlock* block_code;
    //
    char* stack_save_vn;
    char* di_scope;
    //
    int var_count;
};
struct IRBlock {
    char* name;
    Str* code;
};

#endif

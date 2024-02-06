
#ifndef H_IR
#define H_IR

#include "typedefs.h"

// Core
IR* ir_make(Fc* fc);
void ir_gen_globals(IR* ir);
void ir_gen_functions(IR* ir);
void ir_gen_final(IR* ir);
// Type
char *ir_type(IR *ir, Type *type);
char *ir_type_real(IR *ir, Type *type);
char *ir_type_int(IR *ir, int bytes);
void ir_define_struct(IR *ir, Class* class);
char *ir_type_align(IR *ir, Type *type, char* result);
// Func
void ir_func_definition(Str* code, IR* ir, Func *vfunc, bool is_extern);
void ir_define_ext_func(IR* ir, Func* func);
char *ir_alloca(IR *ir, IRFunc* func, Type *type);
// Block
IRBlock *ir_block_make(IR *ir, IRFunc* func);
// AST
void ir_write_ast(IR* ir, Scope* scope);
// Global
void ir_gen_globals(IR* ir);
void *ir_global(IR *ir, Global *g);
// Value
char* ir_value(IR* ir, Scope* scope, Value* v);
char* ir_assign_value(IR* ir, Scope* scope, Value* v);
// Generate
char *ir_var(IRFunc* func);
void ir_jump(IR* ir, IRBlock* block);
void ir_cond_jump(IR* ir, char* cond, IRBlock* block_if, IRBlock* block_else);
char *ir_int(IR* ir, long int value);
Array *ir_fcall_args(IR *ir, Scope *scope, Array *values);
char *ir_func_call(IR *ir, char *on, Array *values, char *lrett, int line, int col);
char *ir_func_ptr(IR *ir, Func *func);
char *ir_string(IR *ir, char *body);
char* ir_load(IR* ir, Type* type, char* var);
void ir_store(IR *ir, Type *type, char *var, char *val);
char *ir_cast(IR *ir, char *lval, Type *from_type, Type *to_type);
char *ir_i1_cast(IR *ir, char *val);
char* ir_op(IR* ir, Scope* scope, int op, char* left, char* right, Type* rett);
char* ir_compare(IR* ir, Scope* scope, int op, Value* left, Value* right);
char *ir_class_pa(IR *ir, Class *class, char *on, ClassProp *prop);
void ir_if(IR *ir, Scope *scope, TIf *ift);
void ir_while(IR *ir, Scope *scope, TWhile *item);

// Structs
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
    IRBlock* block;
    IRFunc* func;
    //
    Array* funcs;
    Array* attrs;
    Array* declared_funcs;
    Array* declared_classes;
    Array* declared_globals;
    // DI
    char *di_cu;
    char *di_file;
    char *di_retained_nodes;
    char *di_type_ptr;
    //
    int string_count;
    int attr_count;
    //
    bool use_stack_save;
    bool debug;
};
struct IRFunc {
    IR* ir;
    Func* func;
    //
    Array* blocks;
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

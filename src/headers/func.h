
#ifndef _H_FUNC
#define _H_FUNC

#include "typedefs.h"

Func* func_make(Allocator* alc, Unit* u, Scope* parent, char* name, char* export_name);
FuncArg* func_arg_make(Allocator* alc, Type* type);
void func_mark_used(Func* func, Func* uses_func);
void parse_handle_func_args(Parser* p, Func* func);
void func_validate_arg_count(Parser* p, Func* func, bool is_static, int arg_count_min, int arg_count_max);
void func_validate_rett_count(Parser* p, Func* func, bool is_static, int rett_count_min, int rett_count_max);
void func_validate_arg_type(Parser* p, Func* func, int index, Array* allowed_types);
void func_validate_rett(Parser* p, Func* func, Array* allowed_types);
void func_validate_rett_void(Parser *p, Func *func);
Func* get_generic_func(Parser* p, Func* func, Array* generic_types);

struct Func {
    char* name;
    char* export_name;
    Build *b;
    Unit* unit;
    Fc* fc;
    Scope* scope;
    Unroll* ur;
    Allocator* ast_alc;
    //
    Chunk* chunk_args;
    Chunk* chunk_rett;
    Chunk* chunk_body;
    Chunk* body_end;
    //
    Map* args;
    Array* arg_types;
    Array* arg_values;
    Type* rett;
    Type* rett_eax;
    Array* rett_types;
    Array* rett_arg_types;
    Array* rett_decls;
    Class* class;
    Array* cached_values;
    Map* errors;
    Array* used_functions;
    Array* called_functions;
    Array* used_classes;
    Type* reference_type;
    //
    Scope* ast_stack_init;
    Scope* ast_start;
    Scope* ast_end;
    //
    Value* v_cache_stack_pos;
    Value* v_cache_alloca;
    //
    Token* t_stack_incr;
    Token* t_stack_decr;
    Token* t_run_gc;
    //
    int act;
    int alloca_size;
    int gc_decl_count;
    int arg_nr;
    int rett_count;
    // Generic
    Array* generic_names;
    Map* generics;
    //
    bool is_static;
    bool is_inline;
    bool can_error;
    bool types_parsed;
    bool in_header;
    bool read_rett_type;
    bool is_test;
    bool is_used;
    bool use_if_class_is_used;
    bool exits;
    bool parsed;
    bool calls_gc_check;
    bool parse_last;
    bool init_thread;
    bool can_create_objects;
    bool inf_args;
    bool is_generic_base;
};
struct FuncArg {
    Type* type;
    Value* value;
    Chunk* chunk;
    Chunk* chunk_value;
    Decl* decl;
};

#endif

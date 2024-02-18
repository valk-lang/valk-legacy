
#ifndef _H_TYPE
#define _H_TYPE

#include "typedefs.h"

Type *read_type(Fc *fc, Allocator *alc, Scope *scope, bool allow_newline);
Type* type_clone(Allocator* alc, Type* type);
Type* type_gen_void(Allocator* alc);
Type* type_gen_null(Allocator* alc, Build* b);
Type* type_gen_class(Allocator* alc, Class* class);
Type* type_gen_func(Allocator* alc, Func* func);
Type* type_gen_volt(Allocator* alc, Build* b, char* name);
Type* type_gen_number(Allocator* alc, Build* b, int size, bool is_float, bool is_signed);
bool type_compat(Type* t1, Type* t2, char** reason);
void type_check(Chunk* chunk, Type* t1, Type* t2);
bool type_is_void(Type* type);
bool type_is_bool(Type* type);
bool type_is_gc(Type* type);
char* type_to_str(Type* t, char* res);
char* type_to_str_export(Type* t, char* res);
void type_to_str_append(Type* t, Str* buf);
int type_get_size(Build* b, Type* type);

struct Type {
    Class* class;
    Array* func_args;
    Array* func_default_values;
    Map* func_errors;
    Type* func_rett;
    int type;
    int size;
    bool nullable;
    bool is_pointer;
    bool is_signed;
    bool func_can_error;
};

#endif


#ifndef _H_TYPE
#define _H_TYPE

#include "typedefs.h"

Type *read_type(Parser *p, Allocator *alc, bool allow_newline);
Type* type_clone(Allocator* alc, Type* type);
Type* type_gen_void(Allocator* alc);
Type* type_gen_null(Allocator* alc, Build* b);
Type* type_gen_class(Allocator* alc, Class* class);
Type* type_gen_func(Allocator* alc, Func* func);
Type* type_gen_valk(Allocator* alc, Build* b, char* name);
char* get_number_type_name(Build* b, int size, bool is_float, bool is_signed);
Type* type_gen_number(Allocator* alc, Build* b, int size, bool is_float, bool is_signed);
Type* type_merge(Allocator* alc, Type* t1, Type* t2);
bool type_compat(Type* t1, Type* t2, char** reason);
void type_check(Parser* p, Type* t1, Type* t2);
bool type_is_void(Type* type);
bool type_is_bool(Type* type);
bool type_is_gc(Type* type);
char* type_to_str(Type* t, char* res);
char* type_to_str_export(Type* t, char* res);
void type_to_str_append(Type* t, Str* buf);
int type_get_size(Build* b, Type* type);
Array* gen_type_array_1(Allocator* alc, Build* b, char* type1, bool nullable);
Array* gen_type_array_2(Allocator* alc, Build* b, char* type1, bool nullable1, char* type2, bool nullable2);
Type* vscope_get_result_type(Array* values);

struct Type {
    Class* class;
    Array* func_args;
    Array* func_default_values;
    Map* func_errors;
    Type* func_rett;
    Array* func_rett_types;
    int type;
    int size;
    bool nullable;
    bool is_pointer;
    bool is_signed;
    bool func_can_error;
    bool ignore_null;
};

#endif

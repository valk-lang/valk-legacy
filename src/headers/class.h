
#ifndef H_CLASS
#define H_CLASS

#include "typedefs.h"

Class* class_make(Allocator* alc, Build* b, int type);
ClassProp* class_prop_make(Build* b, Type* type, bool skip_default_value);
ClassProp* class_get_prop(Build* b, Class* class, char* name);
int class_determine_size(Build* b, Class* class);
Class* get_generic_class(Fc* fc, Class* class, Map* generic_types);
void class_generate_internals(Fc* fc, Build* b, Class* class);
void class_generate_transfer(Fc* fc, Build* b, Class* class, Func* func);
void class_generate_mark(Fc* fc, Build* b, Class* class, Func* func);
void class_generate_free(Fc* fc, Build* b, Class* class, Func* func);

struct Class {
    char* name;
    char* ir_name;
    Build* b;
    Fc* fc;
    Chunk* body;
    Scope* scope;
    Map* props;
    Map* funcs;
    Array* generic_names;
    Map* generic_types;
    Map* generics;
    int type;
    int size;
    int gc_fields;
    int gc_vtable_index;
    bool packed;
    bool is_signed;
    bool allow_math;
    bool is_generic_base;
};
struct ClassProp {
    Type* type;
    Chunk* chunk_type;
    Chunk* chunk_value;
    int index;
    bool skip_default_value;
};

#endif

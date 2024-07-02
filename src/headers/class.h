
#ifndef H_CLASS
#define H_CLASS

#include "typedefs.h"

Class* class_make(Allocator* alc, Build* b, Unit* u, int type);
ClassProp* class_prop_make(Build* b, Type* type, bool skip_default_value);
ClassProp* class_get_prop(Build* b, Class* class, char* name);
void generate_class_pool(Parser* p, Class* class);
int class_determine_size(Build* b, Class* class);
Class* get_generic_class(Parser* p, Class* class, Array* generic_types);
void class_generate_internals(Parser* p, Build* b, Class* class);

struct Class {
    char* name;
    char* ir_name;
    Build* b;
    Unit* unit;
    Fc* fc;
    Chunk* body;
    Scope* scope;
    Map* props;
    Map* funcs;
    Array* generic_names;
    Map* generic_types;
    Map* generics;
    Class* generic_of;
    Global* pool;
    int act;
    int type;
    int size;
    int gc_fields;
    int gc_vtable_index;
    bool packed;
    bool is_signed;
    bool allow_math;
    bool is_generic_base;
    bool in_header;
    bool is_used;
    bool use_gc_alloc;
};
struct ClassProp {
    Type* type;
    Chunk* chunk_type;
    Chunk* chunk_value;
    int index;
    int act;
    bool skip_default_value;
};
struct Trait {
    Chunk* chunk;
    Scope* scope;
    Fc* fc;
    int act;
};

#endif

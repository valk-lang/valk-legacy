
#ifndef _H_SCOPE
#define _H_SCOPE

#include "typedefs.h"

Scope* scope_make(Allocator* alc, int type, Scope* parent);
void scope_set_idf(Scope* scope, char*name, Idf* idf, Parser* p);
void scope_add_decl(Allocator* alc, Scope* scope, Decl* decl);
Scope* scope_sub_make(Allocator* alc, int type, Scope* parent);

struct Scope {
    Scope* parent;
    Scope* idf_parent;
    Map* identifiers;
    Map* type_identifiers;
    Array* ast;
    Type* rett;
    Array* decls;
    int type;
    int gc_decl_count;
    bool must_return;
    bool did_return;
    bool gc_check;
    bool has_gc_decls;
};

#endif
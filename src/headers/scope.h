
#ifndef _H_SCOPE
#define _H_SCOPE

#include "typedefs.h"

Scope* scope_make(Allocator* alc, int type, Scope* parent);
void scope_set_idf(Scope* scope, char*name, Idf* idf, Parser* p);
void scope_add_decl(Allocator* alc, Scope* scope, Decl* decl);
Scope* scope_sub_make(Allocator* alc, int type, Scope* parent);
void scope_apply_issets(Allocator *alc, Scope *scope, Array *issets);
bool scope_delete_idf_by_value(Scope* scope, void* item, bool recursive);
Scope* scope_get_loop_or_func_scope(Scope* scope);

struct Scope {
    Scope* parent;
    Scope* idf_parent;
    Map* identifiers;
    Array* ast;
    // Type* rett;
    Array* decls;
    Func* func;
    int type;
    bool must_return;
    bool did_return;
    bool gc_check;
    bool has_gc_decls;
    bool creates_objects;
};

#endif
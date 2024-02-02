
#ifndef _H_ID
#define _H_ID

#include "typedefs.h"

Idf* idf_make(Allocator* alc, int type, void* item);
Decl* decl_make(Allocator* alc, Type* type, bool is_arg);
char* gen_export_name(Nsc* nsc, char* suffix);
Idf* read_idf(Fc* fc, Scope* scope, char* first_part, bool must_exist);
Idf* scope_find_idf(Scope* scope, char* name, bool recursive);

#endif
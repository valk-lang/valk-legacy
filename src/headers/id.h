
#ifndef _H_ID
#define _H_ID

#include "typedefs.h"

Idf* idf_make(Allocator* alc, int type, void* item);
Decl* decl_make(Allocator* alc, Type* type, bool is_arg);
char* gen_export_name(Nsc* nsc, char* suffix);

#endif
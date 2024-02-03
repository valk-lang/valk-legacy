
#ifndef _H_TYPE
#define _H_TYPE

#include "typedefs.h"

Type *read_type(Fc *fc, Allocator *alc, Scope *scope, bool allow_newline);
Type* type_gen_class(Allocator* alc, Build* b, Class* class);

struct Type {
    Class* class;
    int type;
    int size;
    bool nullable;
};

#endif


#ifndef H_VALUE
#define H_VALUE

#include "typedefs.h"

Value* read_value(Fc* fc, Scope* scope, bool allow_newline, int prio);
bool value_is_assignable(Value *v);
// Gen
Value *value_make(Allocator *alc, int type, void *item, Type* rett);
Value *vgen_func_ptr(Allocator *alc, Func *func, Value *first_arg);

struct Value {
    int type;
    void* item;
    Type* rett;
};
struct VPair {
    Value* left;
    Value* right;
};
struct VFuncPtr {
    Func* func;
    Value* first_arg;
};

#endif

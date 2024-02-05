
#ifndef H_VALUE
#define H_VALUE

#include "typedefs.h"

Value* read_value(Allocator* alc, Fc* fc, Scope* scope, bool allow_newline, int prio);
bool value_is_assignable(Value *v);
void match_value_types(Allocator* alc, Build* b, Value** v1_, Value** v2_);
// Gen
Value *value_make(Allocator *alc, int type, void *item, Type* rett);
Value *vgen_func_ptr(Allocator *alc, Func *func, Value *first_arg);
Value *vgen_func_call(Allocator *alc, Value *on, Array *args);
Value *vgen_int(Allocator *alc, long int value, Type *type);
Value *vgen_class_pa(Allocator *alc, Value *on, ClassProp *prop);
Value *vgen_ptrv(Allocator *alc, Build* b, Value *on, Type* type, Value* index);
Value *vgen_op(Allocator *alc, int op, Value *left, Value* right, Type *rett);
Value *vgen_comp(Allocator *alc, int op, Value *left, Value* right, Type *rett);
Value *vgen_cast(Allocator *alc, Value *val, Type *to_type);

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
struct VFuncCall {
    Value *on;
    Array *args;
    int line;
    int col;
};
struct VInt {
    long int value;
};
struct VClassPA {
    Value* on;
    ClassProp* prop;
};
struct VPtrv {
    Value* on;
    Type* type;
    Value* index;
};
struct VOp {
    Value* left;
    Value* right;
    int op;
};

#endif

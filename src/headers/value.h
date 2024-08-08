
#ifndef H_VALUE
#define H_VALUE

#include "typedefs.h"

Value* read_value(Allocator* alc, Parser* p, bool allow_newline, int prio);
bool value_is_assignable(Value *v);
Value* value_handle_op(Allocator *alc, Parser* p, Value *left, Value* right, int op);
void value_is_mutable(Value* v);
Value* try_convert(Allocator* alc, Parser* p, Scope* scope, Value* val, Type* type);
bool value_needs_gc_buffer(Value* val);
Value *read_err_handler(Allocator* alc, Parser *p, Value* on, TypeFuncInfo *fi);
void value_enable_cached(VIRCached* v);
Array* all_values(Allocator*alc, Value* v);

// Gen
Value *value_make(Allocator *alc, int type, void *item, Type* rett);
Value *vgen_ptr_of(Allocator *alc, Build* b, Value* from);
Value* vgen_bool(Allocator *alc, Build* b, bool value);
Value *vgen_func_ptr(Allocator *alc, Func *func, Value *first_arg);
Value *vgen_func_call(Allocator *alc, Parser* p, Value *on, Array *args);
Value *vgen_func_call_unroll(Allocator *alc, Value *on, Array *args);
Value *vgen_int_parse(Allocator *alc, v_u64 value, bool negative, Type *type, Type *alt_type);
Value *vgen_int(Allocator *alc, v_i64 value, Type *type);
Value *vgen_float(Allocator *alc, double value, Type *type);
Value *vgen_class_pa(Allocator *alc, Value *on, ClassProp *prop);
Value *vgen_ptrv(Allocator *alc, Build* b, Value *on, Type* type, Value* index);
Value *vgen_ptr_offset(Allocator *alc, Build* b, Value *on, Value* index, int size);
Value *vgen_op(Allocator *alc, int op, Value *left, Value* right, Type *rett);
Value *vgen_comp(Allocator *alc, int op, Value *left, Value* right, Type *rett);
Value *vgen_cast(Allocator *alc, Value *val, Type *to_type);
Value* vgen_call_alloc(Allocator* alc, Parser* p, int size, Class* cast_as);
Value* vgen_call_pool_alloc(Allocator* alc, Parser* p, Build* b, Class* class);
// Value* vgen_call_gc_link(Allocator* alc, Build* b, Value* left, Value* right);
Value* vgen_incr(Allocator* alc, Build* b, Value* on, bool increment, bool before);
Value* vgen_ir_cached(Allocator* alc, Value* value);
Value* vgen_null(Allocator* alc, Build* b);
Value* vgen_gc_link(Allocator* alc, Value* on, Value* to, Type* rett);
Value* vgen_var(Allocator* alc, Build* b, Value* value);
Value* vgen_value_scope(Allocator* alc, Build* b, Scope* scope, Array* phi_values, Type* rett);
Value *vgen_isset(Allocator *alc, Build *b, Value *on);
Value *vgen_and_or(Allocator *alc, Build *b, Value *left, Value *right, int op);
Value *vgen_this_or_that(Allocator *alc, Value* cond, Value *v1, Value *v2, Type* rett);
Value *vgen_decl(Allocator *alc, Decl* decl);
Value *vgen_global(Allocator *alc, Global* g);
Value *vgen_stack(Allocator *alc, Build* b, Value* val);
Value *vgen_stack_size(Allocator *alc, Build* b, int size);
Value *vgen_string(Allocator *alc, Unit *u, char *body);
Value* vgen_null_alt_value(Allocator* alc, Value* left, Value* right);
Value* vgen_memset(Allocator* alc, Value* on, Value* len, Value* with);
Value* vgen_this_but_return_that(Allocator* alc, Value* this, Value* that);
Value* vgen_bufferd_value(Allocator* alc, Parser* p, Value* val);
void buffer_values(Allocator* alc, Parser* p, Array* args, bool skip_last);

Value* vgen_multi(Allocator* alc, Array* values);
Value* vgen_phi(Allocator* alc, Value* left, Value* right);

struct Value {
    int type;
    void* item;
    Type* rett;
    Array *issets;
    Array *not_issets;
    Array *extra_values;
    char* ir_v;
};
struct MultiRett {
    Array* types;
    Array* decls;
};
struct VPair {
    Value* left;
    Value* right;
};
struct VDeclVal {
    Decl* decl;
    Value* value;
};
struct VFuncPtr {
    Func* func;
    Value* first_arg;
};
struct VFuncCall {
    Value *on;
    Array *args;
    Array *rett_refs;
    int line;
    int col;
};
struct ErrorHandler {
    int type;
    Value *on;
    Scope *err_scope;
    Value *err_value;
    Decl* err_decl;
    Array *phi_s;
};
struct VNumber {
    v_u64 value_uint;
    double value_float;
    bool negative;
};
struct VGcBuffer {
    VVar* result;
    Scope* scope;
    Value* on;
};
struct VClassInit {
    Value* item;
    Map* prop_values;
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
struct VPtrOffset {
    Value* on;
    Value* index;
    int size;
};
struct VOp {
    Value* left;
    Value* right;
    int op;
};
struct VIncr {
    Value* on;
    bool increment;
    bool before;
};
struct VIRCached {
    Value* value;
    char* ir_value;
    char* ir_var;
    bool used;
};
struct VString {
    char* body;
    char* ir_object_name;
    char* ir_body_name;
};
struct VScope {
    Scope* scope;
    Array* phi_values;
};
struct VPhiValue {
    Value* value;
    IRBlock* block;
    char* var;
};
struct VVar {
    Value* value;
    char* var;
};
struct VThisOrThat {
    Value* cond;
    Value* v1;
    Value* v2;
};
struct VError {
    Array* err_names;
    Array* err_values;
    Decl* decl;
    bool has_unknown;
};
struct VMemset {
    Value* on;
    Value* length;
    Value* with;
};
struct VMemcpy {
    Value* from;
    Value* to;
    Value* length;
};
struct VThisButThat {
    Value* this;
    Value* that;
};
struct VBufferd {
    Decl* decl;
    Value* value;
};

#endif

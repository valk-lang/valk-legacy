
#include "../all.h"

Value *value_make(Allocator *alc, int type, void *item, Type* rett) {
    Value *v = al(alc, sizeof(Value));
    v->type = type;
    v->item = item;
    v->rett = rett;
    return v;
}

Value *vgen_func_ptr(Allocator *alc, Func *func, Value *first_arg) {
    VFuncPtr *item = al(alc, sizeof(VFuncPtr));
    item->func = func;
    item->first_arg = first_arg;
    return value_make(alc, v_func_ptr, item, type_gen_func(alc, func));
}

Value *vgen_func_call(Allocator *alc, Value *on, Array *args) {
    VFuncCall *item = al(alc, sizeof(VFuncCall));
    item->on = on;
    item->args = args;
    item->err_scope = NULL;
    item->err_value = NULL;
    return value_make(alc, v_func_call, item, on->rett->func_rett);
}

Value *vgen_int(Allocator *alc, long int value, Type *type) {
    VNumber *item = al(alc, sizeof(VNumber));
    item->value_int = value;
    return value_make(alc, v_number, item, type);
}
Value *vgen_float(Allocator *alc, double value, Type *type) {
    VNumber *item = al(alc, sizeof(VNumber));
    item->value_float = value;
    return value_make(alc, v_number, item, type);
}

Value *vgen_class_pa(Allocator *alc, Value *on, ClassProp *prop) {
    VClassPA *item = al(alc, sizeof(VClassPA));
    item->on = on;
    item->prop = prop;
    return value_make(alc, v_class_pa, item, prop->type);
}

Value *vgen_ptrv(Allocator *alc, Build* b, Value *on, Type* type, Value* index) {
    if(!index) {
        index = vgen_int(alc, 0, type_gen_volt(alc, b, "int"));
    }
    VPtrv *item = al(alc, sizeof(VPtrv));
    item->on = on;
    item->type = type;
    item->index = index;
    return value_make(alc, v_ptrv, item, type);
}

Value *vgen_op(Allocator *alc, int op, Value *left, Value* right, Type *rett) {
    VOp *item = al(alc, sizeof(VOp));
    item->left = left;
    item->right = right;
    item->op = op;
    return value_make(alc, v_op, item, rett);
}

Value *vgen_comp(Allocator *alc, int op, Value *left, Value* right, Type *rett) {
    VOp *item = al(alc, sizeof(VOp));
    item->left = left;
    item->right = right;
    item->op = op;
    return value_make(alc, v_compare, item, rett);
}

Value *vgen_cast(Allocator *alc, Value *val, Type *to_type) {
    return value_make(alc, v_cast, val, to_type);
}

Value* vgen_call_alloc(Allocator* alc, Build* b, int size, Class* cast_as) {
    Func *func = get_volt_func(b, "mem", "alloc");
    Value *fptr = vgen_func_ptr(alc, func, NULL);
    Array *alloc_values = array_make(alc, func->args->values->length);
    Value *vint = vgen_int(alc, size, type_gen_volt(alc, b, "uint"));
    array_push(alloc_values, vint);
    Value *res = vgen_func_call(alc, fptr, alloc_values);
    if(cast_as)
        res = vgen_cast(alc, res, type_gen_class(alc, cast_as));
    return res;
}
Value* vgen_call_gc_alloc(Allocator* alc, Build* b, int size, int gc_fields, Class* cast_as) {
    Func *func = get_volt_func(b, "mem", "gc_class_alloc");
    Value *fptr = vgen_func_ptr(alc, func, NULL);
    Array *alloc_values = array_make(alc, func->args->values->length);
    Value *v_size = vgen_int(alc, size, type_gen_volt(alc, b, "u32"));
    array_push(alloc_values, v_size);
    Value *v_fields = vgen_int(alc, gc_fields, type_gen_volt(alc, b, "u8"));
    array_push(alloc_values, v_fields);
    Value *res = vgen_func_call(alc, fptr, alloc_values);
    if(cast_as)
        res = vgen_cast(alc, res, type_gen_class(alc, cast_as));
    return res;
}
Value* vgen_call_gc_link(Allocator* alc, Build* b, Value* left, Value* right) {
    Func *func = get_volt_class_func(b, "mem", "Stack", "link");
    Value *fptr = vgen_func_ptr(alc, func, NULL);
    Array *alloc_values = array_make(alc, func->args->values->length);
    array_push(alloc_values, left);
    array_push(alloc_values, right);
    Value *res = vgen_func_call(alc, fptr, alloc_values);
    res = vgen_cast(alc, res, left->rett);
    return res;
}

Value* vgen_incr(Allocator* alc, Build* b, Value* on, bool increment, bool before) {
    VIncr *item = al(alc, sizeof(VIncr));
    item->on = on;
    item->increment = increment;
    item->before = before;
    return value_make(alc, v_incr, item, on->rett);
}
Value* vgen_ir_cached(Allocator* alc, Value* value) {
    VIRCached *item = al(alc, sizeof(VIRCached));
    item->value = value;
    item->ir_value = NULL;
    item->ir_var = NULL;
    return value_make(alc, v_ir_cached, item, value->rett);
}

Value* vgen_null(Allocator* alc, Build* b) {
    return value_make(alc, v_null, NULL, type_gen_null(alc, b));
}

Value* vgen_gc_link(Allocator* alc, Value* on, Value* to, Type* rett) {
    VPair* item = al(alc, sizeof(VPair));
    item->left = on;
    item->right = to;
    return value_make(alc, v_gc_link, item, rett);
}


Value* vgen_gc_buffer(Allocator* alc, Build* b, Scope* scope, Value* val, Array* args) {
    VGcBuffer *buf = al(alc, sizeof(VGcBuffer));
    Scope *before = scope_sub_make(alc, sc_default, scope, scope->chunk_end);
    Scope *after = scope_sub_make(alc, sc_default, scope, scope->chunk_end);
    buf->value = val;
    buf->before = before;
    buf->after = after;
    before->ast = array_make(alc, args->length);
    after->ast = array_make(alc, args->length);

    for (int i = 0; i < args->length; i++) {
        Value* val = array_get_index(args, i);
        if(!value_needs_gc_buffer(val))
            continue;
        // Create temp decl
        Decl *decl = decl_make(alc, val->rett, false);
        array_push(before->ast, tgen_declare(alc, before, decl, val));
        // Replace args
        Value *new_value = value_make(alc, v_decl, decl, decl->type);
        array_set_index(args, i, new_value);
        // Remove from stack
        // array_push(after->ast, tgen_assign(alc, new_value, vgen_null(alc, b)));
    }
    return value_make(alc, v_gc_buffer, buf, val->rett);
}

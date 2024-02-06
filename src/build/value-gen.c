
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
    return value_make(alc, v_func_call, item, on->rett->func_rett);
}

Value *vgen_int(Allocator *alc, long int value, Type *type) {
    VInt *item = al(alc, sizeof(VInt));
    item->value = value;
    return value_make(alc, v_int, item, type);
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

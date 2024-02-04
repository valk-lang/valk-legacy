
#include "../all.h"

Value *value_make(Allocator *alc, int type, void *item, Type* rett) {
    Value *v = al(alc, sizeof(Token));
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

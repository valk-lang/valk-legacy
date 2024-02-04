
#include "../all.h"

Token *value_make(Allocator *alc, int type, void *item) {
    Token *t = al(alc, sizeof(Token));
    t->type = type;
    t->item = item;
    return t;
}

Value *vgen_func_ptr(Allocator *alc, Func *func, Value *first_arg) {
    VFuncPtr *item = al(alc, sizeof(VFuncPtr));
    item->func = func;
    item->first_arg = first_arg;
    return value_init(alc, v_func_ptr, item, type_gen_func(alc, func));
}

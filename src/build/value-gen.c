
#include "../all.h"

Value *value_make(Allocator *alc, int type, void *item, Type* rett) {
    Value *v = al(alc, sizeof(Value));
    v->type = type;
    v->item = item;
    v->rett = rett;
    v->issets = NULL;
    v->mrett = NULL;
    return v;
}

Value* vgen_bool(Allocator *alc, Build* b, bool value) {
    return vgen_int(alc, value, type_gen_valk(alc, b, "bool"));
}

Value *vgen_func_ptr(Allocator *alc, Func *func, Value *first_arg) {
    VFuncPtr *item = al(alc, sizeof(VFuncPtr));
    item->func = func;
    item->first_arg = first_arg;
    return value_make(alc, v_func_ptr, item, type_gen_func(alc, func));
}

Value *vgen_func_call(Allocator *alc, Parser* p, Value *on, Array *args) {
    Build* b = p->b;
    VFuncCall *item = al(alc, sizeof(VFuncCall));
    item->on = on;
    item->args = args;
    item->rett_refs = NULL;
    item->errh = NULL;
    TypeFuncInfo* fi = on->rett->func_info;
    Value* v = value_make(alc, v_func_call, item, fi->rett);
    Array* types = fi->rett_types;
    if(types && types->length > 1) {
        MultiRett* mr = al(alc, sizeof(MultiRett));
        mr->types = types;
        mr->decls = array_make(alc, types->length);
        for(int i = 0; types->length; i++) {
            Type* type = array_get_index(types, i);
            Decl* decl = decl_make(alc, NULL, type, false);
            decl->is_mut = true;
            array_push(mr->decls, decl);
        }
        v->mrett = mr;
        v->rett = NULL;
    }
    return v;
}

Value *vgen_int(Allocator *alc, v_i64 value, Type *type) {
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
        index = vgen_int(alc, 0, type_gen_valk(alc, b, "int"));
    }
    VPtrv *item = al(alc, sizeof(VPtrv));
    item->on = on;
    item->type = type;
    item->index = index;
    return value_make(alc, v_ptrv, item, type);
}
Value *vgen_ptr_offset(Allocator *alc, Build* b, Value *on, Value* index, int size) {
    VPtrOffset *item = al(alc, sizeof(VPtrOffset));
    item->on = on;
    item->index = index;
    item->size = size;
    return value_make(alc, v_ptr_offset, item, type_gen_valk(alc, b, "ptr"));
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

Value* vgen_call_alloc(Allocator* alc, Parser* p, int size, Class* cast_as) {
    Build* b = p->b;
    Func *func = get_valk_func(b, "mem", "alloc");
    Value *fptr = vgen_func_ptr(alc, func, NULL);
    Array *alloc_values = array_make(alc, func->args->values->length);
    Value *vint = vgen_int(alc, size, type_gen_valk(alc, b, "uint"));
    array_push(alloc_values, vint);
    Value *res = vgen_func_call(alc, p, fptr, alloc_values);
    if(cast_as)
        res = vgen_cast(alc, res, type_gen_class(alc, cast_as));
    return res;
}

Value* vgen_call_pool_alloc(Allocator* alc, Parser* p, Build* b, Class* class) {
    Global* pool = class->pool;
    Class* pc = pool->type->class;
    Value* pv = value_make(alc, v_global, pool, pool->type);

    Func *func = map_get(pc->funcs, "get");
    func_mark_used(p->func, func);

    Value *fptr = vgen_func_ptr(alc, func, NULL);
    Array *alloc_values = array_make(alc, func->args->values->length);
    array_push(alloc_values, pv);
    Value *res = vgen_func_call(alc, p, fptr, alloc_values);
    // res->rett = type_gen_class(alc, class);
    return res;
}

// Value* vgen_call_gc_link(Allocator* alc, Build* b, Value* left, Value* right) {
//     Func *func = get_valk_class_func(b, "mem", "Stack", "link");
//     Value *fptr = vgen_func_ptr(alc, func, NULL);
//     Array *alloc_values = array_make(alc, func->args->values->length);
//     array_push(alloc_values, left);
//     array_push(alloc_values, right);
//     Value *res = vgen_func_call(alc, b, fptr, alloc_values);
//     res = vgen_cast(alc, res, left->rett);
//     return res;
// }

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

Value* vgen_var(Allocator* alc, Build* b, Value* value) {
    VVar* item = al(alc, sizeof(VVar));
    item->value = value;
    item->var = NULL;
    return value_make(alc, v_var, item, value->rett);
}

Value* vgen_value_scope(Allocator* alc, Build* b, Scope* scope, Array* phi_values, Type* rett) {
    VScope* item = al(alc, sizeof(VScope));
    item->scope = scope;
    item->phi_values = phi_values;
    return value_make(alc, v_gc_link, item, rett);
}

Value* vgen_gc_buffer(Allocator* alc, Build* b, Scope* scope, Value* val, Array* args, bool store_on_stack) {
    bool contains_gc_values = false;
    for (int i = 0; i < args->length; i++) {
        Value* arg = array_get_index(args, i);
        if(value_needs_gc_buffer(arg)) {
            contains_gc_values = true;
            break;
        }
    }
    if(!contains_gc_values) {
        return val;
    }

    Scope *sub = scope_sub_make(alc, sc_default, scope);
    sub->ast = array_make(alc, 10);

    // Disable gc
    Global *g_disable = get_valk_global(b, "mem", "disable_gc");
    Value *disable = value_make(alc, v_global, g_disable, g_disable->type);
    Value *var_disable = vgen_var(alc, b, disable);
    if(contains_gc_values) {
        array_push(sub->ast, token_make(alc, t_set_var, var_disable->item));
        array_push(sub->ast, tgen_assign(alc, disable, vgen_bool(alc, b, true)));
    }

    // Buffer arguments
    for (int i = 0; i < args->length; i++) {
        Value* arg = array_get_index(args, i);
        Decl *decl = decl_make(alc, NULL, arg->rett, false);
        array_push(sub->ast, tgen_declare(alc, sub, decl, arg));
        arg = value_make(alc, v_decl, decl, decl->type);
        array_set_index(args, i, arg);
    }

    // Set disable_gc to previous value
    if(contains_gc_values) {
        array_push(sub->ast, tgen_assign(alc, disable, var_disable));
    }

    Value *var_result = vgen_var(alc, b, val);
    array_push(sub->ast, token_make(alc, t_set_var, var_result->item));

    VGcBuffer *buf = al(alc, sizeof(VGcBuffer));
    buf->result = var_result->item;
    buf->scope = sub;
    buf->on = val;

    return value_make(alc, v_gc_buffer, buf, val->rett);
}

Value *vgen_isset(Allocator *alc, Build *b, Value *on) {
    return value_make(alc, v_isset, on, type_gen_valk(alc, b, "bool"));
}

Value *vgen_and_or(Allocator *alc, Build *b, Value *left, Value *right, int op) {
    VOp *item = al(alc, sizeof(VOp));
    item->op = op;
    item->left = left;
    item->right = right;
    Type *rett = type_gen_valk(alc, b, "bool");

    Value *result = value_make(alc, v_and_or, item, rett);

    // merge issets when using '&&'
    if (op == op_and && (left->issets || right->issets)) {
        Array *issets = array_make(alc, 4);
        if (left->issets) {
            Array *prev = left->issets;
            for (int i = 0; i < prev->length; i++) {
                array_push(issets, array_get_index(prev, i));
            }
        }
        if (right->issets) {
            Array *prev = right->issets;
            for (int i = 0; i < prev->length; i++) {
                array_push(issets, array_get_index(prev, i));
            }
        }
        result->issets = issets;
    }

    return result;
}

Value *vgen_this_or_that(Allocator *alc, Value* cond, Value *v1, Value *v2, Type* rett) {
    VThisOrThat* item = al(alc, sizeof(VThisOrThat));
    item->cond = cond;
    item->v1 = v1;
    item->v2 = v2;
    return value_make(alc, v_this_or_that, item, rett);
}

Value *vgen_decl(Allocator *alc, Decl* decl) {
    return value_make(alc, v_decl, decl, decl->type);
}
Value *vgen_global(Allocator *alc, Global* g) {
    return value_make(alc, v_global, g, g->type);
}

Value *vgen_string(Allocator *alc, Unit *u, char *body) {
    Build *b = u->b;

    u->string_count++;
    char var[64];
    strcpy(var, "@.str.object.");
    itos(u->id, (char *)((intptr_t)var + 13), 10);
    strcat(var, "_");
    itos(u->string_count, (char *)((intptr_t)var + strlen(var)), 10);
    char *object_name = dups(b->alc, var);

    strcpy(var, "@.str.body.");
    itos(u->id, (char *)((intptr_t)var + 11), 10);
    strcat(var, "_");
    itos(u->string_count, (char *)((intptr_t)var + strlen(var)), 10);
    char *body_name = dups(b->alc, var);

    VString *str = al(b->alc, sizeof(VString));
    str->body = body;
    str->ir_object_name = object_name;
    str->ir_body_name = body_name;

    array_push(b->strings, str);

    return value_make(alc, v_string, str, type_gen_valk(alc, b, "String"));
}

Value* vgen_null_alt_value(Allocator* alc, Value* left, Value* right) {
    VPair* item = al(alc, sizeof(VPair));
    item->left = left;
    item->right = right;
    return value_make(alc, v_null_alt_value, item, right->rett);
}


#include "../all.h"

void unroll_func_start(Unroll* ur, Scope* scope, Array* unroll);
void unroll_func_defer(Unroll* ur, Scope* scope, Array* unroll);

void stage_unroll(Build *b, Func *func) {
    Scope* scope = func->scope;
    Allocator* alc = func->ast_alc;
    Unroll* ur = unroll_make(alc);
    ur->b = b;
    ur->func = func;
    ur->alc = alc;
    func->ur = ur;
}

void unroll_scope(Unroll* ur, Scope* scope) {

    Array* unroll = array_make(ur->alc, scope->ast->length + 1);

    if(scope->type == sc_func) {
        unroll_func_start(ur, scope, unroll);
    }

    Array* scope_ast = scope->ast;
    loop(scope_ast, i) {
        Token* t = array_get_index(scope_ast, i);
        if(t->type == t_return) {
            unroll_func_defer(ur, scope, unroll);
        }
        array_push(unroll, t);
    }

    if(!scope->did_return && scope->type == type_func) {
        unroll_func_defer(ur, scope, unroll);
    }

    scope->ast = unroll;
}

void unroll_func_start(Unroll* ur, Scope* scope, Array* unroll) {

    Func* func = ur->func;
    Build* b = ur->b;
    Allocator* alc = ur->alc;

    if (func == ur->b->func_main_gen || func->init_thread) {
        if (func == b->func_main_gen) {
            Func *f1 = get_valk_class_func(b, "mem", "GcManager", "init");
            Array* args = array_make(alc, 1);
            Value *v = vgen_func_call_unroll(alc, vgen_func_ptr(alc, f1, NULL), args);
            array_push(unroll, token_make(alc, t_statement, v));
        }
        if (func->init_thread) {
            Func *f1 = get_valk_class_func(b, "mem", "Stack", "init");
            Array* args = array_make(alc, 1);
            Value *v = vgen_func_call_unroll(alc, vgen_func_ptr(alc, f1, NULL), args);
            array_push(unroll, token_make(alc, t_statement, v));
        }
    }

    // Global* gs = get_valk_global(b, "mem", "stack");
    if(func->gc_decl_count > 0) {
        Global *gsp = get_valk_global(b, "mem", "stack_pos");
        Value *stack_pos = vgen_ir_cached(alc, vgen_class_pa(alc, vgen_global(alc, gsp), map_get(gsp->type->class->props, "adr")));
        func->v_cache_stack_pos = stack_pos;
        array_push(unroll, token_make(alc, t_statement, stack_pos));

        Value *ms = vgen_memset(alc, func->v_cache_stack_pos, vgen_int(alc, func->gc_decl_count * b->ptr_size, type_cache_uint(b)), vgen_int(alc, 0, type_cache_u8(b)));
        array_push(unroll, token_make(alc, t_statement, ms));

        // Stack reserve
        Value *amount = vgen_int(alc, func->gc_decl_count * b->ptr_size, type_cache_uint(b));
        Value *offset = vgen_ptr_offset(alc, b, func->v_cache_stack_pos, amount, 1);
        func->t_stack_incr = tgen_assign(alc, func->v_cache_stack_pos, offset);
        array_push(unroll, func->t_stack_incr);
    }

    if (func->alloca_size > 0) {
        Value *alloca = vgen_ir_cached(alc, vgen_stack_size(alc, b, func->alloca_size));
        array_push(unroll, token_make(alc, t_statement, alloca));
        func->v_cache_alloca = alloca;
    }

    // Set decls to null
    Array* decls = func->scope->decls;
    loop(decls, i) {
        Decl *decl = array_get_index(decls, i);
        // Set stack offset for variables
        if (decl->offset > -1) {
            if (decl->is_gc) {
                Value* offset = vgen_ptr_offset(alc, b, func->v_cache_stack_pos, vgen_int(alc, decl->offset, type_cache_u32(b)), b->ptr_size);
                array_push(unroll, tgen_decl_set_store(alc, decl, offset));
            } else {
                Value* offset = vgen_ptr_offset(alc, b, func->v_cache_alloca, vgen_int(alc, decl->offset, type_cache_u32(b)), 1);
                array_push(unroll, tgen_decl_set_store(alc, decl, offset));
            }
        }
        // Store args
        if (decl->is_arg) {
            array_push(unroll, tgen_decl_set_arg(alc, decl));
        // } else {
        //     // Set null
        //     if (decl->offset > -1 && decl->is_gc) {
        //         Value* vdecl = vgen_decl(alc, decl);
        //         array_push(start->ast, tgen_assign(alc, vdecl, vgen_null(alc, b)));
        //     }
        }
    }
}
void unroll_func_defer(Unroll* ur, Scope* scope, Array* unroll) {

    Func* func = ur->func;
    Build* b = ur->b;
    Allocator* alc = ur->alc;

    if (func->gc_decl_count > 0) {
        // Stack reduce
        Scope *end = scope_make(alc, sc_default, scope);
        end->ast = array_make(alc, 1);
        func->t_stack_decr = tgen_assign(alc, func->v_cache_stack_pos, func->v_cache_stack_pos);
        array_push(end->ast, func->t_stack_decr);
        func->ast_end = end;
    }
}

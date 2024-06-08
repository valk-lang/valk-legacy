
#include "../all.h"

void ast_func_start(Allocator* alc, Parser* p) {
    Build* b = p->b;
    Scope* scope = p->scope;
    Func* func = p->func;

    if (func == b->func_main_gen || func->init_thread) {
        Scope *init = scope_make(alc, sc_default, scope);
        init->ast = array_make(alc, 2);
        func->ast_stack_init = init;

        if (func == b->func_main_gen) {
            Func *f1 = get_valk_class_func(b, "mem", "GcManager", "init");
            Value *v = vgen_func_call(alc, p, vgen_func_ptr(alc, f1, NULL), array_make(alc, 1));
            array_push(init->ast, token_make(alc, t_statement, v));
        }
        if (func->init_thread) {
            Func *f1 = get_valk_class_func(b, "mem", "Stack", "init");
            Value *v = vgen_func_call(alc, p, vgen_func_ptr(alc, f1, NULL), array_make(alc, 1));
            array_push(init->ast, token_make(alc, t_statement, v));
        }
    }

    Scope *start = scope_make(alc, sc_default, scope);
    start->ast = array_make(alc, 2);
    array_push(scope->ast, token_make(alc, t_ast_scope, start));

    Global* gs = get_valk_global(b, "mem", "stack");
    Global* gsp = get_valk_global(b, "mem", "stack_pos");
    func->v_cache_stack_pos = vgen_ir_cached(alc, vgen_class_pa(alc, vgen_global(alc, gsp), map_get(gsp->type->class->props, "adr")));

    func->ast_start = start;
}

void ast_func_end(Allocator* alc, Parser* p) {
    Build* b = p->b;
    Scope* scope = p->scope;
    Func* func = p->func;
    Scope *start = func->ast_start;

    // Count gc decls
    int gc_decl_count = 0;
    int alloca_size = 0;
    Array *decls = scope->decls;
    for (int i = 0; i < decls->length; i++) {
        Decl *decl = array_get_index(decls, i);
        if (!decl->is_gc) {
            if (decl->is_mut) {
                decl->offset = alloca_size;
                alloca_size += type_get_size(b, decl->type);
                alloca_size += ((b->ptr_size - (alloca_size % b->ptr_size)) % b->ptr_size);
            }
            continue;
        }
        if (decl->is_mut || !decl->is_arg) {
            decl->offset = gc_decl_count++;
        }
    }
    func->alloca_size = alloca_size;

    //
    if (gc_decl_count > 0) {
        value_enable_cached(func->v_cache_stack_pos->item);
    }
    if(((VIRCached*)(func->v_cache_stack_pos->item))->used) {
        array_push(start->ast, token_make(alc, t_statement, func->v_cache_stack_pos));
    }

    if (func->calls_gc_check) {
        Scope *gcscope = gen_snippet_ast(alc, p, get_valk_snippet(b, "mem", "run_gc_check"), map_make(alc), scope);
        array_push(start->ast, token_make(alc, t_ast_scope, gcscope));
    }

    // Stack reserve & reduce
    if (gc_decl_count > 0) {
        // Stack reserve
        // Todo: we can skip this if our function doesnt call other functions
        Value *amount = vgen_int(alc, gc_decl_count * b->ptr_size, type_cache_uint(b));
        Value *offset = vgen_ptr_offset(alc, b, func->v_cache_stack_pos, amount, 1);
        func->t_stack_incr = tgen_assign(alc, func->v_cache_stack_pos, offset);
        array_push(start->ast, func->t_stack_incr);

        // Stack reduce
        // Todo: we can skip this if our function doesnt call other functions
        Scope *end = scope_make(alc, sc_default, scope);
        end->ast = array_make(alc, 1);
        func->t_stack_decr = tgen_assign(alc, func->v_cache_stack_pos, func->v_cache_stack_pos);
        array_push(end->ast, func->t_stack_decr);
        func->ast_end = end;
    }

    Value* alloca = NULL;
    if (alloca_size > 0) {
        alloca = vgen_ir_cached(alc, vgen_stack_size(alc, b, alloca_size));
        array_push(start->ast, token_make(alc, t_statement, alloca));
        func->v_cache_alloca = alloca;
    }

    for (int i = 0; i < decls->length; i++) {
        Decl *decl = array_get_index(decls, i);
        // Set stack offset for variables
        if (decl->offset > -1) {
            if (decl->is_gc) {
                Value* offset = vgen_ptr_offset(alc, b, func->v_cache_stack_pos, vgen_int(alc, decl->offset, type_cache_u32(b)), b->ptr_size);
                array_push(start->ast, tgen_decl_set_store(alc, decl, offset));
            } else {
                Value* offset = vgen_ptr_offset(alc, b, alloca, vgen_int(alc, decl->offset, type_cache_u32(b)), 1);
                array_push(start->ast, tgen_decl_set_store(alc, decl, offset));
            }
        }
        // Store args
        if (decl->is_arg) {
            array_push(start->ast, tgen_decl_set_arg(alc, decl));
        } else {
            // Set null
            if (decl->offset > -1 && decl->is_gc) {
                Value* vdecl = vgen_decl(alc, decl);
                array_push(start->ast, tgen_assign(alc, vdecl, vgen_null(alc, b)));
            }
        }
    }
}
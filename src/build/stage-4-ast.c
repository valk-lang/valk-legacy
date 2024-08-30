
#include "../all.h"

void stage_init_start_functions(Build *b);
Func* stage_generate_set_globals(Build *b);
Func* stage_generate_tests(Build *b);
void stage_generate_main(Build *b);

void stage_unroll(Build *b, Func *func);
void stage_ast_func(Func *func);
void stage_ast_class(Class *class);

void stage_ast(Build *b, void *payload) {

    // Init IR
    Array *units = b->units;
    loop(units, i) {
        Unit *u = array_get_index(units, i);
        u->ir = ir_make(u);
    }

    stage_init_start_functions(b);
    b->building_ast = true;

    // Generate test functions
    if(b->is_test)
        stage_generate_tests(b);

    // Parse AST for used functions
    if (b->func_main)
        stage_ast_func(b->func_main);
    if (b->func_main_tests)
        stage_ast_func(b->func_main_tests);

    stage_ast_func(get_valk_class_func(b, "mem", "Stack", "init"));
    stage_ast_func(get_valk_class_func(b, "mem", "Stack", "link"));
    stage_ast_func(get_valk_class_func(b, "mem", "GcManager", "init"));
    stage_ast_func(get_valk_class_func(b, "core", "Coro", "new"));
    stage_ast_func(get_valk_func(b, "mem", "update_usage"));

    // Generate main and then globals
    stage_generate_main(b);
    stage_generate_set_globals(b);
    stage_ast_func(b->func_main_gen);
    stage_ast_func(b->func_set_globals);

    loop(units, i) {
        Unit *u = array_get_index(units, i);
        loop(u->classes, o) {
            Class* class = array_get_index(u->classes, o);
            if(!class->has_vtable)
                continue;

            Map *funcs = class->funcs;
            Func *hook_transfer = map_get(funcs, "_v_transfer");
            Func *hook_mark = map_get(funcs, "_v_mark");
            Func *hook_mark_shared = map_get(funcs, "_v_mark_shared");
            Func *hook_share = map_get(funcs, "_v_share");
            Func *hook_free = map_get(funcs, "_v_free");
            if (hook_transfer)
                stage_ast_func(hook_transfer);
            if (hook_mark)
                stage_ast_func(hook_mark);
            if (hook_mark_shared)
                stage_ast_func(hook_mark_shared);
            if (hook_share)
                stage_ast_func(hook_share);
            if (hook_free)
                stage_ast_func(hook_free);
        }

        ir_gen_globals(u->ir);
    }

    // Parse all parse-last functions
    b->parse_last = true;

    stage_ast_func(get_valk_func(b, "mem", "init_pools"));

    Array *funcs = b->parse_later;
    loop(funcs, i) {
        Func *func = array_get_index(funcs, i);
        func->parsed = false;
        stage_ast_func(func);
    }

    b->building_ast = false;
    b->parse_last = false;

    // Gen IR
    loop(units, i) {
        Unit *u = array_get_index(units, i);

        // Parse functions from main package, just for validation
        if (u->nsc->pkc == b->pkc_main) {
            Array *funcs = u->funcs;
            loop(funcs, o) {
                Func *func = array_get_index(funcs, o);
                stage_ast_func(func);
            }
        }

        if (b->verbose > 2)
            printf("Stage 4 | Generate IR: %s\n", u->nsc->name);

        usize start = microtime();
        ir_gen_final(u->ir);
        b->time_ir += microtime() - start;
    }
}

void stage_ast_func(Func *func) {

    if (func->parsed || func->in_header)
        return;
    func->parsed = true;
    func->is_used = func->b->building_ast;

    Unit* u = func->unit;
    Build* b = u->b;
    Parser* p = u->parser;

    if(func->parse_last && !b->parse_last) {
        array_push(b->parse_later, func);
        return;
    }

    Allocator *alc = alc_make();
    func->ast_alc = alc;

    if (u->b->verbose > 2)
        printf("Stage 4 | Parse AST: %s\n", func->export_name);

    // Parse function code
    usize start = microtime();

    if (func->chunk_body) {
        *p->chunk = *func->chunk_body;
        p->func = func;
        p->scope = func->scope;
        p->loop_scope = NULL;
        p->vscope_values = NULL;
        read_ast(p, false);
    }

    if (p->cc_index > 0) {
        parse_err(p, -1, "Missing #end token");
    }

    b->time_parse += microtime() - start;

    // Generate AST for sub functions
    if(func->class && !func->class->is_used) {
        stage_ast_class(func->class);
    }
    Array* used = func->used_functions;
    loop(used, i) {
        Func* f = array_get_index(used, i);
        stage_ast_func(f);
    }
    Array *classes = func->used_classes;
    loop(classes, i) {
        Class* class = array_get_index(classes, i);
        stage_ast_class(class);
    }
    Array* called = func->called_functions;
    loop(called, i) {
        Func* f = array_get_index(called, i);
        if(f->can_create_objects) {
            func->can_create_objects = true;
            break;
        }
    }

    if (func->b->building_ast) {
        // Unroll AST
        stage_unroll(b, func);

        // Generate IR
        start = microtime();
        ir_gen_ir_for_func(u->ir, func);
        b->time_ir += microtime() - start;
    }

    // 
    func->ast_alc = NULL;
    alc_delete(alc);
}
void stage_ast_class(Class *class) {
    if (class->is_used)
        return;
    class->is_used = true;
    Array *funcs = class->funcs->values;
    loop(funcs, i) {
        Func *cf = array_get_index(funcs, i);
        if (cf->use_if_class_is_used) {
            stage_ast_func(cf);
        }
    }
}


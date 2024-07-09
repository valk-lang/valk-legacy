
#include "../all.h"

void stage_init_start_functions(Build *b);
Func* stage_generate_set_globals(Build *b);
Func* stage_generate_tests(Build *b);
void stage_generate_main(Build *b);

void stage_ast_func(Func *func);
void stage_ast_class(Class *class);
void ir_vtable_define_extern(Unit* u);

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

    // Generate main and then globals
    stage_generate_main(b);
    stage_ast_func(b->func_main_gen);
    stage_generate_set_globals(b);
    stage_ast_func(b->func_set_globals);

    loop(units, i) {
        Unit *u = array_get_index(units, i);
        ir_gen_globals(u->ir);
    }

    // Parse all parse-last functions
    b->parse_last = true;

    stage_ast_func(get_valk_func(b, "mem", "pools_init"));

    Array *funcs = b->parse_later;
    loop(funcs, i) {
        Func *func = array_get_index(funcs, i);
        func->parsed = false;
        stage_ast_func(func);
    }

    b->building_ast = false;
    b->parse_last = false;

    // Define extern vtable
    Unit *um = b->nsc_main->unit;
    ir_vtable_define_extern(um);

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

    // Generate IR
    if(func->b->building_ast) {
        start = microtime();
        ir_gen_ir_for_func(u->ir, func);
        b->time_ir += microtime() - start;
    }

    // Clear AST allocator
    Allocator *alc = func->b->alc_ast;
    alc_wipe(alc);

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


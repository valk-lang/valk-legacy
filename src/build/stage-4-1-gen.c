
#include "../all.h"

void stage_init_start_functions(Build *b) {

    Unit* u = array_get_index(b->units, 0);
    Scope* scope = scope_make(b->alc, sc_default, NULL);

    // Main
    Func* func = func_make(b->alc, u, scope, "main", "main");
    b->func_main_gen = func;
    func->init_thread = true;

    // Set globals
    func = func_make(b->alc, u, scope, "valk_init_globals", "valk_init_globals");
    b->func_set_globals = func;
    func->scope->ast = array_make(b->alc, 20);

    // Generate main function
    func = func_make(b->alc, u, scope, "valk_tests", "valk_tests");
    b->func_main_tests = func;

}

void stage_generate_main(Build *b) {
    //

    // Generate main function
    Func* func = b->func_main_gen;
    if(!func)
        return;

    Idf *idf = idf_make(b->alc, idf_class, get_valk_class(b, "core", "Coro"));
    scope_set_idf(func->scope, "CORO_CLASS", idf, NULL);

    idf = idf_make(b->alc, idf_scope, get_valk_nsc(b, "mem")->scope);
    scope_set_idf(func->scope, "mem", idf, NULL);

    idf = idf_make(b->alc, idf_func, b->func_set_globals);
    scope_set_idf(func->scope, "VALK_INIT_GLOBALS", idf, NULL);

    // Generate main AST
    Str* code = b->str_buf;
    str_clear(code);

    str_flat(code, "(argc: i32, argv: ptr) i32 {\n\n");

    str_flat(code, "VALK_INIT_GLOBALS();\n\n");

    // CLI args
    str_flat(code, "let arr = Array[String].new(10);\n");
    str_flat(code, "let i = 0\n");
    str_flat(code, "while i < argc {\n");
    str_flat(code, "  let cstr = @ptrv(argv, cstring, i)\n");
    str_flat(code, "  arr.push(cstr)\n");
    str_flat(code, "  i++\n");
    str_flat(code, "}\n\n");

    Nsc* nsc_fs = map_get(b->pkc_valk->namespaces, "fs");
    if(b->target_os == os_macos && nsc_fs) {
        Idf *idf = idf_make(b->alc, idf_scope, nsc_fs->scope);
        scope_set_idf(func->scope, "FS_NSC", idf, NULL);
        str_flat(code, "FS_NSC:FIRST_ARG = arr.get(0) ? null @as String\n");
    }

    if (b->is_test) {
        int count = 0;
        char* buf = b->char_buf;

        Idf *idf = idf_make(b->alc, idf_func, b->func_main_tests);
        scope_set_idf(func->scope, "VALK_TEST_MAIN", idf, NULL);

        str_flat(code, "co VALK_TEST_MAIN()\n");
        str_flat(code, "CORO_CLASS.loop()\n");
        str_flat(code, "return 0;\n");

    } else {
        Func* mainfunc = b->func_main;
        if(!mainfunc) {
            build_err(b, "Missing 'main' function");
        }
        bool main_has_return = mainfunc->rett_types->length > 0;
        bool main_has_arg = mainfunc->rett_types->length > 0;

        Idf *idf = idf_make(b->alc, idf_func, mainfunc);
        scope_set_idf(func->scope, "CODE_MAIN", idf, NULL);

        if (main_has_return)
            str_flat(code, "let main_res = ");
        str_flat(code, "co CODE_MAIN(");
        if (main_has_arg) {
            str_flat(code, "arr");
        }
        str_flat(code, ");\n");

        str_flat(code, "CORO_CLASS.loop()\n");

        if (main_has_return)
            str_flat(code, "return await main_res\n");
        else
            str_flat(code, "return 0\n");
    }

    str_flat(code, "}\n");

    char* content = str_to_chars(b->alc, code);
    Chunk *chunk = chunk_make(b->alc, b, NULL);
    chunk_set_content(b, chunk, content, code->length);

    Parser *p = func->unit->parser;
    *p->chunk = *chunk;
    p->scope = func->scope->parent;
    parse_handle_func_args(p, func);
    stage_types_func(p, func);
}

Func* stage_generate_set_globals(Build *b) {
    //
    Array* all_globals = b->globals;
    Array* globals = array_make(b->alc, all_globals->length + 2);
    loop(all_globals, i) {
        Global* g = array_get_index(all_globals, i);
        if(g->is_used) {
            array_push(globals, g);
        }
    }
    b->used_globals = globals;

    //
    Func* func = b->func_set_globals;

    int count = 0;
    char* buf = b->char_buf;

    // Generate main AST
    Scope* scope = func->scope;
    Parser* p = func->unit->parser;
    p->func = func;

    loop(globals, i) {
        Global* g = array_get_index(globals, i);
        if(!g->chunk_value) {
            if(g->type->is_pointer && !g->type->nullable && !g->type->ignore_null) {
                *p->chunk = *g->chunk_type;
                parse_err(p, -1, "Missing global default value");
            }
            continue;
        }

        *p->chunk = *g->chunk_value;
        p->scope = g->declared_scope;
        Value* v = read_value(b->alc, p, true, 0);

        type_check(p, g->type, v->rett);

        Token* t = tgen_assign(b->alc, vgen_global(b->alc, g), v);
        array_push(scope->ast, t);
    }

    return func;
}

Func* stage_generate_tests(Build *b) {

    Func* func = b->func_main_tests;

    int count = 0;
    char* buf = b->char_buf;

    Idf* idf = idf_make(b->alc, idf_func, get_valk_func(b, "core", "test_init"));
    scope_set_idf(func->scope, "VALK_TEST_INIT", idf, NULL);
    idf = idf_make(b->alc, idf_func, get_valk_func(b, "core", "test_result"));
    scope_set_idf(func->scope, "VALK_TEST_RESULT", idf, NULL);
    idf = idf_make(b->alc, idf_func, get_valk_func(b, "core", "test_final_result"));
    scope_set_idf(func->scope, "VALK_TEST_FINAL_RESULT", idf, NULL);

    // Generate code
    Str* code = b->str_buf;
    str_clear(code);

    str_flat(code, "() {\n");

    // Init test result
    str_flat(code, "let result = VALK_TEST_INIT()\n");
    // Call tests
    Array* units = b->units;
    loop(units, i) {
        Unit* u = array_get_index(units, i);
        Array* tests = u->tests;
        loop(tests, o) {
            Test* t = array_get_index(tests, o);
            Func* tf = t->func;
            // Set name
            sprintf(buf, "VALK_TEST_NAME_%d", count);
            char* name_idf = dups(b->alc, buf);
            Idf *idf = idf_make(b->alc, idf_value, vgen_string(b->alc, u, t->name));
            scope_set_idf(func->scope, name_idf, idf, NULL);
            str_flat(code, "result.reset(");
            str_add(code, name_idf);
            str_flat(code, ")\n");
            // Call test
            sprintf(buf, "VALK_TEST_FUNC_%d", count);
            char* func_idf_name = dups(b->alc, buf);
            idf = idf_make(b->alc, idf_func, tf);
            scope_set_idf(func->scope, func_idf_name, idf, NULL);
            str_add(code, func_idf_name);
            str_flat(code, "(result)\n");
            // Result
            str_flat(code, "VALK_TEST_RESULT(result)\n");
            //
            count++;
        }
    }
    // Result
    str_flat(code, "VALK_TEST_FINAL_RESULT(result)\n");

    str_flat(code, "}\n");

    char* content = str_to_chars(b->alc, code);
    Chunk *chunk = chunk_make(b->alc, b, NULL);
    chunk_set_content(b, chunk, content, code->length);

    Parser *p = func->unit->parser;
    *p->chunk = *chunk;
    p->scope = func->scope->parent;
    parse_handle_func_args(p, func);
    stage_types_func(p, func);

    return func;
}

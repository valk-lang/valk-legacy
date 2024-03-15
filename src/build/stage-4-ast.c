
#include "../all.h"

void stage_ast(Unit *u);
void stage_generate_main(Unit *u);

void stage_4_ast(Unit *u) {

    Build *b = u->b;

    if(u->contains_main_func)
        return;

    usize start = microtime();

    stage_ast(u);

    b->time_parse += microtime() - start;

    stage_4_ir(u);
}

void stage_4_ast_main(Unit *u) {

    Build *b = u->b;

    usize start = microtime();
    stage_ast(u);
    stage_generate_main(u);
    b->time_parse += microtime() - start;

    stage_4_ir(u);
}

void stage_ast(Unit *u) {
    Build* b = u->b;
    Parser *p = b->parser;
    p->unit = u;
    //
    Array *funcs = u->funcs;
    for (int i = 0; i < funcs->length; i++) {

        Func *func = array_get_index(funcs, i);

        if (func->in_header)
            continue;

        if (u->b->verbose > 2)
            printf("Stage 4 | Parse AST: %s\n", func->name);

        *p->chunk = *func->chunk_body;
        p->func = func;
        p->scope = func->scope;
        p->loop_scope = NULL;
        read_ast(p, false);
    }

    p->unit = NULL;
}

void read_ast(Parser *p, bool single_line) {
    //
    Build* b = p->b;
    Allocator *alc = b->alc_ast;
    Scope* scope = p->scope;

    if (!scope->ast)
        scope->ast = array_make(alc, 50);

    bool first = false;

    while (true) {

        if (single_line && first)
            return;

        int before_i = p->chunk->i;
        char t = tok(p, true, true, true);
        char* tkn = p->tkn;

        if (tkn[0] == ';')
            continue;
        if (t == tok_curly_close)
            break;

        first = true;
        //

        if (t == tok_id) {
            if (str_is(tkn, "let")){
                t = tok(p, true, false, true);
                char* name = p->tkn;
                if(t != tok_id) {
                    parse_err(p, -1, "Invalid variable name: '%s'", name);
                }
                t = tok(p, true, false, true);
                Type* type = NULL;
                if(t == tok_colon) {
                    type = read_type(p, alc, true);
                    t = tok(p, true, false, true);
                }
                if(t != tok_eq) {
                    parse_err(p, -1, "Expected '=' here, found: '%s'", p->tkn);
                }

                Value* val = read_value(alc, p, true, 0);
                if(type) {
                    val = try_convert(alc, b, p->scope, val, type);
                    type_check(p, type, val->rett);
                } else {
                    type = val->rett;
                }

                Decl* decl = decl_make(alc, name, type, false);
                Idf *idf = idf_make(b->alc, idf_decl, decl);
                scope_set_idf(scope, name, idf, p);

                array_push(scope->ast, tgen_declare(alc, scope, decl, val));
                continue;
            }
            if (str_is(tkn, "if")){
                token_if(alc, p);
                continue;
            }
            if (str_is(tkn, "while")){
                token_while(alc, p);
                continue;
            }
            if (str_is(tkn, "break") || str_is(tkn, "continue")){
                if(!p->loop_scope) {
                    parse_err(p, -1, "Using 'break' without being inside a loop");
                }
                array_push(scope->ast, token_make(alc, str_is(tkn, "break") ? t_break : t_continue, p->loop_scope));
                scope->did_return = true;
                break;
            }
            if (str_is(tkn, "return")){
                Value* val = NULL;
                if(scope->rett && !type_is_void(scope->rett)) {
                    val = read_value(alc, p, false, 0);
                    val = try_convert(alc, b, p->scope, val, scope->rett);
                    type_check(p, scope->rett, val->rett);
                } else {
                    char t = tok(p, true, false, true);
                    if(t != tok_none && t != tok_semi && t != tok_curly_close) {
                        parse_err(p, -1, "Return statement should not return a value if the function has a 'void' return type");
                    }
                }
                if(val) {
                    // Important for GC
                    val = vgen_var(alc, b, val);
                    array_push(scope->ast, token_make(alc, t_set_var, val->item));
                }

                array_push(scope->ast, tgen_return(alc, val));
                scope->did_return = true;
                break;
            }
            if (str_is(tkn, "throw")){
                char t = tok(p, true, false, true);
                if(t != tok_id) {
                    parse_err(p, -1, "Invalid error name: '%s'", p->tkn);
                }
                char* name = p->tkn;
                Func* func = p->func;
                Scope* fscope = func->scope;
                FuncError* err = NULL;
                if (func->errors) {
                    err = map_get(func->errors, name);
                }
                if(!err) {
                    parse_err(p, -1, "Function has no error defined named: '%s'", name);
                }

                array_push(scope->ast, tgen_throw(alc, b, err, name));
                scope->did_return = true;
                break;
            }
        }
        if (t == tok_at_word) {
            if (str_is(tkn, "@cache_value")){
                tok_expect(p, "(", false, false);
                Value* v = read_value(alc, p, true, 0);
                tok_expect(p, ")", true, true);
                tok_expect(p, "as", true, false);
                t = tok(p, true, false, true);
                if(t != tok_id) {
                    parse_err(p, -1, "Invalid variable name: '%s'", p->tkn);
                }
                char* name = p->tkn;
                Value* vc = vgen_ir_cached(alc, v);
                Idf* idf = idf_make(alc, idf_cached_value, vc);
                scope_set_idf(scope, name, idf, p);
                array_push(scope->ast, token_make(alc, t_statement, vc));
                continue;
            }
        }

        p->chunk->i = before_i;

        Value* left = read_value(alc, p, true, 0);

        t = tok(p, true, false, false);
        if (t >= tok_eq && t <= tok_div_eq) {
            tok(p, true, false, true);
            if (!value_is_assignable(left)) {
                parse_err(p, -1, "Cannot assign to left side");
            }
            value_is_mutable(left);

            Value *right = read_value(alc, p, true, 0);
            if (type_is_void(right->rett)) {
                parse_err(p, -1, "Trying to assign a void value");
            }

            int op = op_eq;
            if (t == tok_eq) {
            } else if (t == tok_plus_eq) {
                op = op_add;
            } else if (t == tok_sub_eq) {
                op = op_sub;
            } else if (t == tok_mul_eq) {
                op = op_mul;
            } else if (t == tok_div_eq) {
                op = op_div;
            }
            if (op != op_eq) {
                right = value_handle_op(alc, p, left, right, op);
            }

            right = try_convert(alc, b, p->scope, right, left->rett);
            type_check(p, left->rett, right->rett);

            array_push(scope->ast, tgen_assign(alc, left, right));
            continue;
        }

        //
        array_push(scope->ast, token_make(alc, t_statement, left));
    }

    if(scope->must_return && !scope->did_return) {
        parse_err(p, -1, "Missing return statement");
    }

    Scope* start;
    Scope* end;
    bool is_main_func_scope = p->func == b->func_main_gen && scope->type == sc_func;
    if (scope->has_gc_decls || scope->gc_check || is_main_func_scope) {
        // Start scope
        start = scope_sub_make(alc, sc_default, scope);
        start->ast = array_make(alc, 10);
        array_shift(scope->ast, token_make(alc, t_ast_scope, start));
        // End scope
        int last_token_index = scope->ast->length - 1;
        end = scope_sub_make(alc, sc_default, scope);
        end->ast = array_make(alc, 10);
        array_push(scope->ast, token_make(alc, t_ast_scope, end));
        // Swap return/continue/break token & end-scope
        if(scope->did_return) {
            int last_index = scope->ast->length - 1;
            void* a = array_get_index(scope->ast, last_token_index);
            void* b = array_get_index(scope->ast, last_index);
            array_set_index(scope->ast, last_token_index, b);
            array_set_index(scope->ast, last_index, a);
        }
    }

    if(is_main_func_scope) {
        Scope *boot = gen_snippet_ast(alc, p, get_volt_snippet(b, "mem", "boot"), map_make(alc), scope);
        array_push(start->ast, token_make(alc, t_ast_scope, boot));
    }

    if(scope->gc_check && (scope->type == sc_loop || scope->type == sc_func)){
        Scope *gcscope = gen_snippet_ast(alc, p, get_volt_snippet(b, "mem", "run_gc_check"), map_make(alc), scope);
        array_push(start->ast, token_make(alc, t_ast_scope, gcscope));
    }

    if (scope->has_gc_decls) {
        // Stack
        Map *idfs = map_make(alc);
        Value *amount = vgen_int(alc, scope->gc_decl_count, type_gen_number(alc, b, b->ptr_size, false, false));
        Idf *idf = idf_make(alc, idf_value, amount);
        map_set(idfs, "amount", idf);

        // Cache stack & stack_adr variable
        Scope *cache = gen_snippet_ast(alc, p, get_volt_snippet(b, "mem", "stack_cache"), idfs, start);
        array_push(start->ast, token_make(alc, t_ast_scope, cache));

        idf = map_get(cache->identifiers, "STACK_ADR");
        Value *stack_adr = idf->item;

        if (scope->type == sc_func) {
            // Stack reserve
            Scope *reserve = gen_snippet_ast(alc, p, get_volt_snippet(b, "mem", "stack_reserve"), idfs, start);
            array_push(start->ast, token_make(alc, t_ast_scope, reserve));

            // Set stack offset for varables
            Array *decls = scope->decls;
            int x = 0;
            for (int i = 0; i < decls->length; i++) {
                Decl *decl = array_get_index(decls, i);
                if (!decl->is_gc)
                    continue;
                Value *offset = vgen_ptrv(alc, b, stack_adr, type_gen_volt(alc, b, "ptr"), vgen_int(alc, x++, type_gen_volt(alc, b, "i32")));
                TDeclare *item = al(alc, sizeof(TDeclare));
                item->decl = decl;
                item->value = offset;
                array_push(start->ast, token_make(alc, t_set_decl_store_var, item));
                array_push(start->ast, tgen_assign(alc, value_make(alc, v_decl, decl, decl->type), vgen_null(alc, b)));
            }

            // Stack reduce
            Func* func = p->func;
            Scope* reduce_scope = scope_sub_make(alc, sc_default, scope);
            reduce_scope->ast = array_make(alc, 10);
            func->scope_stack_reduce = reduce_scope;
            Scope *scope_end = gen_snippet_ast(alc, p, get_volt_snippet(b, "mem", "stack_reduce"), idfs, scope);
            array_push(reduce_scope->ast, token_make(alc, t_ast_scope, scope_end));

        } else {
            // if / while : At end of scope, set local variables to null
            Array *decls = scope->decls;
            for (int i = 0; i < decls->length; i++) {
                Decl *decl = array_get_index(decls, i);
                array_push(end->ast, tgen_assign(alc, value_make(alc, v_decl, decl, decl->type), vgen_null(alc, b)));
            }
        }
    }
    //
    if(is_main_func_scope) {
        Scope *shutdown = gen_snippet_ast(alc, p, get_volt_snippet(b, "mem", "shutdown"), map_make(alc), scope);
        array_push(end->ast, token_make(alc, t_ast_scope, shutdown));
    }
}

void stage_generate_main(Unit *u) {
    //
    Build *b = u->b;
    bool main_has_return = false;
    bool main_has_arg = false;
    if(b->func_main) {
        main_has_return = !type_is_void(b->func_main->rett);
        main_has_arg = b->func_main->arg_types->length > 0;
    }

    // Generate main function
    Scope* scope = b->func_main ? b->func_main->scope->parent : scope_make(b->alc, sc_default, NULL);
    Func* func = func_make(b->alc, u, scope, "main", "main");
    b->func_main_gen = func;

    Map* args = map_make(b->alc);
    map_set(args, "argc", type_gen_volt(b->alc, b, "i32"));
    map_set(args, "argv", type_gen_volt(b->alc, b, "ptr"));
    func_generate_args(b->alc, func, args);

    func->rett = type_gen_volt(b->alc, b, "i32");
    func->scope->must_return = true;
    func->scope->rett = func->rett;

    // Generate main AST
    Str* code = b->str_buf;
    str_clear(code);

    str_flat(code, "{\n");
    // CLI args
    str_flat(code, "let arr = Array[String].new(10);\n");
    str_flat(code, "let i = 0\n");
    str_flat(code, "while i < argc {\n");
    str_flat(code, "let cstr = @ptrv(argv, cstring, i)\n");
    str_flat(code, "arr.push(cstr)\n");
    str_flat(code, "i++\n");
    str_flat(code, "}\n");

    // if (b->test) {
    //     str_flat(code, "ki__test__main();\n");
    //     str_flat(code, "return 0;\n");
    // } else {
        if (b->func_main) {
            if (main_has_return)
                str_flat(code, "return ");
            str_flat(code, "main(");
            if (main_has_arg) {
                str_flat(code, "arr");
            }
            str_flat(code, ");\n");
        }
        if (!main_has_return)
            str_flat(code, "return 0;\n");
    // }

    str_flat(code, "}\n");

    char* content = str_to_chars(b->alc, code);
    Chunk *chunk = chunk_make(b->alc, b, NULL);
    chunk_set_content(b, chunk, content, code->length);

    func->chunk_body = chunk;

    // Parse main AST
    Parser *p = b->parser;
    *p->chunk = *func->chunk_body;
    p->func = func;
    p->scope = func->scope;
    p->loop_scope = NULL;

    // Skip first token & set scope end
    tok(p, true, true, true);

    //
    read_ast(p, false);
}

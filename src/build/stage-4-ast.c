
#include "../all.h"

void stage_ast_func(Func *func);
void stage_ast_class(Class *class);
void ir_vtable_define_extern(Unit* u);
void loop_defer(Allocator* alc, Parser* p);

void stage_4_ast(Build *b) {

    Array* units = b->units;
    loop(units, i) {
        Unit* u = array_get_index(units, i);
        u->ir = ir_make(u);
    }

    b->building_ast = true;

    stage_ast_func(b->func_main_gen);
    stage_ast_func(get_valk_class_func(b, "mem", "Stack", "link"));
    stage_ast_func(get_valk_class_func(b, "mem", "Stack", "init"));
    stage_ast_func(get_valk_class_func(b, "mem", "GcManager", "init"));
    // TODO: only include Coro.new if an async function was used
    stage_ast_func(get_valk_class_func(b, "core", "Coro", "new"));
    // stage_ast_func(get_valk_class_func(b, "core", "Coro", "await_fd"));
    // stage_ast_func(get_valk_class_func(b, "core", "Coro", "await_coro"));
    // stage_ast_func(get_valk_class_func(b, "core", "Coro", "complete"));

    b->parse_last = true;

    loop(units, i) {
        Unit* u = array_get_index(units, i);
        ir_gen_globals(u->ir);
    }

    stage_ast_func(get_valk_func(b, "mem", "pools_init"));

    Array* funcs = b->parse_later;
    loop(funcs, i) {
        Func* func = array_get_index(funcs, i);
        func->parsed = false;
        stage_ast_func(func);
    }

    b->building_ast = false;
    b->parse_last = false;

    Unit* um = b->nsc_main->unit;
    ir_vtable_define_extern(um);

    loop(units, i) {
        Unit* u = array_get_index(units, i);

        // Parse functions from main package, just for validation
        if(u->nsc->pkc == b->pkc_main) {
            Array* funcs = u->funcs;
            loop(funcs, o) {
                Func* func = array_get_index(funcs, o);
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

    *p->chunk = *func->chunk_body;
    p->func = func;
    p->scope = func->scope;
    p->loop_scope = NULL;
    p->vscope_values = NULL;
    read_ast(p, false);

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
    // Detect if function can create objects (for optimizations)
    // Array* called = func->called_functions;
    // loop(called, i) {
    //     Func* f = array_get_index(called, i);
    //     if(f->can_create_objects) {
    //         func->can_create_objects = true;
    //         break;
    //     }
    // }
    // if(!func->can_create_objects) {
    //     if(func->t_run_gc) {
    //         disable_token(func->t_run_gc);
    //     }
    // }
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

void read_ast(Parser *p, bool single_line) {
    //
    Build* b = p->b;
    Allocator *alc = b->alc_ast;
    Scope* scope = p->scope;

    if (!scope->ast)
        scope->ast = array_make(alc, 50);

    int scope_type = scope->type;
    bool is_func_scope = scope_type == sc_func;
    if(is_func_scope) {
        ast_func_start(alc, p);
    }

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
        if (t == tok_hashtag && p->on_newline) {
            cc_parse(p);
            continue;
        }

        if(scope->did_return)
            continue;

        if (t == tok_id) {
            if (str_is(tkn, "let")){
                Array *names = array_make(alc, 2);
                Array *types = array_make(alc, 2);
                while (true) {
                    t = tok(p, true, false, true);
                    char *name = p->tkn;
                    if (t != tok_id) {
                        parse_err(p, -1, "Invalid variable name: '%s'", name);
                    }
                    t = tok(p, true, false, true);
                    Type *type = NULL;
                    if (t == tok_colon) {
                        type = read_type(p, alc, true);
                        t = tok(p, true, false, true);
                        if(type_is_void(type)) {
                            parse_err(p, -1, "Variables cannot have a 'void' type, variable: '%s'", name);
                        }
                    }
                    array_push(names, name);
                    array_push(types, type);

                    if(t != tok_comma)
                        break;
                }
                if(t != tok_eq) {
                    parse_err(p, -1, "Expected '=' here, found: '%s'", p->tkn);
                }

                Type* tcv_prev = p->try_conv;
                Type* tcv = array_get_index(types, 0);
                p->try_conv = tcv;

                Value* val = read_value(alc, p, true, 0);

                p->try_conv = tcv_prev;

                if (type_is_void(val->rett)) {
                    parse_err(p, -1, "Right side returns a 'void' value");
                }

                Array* values = all_values(alc, val);

                loop(names, i) {
                    if(i == values->length) {
                        parse_err(p, -1, "Right side does not return enough values to fit all your variables");
                    }

                    char *name = array_get_index(names, i);
                    Type *type = array_get_index(types, i);

                    Value* right = array_get_index(values, i);
                    if(type) {
                        right = try_convert(alc, p, scope, right, type);
                        type_check(p, type, right->rett);
                    } else {
                        type = right->rett;
                    }

                    Decl *decl = decl_make(alc, p->func, name, type, false);
                    Idf *idf = idf_make(b->alc, idf_decl, decl);
                    scope_set_idf(scope, name, idf, p);

                    array_push(scope->ast, tgen_declare(alc, scope, decl, right));
                }
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
                loop_defer(alc, p);
                array_push(scope->ast, token_make(alc, str_is(tkn, "break") ? t_break : t_continue, p->loop_scope));
                scope->did_return = true;
                continue;
            }
            if (str_is(tkn, "return")){

                if(p->vscope_values) {
                    Array *values = p->vscope_values;

                    Type *tcv_prev = p->try_conv;
                    Type *tcv = vscope_get_result_type(values);
                    p->try_conv = tcv ? tcv : tcv_prev;

                    Value *val = read_value(alc, p, false, 0);
                    // TODO: support multiple return types

                    p->try_conv = tcv_prev;

                    if(tcv) {
                        val = try_convert(alc, p, scope, val, tcv);
                        type_check(p, tcv, val->rett);
                    }
                    array_push(values, val);
                    array_push(scope->ast, token_make(alc, t_return_vscope, val));
                    scope->did_return = true;
                    continue;
                }

                Func* func = p->func;
                if(!func) {
                    parse_err(p, -1, "Using 'return' outside a function scope");
                }

                bool buffer_retv = func->rett_eax && type_is_gc(func->rett_eax) && func->rett_arg_types > 0;

                Type* rett = func->rett;
                Array* rett_decls = func->rett_decls;
                Array* rett_types = func->rett_types;
                int rettc = func->rett_count;
                int rett_decl_i = 0;
                Value* main_retv = NULL;
                int i = 0;
                while (i < rett_types->length) {
                    if (i > 0) {
                        tok_expect(p, ",", true, true);
                    }
                    Type *rett = array_get_index(rett_types, i);

                    Type *tcv_prev = p->try_conv;
                    Type *tcv = rett;
                    p->try_conv = tcv;

                    Value *val = read_value(alc, p, false, 0);

                    p->try_conv = tcv_prev;

                    val = try_convert(alc, p, p->scope, val, rett);
                    type_check(p, rett, val->rett);

                    if (i == 0 && func->rett_eax) {
                        if(buffer_retv) {
                            Decl *decl = decl_make(alc, p->func, NULL, func->rett_eax, false);
                            main_retv = vgen_decl(alc, decl);
                            array_push(scope->ast, tgen_declare(alc, scope, decl, val));
                        } else {
                            main_retv = vgen_var(alc, b, val);
                            array_push(scope->ast, token_make(alc, t_set_var, main_retv->item));
                        }
                    } else {
                        Decl *decl = array_get_index(func->rett_decls, rett_decl_i++);
                        Value *var = vgen_ptrv(alc, b, vgen_decl(alc, decl), rett, vgen_int(alc, 0, type_cache_i32(b)));
                        array_push(scope->ast, tgen_assign(alc, var, val));
                    }
                    i++;
                }

                array_push(scope->ast, tgen_return(alc, main_retv));
                scope->did_return = true;
                continue;
            }
            if (str_is(tkn, "throw")){
                char t = tok(p, true, false, true);
                if(t != tok_id) {
                    parse_err(p, -1, "Invalid error name: '%s'", p->tkn);
                }
                char* name = p->tkn;
                Func* func = p->func;
                Scope* fscope = func->scope;
                unsigned int err = 0;
                if (func->errors) {
                    err = (unsigned int)(intptr_t)map_get(func->errors, name);
                }
                if(err == 0) {
                    parse_err(p, -1, "Function has no error defined named: '%s'", name);
                }

                array_push(scope->ast, tgen_throw(alc, b, p->unit, err, name));
                scope->did_return = true;
                continue;
            }
            if (str_is(tkn, "each")){
                Value* on = read_value(alc, p, true, 0);
                Func* func = on->rett->class ? map_get(on->rett->class->funcs, "_each") : NULL;
                if(!func) {
                    parse_err(p, -1, "Cannot use 'each' on this value. The class of it's type does not contain an '_each' method");
                }
                func_mark_used(p->func, func);

                tok_expect(p, "as", true, true);
                t = tok(p, true, false, true);
                if(t != tok_id) {
                    parse_err(p, -1, "Invalid variable name: '%s'", p->tkn);
                }
                char* kname = NULL;
                char* vname = p->tkn;
                t = tok(p, true, false, false);
                if(t == tok_comma) {
                    tok(p, true, false, true);
                    kname = vname;
                    t = tok(p, true, false, true);
                    if (t != tok_id) {
                        parse_err(p, -1, "Invalid variable name: '%s'", p->tkn);
                    }
                    vname = p->tkn;
                }
                if(kname && str_is(kname, vname)) {
                    parse_err(p, -1, "Key/value variable names cannot be the same: '%s'", kname);
                }

                char t = tok(p, true, true, true);
                int scope_end_i = -1;
                bool single = false;
                if (t == tok_curly_open) {
                    scope_end_i = p->scope_end_i;
                } else if (t == tok_colon) {
                    single = true;
                } else {
                    parse_err(p, -1, "Expected '{' or ':' at the end of the 'each' statement, but found: '%s'", p->tkn);
                }

                Scope *scope = p->scope;
                Scope *ls = p->loop_scope;
                Scope *scope_each = scope_sub_make(alc, sc_loop, scope);

                Decl *on_decl = decl_make(alc, p->func, NULL, on->rett, false);
                on_decl->is_mut = true;
                array_push(scope->ast, tgen_declare(alc, scope, on_decl, on));

                Decl *kd = decl_make(alc, p->func, kname, array_get_index(func->rett_types, 1), false);
                kd->is_mut = true;
                Idf *idf = idf_make(b->alc, idf_decl, kd);
                scope_add_decl(alc, scope, kd);
                if (kname) {
                    scope_set_idf(scope_each, kname, idf, p);
                }

                Decl *vd = decl_make(alc, p->func, vname, array_get_index(func->rett_types, 0), false);
                vd->is_mut = true;
                idf = idf_make(b->alc, idf_decl, vd);
                scope_set_idf(scope_each, vname, idf, p);
                scope_add_decl(alc, scope, vd);
                // Index
                Decl *index = decl_make(alc, p->func, NULL, type_gen_valk(alc, b, "uint"), false);
                index->is_mut = true;
                scope_add_decl(alc, scope, index);
                Value* vindex = value_make(alc, v_decl, index, index->type);
                // Set index to 0
                array_push(scope->ast, tgen_assign(alc, vindex, vgen_int(alc, 0, index->type)));

                p->scope = scope_each;
                p->loop_scope = scope_each;
                read_ast(p, single);
                p->scope = scope;
                p->loop_scope = ls;
                if (!single)
                    p->chunk->i = scope_end_i;

                array_push(scope->ast, tgen_each(alc, p, vgen_decl(alc, on_decl), func, kd, vd, scope_each, index, vindex));
                continue;
            }
            if (str_is(tkn, "await_fd")){

                tok_expect(p, "(", false, false);
                Value* fd = read_value(alc, p, true, 0);
                type_check(p, type_gen_valk(alc, b, "FD"), fd->rett);
                tok_expect(p, ",", true, true);
                Value* read = read_value(alc, p, true, 0);
                type_check(p, type_gen_valk(alc, b, "bool"), read->rett);
                tok_expect(p, ",", true, true);
                Value* write = read_value(alc, p, true, 0);
                type_check(p, type_gen_valk(alc, b, "bool"), write->rett);
                tok_expect(p, ")", true, true);

                // Current coro
                Global *g_coro = get_valk_global(b, "core", "current_coro");
                Value *coro = value_make(alc, v_global, g_coro, g_coro->type);
                // Call Coro.await_fd
                Class *coro_class = get_valk_class(b, "core", "Coro");
                Func* f = map_get(coro_class->funcs, "await_fd");
                func_mark_used(p->func, f);
                Value* fptr = vgen_func_ptr(alc, f, NULL);
                Array* args = array_make(alc, 4);
                array_push(args, coro);
                array_push(args, fd);
                array_push(args, read);
                array_push(args, write);
                Value* fcall = vgen_func_call(alc, p, fptr, args);
                array_push(scope->ast, token_make(alc, t_statement, fcall));
                continue;

            } else if (str_is(tkn, "await_last_coro")) {
                //
                Global *g_coro = get_valk_global(b, "core", "current_coro");
                Value *coro = value_make(alc, v_global, g_coro, g_coro->type);
                // Call Coro.await_last_coro
                Class *coro_class = get_valk_class(b, "core", "Coro");
                Func* f = map_get(coro_class->funcs, "await_last_coro");
                func_mark_used(p->func, f);
                Value* fptr = vgen_func_ptr(alc, f, NULL);
                Array* args = array_make(alc, 4);
                array_push(args, coro);
                Value* fcall = vgen_func_call(alc, p, fptr, args);
                array_push(scope->ast, token_make(alc, t_statement, fcall));
                continue;
            }
            // Check if macro
            Idf* idf = scope_find_idf(scope, tkn, true);
            if(idf && idf->type == idf_macro) {
                Macro* m = idf->item;
                macro_read_ast(alc, m, p);
                continue;
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
            if (str_is(tkn, "@gc_share")) {
                tok_expect(p, "(", false, false);
                Value* v = read_value(alc, p, true, 0);
                Type* rett = v->rett;
                if(!rett->class || (rett->class->type != ct_class && rett->class->type != ct_ptr) || !rett->is_pointer) {
                    parse_err(p, -1, "Invalid @gc_share value. It must be a pointer to an instance of a class");
                }
                tok_expect(p, ")", true, true);

                Func *share = get_valk_func(b, "mem", "gc_share");
                func_mark_used(p->func, share);
                Array* args = array_make(alc, 2);
                array_push(args, v);
                Value *on = vgen_func_ptr(alc, share, NULL);
                Value *fcall = vgen_func_call(alc, p, on, args);
                array_push(scope->ast, token_make(alc, t_statement, fcall));
                continue;
            }
        }
        if (t == tok_curly_open) {
            Scope* sub = scope_sub_make(alc, sc_default, scope);
            p->scope = sub;
            read_ast(p, false);
            p->scope = scope;
            if(sub->did_return)
                scope->did_return = true;
            array_push(scope->ast, token_make(alc, t_ast_scope, sub));
            continue;
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

            Type *tcv_prev = p->try_conv;
            Type *tcv = left->rett;

            p->try_conv = tcv;
            Value *right = read_value(alc, p, true, 0);
            p->try_conv = tcv_prev;

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

            right = try_convert(alc, p, p->scope, right, left->rett);
            if(op == op_eq && left->type == v_decl_overwrite) {
                char* reason;
                if(!type_compat(left->rett, right->rett, &reason)) {
                    // Remove overwrite
                    DeclOverwrite* dov = left->item;
                    scope_delete_idf_by_value(scope, dov, true);
                    left = vgen_decl(alc, dov->decl);
                }
            }
            type_check(p, left->rett, right->rett);

            if(op == op_eq && left->type == v_global) {
                Global *g = left->item;
                if(g->is_shared && type_is_gc(g->type)) {
                    // Replace right with a temporary variable
                    right = vgen_var(alc, b, right);
                    array_push(scope->ast, token_make(alc, t_set_var, right->item));
                    // call gc_share(right)
                    Func *share = get_valk_func(b, "mem", "gc_share");
                    Array *args = array_make(alc, 2);
                    array_push(args, right);
                    Value *on = vgen_func_ptr(alc, share, NULL);
                    Value *fcall = vgen_func_call(alc, p, on, args);
                    array_push(scope->ast, token_make(alc, t_statement, fcall));
                }
            }

            array_push(scope->ast, tgen_assign(alc, left, right));
            continue;

        // } else if (t == tok_comma) {
        //     // Multi assign
        //     Array* values = array_make(alc, 4);
        //     array_push(values, left);
        //     value_is_mutable(left);

        //     while(t == tok_comma) {
        //         tok(p, true, false, true);
        //         Value* v = read_value(alc, p, true, 0);
        //         array_push(values, v);
        //         t = tok(p, true, false, false);
        //     }

        //     tok_expect(p, "=", true, true);

        //     Value *val = read_value(alc, p, true, 0);

        //     VFuncCall *fcall = value_extract_func_call(val);
        //     if (!fcall) {
        //         parse_err(p, -1, "Right side does not return multiple values");
        //     }
        //     Value *on = fcall->on;

        //     TypeFuncInfo *fi = on->rett->func_info;
        //     Type *rett = fi->rett;
        //     Array *rett_types = rett_types_of(alc, rett);
        //     if (rett_types->length < values->length) {
        //         parse_err(p, -1, "Trying to declare %d variables, but the function only returns %d value(s)", values->length, fi->rett_types->length);
        //     }
        //     fcall->rett_refs = array_make(alc, values->length);

        //     loop(values, i) {
        //         Value *v = array_get_index(values, i);
        //         Type *type = v->rett;

        //         if (i == 0) {
        //             val = try_convert(alc, p, scope, val, type);
        //             type_check(p, type, val->rett);
        //         } else {
        //             type_check(p, type, array_get_index(rett_types, i));
        //         }

        //         if (i == 0) {
        //             array_push(scope->ast, tgen_assign(alc, v, val));
        //         } else {
        //             array_push(fcall->rett_refs, v);
        //         }
        //     }
        //     continue;
        }

        //
        array_push(scope->ast, token_make(alc, t_statement, left));
        //
        if(scope->did_return && p->func) {
            // Value contains an exit function call
            array_push(scope->ast, token_make(alc, t_return, NULL));
        }
    }

    if(scope->must_return && !scope->did_return) {
        parse_err(p, -1, "Missing return statement");
    }

    if (scope->gc_check) {
        p->func->calls_gc_check = true;
    }

    if (scope_type == sc_loop && !scope->did_return) {
        loop_defer(alc, p);
    }

    if(is_func_scope) {
        ast_func_end(alc, p);
    }
}

void loop_defer(Allocator* alc, Parser* p) {
    Scope *ast_scope = p->scope;
    Scope *loop_scope = p->loop_scope;
    if(!loop_scope || !loop_scope->has_gc_decls) {
        return;
    }
    Build* b = p->b;
    Array *decls = loop_scope->decls;
    loop(decls, i) {
        Decl *decl = array_get_index(decls, i);
        if (!decl->is_gc)
            continue;
        array_push(ast_scope->ast, tgen_assign(alc, vgen_decl(alc, decl), vgen_null(alc, b)));
    }

    Scope *gcscope = gen_snippet_ast(alc, p, get_valk_snippet(b, "mem", "run_gc_check"), map_make(alc), ast_scope);
    Token* t = token_make(alc, t_ast_scope, gcscope);
    array_push(ast_scope->ast, t);
}

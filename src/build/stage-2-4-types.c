
#include "../all.h"

void stage_types_global(Parser* p, Global* g);

void stage_2_types(Unit* u) {
    Build* b = u->b;
    Parser* p = u->parser;

    if (b->verbose > 2)
        printf("Stage 2 | Scan types: %s\n", u->nsc->name);

    usize start = microtime();

    Array* funcs = u->funcs;
    for(int i = 0; i < funcs->length; i++) {
        Func* func = array_get_index(funcs, i);
        stage_types_func(p, func);
    }
    Array* classes = u->classes;
    for(int i = 0; i < classes->length; i++) {
        Class* class = array_get_index(classes, i);
        stage_types_class(p, class);
    }
    Array* globals = u->globals;
    for(int i = 0; i < globals->length; i++) {
        Global* g = array_get_index(globals, i);
        if(g->chunk_type)
            stage_types_global(p, g);
    }

    unit_validate(u, p);

    b->time_parse += microtime() - start;

    stage_add_item(b->stage_3_values, u);
}

void stage_types_func(Parser* p, Func* func) {

    if(func->types_parsed)
        return;
    func->types_parsed = true;

    Build* b = p->b;
    Allocator* alc = p->b->alc;

    if(func->is_test) {
        Type* type = type_gen_class(alc, get_valk_class(b, "core", "TestResult"));
        FuncArg* arg = func_arg_make(alc, type);
        map_set_force_new(func->args, "VALK_TEST_RESULT", arg);
        array_push(func->arg_types, type);

        Decl* decl = decl_make(alc, "VALK_TEST_RESULT", type, !func->is_async);
        Idf* idf = idf_make(alc, idf_decl, decl);
        scope_set_idf(func->scope, "VALK_TEST_RESULT", idf, p);
        arg->decl = decl;

        func->rett = type_gen_void(alc);
        return;
    }

    p->scope = func->scope;

    if(func->class && !func->is_static) {
        FuncArg *arg = func_arg_make(b->alc, type_gen_class(b->alc, func->class));
        map_set_force_new(func->args, "this", arg);
        array_push(func->arg_types, arg->type);
        Decl *decl = decl_make(b->alc, "this", arg->type, !func->is_async);
        Idf *idf = idf_make(b->alc, idf_decl, decl);
        scope_set_idf(func->scope, "this", idf, p);
        arg->decl = decl;
        array_push(func->arg_values, NULL);
    }

    if(func->chunk_args) {
        *p->chunk = *func->chunk_args;
        char t = tok(p, true, true, true);
        while(t != tok_bracket_close) {

            Chunk* arg_chunk = chunk_clone(b->alc, p->chunk);

            char* name = p->tkn;
            if(!is_valid_varname(name)) {
                parse_err(p, -1, "Invalid function argument name: '%s'", name);
            }
            if(map_contains(func->args, name)) {
                parse_err(p, -1, "Duplicate argument name: '%s'", name);
            }

            tok_expect(p, ":", true, false);
            Type* type = read_type(p, b->alc, false);
            if(type->type == type_void) {
                parse_err(p, -1, "You cannot use void types for arguments");
            }

            FuncArg* arg = func_arg_make(b->alc, type);
            arg->chunk = arg_chunk;
            map_set_force_new(func->args, name, arg);
            array_push(func->arg_types, type);

            Decl* decl = decl_make(b->alc, name, type, !func->is_async);
            Idf* idf = idf_make(b->alc, idf_decl, decl);
            scope_set_idf(func->scope, name, idf, p);
            arg->decl = decl;

            t = tok(p, true, true, true);
            if(t == tok_bracket_open) {
                arg->chunk_value = chunk_clone(b->alc, p->chunk);
                array_push(func->arg_values, arg->chunk_value);
                skip_body(p);
                t = tok(p, true, true, true);
            } else {
                array_push(func->arg_values, NULL);
            }

            if(t == tok_comma) {
                t = tok(p, true, true, true);
                continue;
            }
            if(t != tok_bracket_close) {
                parse_err(p, -1, "Unexpected token: '%s', expected ')'", p->tkn);
            }
        }
    }
    if(func->has_rett) {
        *p->chunk = *func->chunk_rett;
        if(func->multi_rett) {
            tok_expect(p, "(", true, true);
            // Return types
            Array* types = array_make(b->alc, 2);
            while(true) {
                Type *type = read_type(p, b->alc, false);
                if(type_is_void(type)) {
                    parse_err(p, -1, "You cannot use 'void' here");
                }
                array_push(types, type);
                // next
                char t = tok_expect_two(p, ",", ")", true, true);
                if(t == tok_bracket_close)
                    break;
            }
            func->rett_types = types;
            func->rett = array_get_index(types, 0);
        } else {
            Type *type = read_type(p, b->alc, false);
            func->rett = type;
            array_push(func->rett_types, type);
        }
        func->scope->must_return = !type_is_void(func->rett);
        func->scope->rett = func->rett;
    } else {
        func->rett = type_gen_void(b->alc);
    }

    if(func->is_async && !type_is_void(func->rett)) {
        func->result_decl = decl_make(alc, NULL, func->rett, false);
        scope_add_decl(alc, func->scope, func->result_decl);
    }

    type_gen_func(alc, func);
}

void stage_types_class(Parser* p, Class* class) {

    Build *b = p->b;
    p->scope = class->scope;

    Array* props = class->props->values;
    for(int i = 0; i < props->length; i++) {
        ClassProp *prop = array_get_index(props, i);
        if (prop->chunk_type) {
            *p->chunk = *prop->chunk_type;
            Type *type = read_type(p, b->alc, false);
            prop->type = type;
        }
    }
}

void stage_types_global(Parser* p, Global* g) {

    Build *b = p->b;
    p->scope = g->declared_scope;

    *p->chunk = *g->chunk_type;
    Type *type = read_type(p, b->alc, false);
    g->type = type;

    if (g->is_shared && type_is_gc(type)) {
        parse_err(p, -1, "The compiler currently does not yet support classes in shared globals");
    }
}


#include "../all.h"

void stage_types_global(Parser* p, Global* g);

void stage_2_types(Unit* u) {
    Build* b = u->b;
    Parser* p = u->parser;

    if (b->verbose > 2)
        printf("Stage 2 | Scan types: %s\n", u->nsc->name);

    usize start = microtime();

    Array* funcs = u->funcs;
    loop(funcs, i) {
        Func* func = array_get_index(funcs, i);
        stage_types_func(p, func);
    }
    Array* classes = u->classes;
    loop(classes, i) {
        Class* class = array_get_index(classes, i);
        stage_types_class(p, class);
    }
    Array* globals = u->globals;
    loop(globals, i) {
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

        Decl* decl = decl_make(alc, func, NULL, type, true);
        scope_add_decl(alc, func->scope, decl);
        Idf* idf = idf_make(alc, idf_decl, decl);
        scope_set_idf(func->scope, "VALK_TEST_RESULT", idf, p);
        arg->decl = decl;
        return;
    }

    p->scope = func->scope;

    if(func->class && !func->is_static) {
        FuncArg *arg = func_arg_make(b->alc, type_gen_class(b->alc, func->class));
        map_set_force_new(func->args, "this", arg);
        array_push(func->arg_types, arg->type);
        Decl *decl = decl_make(b->alc, func, NULL, arg->type, true);
        scope_add_decl(alc, func->scope, decl);
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
            if(t == tok_at_word) {
                if(str_is(name, "@infinite")) {
                    func->inf_args = true;
                    tok_expect(p, ")", true, true);
                    break;
                }
            }

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

            Decl* decl = decl_make(b->alc, func, name, type, true);
            scope_add_decl(alc, func->scope, decl);
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
    if(func->read_rett_type) {
        *p->chunk = *func->chunk_rett;
        p->allow_multi_type = true;
        Type *rett = read_type(p, b->alc, false);
        if(type_is_void(rett)) {
            rett = NULL;
        }
        func->rett = rett;
        func->rett_eax = rett;
        func->scope->must_return = rett != NULL;

        // Argument based returns
        if(rett && rett->type == type_multi) {
            func->rett_eax = NULL;
            Array* types = rett->multi_types;
            Array * rett_decls = array_make(alc, 2);
            Array * rett_arg_types = array_make(alc, 2);
            loop(types, i) {
                Type* type = array_get_index(types, i);
                array_push(func->rett_types, type);
                if(i == 0 && type_fits_pointer(type, b)) {
                    func->rett_eax = type;
                    continue;
                }
                Type* ptr = type_cache_ptr(b); // TOBE: type_ptr_of(type)
                Decl *decl = decl_make(alc, func, NULL, ptr, true);
                array_push(rett_decls, decl);
                array_push(rett_arg_types, ptr);
                scope_add_decl(alc, func->scope, decl);
            }
            func->rett_decls = rett_decls;
            func->rett_arg_types = rett_arg_types;
        } else {
            if(rett)
                array_push(func->rett_types, rett);
        }

        // Count return types
        func->rett_count = rett ? (rett->type == type_multi ? rett->multi_types->length : 1) : 0;
    }

    type_gen_func(alc, func);
}

void stage_types_class(Parser* p, Class* class) {

    Build *b = p->b;
    p->scope = class->scope;

    Array* props = class->props->values;
    loop(props, i) {
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


#include "../all.h"

void stage_parse(Parser* p, Unit* u, Fc* fc);
void stage_1_func(Parser* p, Unit* u, int act, Fc* fc);
void stage_1_header(Parser* p, Unit* u, Fc* fc);
void stage_1_class(Parser* p, Unit* u, int type, int act, Fc* fc);
void stage_1_trait(Parser* p, Unit* u, int act, Fc* fc);
void stage_1_use(Parser* p, Unit* u);
void stage_1_global(Parser* p, Unit* u, bool shared, int act, Fc* fc);
void stage_1_value_alias(Parser* p, Unit* u, int act, Fc* fc);
void stage_1_alias(Parser* p, Unit* u, Fc* fc);
void stage_1_test(Parser* p, Unit* u, Fc* fc);
void stage_1_snippet(Parser* p, Unit* u);
void stage_1_link(Parser* p, Unit* u, int link_dynamic);

void stage_1_parse(Fc* fc) {
    Unit* u = fc->nsc->unit;
    Parser* p = u->parser;
    Build* b = u->b;

    if (b->verbose > 2)
        printf("Stage 1 | Parse: %s\n", fc->path);

    *p->chunk = *fc->content;
    p->in_header = fc->is_header;
    p->scope = fc->scope;

    usize start = microtime();
    stage_parse(p, u, fc);
    b->time_parse += microtime() - start;

    p->in_header = false;
    p->scope = NULL;
}

void stage_parse(Parser* p, Unit* u, Fc* fc) {

    while(true) {

        char t = tok(p, true, true, true);

        if (t == tok_eof) {
            break;
        }
        if (t == tok_semi) {
            continue;
        }
        if (t == tok_hashtag && p->on_newline) {
            cc_parse(p);
            continue;
        }

        int act = act_public;
        if (t == tok_sub) {
            act = act_private_fc;
        } else if (t == tok_subsub) {
            act = act_private_nsc;
        } else if (t == tok_triple_sub) {
            act = act_private_pkc;
        }

        char* tkn = p->tkn;
        char* act_tkn;

        if(act != act_public) {
            t = tok(p, true, false, true);
            act_tkn = tkn;
            tkn = p->tkn;
        }

        if (t == tok_id) {
            if (str_is(tkn, "fn")) {
                stage_1_func(p, u, act, fc);
                continue;
            }
            if (str_is(tkn, "struct")) {
                stage_1_class(p, u, ct_struct, act, fc);
                continue;
            }
            if (str_is(tkn, "class")) {
                stage_1_class(p, u, ct_class, act, fc);
                continue;
            }
            if (str_is(tkn, "pointer")) {
                stage_1_class(p, u, ct_ptr, act, fc);
                continue;
            }
            if (str_is(tkn, "integer")) {
                stage_1_class(p, u, ct_int, act, fc);
                continue;
            }
            if (str_is(tkn, "float")) {
                stage_1_class(p, u, ct_float, act, fc);
                continue;
            }
            if (str_is(tkn, "boolean")) {
                stage_1_class(p, u, ct_bool, act, fc);
                continue;
            }
            if (str_is(tkn, "trait")) {
                stage_1_trait(p, u, act, fc);
                continue;
            }
            if (str_is(tkn, "shared")) {
                stage_1_global(p, u, true, act, fc);
                continue;
            }
            if (str_is(tkn, "global")) {
                stage_1_global(p, u, false, act, fc);
                continue;
            }
            if (str_is(tkn, "value")) {
                stage_1_value_alias(p, u, act, fc);
                continue;
            }
            if (str_is(tkn, "alias")) {
                stage_1_alias(p, u, fc);
                continue;
            }

            if (act != act_public) {
                parse_err(p, -1, "Unexpected '%s' before '%s'", act_tkn, tkn);
            }

            if (str_is(tkn, "header")) {
                stage_1_header(p, u, fc);
                continue;
            }
            if (str_is(tkn, "use")) {
                stage_1_use(p, u);
                continue;
            }
            if (str_is(tkn, "test")) {
                stage_1_test(p, u, fc);
                continue;
            }
            if (str_is(tkn, "snippet")) {
                stage_1_snippet(p, u);
                continue;
            }
            if (str_is(tkn, "link_dynamic")) {
                stage_1_link(p, u, link_dynamic);
                continue;
            }
        }

        parse_err(p, -1, "Unexpected token: '%s'", tkn);
    }
}

void stage_1_func(Parser *p, Unit *u, int act, Fc* fc) {
    Build* b = p->b;

    char t = tok(p, true, false, true);
    char* name = p->tkn;
    if(t != tok_id) {
        parse_err(p, -1, "Invalid function name: '%s'", name);
    }

    Func* func = func_make(b->alc, u, p->scope, name, NULL);
    func->act = act;
    func->fc = fc;
    func->in_header = p->in_header;

    Idf* idf = idf_make(b->alc, idf_func, func);
    scope_set_idf(p->scope->parent, name, idf, p);

    if (str_is(name, "main")) {
        b->func_main = func;
    }
    if (p->in_header) {
        func->export_name = name;
    }

    parse_handle_func_args(p, func);
}

void stage_1_header(Parser *p, Unit *u, Fc* fc){
    Build* b = p->b;
    
    char t = tok(p, true, false, true);
    if(t != tok_string) {
        parse_err(p, -1, "Expected a header name here wrapped in double-quotes");
    }
    char* fn = p->tkn;

    if(p->in_header) {
        Fc *hfc = pkc_load_header(fc->header_pkc, fn, p, true);
        parser_new_context(&p);
        *p->chunk = *hfc->content;
        stage_parse(p, u, hfc);
        parser_pop_context(&p);
        return;
    }

    Pkc *pkc = u->nsc->pkc;
    Fc *hfc = pkc_load_header(pkc, fn, p, false);

    tok_expect(p, "as", true, false);

    t = tok(p, true, false, true);
    char* name = p->tkn;
    if(!is_valid_varname(name)) {
        parse_err(p, -1, "Invalid name: '%s'", name);
    }

    Idf* idf = idf_make(b->alc, idf_scope, hfc->scope);
    scope_set_idf(p->scope, name, idf, p);
}

void stage_1_class(Parser* p, Unit* u, int type, int act, Fc* fc) {
    Build* b = p->b;

    char t = tok(p, true, false, true);
    char *name = p->tkn;
    if(t != tok_id) {
        parse_err(p, -1, "Invalid type name: '%s'", name);
    }

    Class* class = class_make(b->alc, b, type);
    class->act = act;
    class->fc = fc;
    class->unit = u;
    class->name = name;
    class->ir_name = gen_export_name(u->nsc, name);
    class->scope = scope_sub_make(b->alc, sc_default, p->scope);
    class->in_header = p->in_header;

    t = tok(p, false, false, false);
    if(t == tok_sq_bracket_open) {
        tok(p, false, false, true);
        Array* generic_names = array_make(b->alc, 2);
        while (true) {
            t = tok(p, true, false, true);
            if(t != tok_id){
                parse_err(p, -1, "Invalid generic type name: '%s'", p->tkn);
            }
            if (array_contains(generic_names, p->tkn, arr_find_str)) {
                parse_err(p, -1, "Duplicate generic type name: '%s'", p->tkn);
            }
            array_push(generic_names, p->tkn);
            if (tok_expect_two(p, ",", "]", true, false) == tok_sq_bracket_close)
                break;
        }
        class->generics = map_make(b->alc);
        class->generic_names = generic_names;
        class->is_generic_base = true;
    }

    Scope* nsc_scope = u->nsc->scope;
    Idf* idf = idf_make(b->alc, idf_class, class);
    scope_set_idf(nsc_scope, name, idf, p);
    array_push(u->classes, class);
    if(!class->is_generic_base) {
        scope_set_idf(class->scope, "CLASS", idf, p);
        array_push(b->classes, class);
        if(class->type == ct_class) {
            class->gc_vtable_index = ++b->gc_vtables;
        }
    }

    //
    if(type == ct_int) {
        class->size = b->ptr_size;
        class->allow_math = true;
        char t = tok(p, true, false, false);
        if(t == tok_number) {
            tok(p, true, false, true);
            int size = atoi(p->tkn);
            if (size != 1 && size != 2 && size != 4 && size != 8) {
                parse_err(p, -1, "Invalid integer size: '%d'", size);
            }
            class->size = size;
            t = tok(p, true, false, false);
        }
        if(t == tok_id && str_is(p->tkn, "unsigned")) {
            tok(p, true, false, true);
            class->is_signed = false;
            t = tok(p, true, false, false);
        }
    } else if(type == ct_float) {
        class->size = b->ptr_size;
        class->allow_math = true;
        char t = tok(p, true, false, false);
        if(t == tok_number) {
            tok(p, true, false, true);
            int size = atoi(p->tkn);
            if (size != 4 && size != 8) {
                parse_err(p, -1, "Invalid float size: '%d'", size);
            }
            class->size = size;
            t = tok(p, true, false, false);
        }
    } else if(type == ct_ptr) {
        class->size = b->ptr_size;
        char t = tok(p, true, false, false);
        if(t == tok_id && str_is(p->tkn, "math")) {
            tok(p, true, false, true);
            class->allow_math = true;
            t = tok(p, true, false, false);
        }
    } else if(type == ct_bool) {
        class->size = 1;
    } else if(type == ct_struct) {
        char t = tok(p, true, false, false);
        if(t == tok_id && str_is(p->tkn, "packed")) {
            tok(p, true, false, true);
            class->packed = true;
            t = tok(p, true, false, false);
        }
    }

    tok_expect(p, "{", true, true);

    class->body = chunk_clone(b->alc, p->chunk);

    skip_body(p);
}

void stage_1_trait(Parser* p, Unit* u, int act, Fc* fc) {
    Build* b = p->b;

    char t = tok(p, true, false, true);
    char *name = p->tkn;
    if(t != tok_id) {
        parse_err(p, -1, "Invalid type name: '%s'", name);
    }

    tok_expect(p, "{", true, true);

    Trait* trait = al(b->alc, sizeof(Trait));
    trait->fc = fc;
    trait->chunk = chunk_clone(b->alc, p->chunk);
    trait->scope = p->scope;
    trait->act = act;

    Scope* nsc_scope = u->nsc->scope;
    Idf* idf = idf_make(b->alc, idf_trait, trait);
    scope_set_idf(nsc_scope, name, idf, p);

    skip_body(p);
}

void stage_1_use(Parser *p, Unit *u){

    char* pk = NULL;
    char t = tok(p, true, false, true);
    char* ns = p->tkn;
    int next = tok(p, false, false, false);
    if(next == tok_colon) {
        next = tok(p, false, false, true);
        pk = ns;
        t = tok(p, false, false, true);
        ns = p->tkn;
    }

    Pkc* pkc = u->nsc->pkc;
    if(pk) {
        pkc = pkc_load_pkc(pkc, pk, p);
    }

    Nsc* nsc = nsc_load(pkc, ns, true, p);

    Build *b = p->b;
    Idf* idf = idf_make(b->alc, idf_scope, nsc->scope);
    scope_set_idf(p->scope, ns, idf, p);
}

void stage_1_global(Parser *p, Unit *u, bool shared, int act, Fc *fc) {

    Build *b = p->b;
    char t = tok(p, true, false, true);
    char* name = p->tkn;

    Global* g = al(b->alc, sizeof(Global));
    g->act = act;
    g->fc = fc;
    g->name = name;
    g->export_name = gen_export_name(u->nsc, name);
    g->type = NULL;
    g->value = NULL;
    g->chunk_type = NULL;
    g->chunk_value = NULL;
    g->declared_scope = p->scope;
    g->is_shared = shared;
    g->is_mut = true;

    Idf* idf = idf_make(b->alc, idf_global, g);
    scope_set_idf(u->nsc->scope, name, idf, p);
    array_push(u->globals, g);

    tok_expect(p, ":", true, false);

    g->chunk_type = chunk_clone(b->alc, p->chunk);

    skip_type(p);

    t = tok(p, true, false, false);
    if (t == tok_bracket_open) {
        t = tok(p, true, false, true);
        g->chunk_value = chunk_clone(b->alc, p->chunk);
        skip_body(p);
    }
}

void stage_1_value_alias(Parser *p, Unit *u, int act, Fc* fc) {
    Build *b = u->b;
    char t = tok(p, true, false, true);
    char* name = p->tkn;
    if(t != tok_id) {
        parse_err(p, -1, "Invalid value name: '%s'", name);
    }

    tok_expect(p, "(", true, false);

    Chunk *chunk = chunk_clone(b->alc, p->chunk);
    ValueAlias *va = al(b->alc, sizeof(ValueAlias));
    va->chunk = chunk;
    va->scope = p->scope;
    va->fc = fc;
    va->act = act;

    skip_body(p);

    Idf* idf = idf_make(b->alc, idf_value_alias, va);
    scope_set_idf(u->nsc->scope, name, idf, p);
}

void stage_1_alias(Parser* p, Unit* u, Fc* fc) {
    Build *b = u->b;
    char t = tok(p, true, false, true);
    char* name = p->tkn;
    if(t != tok_id) {
        parse_err(p, -1, "Invalid alias name: '%s'", name);
    }

    tok_expect(p, "as", true, false);

    Alias* a = al(b->alc, sizeof(Alias));
    a->name = name;
    a->idf = NULL;
    a->chunk = chunk_clone(b->alc, p->chunk);
    a->scope = p->scope;

    array_push(u->aliasses, a);

    skip_id(p);
}

void stage_1_test(Parser* p, Unit* u, Fc* fc){
    Build *b = p->b;
    Allocator *alc = b->alc;

    int t = tok(p, true, false, true);
    if(t != tok_string) {
        parse_err(p, -1, "Missing test name");
    }
    char* name = p->tkn;
    tok_expect(p, "{", true, true);

    if (b->is_test && fc->nsc->pkc == b->pkc_main) {
        Test *test = al(alc, sizeof(Test));
        test->name = name;
        test->body = chunk_clone(alc, p->chunk);

        char *buf = b->char_buf;
        strcpy(buf, "vali_test_gen_");
        sprintf(buf, "vali_test_gen_%d_%s", u->tests->length, u->unique_hash);
        char *fn = dups(alc, buf);

        Func *func = func_make(b->alc, u, p->scope, fn, NULL);
        func->in_header = p->in_header;
        func->chunk_body = chunk_clone(alc, p->chunk);
        func->is_test = true;
        test->func = func;

        array_push(u->tests, test);
    }

    skip_body(p);
}

void stage_1_snippet(Parser *p, Unit *u) {
    Build *b = p->b;
    Allocator *alc = b->alc;

    char t = tok(p, true, false, true);
    char* name = p->tkn;
    if(t != tok_id) {
        parse_err(p, -1, "Invalid snippet name: '%s'", name);
    }

    tok_expect(p, "(", false, false);
    Array* args = array_make(alc, 4);
    t = tok(p, true, true, true);
    while(t != tok_bracket_close) {
        char* name = p->tkn;
        if(t != tok_id) {
            parse_err(p, -1, "Invalid snippet argument name: '%s'", name);
        }
        tok_expect(p, ":", true, false);
        t = tok(p, true, true, true);
        int type = -1;
        if(str_is(p->tkn, "V")) {
            type = snip_value;
        } else if(str_is(p->tkn, "T")) {
            type = snip_type;
        } else {
            parse_err(p, -1, "Expect 'V' (value) or 'T' (type), Found: '%s'", p->tkn);
        }

        SnipArg* sa = al(alc, sizeof(SnipArg));
        sa->name = name;
        sa->type = type;
        array_push(args, sa);

        t = tok_expect_two(p, ",", ")", true, true);
        if(t == tok_comma) {
            t = tok(p, true, true, true);
        }
    }
    tok_expect(p, "{", true, true);

    Snippet* snip = al(alc, sizeof(Snippet));
    snip->chunk = chunk_clone(alc, p->chunk);
    snip->args = args;
    snip->fc_scope = p->scope;
    snip->exports = NULL;

    Idf* idf = idf_make(alc, idf_snippet, snip);
    scope_set_idf(u->nsc->scope, name, idf, p);

    skip_body(p);

    t = tok(p, true, true, false);

    if(t == tok_eqgt) {
        tok(p, true, true, true);
        tok_expect(p, "(", true, true);
        Array* exports = array_make(alc, 4);
        t = tok(p, true, true, true);
        while(t != tok_bracket_close) {
            char* name = p->tkn;
            if(t != tok_id) {
                parse_err(p, -1, "Invalid snippet argument name: '%s'", name);
            }
            array_push(exports, name);
            t = tok_expect_two(p, ",", ")", true, true);
            if(t == tok_comma) {
                t = tok(p, true, true, true);
            }
        }
        snip->exports = exports;
    }
}

void stage_1_link(Parser* p, Unit* u, int type) {
    //
    Build* b = u->b;
    Allocator* alc = b->alc;

    char t = tok(p, true, false, true);
    if(t != tok_string) {
        parse_err(p, -1, "Expected a library name here wrapped in dubbel-quotes, e.g. \"pthread\"");
    }
    char* fn = p->tkn;
    if (type == link_default) {
        // type = b->link_static ? link_static : link_dynamic;
        type = link_dynamic;
    }

    Link *link = map_get(b->link_settings, fn);
    if (!link) {
        link = al(alc, sizeof(Link));
        link->type = type;
        link->name = fn;
        map_set(b->link_settings, fn, link);
    }
    if (type == link_dynamic && link->type != link_dynamic) {
        link->type = link_dynamic;
    }

    link = al(alc, sizeof(Link));
    link->type = type;
    link->name = fn;
    array_push(b->links, link);
}
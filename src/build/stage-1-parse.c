
#include "../all.h"

void stage_parse(Fc *fc);
void stage_1_func(Fc* fc, int act);
void stage_1_header(Fc* fc);
void stage_1_class(Fc* fc, int type, int act);
void stage_1_use(Fc* fc);
void stage_1_global(Fc* fc, bool shared);
void stage_1_value_alias(Fc* fc);
void stage_1_snippet(Fc* fc);

void stage_1_parse(Fc* fc) {
    Build* b = fc->b;

    if (b->verbose > 2)
        printf("Stage 1 | Parse: %s\n", fc->path);

    usize start = microtime();
    stage_parse(fc);
    b->time_parse += microtime() - start;

    stage_add_item(b->stage_2_alias, fc);
}

void stage_parse(Fc *fc) {
    Build* b = fc->b;
    Chunk* chunk = fc->chunk_parse;

    while(true) {

        char* tkn = tok(fc, true, true, true);
        if (tkn[0] == 0)
            break;

        int act = act_public;

        if(str_is(tkn, ";")) {
            continue;
        }
        if(str_is(tkn, "fn")) {
            stage_1_func(fc, act);
            continue;
        }
        if(str_is(tkn, "struct")) {
            stage_1_class(fc, ct_struct, act);
            continue;
        }
        if(str_is(tkn, "class")) {
            stage_1_class(fc, ct_class, act);
            continue;
        }
        if(str_is(tkn, "pointer")) {
            stage_1_class(fc, ct_ptr, act);
            continue;
        }
        if(str_is(tkn, "integer")) {
            stage_1_class(fc, ct_int, act);
            continue;
        }
        if(str_is(tkn, "float")) {
            stage_1_class(fc, ct_float, act);
            continue;
        }
        if(str_is(tkn, "boolean")) {
            stage_1_class(fc, ct_bool, act);
            continue;
        }
        if(str_is(tkn, "header")) {
            stage_1_header(fc);
            continue;
        }
        if(str_is(tkn, "use")) {
            stage_1_use(fc);
            continue;
        }
        if(str_is(tkn, "shared")) {
            stage_1_global(fc, true);
            continue;
        }
        if(str_is(tkn, "global")) {
            stage_1_global(fc, false);
            continue;
        }
        if(str_is(tkn, "value")) {
            stage_1_value_alias(fc);
            continue;
        }
        if(str_is(tkn, "snippet")) {
            stage_1_snippet(fc);
            continue;
        }

        sprintf(b->char_buf, "Unexpected token: '%s'", tkn);
        parse_err(chunk, b->char_buf);
    }
}

void stage_1_func(Fc* fc, int act) {
    Build* b = fc->b;

    char* name = tok(fc, true, false, true);
    if(!is_valid_varname(name)) {
        sprintf(b->char_buf, "Invalid function name: '%s'", name);
        parse_err(fc->chunk_parse, b->char_buf);
    }

    Func* func = func_make(b->alc, fc, fc->scope, name, NULL);
    Idf* idf = idf_make(b->alc, idf_func, func);
    scope_set_idf(fc->nsc->scope, name, idf, fc);
    array_push(fc->funcs, func);

    if (str_is(name, "main")) {
        b->func_main = func;
        func->export_name = "main";
        fc->contains_main_func = true;
    }
    if (fc->is_header) {
        func->export_name = name;
    }

    parse_handle_func_args(fc, func);
}

void stage_1_header(Fc* fc){
    Build* b = fc->b;
    
    char* fn = tok(fc, true, false, true);
    int t = fc->chunk_parse->token;
    if(t != tok_string) {
        parse_err(fc->chunk_parse, "Expected a header name here wrapped in double-quotes");
    }

    Pkc *pkc = fc->nsc->pkc;
    Fc *hfc = pkc_load_header(pkc, fn, fc->chunk_parse);

    tok_expect(fc, "as", true, false);

    char* name = tok(fc, true, false, true);
    if(!is_valid_varname(name)) {
        sprintf(b->char_buf, "Invalid name: '%s'", name);
        parse_err(fc->chunk_parse, b->char_buf);
    }

    Idf* idf = idf_make(b->alc, idf_scope, hfc->scope);
    scope_set_idf(fc->scope, name, idf, fc);
}

void stage_1_class(Fc *fc, int type, int act) {
    Build* b = fc->b;
    
    char* name = tok(fc, true, false, true);
    if(!is_valid_varname(name)) {
        sprintf(b->char_buf, "Invalid type name: '%s'", name);
        parse_err(fc->chunk_parse, b->char_buf);
    }

    Class* class = class_make(b->alc, b, type);
    class->fc = fc;
    class->name = name;
    class->ir_name = gen_export_name(fc->nsc, name);
    class->scope = scope_sub_make(b->alc, sc_default, fc->scope, NULL);

    if(str_is(tok(fc, false, false, false), "[")) {
        char* tkn = tok(fc, false, false, true);
        Array* generic_names = array_make(b->alc, 2);
        while (true) {
            tkn = tok(fc, true, false, true);
            if(!is_valid_varname(tkn)){
                sprintf(b->char_buf, "Invalid generic type name: '%s'", tkn);
                parse_err(fc->chunk_parse, b->char_buf);
            }
            if (array_contains(generic_names, tkn, arr_find_str)) {
                sprintf(b->char_buf, "Duplicate generic type name: '%s'", tkn);
                parse_err(fc->chunk_parse, b->char_buf);
            }
            array_push(generic_names, tkn);
            if (str_is(tok_expect_two(fc, ",", "]", true, false), "]"))
                break;
        }
        class->generics = map_make(b->alc);
        class->generic_names = generic_names;
        class->is_generic_base = true;
    }

    Scope* nsc_scope = fc->nsc->scope;
    Idf* idf = idf_make(b->alc, idf_class, class);
    scope_set_idf(nsc_scope, name, idf, fc);
    array_push(fc->classes, class);
    if(!nsc_scope->type_identifiers)
        nsc_scope->type_identifiers = map_make(b->alc);
    map_set_force_new(nsc_scope->type_identifiers, name, idf);
    if(!class->is_generic_base) {
        scope_set_idf(class->scope, "CLASS", idf, fc);
        array_push(b->classes, class);
        if(class->type == ct_class) {
            class->gc_vtable_index = ++b->gc_vtables;
        }
    }

    //
    if(type == ct_int) {
        class->size = b->ptr_size;
        class->allow_math = true;
        char* tkn = tok(fc, true, false, true);
        if(is_valid_number(tkn)) {
            int size = atoi(tkn);
            if (size != 1 && size != 2 && size != 4 && size != 8) {
                sprintf(b->char_buf, "Invalid integer size: '%d'", size);
                parse_err(fc->chunk_parse, b->char_buf);
            }
            class->size = size;
            tkn = tok(fc, true, false, true);
        }
        if(str_is(tkn, "unsigned")) {
            class->is_signed = false;
            tkn = tok(fc, true, false, true);
        }
        tok_back(fc);
    } else if(type == ct_float) {
        class->size = b->ptr_size;
        class->allow_math = true;
        char* tkn = tok(fc, true, false, true);
        if(is_valid_number(tkn)) {
            int size = atoi(tkn);
            if (size != 4 && size != 8) {
                sprintf(b->char_buf, "Invalid float size: '%d'", size);
                parse_err(fc->chunk_parse, b->char_buf);
            }
            class->size = size;
            tkn = tok(fc, true, false, true);
        }
        tok_back(fc);
    } else if(type == ct_ptr) {
        class->size = b->ptr_size;
        char* tkn = tok(fc, true, false, true);
        if(str_is(tkn, "math")) {
            class->allow_math = true;
            tkn = tok(fc, true, false, true);
        }
        tok_back(fc);
    } else if(type == ct_bool) {
        class->size = 1;
    } else if(type == ct_struct) {
        char* tkn = tok(fc, true, false, true);
        if(str_is(tkn, "packed")) {
            class->packed = true;
            tkn = tok(fc, true, false, true);
        }
        tok_back(fc);
    }

    tok_expect(fc, "{", true, true);

    class->body = chunk_clone(b->alc, fc->chunk_parse);

    skip_body(fc);
}

void stage_1_use(Fc* fc){

    char* pk = NULL;
    char* ns = tok(fc, true, false, true);
    if(tok_id_next(fc) == tok_char && tok_read_byte(fc, 1) == ':') {
        pk = ns;
        tok(fc, false, false, true);
        ns = tok(fc, false, false, true);
    }

    Pkc* pkc = fc->nsc->pkc;
    if(pk) {
        pkc = pkc_load_pkc(pkc, pk, fc->chunk_parse);
    }

    Nsc* nsc = nsc_load(pkc, ns, true, fc->chunk_parse);

    Build *b = fc->b;
    Idf* idf = idf_make(b->alc, idf_scope, nsc->scope);
    scope_set_idf(fc->scope, ns, idf, fc);
}

void stage_1_global(Fc* fc, bool shared){

    Build *b = fc->b;
    char* name = tok(fc, true, false, true);

    Global* g = al(fc->alc, sizeof(Global));
    g->name = name;
    g->export_name = gen_export_name(fc->nsc, name);
    g->type = NULL;
    g->value = NULL;
    g->chunk_type = NULL;
    g->chunk_value = NULL;
    g->is_shared = shared;
    g->is_mut = true;

    Idf* idf = idf_make(b->alc, idf_global, g);
    scope_set_idf(fc->nsc->scope, name, idf, fc);
    array_push(fc->globals, g);

    tok_expect(fc, ":", true, false);

    g->chunk_type = chunk_clone(fc->alc, fc->chunk_parse);

    skip_type(fc);

    tok_skip_whitespace(fc);
    if (tok_read_byte(fc, 0) == tok_scope_open && tok_read_byte(fc, 1 + sizeof(int)) == '(') {
        char *tkn = tok(fc, true, true, true);
        g->chunk_value = chunk_clone(b->alc, fc->chunk_parse);
        skip_body(fc);
    }
}

void stage_1_value_alias(Fc* fc) {
    Build *b = fc->b;
    char* name = tok(fc, true, false, true);
    if(!is_valid_varname(name)) {
        sprintf(b->char_buf, "Invalid value alias name: '%s'", name);
        parse_err(fc->chunk_parse, b->char_buf);
    }

    tok_expect(fc, "(", true, false);

    Chunk *chunk = chunk_clone(fc->alc, fc->chunk_parse);
    ValueAlias *va = al(fc->alc, sizeof(ValueAlias));
    va->chunk = chunk;
    va->fc = fc;

    skip_body(fc);

    Idf* idf = idf_make(b->alc, idf_value_alias, va);
    scope_set_idf(fc->nsc->scope, name, idf, fc);
}

void stage_1_snippet(Fc* fc) {
    Build *b = fc->b;
    Allocator *alc = fc->alc;

    char* name = tok(fc, true, false, true);
    if(!is_valid_varname(name)) {
        sprintf(b->char_buf, "Invalid snippet name: '%s'", name);
        parse_err(fc->chunk_parse, b->char_buf);
    }

    tok_expect(fc, "(", false, false);
    Array* args = array_make(alc, 4);
    char* tkn = tok(fc, true, true, true);
    while(!str_is(tkn, ")")) {
        if(!is_valid_varname(tkn)) {
            sprintf(b->char_buf, "Invalid snippet argument name: '%s'", name);
            parse_err(fc->chunk_parse, b->char_buf);
        }
        char* fn = tkn;
        tok_expect(fc, ":", true, false);
        tkn = tok(fc, true, true, true);
        int type = -1;
        if(str_is(tkn, "V")) {
            type = snip_value;
        } else if(str_is(tkn, "T")) {
            type = snip_type;
        } else {
            sprintf(b->char_buf, "Expect 'V' (value) or 'T' (type), Found: '%s'", tkn);
            parse_err(fc->chunk_parse, b->char_buf);
        }

        SnipArg* sa = al(alc, sizeof(SnipArg));
        sa->name = fn;
        sa->type = type;
        array_push(args, sa);

        tkn = tok_expect_two(fc, ",", ")", true, true);
        if(str_is(tkn, ",")) {
            tkn = tok(fc, true, true, true);
        }
    }
    tok_expect(fc, "{", true, true);

    Snippet* snip = al(alc, sizeof(Snippet));
    snip->chunk = chunk_clone(alc, fc->chunk_parse);
    snip->args = args;
    snip->fc_scope = fc->scope;
    snip->exports = NULL;

    Idf* idf = idf_make(alc, idf_snippet, snip);
    scope_set_idf(fc->nsc->scope, name, idf, fc);

    skip_body(fc);

    tkn = tok(fc, true, true, true);

    if(str_is(tkn, "=>")) {
        tok_expect(fc, "(", true, true);
        Array* exports = array_make(alc, 4);
        tkn = tok(fc, true, true, true);
        while(!str_is(tkn, ")")) {
            if(!is_valid_varname(tkn)) {
                sprintf(b->char_buf, "Invalid snippet argument name: '%s'", name);
                parse_err(fc->chunk_parse, b->char_buf);
            }
            array_push(exports, tkn);
            tkn = tok_expect_two(fc, ",", ")", true, true);
            if(str_is(tkn, ",")) {
                tkn = tok(fc, true, true, true);
            }
        }
        snip->exports = exports;
    } else {
        tok_back(fc);
    }

}

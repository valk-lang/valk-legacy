
#include "../all.h"

void stage_parse(Fc *fc);
void stage_1_func(Fc* fc, int act);
void stage_1_header(Fc* fc);
void stage_1_class(Fc* fc, int type, int act);
void stage_1_use(Fc* fc);
void stage_1_global(Fc* fc, bool shared);

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

    Func* func = func_make(b->alc, fc, name, NULL);
    Idf* idf = idf_make(b->alc, idf_func, func);
    scope_set_idf(fc->nsc->scope, name, idf, fc);
    array_push(fc->funcs, func);

    if (str_is(name, "main")) {
        b->func_main = func;
        func->export_name = "main";
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
    class->name = name;
    class->ir_name = gen_export_name(fc->nsc, name);

    Idf* idf = idf_make(b->alc, idf_class, class);
    scope_set_idf(fc->nsc->scope, name, idf, fc);
    array_push(fc->classes, class);

    //
    if(type == ct_int) {
        class->allow_math = true;
        class->size = b->ptr_size;
        char* tkn = tok(fc, true, true, true);
        if(is_valid_number(tkn)) {
            int size = atoi(tkn);
            if (size != 1 && size != 2 && size != 4 && size != 8 && size != 16) {
                sprintf(b->char_buf, "Invalid integer size: '%d'", size);
                parse_err(fc->chunk_parse, b->char_buf);
            }
            class->size = size;
            tkn = tok(fc, true, true, true);
        }
        if(str_is(tkn, "unsigned")) {
            class->is_signed = false;
            tkn = tok(fc, true, true, true);
        }
        tok_back(fc);
    } else if(type == ct_float) {
        class->allow_math = true;
    } else if(type == ct_ptr) {
        char* tkn = tok(fc, true, true, true);
        if(str_is(tkn, "math")) {
            class->allow_math = true;
            tkn = tok(fc, true, true, true);
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

    tok_expect(fc, ":", true, false);

    g->chunk_type = chunk_clone(fc->alc, fc->chunk_parse);
    char* tkn = tok(fc, true, true, true);

    skip_type(fc);

    tok_skip_whitespace(fc);
    if (tok_read_byte(fc, 0) == tok_scope_open && tok_read_byte(fc, 1 + sizeof(int)) == '(') {
        tkn = tok(fc, true, true, true);
        g->chunk_value = chunk_clone(b->alc, fc->chunk_parse);
        skip_body(fc);
    }
}
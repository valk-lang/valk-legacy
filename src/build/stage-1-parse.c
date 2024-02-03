
#include "../all.h"

void stage_parse(Fc *fc);
void stage_1_func(Fc* fc, int act);
void stage_1_header(Fc* fc);
void stage_1_class(Fc* fc, int type, int act);

void stage_1_parse(Fc* fc) {
    Build* b = fc->b;

    if (b->verbose > 2)
        printf("Stage 1 | Parse: %s\n", fc->path);

    stage_parse(fc);

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
        if(str_is(tkn, "header")) {
            stage_1_header(fc);
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
        if(str_is(tkn, "int")) {
            stage_1_class(fc, ct_int, act);
            continue;
        }
        if(str_is(tkn, "float")) {
            stage_1_class(fc, ct_float, act);
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
    }

    tok_expect(fc, "(", true, false);
    func->chunk_args = chunk_clone(fc->alc, fc->chunk_parse);
    skip_body(fc);

    char *tkn = tok(fc, true, true, true);
    char* end = fc->is_header ? ";" : "{";

    if(!str_is(tkn, end)) {
        // Has return type
        tok_back(fc);
        func->chunk_rett = chunk_clone(fc->alc, fc->chunk_parse);
        skip_type(fc);
        tok_expect(fc, end, true, true);
    }

    if(!fc->is_header) {
        func->chunk_body = chunk_clone(fc->alc, fc->chunk_parse);
        skip_body(fc);
    }
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

    tok_expect(fc, "{", true, true);

    Class* class = class_make(b->alc);
    class->name = name;
    class->body = chunk_clone(b->alc, fc->chunk_parse);

    Idf* idf = idf_make(b->alc, idf_class, class);
    scope_set_idf(fc->nsc->scope, name, idf, fc);
    array_push(fc->classes, class);

    skip_body(fc);
}

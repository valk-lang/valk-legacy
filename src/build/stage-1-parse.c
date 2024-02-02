
#include "../all.h"

void stage_parse(Fc *fc);
void stage_1_func(Fc* fc, int act);

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

    if (str_is(name, "main")) {
        b->func_main = func;
    }

    bool newline = false;
    char* tkn = tok(fc, true, false, true);
    if(str_is(tkn, "")) {
        newline = true;
        tkn = tok(fc, true, true, true);
    }
    if(str_is(tkn, "(")) {
        // Has args
        if(newline) {
            parse_err(fc->chunk_parse, "The '(' character must be placed on the same line as the function name");
        }

        func->chunk_args = chunk_clone(fc->alc, fc->chunk_parse);
        skip_body(fc);

        tkn = tok(fc, true, true, true);
    }
    if(!str_is(tkn, "{")) {
        // Has return type
        tok_back(fc);
        func->chunk_rett = chunk_clone(fc->alc, fc->chunk_parse);
        // Skip type
        tok_expect(fc, "{", true, true);
    }

    func->chunk_body = chunk_clone(fc->alc, fc->chunk_parse);
    skip_body(fc);
}

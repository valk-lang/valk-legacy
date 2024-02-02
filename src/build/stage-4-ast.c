
#include "../all.h"

void stage_ast(Fc *fc);

void stage_4_ast(Fc* fc) {
    Build* b = fc->b;

    if (b->verbose > 2)
        printf("Stage 4 | Parse AST: %s\n", fc->path);

    fc->alc_ast = b->alc;

    stage_ast(fc);

    stage_4_ir(fc);
}

void stage_ast(Fc *fc) {
    if(fc->is_header)
        return;

    usize start = microtime();

    Array* funcs = fc->funcs;
    for(int i = 0; i < funcs->length; i++) {
        Func* func = array_get_index(funcs, i);
        *fc->chunk_parse = *func->chunk_body;
        read_ast(fc, func->scope, false);
    }

    fc->b->time_parse += microtime() - start;
}

void read_ast(Fc *fc, Scope *scope, bool single_line) {
    Allocator* alc = fc->alc_ast;
    Build* b = fc->b;
    Chunk* chunk = fc->chunk_parse;

    if(!scope->ast)
        scope->ast = array_make(alc, 200);

    bool first = false;

    while(true) {

        if(single_line && first)
            return;

        char* tkn = tok(fc, true, true, true);

        if(tkn[0] == 0) {
            sprintf(b->char_buf, "Unexpected end of file: '%s'", tkn);
            parse_err(chunk, b->char_buf);
        }

        int t = chunk->token;
        if(t == tok_scope_close)
            break;

        first = true;

        //

        //
        sprintf(b->char_buf, "Unexpected token: '%s'", tkn);
        parse_err(chunk, b->char_buf);
    }
}


#include "../all.h"

void stage_ast(Fc *fc);
void ast_handle_idf(Fc *fc, Scope *scope, Idf *idf);

void stage_4_ast(Fc *fc) {
    if (fc->is_header)
        return;

    Build *b = fc->b;

    if (b->verbose > 2)
        printf("Stage 4 | Parse AST: %s\n", fc->path);

    fc->alc_ast = b->alc;

    stage_ast(fc);

    stage_4_ir(fc);
}

void stage_ast(Fc *fc) {

    usize start = microtime();

    Array *funcs = fc->funcs;
    for (int i = 0; i < funcs->length; i++) {
        Func *func = array_get_index(funcs, i);
        *fc->chunk_parse = *func->chunk_body;
        read_ast(fc, func->scope, false);
    }

    fc->b->time_parse += microtime() - start;
}

void read_ast(Fc *fc, Scope *scope, bool single_line) {
    Allocator *alc = fc->alc_ast;
    Build *b = fc->b;
    Chunk *chunk = fc->chunk_parse;

    if (!scope->ast)
        scope->ast = array_make(alc, 200);

    bool first = false;

    while (true) {

        if (single_line && first)
            return;

        char *tkn = tok(fc, true, true, true);
        int t = chunk->token;

        if (tkn[0] == 0) {
            sprintf(b->char_buf, "Unexpected end of file: '%s'", tkn);
            parse_err(chunk, b->char_buf);
        }

        if (t == tok_scope_close)
            break;

        first = true;
        //

        if (t == tok_id) {
            Idf *idf = read_idf(fc, scope, tkn, true);
            ast_handle_idf(fc, scope, idf);
            continue;
        }

        //
        sprintf(b->char_buf, "Unexpected token: '%s'", tkn);
        parse_err(chunk, b->char_buf);
    }
}

void ast_handle_idf(Fc *fc, Scope *scope, Idf *idf) {
    Build *b = fc->b;
    Allocator *alc = fc->alc_ast;
    Chunk *chunk = fc->chunk_parse;

    int type = idf->type;

    if (type == idf_scope) {
        tok_expect(fc, ".", false, false);
        char *tkn = tok(fc, false, false, true);
    }

    sprintf(b->char_buf, "This cannot be used inside a function. (identifier-type:%d)", idf->type);
    parse_err(chunk, b->char_buf);
}

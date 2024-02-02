
#include "../all.h"

void stage_ast(Fc *fc);

void stage_4_ast(Fc* fc) {
    Build* b = fc->b;

    if (b->verbose > 2)
        printf("Stage 4 | Parse AST: %s\n", fc->path);

    stage_ast(fc);

    stage_4_ir(fc);
}

void stage_ast(Fc *fc) {
    if(fc->is_header)
        return;

    Array* funcs = fc->funcs;
    for(int i = 0; i < funcs->length; i++) {
        Func* func = array_get_index(funcs, i);
        *fc->chunk_parse = *func->chunk_body;
        read_ast(fc, func->scope, false);
    }
}

void read_ast(Fc *fc, Scope *scope, bool single_line) {
}

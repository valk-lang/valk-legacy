
#include "../all.h"

void stage_ast(Fc *fc);

void stage_4_ast(Fc* fc) {
    Build* b = fc->b;

    if (b->verbose > 2)
        printf("Stage 4 | Parse AST: %s\n", fc->path);

    stage_ast(fc);
}

void stage_ast(Fc *fc) {
}

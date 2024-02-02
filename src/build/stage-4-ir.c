
#include "../all.h"

void stage_4_ir(Fc* fc) {
    if(fc->is_header)
        return;

    Build* b = fc->b;

    if (b->verbose > 2)
        printf("Stage 4 | Generate IR: %s\n", fc->path);

    Allocator* alc = fc->alc_ast;
    usize start = microtime();



    b->time_ir += microtime() - start;
    alc_wipe(alc);
}

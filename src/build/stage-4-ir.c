
#include "../all.h"

void stage_4_ir(Fc* fc) {
    if(fc->is_header)
        return;

    Build* b = fc->b;

    if (b->verbose > 2)
        printf("Stage 4 | Generate IR: %s\n", fc->path);

    Allocator* alc = fc->alc_ast;
    usize start = microtime();

    IR* ir = ir_make(fc);
    char* ir_code = str_to_chars(alc, ir->code_final);
    char *ir_hash = al(b->alc, 64);
    ctxhash(ir_code, ir_hash);

    b->time_ir += microtime() - start;

    char* old_hash = "";
    if(file_exists(fc->path_cache)) {
        Str* buf = str_make(alc, 100);
        file_get_contents(buf, fc->path_cache);
    }
    if(!str_is(old_hash, ir_hash)) {
        fc->ir_changed = true;
        fc->hash = ir_hash;
        write_file(fc->path_ir, ir_code, false);
        if(b->verbose > 2)
            printf("> IR changed: %s\n", fc->path_ir);
    }

    alc_wipe(alc);
}

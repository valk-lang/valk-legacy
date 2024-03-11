
#include "../all.h"

void stage_4_ir(Unit* u) {

    Build* b = u->b;

    // printf("💾 Before: %.2f MB\n", (double)(get_mem_usage()) / (1024 * 1024));

    if (b->verbose > 2)
        printf("Stage 4 | Generate IR: %s\n", u->nsc->name);

    Allocator* alc = u->b->alc_ast;
    usize start = microtime();

    IR* ir = ir_make(u, b->parser);
    char* ir_code = str_to_chars(alc, ir->code_final);
    char *ir_hash = al(b->alc, 64);
    ctxhash(ir_code, ir_hash);

    b->time_ir += microtime() - start;

    char* old_hash = "";
    if(file_exists(u->path_cache)) {
        usize start = microtime();
        Str* buf = str_make(alc, 100);
        file_get_contents(buf, u->path_cache);
        b->time_io += microtime() - start;
        old_hash = str_to_chars(alc, buf);
    }
    if(!str_is(old_hash, ir_hash)) {
        usize start = microtime();
        u->ir_changed = true;
        u->hash = ir_hash;
        write_file(u->path_ir, ir_code, false);
        if(b->verbose > 2)
            printf("> IR changed: %s\n", u->path_ir);
        b->time_io += microtime() - start;
    }

    // printf("💾 After: %.2f MB\n", (double)(get_mem_usage()) / (1024 * 1024));
    size_t mem = get_mem_usage();
    if (mem > b->mem_parse)
        b->mem_parse = mem;

    alc_wipe(alc);
}

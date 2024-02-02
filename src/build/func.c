
#include "../all.h"

Func* func_make(Allocator* alc, Fc* fc, char* name, char* export_name) {
    Func* f = al(alc, sizeof(Func));
    f->fc = fc;
    f->name = name;
    f->export_name = export_name;
    f->scope = scope_make(alc, fc->nsc->scope);
    f->chunk_args = NULL;
    f->chunk_rett = NULL;
    f->chunk_body = NULL;

    if(!f->export_name) {
        f->export_name = gen_export_name(fc->nsc, name);
    }

    return f;
}

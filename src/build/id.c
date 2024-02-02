
#include "../all.h"

Idf* idf_make(Allocator* alc, int type, void* item) {
    Idf* idf = al(alc, sizeof(Idf));
    idf->type = type;
    idf->item = item;
    return idf;
}
Decl* decl_make(Allocator* alc, Type* type, bool is_arg) {
    Decl* d = al(alc, sizeof(Decl));
    d->type = type;
    d->ir_var = NULL;
    d->is_arg = is_arg;
    d->is_mut = false;
    return d;
}

char* gen_export_name(Nsc* nsc, char* suffix) {
    char name[512];
    Pkc* pkc = nsc->pkc;
    Build* b = pkc->b;
    sprintf(name, "%s__%s__%s_%d", pkc->name, nsc->name, suffix, b->export_count++);
    return dups(b->alc, name);
}

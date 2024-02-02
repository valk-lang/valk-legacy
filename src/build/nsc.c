
#include "../all.h"

Nsc* nsc_make(Allocator* alc, Pkc* pkc, char* name, char* dir) {
    Nsc* nsc = al(alc, sizeof(Nsc));
    nsc->pkc = pkc;
    nsc->name = name;
    nsc->dir = dir;
    nsc->scope = scope_make(alc, NULL);
    return nsc;
}

Nsc* nsc_try_load(Pkc* pkc, char* name) {
    Nsc* nsc = map_get(pkc->pkc_by_name, name);
    if(nsc)
        return nsc;
    // Check config

    return nsc;
}


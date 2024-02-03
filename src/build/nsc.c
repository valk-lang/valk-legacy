
#include "../all.h"

Nsc* nsc_make(Allocator* alc, Pkc* pkc, char* name, char* dir) {
    Nsc* nsc = al(alc, sizeof(Nsc));
    nsc->pkc = pkc;
    nsc->name = name;
    nsc->dir = dir;
    nsc->scope = scope_make(alc, NULL);
    return nsc;
}

Nsc* nsc_load(Pkc* pkc, char* name, bool must_exist) {
    Nsc* nsc = map_get(pkc->pkc_by_name, name);
    if(nsc)
        return nsc;

    // Check config
    Build* b = pkc->b;
    if(!pkc->config) {
        if(!must_exist)
            return NULL;
        sprintf(b->char_buf, "Trying to load namespace '%s' from a package without a config", name);
        build_err(b, b->char_buf);
    }

    char* dir = cfg_get_nsc_dir(pkc->config, name, b->alc);
    if(!dir) {
        if(!must_exist)
            return NULL;
        sprintf(b->char_buf, "Namespace '%s' not found in config: '%s'", name, pkc->config->path);
        build_err(b, b->char_buf);
    }
    if(!file_exists(dir)) {
        if(!must_exist)
            return NULL;
        sprintf(b->char_buf, "Namespace directory of '%s' does not exist: '%s'", name, dir);
        build_err(b, b->char_buf);
    }

    Nsc* ns2 = map_get(b->nsc_by_path, dir);
    if(ns2) {
        sprintf(b->char_buf, "There are 2 namesapces pointing to the same directory: '%s' | '%s' => '%s' ", ns2->name, name, dir);
        build_err(b, b->char_buf);
    }

    nsc = nsc_make(b->alc, pkc, name, dir);

    Array* files = get_subfiles(b->alc, dir, false, true);
    for(int i = 0; i < files->length; i++) {
        char* path = array_get_index(files, i);
        if(ends_with(path, ".vo"))
            fc_make(nsc, path);
    }

    map_set(pkc->pkc_by_name, name, nsc);
    return nsc;
}


Nsc* get_volt_nsc(Build* b, char* name) {
    //
    Pkc* pkc = b->pkc_volt;
    if(!pkc)
        return NULL;
    return nsc_load(pkc, name, true);
}

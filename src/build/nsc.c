
#include "../all.h"

Nsc* nsc_make(Allocator* alc, Pkc* pkc, char* name, char* dir) {
    Nsc* nsc = al(alc, sizeof(Nsc));
    nsc->pkc = pkc;
    nsc->name = name;
    nsc->dir = dir;
    nsc->scope = scope_make(alc, NULL);
    nsc->fcs = array_make(alc, 20);

    char *path_o = al(alc, VOLT_PATH_MAX);
    sprintf(path_o, "%s%s_%s.o", pkc->b->cache_dir, nsc->name, nsc->pkc->name);
    nsc->path_o = path_o;

    return nsc;
}

Nsc* nsc_load(Pkc* pkc, char* name, bool must_exist, Chunk* chunk) {
    Nsc* nsc = map_get(pkc->namespaces, name);
    if(nsc)
        return nsc;

    // Check config
    Build* b = pkc->b;
    if(!pkc->config) {
        if(!must_exist)
            return NULL;
        sprintf(b->char_buf, "Trying to load namespace '%s' from a package without a config", name);
        chunk ? parse_err(chunk, b->char_buf) : build_err(b, b->char_buf);
    }

    char* dir = cfg_get_nsc_dir(pkc->config, name, b->alc);
    if(!dir) {
        if(!must_exist)
            return NULL;
        sprintf(b->char_buf, "Namespace '%s' not found in config: '%s'", name, pkc->config->path);
        chunk ? parse_err(chunk, b->char_buf) : build_err(b, b->char_buf);
    }
    if(!file_exists(dir)) {
        if(!must_exist)
            return NULL;
        sprintf(b->char_buf, "Namespace directory of '%s' does not exist: '%s'", name, dir);
        chunk ? parse_err(chunk, b->char_buf) : build_err(b, b->char_buf);
    }

    Nsc* ns2 = map_get(b->nsc_by_path, dir);
    if(ns2) {
        sprintf(b->char_buf, "There are 2 namesapces pointing to the same directory: '%s' | '%s' => '%s' ", ns2->name, name, dir);
        chunk ? parse_err(chunk, b->char_buf) : build_err(b, b->char_buf);
    }

    nsc = nsc_make(b->alc, pkc, name, dir);

    Array* files = get_subfiles(b->alc, dir, false, true);
    for(int i = 0; i < files->length; i++) {
        char* path = array_get_index(files, i);
        if(ends_with(path, ".vo"))
            fc_make(nsc, path);
    }

    map_set(pkc->namespaces, name, nsc);
    return nsc;
}


Nsc* get_volt_nsc(Build* b, char* name) {
    //
    Pkc* pkc = b->pkc_volt;
    if(!pkc)
        return NULL;
    return nsc_load(pkc, name, true, NULL);
}

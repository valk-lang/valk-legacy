
#include "../all.h"

Nsc* nsc_make(Allocator* alc, Pkc* pkc, char* name, char* dir) {
    Nsc* nsc = al(alc, sizeof(Nsc));
    nsc->pkc = pkc;
    nsc->name = name;
    nsc->dir = dir;
    nsc->scope = scope_make(alc, sc_default, NULL);

    if(dir && watch_dirs) {
        array_push_unique_chars(watch_dirs, dir);
    }

    char *path_o = al(alc, VALK_PATH_MAX);
    sprintf(path_o, "%s%s_%s.o", pkc->b->cache_dir, nsc->name, nsc->pkc->name);
    char *path_a = al(alc, VALK_PATH_MAX);
    sprintf(path_a, "%s%s_%s.a", pkc->b->cache_dir, nsc->name, nsc->pkc->name);
    char *path_ir = al(alc, VALK_PATH_MAX);
    sprintf(path_ir, "%s%s_%s.ir", pkc->b->cache_dir, nsc->name, nsc->pkc->name);
    char *path_cache = al(alc, VALK_PATH_MAX);
    sprintf(path_cache, "%s%s_%s.json", pkc->b->cache_dir, nsc->name, nsc->pkc->name);

    char uh[64];
    ctxhash(dir ? dir : path_o, uh);

    Unit* u = al(alc, sizeof(Unit));
    u->b = pkc->b;
    u->nsc = nsc;
    //
    u->path_o = path_o;
    u->path_a = path_a;
    u->path_ir = path_ir;
    u->path_cache = path_cache;
    u->unique_hash = dups(alc, uh);
    u->hash = NULL;
    //
    u->func_irs = array_make(alc, 50);
    u->ir_start = NULL;
    u->ir_end = NULL;
    //
    u->parser = parser_make(alc, u);
    //
    u->funcs = array_make(alc, 50);
    u->classes = array_make(alc, 50);
    u->aliasses = array_make(alc, 20);
    u->globals = array_make(alc, 20);
    u->tests = array_make(alc, 20);
    //
    u->pool_parsers = array_make(alc, 10);

    // Cache
    u->cache = NULL;
    u->nsc_deps_hash = NULL;
    u->nsc_deps = array_make(alc, 8);
    u->c_modtime = 0;
    u->c_filecount = 0;
    u->changed = true;

    u->id = pkc->b->units->length;

    u->string_count = 0;
    u->export_count = 0;

    u->ir_changed = false;
    u->is_main = false;

    unit_load_cache(u);

    nsc->unit = u;

    array_push(pkc->b->units, u);

    return nsc;
}

Nsc* nsc_load(Pkc* pkc, char* name, bool must_exist, Parser* p) {
    Nsc* nsc = map_get(pkc->namespaces, name);
    if(nsc)
        return nsc;

    // Check config
    Build* b = pkc->b;
    if(!pkc->config) {
        if(!must_exist)
            return NULL;
        p ? parse_err(p, -1, "Trying to load namespace '%s' from a package without a config", name)
        : build_err(b, "Trying to load namespace '%s' from a package without a config", name);
    }

    char* dir = cfg_get_nsc_dir(pkc->config, name, b->alc);
    if(!dir) {
        if(!must_exist)
            return NULL;
        p ? parse_err(p, -1, "Namespace '%s' not found in config: '%s'", name, pkc->config->path)
        : build_err(b, "Namespace '%s' not found in config: '%s'", name, pkc->config->path);
    }
    if(!file_exists(dir)) {
        if(!must_exist)
            return NULL;
        p ? parse_err(p, -1, "Namespace directory of '%s' does not exist: '%s'", name, dir)
        : build_err(b, "Namespace directory of '%s' does not exist: '%s'", name, dir);
    }

    Nsc* ns2 = map_get(b->nsc_by_path, dir);
    if(ns2) {
        p ? parse_err(p, -1, "There are 2 namesapces pointing to the same directory: '%s' | '%s' => '%s' ", ns2->name, name, dir)
        : build_err(b, "There are 2 namesapces pointing to the same directory: '%s' | '%s' => '%s' ", ns2->name, name, dir);
    }

    nsc = nsc_make(b->alc, pkc, name, dir);

    Array* files = get_subfiles(b->alc, dir, false, true);
    int mod_time = 0;
    int file_count = 0;
    loop(files, i) {
        char* path = array_get_index(files, i);
        if(ends_with(path, ".valk")) {
            Fc* fc = fc_make(nsc, path, false);
            int time = get_mod_time(fc->path);
            if(time > mod_time)
                mod_time = time;
            file_count++;
        }
    }
    nsc->mod_time = mod_time;
    nsc->file_count = file_count;

    map_set_force_new(pkc->namespaces, name, nsc);
    return nsc;
}


Nsc* get_valk_nsc(Build* b, char* name) {
    //
    Pkc* pkc = b->pkc_valk;
    if(!pkc)
        return NULL;
    return nsc_load(pkc, name, true, NULL);
}

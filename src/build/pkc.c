
#include "../all.h"

Pkc* pkc_load_from_dir(Build* b, char* dir, char* name_suggestion);

Pkc* pkc_make(Allocator* alc, Build* b, char* name_suggestion) {
    Pkc* pkc = al(alc, sizeof(Pkc));
    pkc->b = b;
    pkc->dir = NULL;
    pkc->namespaces = map_make(alc);
    pkc->pkc_by_name = map_make(alc);
    pkc->header_dirs = NULL;
    pkc->headers_by_fn = NULL;

    char* buf = b->char_buf;
    sprintf(buf, "%s", name_suggestion);
    int i = 0;
    while(array_contains(b->used_pkc_names, name_suggestion, arr_find_str)){
        i++;
        sprintf(buf, "%s_%d", name_suggestion, i);
    }

    pkc->name = dups(alc, buf);

    array_push(b->pkcs, pkc);

    return pkc;
}

void pkc_set_dir(Pkc* pkc, char* dir) {
    Build* b = pkc->b;
    if(map_contains(b->pkc_by_dir, dir)) {
        sprintf(b->char_buf, "Compiler error: 2 packages with the same directory (%s)", dir);
        build_err(b, b->char_buf);
    }
    pkc->dir = dir;
    map_set(b->pkc_by_dir, dir, pkc);
    // Load config
    PkgConfig* cfg = load_config(b->alc, dir, b->str_buf, true);
    pkc->config = cfg;
}

Pkc* pkc_load_pkc(Pkc* pkc, char* name, Chunk* parsing_chunk) {
    Pkc* sub = map_get(pkc->pkc_by_name, name);
    if(sub)
        return sub;
    Build* b = pkc->b;

    char* dir;
    if(str_is(name, "volt")){
        // Get default volt package
        dir = al(b->alc, VOLT_PATH_MAX);
        strcpy(dir, get_binary_dir());
        strcat(dir, "lib/");
    } else {
        if(!pkc->config || !cfg_has_package(pkc->config, name)) {
            sprintf(b->char_buf, "Package '%s' not found", name);
            if(parsing_chunk) parse_err(parsing_chunk, b->char_buf);
            else build_err(b, b->char_buf);
        }
        dir = cfg_get_pkg_dir(pkc->config, name, b->alc);
    }
    if(!dir) {
        sprintf(b->char_buf, "Package directory for '%s' not found", name);
        build_err(b, b->char_buf);
    }
    sub = pkc_load_from_dir(pkc->b, dir, name);
    map_set(pkc->pkc_by_name, name, sub);

    if(b->verbose > 2)
        printf("Package '%s' loaded from '%s'\n", name, dir);

    return sub;
}

Pkc* pkc_load_from_dir(Build* b, char* dir, char* name_suggestion) {
    Pkc* pkc = map_get(b->pkc_by_dir, dir);
    if(pkc)
        return pkc;
    // Make new
    pkc = pkc_make(b->alc, b, name_suggestion);
    pkc_set_dir(pkc, dir);
    return pkc;
}

Fc* pkc_load_header(Pkc* pkc, char* fn, Chunk* chunk) {
    Build *b = pkc->b;
    if(pkc->headers_by_fn) {
        Fc *hfc = map_get(pkc->headers_by_fn, fn);
        if (hfc)
            return hfc;
    }

    if(!pkc->header_dirs) {
        if (!pkc->config) {
            sprintf(b->char_buf, "Trying to load header '%s' from a package without a config", fn);
            parse_err(chunk, b->char_buf);
        }

        pkc->header_dirs = cfg_get_header_dirs(pkc->config, b->alc, pkc->dir);
        pkc->headers_by_fn = map_make(b->alc);
    }
    Array* dirs = pkc->header_dirs;
    if(dirs->length == 0) {
        sprintf(b->char_buf, "Package config has no header directories defined in 'headers.dirs' | config: '%s'", pkc->config->path);
        parse_err(chunk, b->char_buf);
    }

    char path[VOLT_PATH_MAX];
    for (int i = 0; i < dirs->length; i++) {
        char* dir = array_get_index(dirs, i);
        sprintf(path, "%s%s.vh", dir, fn);
        if(file_exists(path)) {
            Fc* hfc = fc_make(b->nsc_main, dups(b->alc, path));
            return hfc;
        }
    }

    sprintf(b->char_buf, "Header '%s' not found in header directories from config: '%s'", fn, pkc->config->path);
    parse_err(chunk, b->char_buf);
    return NULL;
}


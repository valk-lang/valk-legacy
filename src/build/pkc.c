
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
    map_set_force_new(b->pkc_by_dir, dir, pkc);
    // Load config
    usize start = microtime();
    PkgConfig* cfg = load_config(b->alc, dir, b->str_buf, true);
    pkc->config = cfg;
    usize time = microtime() - start;
    b->time_io += time;
    if(b->parser_started) b->time_parse -= time;
}

Pkc* pkc_load_pkc(Pkc* pkc, char* name, Parser* p) {
    Pkc* sub = map_get(pkc->pkc_by_name, name);
    if(sub)
        return sub;
    Build* b = pkc->b;

    char* dir;
    if(str_is(name, "valk")){
        // Get default valk package
        dir = al(b->alc, VALK_PATH_MAX);
        strcpy(dir, get_binary_dir());
        strcat(dir, "lib/");
    } else {
        if(!pkc->config || !cfg_has_package(pkc->config, name)) {
            if(p) parse_err(p, -1, "Package '%s' not found", name);
            else build_err(b, b->char_buf);
        }
        dir = cfg_get_pkg_dir(pkc->config, name, b->alc);
    }
    if(!dir) {
        sprintf(b->char_buf, "Package directory for '%s' not found", name);
        build_err(b, b->char_buf);
    }
    sub = pkc_load_from_dir(pkc->b, dir, name);
    map_set_force_new(pkc->pkc_by_name, name, sub);

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

Fc* pkc_load_header(Pkc* pkc, char* fn, Parser* p, bool is_sub_header) {
    Build *b = pkc->b;
    if(pkc->headers_by_fn) {
        Fc *hfc = map_get(pkc->headers_by_fn, fn);
        if (hfc)
            return hfc;
    }

    if(!pkc->header_dirs) {
        if (!pkc->config) {
            parse_err(p, -1, "Trying to load header '%s' from a package without a config", fn);
        }

        pkc->header_dirs = cfg_get_header_dirs(pkc->config, b->alc, pkc->dir);
        pkc->headers_by_fn = map_make(b->alc);
    }
    Array* dirs = pkc->header_dirs;
    if(dirs->length == 0) {
        parse_err(p, -1, "Package config has no header directories defined in 'headers.dirs' | config: '%s'", pkc->config->path);
    }

    char path[VALK_PATH_MAX];
    loop(dirs, i) {
        char* dir = array_get_index(dirs, i);
        sprintf(path, "%s%s.vh", dir, fn);
        if(file_exists(path)) {
            Fc* hfc = fc_make(b->nsc_main, dups(b->alc, path), is_sub_header);
            hfc->header_pkc = pkc;
            map_set_force_new(pkc->headers_by_fn, fn, hfc);
            return hfc;
        }
    }

    parse_err(p, -1, "Header '%s' not found in header directories from config: '%s'", fn, pkc->config->path);
    return NULL;
}


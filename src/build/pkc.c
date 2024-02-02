
#include "../all.h"

Pkc* pkc_load_from_dir(Build* b, char* dir, char* name_suggestion);

Pkc* pkc_make(Allocator* alc, Build* b, char* name_suggestion) {
    Pkc* pkc = al(alc, sizeof(Pkc));
    pkc->b = b;
    pkc->dir = NULL;
    pkc->namespaces = map_make(alc);
    pkc->pkc_by_name = map_make(alc);
    pkc->is_main = false;

    char* buf = b->char_buf;
    sprintf(buf, "%s", name_suggestion);
    int i = 0;
    while(array_contains(b->used_pkc_names, name_suggestion, arr_find_str)){
        i++;
        sprintf(buf, "%s_%d", name_suggestion, i);
    }

    pkc->name = dups(alc, buf);

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
    // If no config -> build_err
}

Pkc* pkc_load_pkc(Pkc* pkc, char* name, Chunk* parsing_chunk) {
    Pkc* sub = map_get(pkc->pkc_by_name, name);
    if(sub)
        return sub;
    // Check config
    // Get pkg dir via config
    // if not found > parse error / build error
    // sub = pkc_load_from_dir(pkc->b, dir, name)
    // set pkc->pkc_by_name[dir] = sub;
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

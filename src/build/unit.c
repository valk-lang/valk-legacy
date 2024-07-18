
#include "../all.h"

void unit_load_cache(Unit* u) {
    if(!u->path_cache)
        return;
    if(!file_exists(u->path_cache)) {
        return;
    }
    Str* str_buf = u->b->str_buf;
    Allocator* alc = u->b->alc;

    file_get_contents(str_buf, u->path_cache);
    char *content = str_to_chars(alc, str_buf);
    cJSON *cache = cJSON_ParseWithLength(content, str_buf->length);
    u->cache = cache;
}

int unit_mod_time(Unit* u) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(u->cache, "mod_time");
    if(!item)
        return 0;
    return item->valueint;
}
void unit_set_mod_time(Unit* u, int time) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(u->cache, "mod_time");
    if(!item) {
        item = cJSON_CreateNumber(0);
        cJSON_AddItemToObject(u->cache, "mod_time", item);
    }
    cJSON_SetIntValue(item, time);
}
int unit_filecount(Unit* u) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(u->cache, "file_count");
    if(!item)
        return 0;
    return item->valueint;
}
void unit_set_filecount(Unit* u, int time) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(u->cache, "file_count");
    if(!item) {
        item = cJSON_CreateNumber(0);
        cJSON_AddItemToObject(u->cache, "file_count", item);
    }
    cJSON_SetIntValue(item, time);
}


bool unit_intern_changed(Unit* u) {
    if(u->b->is_clean)
        return true;
    if(!u->cache)
        return true;
    return unit_mod_time(u) != u->nsc->mod_time || unit_filecount(u) != u->nsc->file_count;
}
bool unit_extern_changed(Unit* u) {
    if(u->b->is_clean)
        return true;
    if(!u->cache)
        return true;
    cJSON *item = cJSON_GetObjectItemCaseSensitive(u->cache, "dependency_hash");
    if(!item)
        return true;

    char* cmp = item->valuestring;
    return !str_is(u->nsc_deps_hash, cmp);
}
void unit_gen_dep_hash(Unit* u) {
    Str *buf = u->b->str_buf;
    str_clear(buf);
    loop(u->nsc_deps, i) {
        Nsc* dep = array_get_index(u->nsc_deps, i);
        str_append_chars(buf, dep->dir ? dep->dir : "NO_DIR");
        str_append_char(buf, ';');
        str_append_int(buf, dep->mod_time);
        str_append_char(buf, ';');
        str_append_int(buf, dep->file_count);
    }
    u->nsc_deps_hash = str_to_chars(u->b->alc, buf);
}

void unit_update_cache(Unit* u) {
    cJSON* cache = u->cache;
    if(!cache || !cJSON_IsObject(cache)) {
        cache = cJSON_CreateObject();
        u->cache = cache;
    }
    cJSON *item_dh = cJSON_GetObjectItemCaseSensitive(cache, "dependency_hash");
    if(!item_dh) {
        item_dh = cJSON_CreateString(u->nsc_deps_hash);
        cJSON_AddItemToObject(cache, "dependency_hash", item_dh);
    } else {
        cJSON_SetValuestring(item_dh, u->nsc_deps_hash);
    }
    unit_set_mod_time(u, u->nsc->mod_time);
    unit_set_filecount(u, u->nsc->file_count);
    //
    char* content = cJSON_Print(cache);
    // printf("write: %s\n", content);
    // printf("write: %s\n", u->path_cache);
    write_file(u->path_cache, content, false);
    free(content);
}

void unit_check_cache(Unit* u) {
    if(u->nsc_deps_hash)
        return;
    unit_gen_dep_hash(u);
    u->changed = unit_intern_changed(u) || unit_extern_changed(u);
}


void unit_validate(Unit *u, Parser *p) {

    Build* b = u->b;
    Allocator* alc = b->alc; 

    Array* classes = u->classes;
    int class_count = classes->length;
    for (int i = 0; i < class_count; i++) {
        Class *class = array_get_index(classes, i);
        validate_class(p, class);
    }

    if(u->is_main) {
        Func *main = b->func_main;
        if (main) {
            func_validate_arg_count(p, main, false, 0, 1);

            if(main->arg_types->length > 0) {
                Array *types = array_make(b->alc, 2);
                Class *class_array = get_valk_class(b, "type", "Array");
                Class *gclass = get_generic_class(p, class_array, gen_type_array_1(alc, b, "String", false));
                array_push(types, type_gen_class(b->alc, gclass));

                func_validate_arg_type(p, main, 0, types);
            }

            func_validate_rett(p, main, gen_type_array_2(alc, b, NULL, false, "i32", false));
        }
    }
}

void validate_class(Parser *p, Class* class) {

    Build *b = p->b;
    Allocator* alc = b->alc; 

    Map *funcs = class->funcs;

    Func *add = map_get(funcs, "_add");
    if (add)
        func_validate_arg_count(p, add, false, 2, 2);

    Func *string = map_get(funcs, "_string");
    if (string) {
        func_validate_arg_count(p, string, false, 1, 1);
        func_validate_rett(p, string, gen_type_array_1(alc, b, "String", false));
    }

    Func *gc_free = map_get(funcs, "_gc_free");
    if (gc_free) {
        gc_free->use_if_class_is_used = true;
        func_validate_arg_count(p, gc_free, false, 1, 1);
        func_validate_rett_void(p, gc_free);
    }

    Func *gc_transfer = map_get(funcs, "_gc_transfer");
    if (gc_transfer) {
        func_validate_arg_count(p, gc_transfer, false, 1, 1);
        func_validate_rett_void(p, gc_transfer);
    }

    Func *gc_mark = map_get(funcs, "_gc_mark");
    if (gc_mark) {
        func_validate_arg_count(p, gc_mark, false, 1, 1);
        func_validate_rett_void(p, gc_mark);
    }

    Func *gc_mark_shared = map_get(funcs, "_gc_mark_shared");
    if (gc_mark_shared) {
        func_validate_arg_count(p, gc_mark_shared, false, 1, 1);
        func_validate_rett_void(p, gc_mark_shared);
    }

    Func *gc_share = map_get(funcs, "_gc_share");
    if (gc_share) {
        func_validate_arg_count(p, gc_share, false, 1, 1);
        func_validate_rett_void(p, gc_share);
    }

    Func *each = map_get(funcs, "_each");
    if (each) {
        func_validate_arg_count(p, each, false, 2, 2);
        func_validate_rett_count(p, each, false, 2, 2);
    }
}

Unit* unit_make(Build* b, Nsc* nsc, char* name) {
    Allocator* alc = b->alc;

    char *path_o = al(alc, VALK_PATH_MAX);
    sprintf(path_o, "%s%s.o", b->cache_dir, name);
    char *path_a = al(alc, VALK_PATH_MAX);
    sprintf(path_a, "%s%s.a", b->cache_dir, name);
    char *path_ir = al(alc, VALK_PATH_MAX);
    sprintf(path_ir, "%s%s.ir", b->cache_dir, name);
    char *path_cache = al(alc, VALK_PATH_MAX);
    sprintf(path_cache, "%s%s.json", b->cache_dir, name);

    char uh[64];
    ctxhash(path_o, uh);

    Unit *u = al(alc, sizeof(Unit));
    u->b = b;
    u->nsc = nsc;
    //
    u->path_o = path_o;
    u->path_a = path_a;
    u->path_ir = path_ir;
    u->path_cache = path_cache;
    u->unique_hash = uh;
    u->hash = NULL;
    //
    u->func_irs = array_make(alc, 50);
    u->ir_start = NULL;
    u->ir_end = NULL;
    //
    u->parser = parser_make(alc, u);
    //
    u->funcs = array_make(alc, 20);
    u->classes = array_make(alc, 10);
    u->aliasses = array_make(alc, 10);
    u->globals = array_make(alc, 10);
    u->tests = array_make(alc, 10);
    //
    u->pool_parsers = array_make(alc, 10);

    // Cache
    u->cache = NULL;
    u->nsc_deps_hash = NULL;
    u->nsc_deps = array_make(alc, 8);
    u->c_modtime = 0;
    u->c_filecount = 0;
    u->changed = true;

    u->id = b->units->length;

    u->string_count = 0;
    u->export_count = 0;

    u->ir_changed = false;
    u->is_main = false;

    unit_load_cache(u);

    if(nsc)
        nsc->unit = u;

    array_push(b->units, u);

    return u;
}

Unit* unit_make_for_generic(Build* b, char* hash, Class* base, Array* types) {
    Unit *u = unit_make(b, b->nsc_generated, hash);

    // Namespace dependencies
    array_push_unique_adr(u->nsc_deps, base->unit->nsc);
    loop(types, i) {
        Type* type = array_get_index(types, i);
        if(!type->class)
            continue;
        Nsc* nsc = type->class->unit->nsc;
        if(nsc) {
            array_push_unique_adr(u->nsc_deps, nsc);
        } else {
            // Generic namespace
            Unit* gu = type->class->unit;
            loop(gu->nsc_deps, o) {
                Nsc* nsc = array_get_index(gu->nsc_deps, o);
                array_push_unique_adr(u->nsc_deps, nsc);
            }
        }
    }

    unit_check_cache(u);

    return u;
}

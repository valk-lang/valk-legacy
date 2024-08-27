
#include "../all.h"

void class_generate_transfer(Parser* p, Build* b, Class* class, Func* func);
void class_generate_mark(Parser* p, Build* b, Class* class, Func* func);
void class_generate_mark_shared(Parser* p, Build* b, Class* class, Func* func);
void class_generate_share(Parser* p, Build* b, Class* class, Func* func);
void class_generate_free(Parser* p, Build* b, Class* class, Func* func);

Class* class_make(Allocator* alc, Build* b, Unit* u, int type) {
    Class* c = al(alc, sizeof(Class));
    c->b = b;
    c->type = type;
    c->act = act_public;
    c->fc = NULL;
    c->unit = u;
    c->name = NULL;
    c->ir_name = NULL;
    c->body = NULL;
    c->scope = NULL;
    c->props = map_make(alc);
    c->funcs = map_make(alc);
    //
    c->size = -1;
    c->gc_fields = 0;
    c->pool_index = 0;
    c->vtable = NULL;
    //
    c->packed = false;
    c->is_signed = true;
    //
    c->generics = NULL;
    c->generic_names = NULL;
    c->generic_types = NULL;
    c->generic_of = NULL;
    c->is_generic_base = false;
    c->in_header = false;
    c->is_used = false;
    c->use_gc_alloc = false;
    c->has_vtable = false;
    //
    return c;
}
ClassProp* class_prop_make(Build* b, Type* type, bool skip_default_value) {
    ClassProp* prop = al(b->alc, sizeof(ClassProp));
    prop->chunk_type = NULL;
    prop->chunk_value = NULL;
    prop->index = -1;
    prop->type = type;
    prop->skip_default_value = skip_default_value;
    return prop;
}

ClassProp* class_get_prop(Build* b, Class* class, char* name) {
    ClassProp* prop = map_get(class->props, name);
    if(!prop) {
        sprintf(b->char_buf, "Class property '%s' not found", name);
        build_err(b, b->char_buf);
    }
    return prop;
}

void class_create_vtable(Build* b, Class* class) {
    char tmp[1024];
    strcpy(tmp, class->ir_name);
    strcat(tmp, "_VTABLE");
    
    char* name = dups(b->alc, tmp);

    Global *g = global_make(b->alc, class->unit, class->fc, act_private_fc, name, false, true);
    g->type = type_cache_ptr(b);

    class->vtable = g;
    class->has_vtable = true;

    // Map* funcs = class->funcs;
    // Func* hook_transfer = map_get(funcs, "_gc_transfer");
    // Func* hook_mark = map_get(funcs, "_gc_mark");
    // Func* hook_mark_shared = map_get(funcs, "_gc_mark_shared");
    // Func* hook_free = map_get(funcs, "_gc_free");
    // if (hook_transfer)
    //     func_mark_used(hook_transfer);

    array_push(class->unit->globals, g);
    array_push(b->globals, g);
}

int class_pool_index(Class* class) {
    int size = class->size;
    int offset = map_contains(class->funcs, "_gc_free") ? 1 : 0;
    if(size <= sizeof(void *) * 2) {
        return 0 + offset;
    }
    if(size <= 128) {
        return (size / sizeof(void *) - 2) * 2 + offset;
    }
    int index = 34;
    int alc_size = 256;
    while(alc_size < size) {
        index += 2;
        alc_size *= 2;
    }
    return index + offset;
}

int class_determine_size(Build* b, Class* class) {

    int type = class->type;
    if(type != ct_class && type != ct_struct) {
        return class->size;
    }

    int size = 0;
    int largest = 0;
    int propc = class->props->values->length;
    for (int i = 0; i < propc; i++) {
        //
        ClassProp *prop = array_get_index(class->props->values, i);
        int prop_size = type_get_size(b, prop->type);
        if (prop_size == -1) {
            return -1;
        }
        if (prop_size > largest) {
            largest = prop_size;
        }
        size += prop_size;
        //
        if (class->packed)
            continue;
        // Calculate padding
        int next_i = i + 1;
        if (next_i < propc) {
            ClassProp *prop = array_get_index(class->props->values, i);
            int next_size = type_get_size(b, prop->type);
            if (next_size == -1) {
                return -1;
            }
            if (next_size > b->ptr_size)
                next_size = b->ptr_size;
            int rem = size % next_size;
            if (rem > 0) {
                // Add padding
                size += next_size - rem;
            }
        }
    }
    if (!class->packed) {
        if (largest > b->ptr_size)
            largest = b->ptr_size;
        if (largest > 0) {
            int rem = size % largest;
            if(rem > 0)
                size += largest - rem;
        }
    }

    if (size == 0) {
        size = 8;
    }
    class->size = size;
    class->pool_index = class_pool_index(class);

    return size;
}

void class_generate_internals(Parser* p, Build* b, Class* class) {

    Allocator *alc = b->alc;

    if (class->type == ct_class && map_get(class->funcs, "_v_transfer") == NULL) {
        char* buf = b->char_buf;

        // VTABLE_INDEX
        // if(b->verbose > 2)
        //     printf("Class: %s | vtable: %d | size: %d\n", class->name, class->gc_vtable_index, class->size);
        //
        // Idf* idf = idf_make(b->alc, idf_value, vgen_int(b->alc, class->gc_vtable_index, type_cache_u32(b)));
        // scope_set_idf(class->scope, "VTABLE_INDEX", idf, p);
        //
        Idf* idf = idf_make(b->alc, idf_global, get_valk_global(b, "mem", "stack"));
        scope_set_idf(class->scope, "STACK", idf, p);
        idf = idf_make(b->alc, idf_global, get_valk_global(b, "mem", "shared_age"));
        scope_set_idf(class->scope, "GC_AGE", idf, p);
        //
        idf = idf_make(b->alc, idf_value, vgen_int(b->alc, class->size + 8, type_cache_uint(b)));
        scope_set_idf(class->scope, "SIZE", idf, p);
        //
        idf = idf_make(b->alc, idf_global, get_valk_global(b, "mem", "mem_active"));
        scope_set_idf(class->scope, "GC_ACTIVE_SIZE", idf, p);
        idf = idf_make(b->alc, idf_global, get_valk_global(b, "mem", "mem_marked"));
        scope_set_idf(class->scope, "GC_MARK_SIZE", idf, p);
        idf = idf_make(b->alc, idf_func, get_valk_func(b, "mem", "gc_transfer"));
        scope_set_idf(class->scope, "GC_TRANSFER_FUNC", idf, p);
        idf = idf_make(b->alc, idf_func, get_valk_func(b, "mem", "gc_mark"));
        scope_set_idf(class->scope, "GC_MARK_FUNC", idf, p);
        idf = idf_make(b->alc, idf_func, get_valk_func(b, "mem", "gc_mark_shared"));
        scope_set_idf(class->scope, "GC_MARK_SHARED_FUNC", idf, p);
        idf = idf_make(b->alc, idf_func, get_valk_func(b, "mem", "gc_share"));
        scope_set_idf(class->scope, "GC_SHARE_FUNC", idf, p);
        idf = idf_make(b->alc, idf_func, get_valk_func(b, "mem", "gc_free"));
        scope_set_idf(class->scope, "GC_FREE_FUNC", idf, p);

        // Transfer
        strcpy(buf, class->name);
        strcat(buf, "__v_transfer");
        char* name = dups(alc, buf);
        strcpy(buf, class->ir_name);
        strcat(buf, "__v_transfer");
        char* export_name = dups(alc, buf);
        Func *transfer = func_make(b->alc, class->unit, class->scope, name, export_name);
        transfer->class = class;
        transfer->is_static = false;
        transfer->use_if_class_is_used = true;
        map_set_force_new(class->funcs, "_v_transfer", transfer);

        // Mark
        strcpy(buf, class->name);
        strcat(buf, "__v_mark");
        name = dups(alc, buf);
        strcpy(buf, class->ir_name);
        strcat(buf, "__v_mark");
        export_name = dups(alc, buf);
        Func *mark = func_make(b->alc, class->unit, class->scope, name, export_name);
        mark->class = class;
        mark->is_static = false;
        mark->use_if_class_is_used = true;
        map_set_force_new(class->funcs, "_v_mark", mark);

        // Mark GC
        strcpy(buf, class->name);
        strcat(buf, "__v_mark_shared");
        name = dups(alc, buf);
        strcpy(buf, class->ir_name);
        strcat(buf, "__v_mark_shared");
        export_name = dups(alc, buf);
        Func *mark_shared = func_make(b->alc, class->unit, class->scope, name, export_name);
        mark_shared->class = class;
        mark_shared->is_static = false;
        mark_shared->use_if_class_is_used = true;
        map_set_force_new(class->funcs, "_v_mark_shared", mark_shared);

        // Share
        strcpy(buf, class->name);
        strcat(buf, "__v_share");
        name = dups(alc, buf);
        strcpy(buf, class->ir_name);
        strcat(buf, "__v_share");
        export_name = dups(alc, buf);
        Func *share = func_make(b->alc, class->unit, class->scope, name, export_name);
        share->class = class;
        share->is_static = false;
        share->use_if_class_is_used = true;
        map_set_force_new(class->funcs, "_v_share", share);

        // Free
        strcpy(buf, class->name);
        strcat(buf, "__v_free");
        name = dups(alc, buf);
        strcpy(buf, class->ir_name);
        strcat(buf, "__v_free");
        export_name = dups(alc, buf);
        Func *fr = func_make(b->alc, class->unit, class->scope, name, export_name);
        fr->class = class;
        fr->is_static = false;
        fr->use_if_class_is_used = true;
        map_set_force_new(class->funcs, "_v_free", fr);

        // AST
        class_generate_transfer(p, b, class, transfer);
        class_generate_mark(p, b, class, mark);
        class_generate_mark_shared(p, b, class, mark_shared);
        class_generate_share(p, b, class, share);
        class_generate_free(p, b, class, fr);
    }
}

void class_generate_transfer(Parser* p, Build* b, Class* class, Func* func) {

    Map* props = class->props;

    Str* code = b->str_buf;
    str_clear(code);

    str_flat(code, "() void {\n");
    str_flat(code, "  if @ptrv(this, u8, -8) > 2 : return\n");
    str_flat(code, "  @ptrv(this, u8, -8) = 4\n");
    str_flat(code, "  GC_ACTIVE_SIZE += SIZE\n");

    str_flat(code, "  let index = @ptrv(this, u8, -4) @as uint\n");
    str_flat(code, "  let data = (this @as ptr) - index * SIZE - 8\n");
    str_flat(code, "  @ptrv(data, uint, -2)++\n");

    // Props
    loop(props->values, i) {
        ClassProp* p = array_get_index(props->values, i);
        char* pn = array_get_index(props->keys, i);
        if(!type_is_gc(p->type))
            continue;
        Class* subclass = p->type->class;
        char var[32];
        strcpy(var, "prop_");
        itos(i, var + 5, 10);

        str_flat(code, "let ");
        str_add(code, var);
        str_flat(code, " = this.");
        str_add(code, pn);
        str_flat(code, "\n");
        if(p->type->nullable) {
            str_flat(code, "if isset(");
            str_add(code, var);
            str_flat(code, ") {\n");
        }
        // if(subclass->use_gc_alloc) {
        //     str_flat(code, "GC_TRANSFER_FUNC(");
        //     str_add(code, var);
        //     str_flat(code, ")\n");
        // } else {
            str_add(code, var);
            str_flat(code, "._v_transfer()\n");
        // }

        str_add(code, var);
        str_flat(code, "._RC++\n");

        if(p->type->nullable) {
            str_flat(code, "}\n");
        }
    }

    Func* hook = map_get(class->funcs, "_gc_transfer");
    if(hook) {
        str_flat(code, "  this._gc_transfer()\n");
    }

    str_flat(code, "}\n");

    char* content = str_to_chars(b->alc, code);
    Chunk *chunk = chunk_make(b->alc, b, NULL);
    chunk_set_content(b, chunk, content, code->length);

    *p->chunk = *chunk;
    parse_handle_func_args(p, func);
}
void class_generate_mark(Parser* p, Build* b, Class* class, Func* func) {

    Map* props = class->props;

    Str* code = b->str_buf;
    str_clear(code);

    str_flat(code, "(age: u8) void {\n");
    str_flat(code, "  if @ptrv(this, u8, -8) > 8 { return }\n");
    str_flat(code, "  if @ptrv(this, u8, -7) == age { return }\n");
    str_flat(code, "  @ptrv(this, u8, -7) = age\n");
    str_flat(code, "  GC_MARK_SIZE += SIZE\n");

    // Props
    loop(props->values, i) {
        ClassProp* p = array_get_index(props->values, i);
        char* pn = array_get_index(props->keys, i);
        if(!type_is_gc(p->type))
            continue;
        char var[32];
        strcpy(var, "prop_");
        itos(i, var + 5, 10);

        str_flat(code, "let ");
        str_add(code, var);
        str_flat(code, " = this.");
        str_add(code, pn);
        str_flat(code, "\n");
        if(p->type->nullable) {
            str_flat(code, "if isset(");
            str_add(code, var);
            str_flat(code, ") : ");
        }
        // if(p->type->class->use_gc_alloc) {
        //     str_flat(code, "GC_MARK_FUNC(");
        //     str_add(code, var);
        //     str_flat(code, ", age)\n");
        // } else {
            str_add(code, var);
            str_flat(code, "._v_mark(age)\n");
        // }
    }

    if (map_get(class->funcs, "_gc_mark")) {
        str_flat(code, "  this._gc_mark(age)\n");
    }

    str_flat(code, "}\n");

    char* content = str_to_chars(b->alc, code);
    Chunk *chunk = chunk_make(b->alc, b, NULL);
    chunk_set_content(b, chunk, content, code->length);

    *p->chunk = *chunk;
    parse_handle_func_args(p, func);
}

void class_generate_mark_shared(Parser* p, Build* b, Class* class, Func* func) {

    Map* props = class->props;

    Str* code = b->str_buf;
    str_clear(code);

    str_flat(code, "(age: u8) void {\n");
    str_flat(code, "  if @ptrv(this, u8, -6) == age { return }\n");
    str_flat(code, "  @ptrv(this, u8, -6) = age\n");

    // Props
    loop(props->values, i) {
        ClassProp* p = array_get_index(props->values, i);
        char* pn = array_get_index(props->keys, i);
        if(!type_is_gc(p->type))
            continue;
        char var[32];
        strcpy(var, "prop_");
        itos(i, var + 5, 10);

        str_flat(code, "let ");
        str_add(code, var);
        str_flat(code, " = this.");
        str_add(code, pn);
        str_flat(code, "\n");
        if(p->type->nullable) {
            str_flat(code, "if isset(");
            str_add(code, var);
            str_flat(code, ") : ");
        }
        // if(p->type->class->use_gc_alloc) {
        //     str_flat(code, "GC_MARK_SHARED_FUNC(");
        //     str_add(code, var);
        //     str_flat(code, ", age)\n");
        // } else {
            str_add(code, var);
            str_flat(code, "._v_mark_shared(age)\n");
        // }
    }

    if (map_get(class->funcs, "_gc_mark_shared")) {
        str_flat(code, "  this._gc_mark_shared(age)\n");
    }

    str_flat(code, "}\n");

    char* content = str_to_chars(b->alc, code);
    Chunk *chunk = chunk_make(b->alc, b, NULL);
    chunk_set_content(b, chunk, content, code->length);

    *p->chunk = *chunk;
    parse_handle_func_args(p, func);
}

void class_generate_share(Parser* p, Build* b, Class* class, Func* func) {

    Map* props = class->props;

    Str* code = b->str_buf;
    str_clear(code);

    str_flat(code, "() void {\n");
    str_flat(code, "  let state = @ptrv(this, u8, -8)\n");
    str_flat(code, "  if state > 4 { return }\n");
    str_flat(code, "  @ptrv(this, u8, -8) = 10\n");
    str_flat(code, "  @ptrv(this, u8, -6) = GC_AGE\n");

    str_flat(code, "  if state < 4 {\n");
    str_flat(code, "  let index = @ptrv(this, u8, -4) @as uint\n");
    str_flat(code, "  let data = (this @as ptr) - index * SIZE - 8\n");
    str_flat(code, "  @ptrv(data, uint, -2)++\n");
    // str_flat(code, "  GC_ACTIVE_SIZE += SIZE\n");
    str_flat(code, "  }\n");

    str_flat(code, "  STACK.add_shared(this)\n");

    // Props
    loop(props->values, i) {
        ClassProp* p = array_get_index(props->values, i);
        char* pn = array_get_index(props->keys, i);
        if(!type_is_gc(p->type))
            continue;
        char var[32];
        strcpy(var, "prop_");
        itos(i, var + 5, 10);

        str_flat(code, "let ");
        str_add(code, var);
        str_flat(code, " = this.");
        str_add(code, pn);
        str_flat(code, "\n");
        if(p->type->nullable) {
            str_flat(code, "if isset(");
            str_add(code, var);
            str_flat(code, ") : ");
        }
        // if(p->type->class->use_gc_alloc) {
        //     str_flat(code, "GC_SHARE_FUNC(");
        //     str_add(code, var);
        //     str_flat(code, ")\n");
        // } else {
            str_add(code, var);
            str_flat(code, "._v_share()\n");
        // }
    }

    Func* hook = map_get(class->funcs, "_gc_share");
    if(hook) {
        str_flat(code, "  this._gc_share()\n");
    }

    str_flat(code, "}\n");

    char* content = str_to_chars(b->alc, code);
    Chunk *chunk = chunk_make(b->alc, b, NULL);
    chunk_set_content(b, chunk, content, code->length);

    *p->chunk = *chunk;
    parse_handle_func_args(p, func);
}

void class_generate_free(Parser* p, Build* b, Class* class, Func* func) {

    Map* props = class->props;

    Str* code = b->str_buf;
    str_clear(code);

    str_flat(code, "() void {\n");
    str_flat(code, "  if @ptrv(this, u8, -8) != 4 : return\n");
    str_flat(code, "  @ptrv(this, u8, -8) = 0\n");
    str_flat(code, "  @ptrv(this, u8, -7) = 0\n");
    str_flat(code, "  @ptrv(this, u8, -6) = 0\n");

    str_flat(code, "  GC_ACTIVE_SIZE -= SIZE\n");

    str_flat(code, "  let index = @ptrv(this, u8, -4) @as uint\n");
    str_flat(code, "  let data = (this @as ptr) - index * SIZE - 8\n");
    str_flat(code, "  @ptrv(data, uint, -2)--\n");

    // Props
    loop(props->values, i) {
        ClassProp* p = array_get_index(props->values, i);
        char* pn = array_get_index(props->keys, i);
        if(!type_is_gc(p->type))
            continue;
        char var[32];
        strcpy(var, "prop_");
        itos(i, var + 5, 10);

        str_flat(code, "let ");
        str_add(code, var);
        str_flat(code, " = this.");
        str_add(code, pn);
        str_flat(code, "\n");
        if(p->type->nullable) {
            str_flat(code, "if isset(");
            str_add(code, var);
            str_flat(code, ") {\n");
        }
        // if(p->type->class->use_gc_alloc) {
        //     str_flat(code, "GC_FREE_FUNC(");
        //     str_add(code, var);
        //     str_flat(code, ")\n");
        // } else {
            // str_add(code, var);
            // str_flat(code, "._v_free()\n");
        // }

        str_flat(code, "if atomic(");
        str_add(code, var);
        str_flat(code, "._RC - 1) == 1 : ");
        str_add(code, var);
        str_flat(code, "._v_free()\n");

        if(p->type->nullable) {
            str_flat(code, "}\n");
        }
    }

    if (map_get(class->funcs, "_gc_free")) {
        str_flat(code, "  this._gc_free()\n");
    }

    str_flat(code, "}\n");

    char* content = str_to_chars(b->alc, code);
    Chunk *chunk = chunk_make(b->alc, b, NULL);
    chunk_set_content(b, chunk, content, code->length);

    *p->chunk = *chunk;
    parse_handle_func_args(p, func);
}

Class* get_generic_class(Parser* p, Class* class, Array* generic_types) {
    Build* b = p->b;
    if(b->parse_last) {
        parse_err(p, -1, "You cannot generate a new generic class inside a #parse_last function");
    }
    //
    Str* hash = build_get_str_buf(b);
    loop(generic_types, i) {
        Type* type = array_get_index(generic_types, i);
        type_to_str_buf_append(type, hash);
        str_flat(hash, "|");
    }
    char* h = str_temp_chars(hash);
    Class *gclass = map_get(class->generics, h);
    if (gclass) {
        build_return_str_buf(b, hash);
        return gclass;
    }
    if(b->verbose > 2)
        printf("Create new generic class for: %s\n", h);

    // Generate new
    h = str_to_chars(b->alc, hash);

    // Name
    str_clear(hash);
    str_add(hash, class->name);
    str_flat(hash, "[");
    loop(generic_types, i) {
        if(i > 0)
            str_flat(hash, ", ");
        char buf[256];
        Type* type = array_get_index(generic_types, i);
        type_to_str(type, buf);
        str_add(hash, buf);
    }
    str_flat(hash, "]");
    char* name = str_to_chars(b->alc, hash);

    // Export name
    str_clear(hash);
    str_add(hash, class->ir_name);
    str_flat(hash, "__");
    loop(generic_types, i) {
        if(i > 0)
            str_flat(hash, ", ");
        char buf[256];
        Type* type = array_get_index(generic_types, i);
        type_to_str_export(type, buf);
        str_add(hash, buf);
    }
    str_flat(hash, "__");
    char* export_name = str_to_chars(b->alc, hash);

    gclass = class_make(b->alc, b, p->unit, class->type);
    gclass->body = chunk_clone(b->alc, class->body);
    gclass->scope = scope_sub_make(b->alc, sc_default, class->scope->parent);
    gclass->type = class->type;
    gclass->b = class->b;
    gclass->act = class->act;
    gclass->fc = class->fc;
    gclass->packed = class->packed;
    gclass->generic_of = class;

    gclass->name = name;
    gclass->ir_name = export_name;

    array_push(b->classes, gclass);
    array_push(gclass->unit->classes, gclass);
    // if (b->building_ast) {
        // if (gclass->type == ct_class) {
        //     gclass->gc_vtable_index = ++b->gc_vtables;
        // }
    // }

    map_set(class->generics, h, gclass);

    // Set type identifiers
    loop(generic_types, i) {
        char* name = array_get_index(class->generic_names, i);
        Type* type_ = array_get_index(generic_types, i);
        Type* type = type_clone(b->alc, type_);
        Idf* idf = idf_make(b->alc, idf_type, type);
        scope_set_idf(gclass->scope, name, idf, p);
    }
    // Set CLASS identifier
    Idf* idf = idf_make(b->alc, idf_class, gclass);
    scope_set_idf(gclass->scope, "SELF", idf, p);

    // Create new parser
    parser_new_context(&p);

    // Stage 2
    p->scope = gclass->scope;
    *p->chunk = *gclass->body;
    stage_props_class(p, gclass, false);

    // Class size
    int size = class_determine_size(b, gclass);
    if(size == -1) {
        parse_err(p, -1, "Cannot determine size of class: '%s'", gclass->name);
    }
    // Internals
    class_generate_internals(p, b, gclass);

    // Types
    stage_types_class(p, gclass);
    Array* funcs = gclass->funcs->values;
    loop(funcs, i) {
        Func* func = array_get_index(funcs, i);
        stage_types_func(p, func);
    }
    validate_class(p, gclass);

    // Return parser
    parser_pop_context(&p);

    //
    build_return_str_buf(b, hash);
    return gclass;
}


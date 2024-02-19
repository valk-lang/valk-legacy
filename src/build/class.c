
#include "../all.h"

Class* class_make(Allocator* alc, Build* b, int type) {
    Class* c = al(alc, sizeof(Class));
    c->b = b;
    c->type = type;
    c->name = NULL;
    c->ir_name = NULL;
    c->body = NULL;
    c->scope = NULL;
    c->props = map_make(alc);
    c->funcs = map_make(alc);
    //
    c->size = -1;
    c->gc_fields = 0;
    c->gc_vtable_index = 0;
    //
    c->packed = false;
    c->is_signed = true;
    //
    c->generics = NULL;
    c->generic_names = NULL;
    c->generic_types = NULL;
    c->is_generic_base = false;
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
    // if (!class->packed) {
    //     if (largest > b->ptr_size)
    //         largest = b->ptr_size;
    //     int rem = size % largest;
    //     size += rem;
    // }

    class->size = size;

    return size;
}


Class* get_generic_class(Fc* fc, Class* class, Map* generic_types) {
    Build* b = fc->b;
    //
    Str* hash = build_get_str_buf(b);
    Array* names = generic_types->keys;
    Array* types = generic_types->values;
    for (int i = 0; i < types->length; i++) {
        char* name = array_get_index(names, i);
        Type* type = array_get_index(types, i);
        type_to_str_append(type, hash);
        str_flat(hash, "|");
    }
    char* h = str_temp_chars(hash);
    Class* gclass = map_get(class->generics, h);
    if (gclass) {
        build_return_str_buf(b, hash);
        return gclass;
    }
    // Generate new
    h = str_to_chars(b->alc, hash);

    // Name
    str_clear(hash);
    str_add(hash, class->name);
    str_flat(hash, "[");
    for (int i = 0; i < types->length; i++) {
        if(i > 0)
            str_flat(hash, ", ");
        char buf[256];
        Type* type = array_get_index(types, i);
        type_to_str(type, buf);
        str_add(hash, buf);
    }
    str_flat(hash, "]");
    char* name = str_to_chars(b->alc, hash);

    // Export name
    str_clear(hash);
    str_add(hash, class->name);
    str_flat(hash, "__");
    for (int i = 0; i < types->length; i++) {
        if(i > 0)
            str_flat(hash, ", ");
        char buf[256];
        Type* type = array_get_index(types, i);
        type_to_str_export(type, buf);
        str_add(hash, buf);
    }
    str_flat(hash, "__");
    char* export_name = str_to_chars(b->alc, hash);

    gclass = class_make(b->alc, b, ct_struct);
    gclass->body = chunk_clone(b->alc, class->body);
    gclass->scope = scope_sub_make(b->alc, sc_default, class->fc->scope, NULL);
    gclass->type = class->type;
    gclass->b = class->b;
    gclass->fc = class->fc;
    gclass->packed = class->packed;

    gclass->name = name;
    gclass->ir_name = gen_export_name(gclass->fc->nsc, export_name);

    map_set(class->generics, h, gclass);

    // Set type identifiers
    for (int i = 0; i < types->length; i++) {
        char* name = array_get_index(names, i);
        Type* type = array_get_index(types, i);
        Idf* idf = idf_make(b->alc, idf_type, type);
        scope_set_idf(gclass->scope, name, idf, fc);
    }
    // Set CLASS identifier
    Idf* idf = idf_make(b->alc, idf_class, gclass);
    scope_set_idf(gclass->scope, "CLASS", idf, fc);

    // Save chunk for parser
    Chunk ch;
    ch = *fc->chunk_parse;

    // Stage 2
    stage_props_class(fc, gclass);
    // Class size
    int size = class_determine_size(b, gclass);
    if(size == -1) {
        sprintf(b->char_buf, "Cannot determine size of class: '%s'\n", gclass->name);
        parse_err(fc->chunk_parse, b->char_buf);
    }
    // Types
    stage_types_class(fc, gclass);
    Array* funcs = gclass->funcs->values;
    for (int i = 0; i < funcs->length; i++) {
        Func* func = array_get_index(funcs, i);
        stage_types_func(fc, func);
    }

    // Restore chunk
    *fc->chunk_parse = ch;
    //
    build_return_str_buf(b, hash);
    return gclass;
}

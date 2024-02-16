
#include "../all.h"

Class* class_make(Allocator* alc, Build* b, int type) {
    Class* c = al(alc, sizeof(Class));
    c->b = b;
    c->type = type;
    c->name = NULL;
    c->ir_name = NULL;
    c->body = NULL;
    c->scope = NULL;
    c->size = -1;
    c->gc_fields = 0;
    // c->size = 200; // Temporary
    c->props = map_make(alc);
    c->funcs = map_make(alc);
    c->packed = false;
    c->is_signed = true;
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



#include "../all.h"

Class* class_make(Allocator* alc, Build* b, int type) {
    Class* c = al(alc, sizeof(Class));
    c->b = b;
    c->type = type;
    c->name = NULL;
    c->ir_name = NULL;
    c->body = NULL;
    c->size = 0;
    c->size = 200; // Temporary
    c->props = map_make(alc);
    c->funcs = map_make(alc);
    c->packed = false;
    c->is_signed = true;
    return c;
}


bool class_determine_size(Build* b, Class* class) {

    int type = class->type;
    if(type == ct_bool) {
        class->size = 1;
        return true;
    }
    if(type == ct_ptr) {
        class->size = b->ptr_size;
        return true;
    }
    if(type == ct_int) {
        class->size = 1;
        return true;
    }

    return false;
}


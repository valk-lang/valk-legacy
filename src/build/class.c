
#include "../all.h"

Class* class_make(Allocator* alc, Build* b, int type) {
    Class* c = al(alc, sizeof(Class));
    c->b = b;
    c->type = type;
    c->name = NULL;
    c->ir_name = NULL;
    c->body = NULL;
    c->size = 0;
    c->props = map_make(alc);
    c->funcs = map_make(alc);
    c->packed = false;
    return c;
}

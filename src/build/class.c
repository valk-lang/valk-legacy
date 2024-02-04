
#include "../all.h"

Class* class_make(Allocator* alc, Build* b) {
    Class* c = al(alc, sizeof(Class));
    c->b = b;
    c->name = NULL;
    c->body = NULL;
    c->size = 0;
    return c;
}

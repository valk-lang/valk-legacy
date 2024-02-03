
#include "../all.h"

Class* class_make(Allocator* alc) {
    Class* c = al(alc, sizeof(Class));
    c->name = NULL;
    c->body = NULL;
    c->size = 0;
    return c;
}

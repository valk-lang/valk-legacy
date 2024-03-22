
#include "../all.h"

Parser* pool_get_parser(Unit* u) {
    Array* pool = u->pool_parsers;
    Parser* p = array_pop(pool);
    if(!p) {
        p = parser_make(u->b->alc, u);
    }
    return p;
}
void pool_return_parser(Unit* u, Parser* p) {
    array_push(u->pool_parsers, p);
}


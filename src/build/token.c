
#include "../all.h"

Token *token_make(Allocator *alc, int type, void *item) {
    Token *t = al(alc, sizeof(Token));
    t->type = type;
    t->item = item;
    return t;
}

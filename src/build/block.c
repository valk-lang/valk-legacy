
#include "../all.h"

Block* block_make(Allocator* alc, char* name) {
    Block* b = al(alc, sizeof(Block));
    b->ast = array_make(alc, 4);
    b->name = name;
    return b;
}

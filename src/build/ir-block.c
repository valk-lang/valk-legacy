
#include "../all.h"

IRBlock *ir_block_make(IR *ir, IRFunc *func) {
    char *name = al(ir->alc, 20);
    sprintf(name, "block_%d", func->blocks->length);

    IRBlock *block = al(ir->alc, sizeof(IRBlock));
    block->name = name;
    block->code = str_make(ir->alc, 500);

    array_push(func->blocks, block);

    return block;
}

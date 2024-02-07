
#include "../all.h"

IRBlock *ir_block_make(IR *ir, IRFunc *func) {
    char *name = al(ir->alc, 20);
    strcpy(name, "block_");
    itoa(func->blocks->length, name + 6, 10);

    IRBlock *block = al(ir->alc, sizeof(IRBlock));
    block->name = name;
    block->code = str_make(ir->alc, 400);

    array_push(func->blocks, block);

    return block;
}

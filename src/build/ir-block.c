
#include "../all.h"

IRBlock *ir_block_make(IR *ir, IRFunc *func, char* prefix) {
    int len = strlen(prefix);
    char *name = al(ir->alc, len + 10);
    strcpy(name, prefix);
    itoa(func->blocks->length, name + len, 10);

    IRBlock *block = al(ir->alc, sizeof(IRBlock));
    block->name = name;
    block->code = str_make(ir->alc, 400);

    array_push(func->blocks, block);

    return block;
}

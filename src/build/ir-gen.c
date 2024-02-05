
#include "../all.h"

char *ir_var(IRFunc* func) {
    char *res = al(func->ir->alc, 10);
    sprintf(res, "%%.%d", func->var_count++);
    return res;
}

void ir_jump(Str* code, IRBlock* block) {
    str_append_chars(code, "  br label %");
    str_append_chars(code, block->name);
    str_append_chars(code, "\n");
}

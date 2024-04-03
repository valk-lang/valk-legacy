
#include "../all.h"

IR* ir_make(Unit* u) {
    //
    Build* b = u->b;
    Allocator* alc = b->alc;

    IR* ir = al(alc, sizeof(IR));
    ir->unit = u;
    ir->b = b;
    ir->alc = b->alc_ast;
    ir->char_buf = b->char_buf;

    ir->code_struct = str_make(alc, 2000);
    ir->code_global = str_make(alc, 4000);
    ir->code_extern = str_make(alc, 4000);
    ir->code_attr = str_make(alc, 4000);

    ir->func = NULL;
    ir->block = NULL;

    ir->block_after = NULL;
    ir->block_cond = NULL;
    ir->vscope_after = NULL;
    ir->vscope_values = NULL;

    ir->funcs = array_make(alc, 50);
    ir->attrs = array_make(alc, 50);
    ir->declared_funcs = array_make(alc, 50);
    ir->declared_classes = array_make(alc, 50);
    ir->declared_globals = array_make(alc, 50);

    ir->di_cu = NULL;
    ir->di_file = NULL;
    ir->di_retained_nodes = NULL;
    ir->di_type_ptr = NULL;

    ir->string_count = 0;
    ir->attr_count = 0;

    ir->debug = false;

    return ir;
}

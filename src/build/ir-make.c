
#include "../all.h"

IR* ir_make(Unit* u, Parser* p) {
    //
    Build* b = u->b;
    Allocator* alc = b->alc_ast;

    IR* ir = al(alc, sizeof(IR));
    ir->unit = u;
    ir->parser = p;
    ir->b = b;
    ir->alc = alc;
    ir->char_buf = b->char_buf;

    ir->code_final = str_make(alc, 50000);
    ir->code_struct = str_make(alc, 10000);
    ir->code_global = str_make(alc, 10000);
    ir->code_extern = str_make(alc, 10000);
    ir->code_attr = str_make(alc, 10000);

    ir->func = NULL;
    ir->block = NULL;

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

    ir_gen_globals(ir);
    ir_gen_functions(ir);

    ir_gen_final(ir);

    return ir;
}

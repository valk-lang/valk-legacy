
#include "../all.h"

IR* ir_make(Fc* fc) {
    //
    Build* b = fc->b;
    Allocator* alc = fc->alc_ast;

    IR* ir = al(alc, sizeof(IR));
    ir->fc = fc;
    ir->b = b;
    ir->alc = alc;

    ir->code_final = str_make(alc, 50000);
    ir->code_struct = str_make(alc, 10000);
    ir->code_global = str_make(alc, 10000);
    ir->code_extern = str_make(alc, 10000);
    ir->code_attr = str_make(alc, 10000);

    ir->func = NULL;
    ir->funcs = array_make(alc, 50);
    ir->attrs = array_make(alc, 50);
    ir->declared_funcs = array_make(alc, 50);
    ir->declared_classes = array_make(alc, 50);
    ir->globals = map_make(alc);

    ir->di_cu = NULL;
    ir->di_file = NULL;
    ir->di_retained_nodes = NULL;
    ir->di_type_ptr = NULL;

    ir->c_string = 0;
    ir->c_attr = 0;

    ir->debug = false;

    ir_gen_globals(ir);
    ir_gen_functions(ir);

    ir_gen_final(ir);

    return ir;
}

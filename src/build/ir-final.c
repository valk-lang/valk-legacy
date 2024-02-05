
#include "../all.h"

void ir_gen_final(IR* ir) {
    //
    Str* code = ir->code_final;

    // Structs & Globals
    str_append(code, ir->code_struct);
    str_append_chars(code, "\n");
    str_append(code, ir->code_global);
    str_append_chars(code, "\n");

    // Functions
    for (int i = 0; i < ir->funcs->length; i++) {
        IRFunc *func = array_get_index(ir->funcs, i);
        str_append(code, func->code);
    }
    str_append_chars(code, "\n");

    // Extern
    str_append(code, ir->code_extern);
    str_append_chars(code, "\n");

    // Attrs
    for (int i = 0; i < ir->attrs->length; i++) {
        str_append_chars(code, array_get_index(ir->attrs, i));
        str_append_chars(code, "\n");
    }
    str_append(code, ir->code_attr);
    str_append_chars(code, "\n");
}

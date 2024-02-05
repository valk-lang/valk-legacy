
#include "../all.h"

char* ir_value(IR* ir, Scope* scope, Value* v) {

    if (v->type == v_string) {
        return ir_string(ir, v->item);
    }
    if (v->type == v_func_call) {
        VFuncCall *fcall = v->item;
        char *on = ir_value(ir, scope, fcall->on);
        Array *values = ir_fcall_args(ir, scope, fcall->args);
        char *res = ir_func_call(ir, on, values, ir_type(ir, v->rett), fcall->line, fcall->col);
        return res;
    }
    if (v->type == v_func_ptr) {
        VFuncPtr *fptr = v->item;
        return ir_func_ptr(ir, fptr->func);
    }

    return "???";
}

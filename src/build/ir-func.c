
#include "../all.h"

void ir_gen_func(IR *ir, IRFunc *func);

void ir_gen_functions(IR* ir) {
    //
    Array* funcs = ir->fc->funcs;

    for (int i = 0; i < funcs->length; i++) {
        Func* vfunc = array_get_index(funcs, i);
        IRFunc* func = al(ir->alc, sizeof(IRFunc));
        func->ir = ir;
        func->func = vfunc;
        //
        func->blocks = array_make(ir->alc, 20);
        //
        func->stack_save_vn = NULL;
        func->di_scope = NULL;
        //
        func->var_count = 0;

        IRBlock* start = ir_block_make(ir, func);
        IRBlock* code = ir_block_make(ir, func);

        func->block_start = start;
        func->block_code = code;
        func->block = code;

        array_push(ir->funcs, func);

        ir_gen_func(ir, func);
    }
}

void ir_gen_func(IR *ir, IRFunc *func) {
    //
    Str *start = func->block_start->code;
    Func* vfunc = func->func;

    Array *args = vfunc->args->values;
    int argc = args->length;

    str_append_chars(start, "define dso_local ");
    str_append_chars(start, ir_type(ir, vfunc->rett));
    str_append_chars(start, " @");
    str_append_chars(start, vfunc->export_name);
    str_append_chars(start, "(");

    // Args
    for (int i = 0; i < argc; i++) {
        FuncArg *arg = array_get_index(args, i);
        if (i > 0)
            str_append_chars(start, ", ");
        str_append_chars(start, ir_type(ir, arg->type));
        str_append_chars(start, " noundef ");
        if (arg->type->is_pointer && !arg->type->nullable)
            str_append_chars(start, "nonnull ");

        char *v = ir_var(func);
        str_append_chars(start, v);

        arg->decl->ir_var = v;
    }
    str_append_chars(start, ")");

    if(vfunc->is_inline)
        str_append_chars(start, " alwaysinline");

    // Body
    str_append_chars(start, " {\n");

    // End
    str_append_chars(start, "}\n\n");
}

char *ir_var(IRFunc* func) {
    char *res = al(func->ir->alc, 10);
    sprintf(res, "%%.%d", func->var_count++);
    return res;
}

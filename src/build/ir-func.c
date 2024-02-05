
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

        array_push(ir->funcs, func);

        ir_gen_func(ir, func);
    }
}

void ir_gen_func(IR *ir, IRFunc *func) {
    //
    Func* vfunc = func->func;
    // AST
    ir->func = func;
    ir->block = func->block_code;
    ir_write_ast(ir, vfunc->scope);
    ir->func = NULL;
    ir->block = NULL;

    // Jump from start to code block
    ir_jump(func->block_start->code, func->block_code);
}

void ir_func_definition(Str* code, IRFunc *func) {

    IR* ir = func->ir;
    Func* vfunc = func->func;

    Array *args = vfunc->args->values;
    int argc = args->length;

    str_append_chars(code, "define dso_local ");
    str_append_chars(code, ir_type(ir, vfunc->rett));
    str_append_chars(code, " @");
    str_append_chars(code, vfunc->export_name);
    str_append_chars(code, "(");

    // Args
    for (int i = 0; i < argc; i++) {
        FuncArg *arg = array_get_index(args, i);
        if (i > 0)
            str_append_chars(code, ", ");
        str_append_chars(code, ir_type(ir, arg->type));
        str_append_chars(code, " noundef ");
        if (arg->type->is_pointer && !arg->type->nullable)
            str_append_chars(code, "nonnull ");

        char *v = ir_var(func);
        str_append_chars(code, v);

        arg->decl->ir_var = v;
    }
    str_append_chars(code, ")");

    if(vfunc->is_inline)
        str_append_chars(code, " alwaysinline");

    str_append_chars(code, " {\n");
}

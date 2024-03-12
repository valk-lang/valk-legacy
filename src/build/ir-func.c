
#include "../all.h"

void ir_gen_func(IR *ir, IRFunc *func);

void ir_gen_functions(IR* ir) {
    //
    Array* funcs = ir->unit->funcs;

    for (int i = 0; i < funcs->length; i++) {
        Func* vfunc = array_get_index(funcs, i);
        if(vfunc->in_header)
            continue;

        IRFunc* func = al(ir->alc, sizeof(IRFunc));
        func->ir = ir;
        func->func = vfunc;
        //
        func->blocks = array_make(ir->alc, 20);
        //
        func->stack_save_vn = NULL;
        func->di_scope = NULL;
        func->gc_stack = NULL;
        func->gc_stack_adr = NULL;
        func->gc_stack_adr_val = NULL;
        //
        func->var_count = 0;
        func->gc_count = 0;

        IRBlock* start = ir_block_make(ir, func, "start_");
        IRBlock* code = ir_block_make(ir, func, "code_");

        func->block_start = start;
        func->block_code = code;

        array_push(ir->funcs, func);

        ir_gen_func(ir, func);
    }
}

void ir_gen_func(IR *ir, IRFunc *func) {
    Allocator* alc = ir->alc;
    Build* b = ir->b;
    Func* vfunc = func->func;

    // Arg vars
    Array *args = vfunc->args->values;
    for (int i = 0; i < args->length; i++) {
        FuncArg *arg = array_get_index(args, i);
        Decl* decl = arg->decl;
        char* var = ir_var(func);
        decl->ir_var = var;
        if(decl->is_mut) {
            decl->ir_store_var = ir_alloca(ir, func, decl->type);
        }
    }

    // Decls
    Scope* scope = vfunc->scope;
    Array* decls = scope->decls;
    for (int i = 0; i < decls->length; i++) {
        Decl* decl = array_get_index(decls, i);
        if(decl->is_mut && !decl->is_gc) {
            decl->ir_store_var = ir_alloca(ir, func, decl->type);
        }
    }

    ir->func = func;
    ir->block = func->block_code;

    if(ir->b->verbose > 2)
        printf("> Func IR: %s\n", vfunc->export_name);

    // Start GC
    if(func->func == ir->b->func_main) {
        // Gc
        Func* gc_start = get_volt_class_func(b, "mem", "GcManager", "init");
        ir_func_call(ir, ir_func_ptr(ir, gc_start), NULL, ir_type(ir, gc_start->rett), 0, 0);
        Func* stack_new = get_volt_class_func(b, "mem", "Stack", "init");
        char* stack_ob = ir_func_call(ir, ir_func_ptr(ir, stack_new), NULL, ir_type(ir, stack_new->rett), 0, 0);
    }

    // Store arg values
    for (int i = 0; i < args->length; i++) {
        FuncArg *arg = array_get_index(args, i);
        Decl* decl = arg->decl;
        if(decl->is_mut) {
            // Store passed argument in storage var
            ir_store_old(ir, decl->type, decl->ir_store_var, decl->ir_var);
        }
    }

    // AST
    ir_write_ast(ir, vfunc->scope);
    //
    if (!vfunc->scope->did_return) {
        ir_func_return(ir, NULL, "void");
    }

    // Jump from start to code block
    ir->block = func->block_start;
    ir_jump(ir, func->block_code);

    ir->func = NULL;
    ir->block = NULL;
}

void ir_func_definition(Str* code, IR* ir, Func *vfunc, bool is_extern) {

    Array *args = vfunc->args->values;
    int argc = args->length;

    if (is_extern) {
        str_flat(code, "declare ");
    } else {
        str_flat(code, "define dso_local ");
    }
    str_add(code, ir_type(ir, vfunc->rett));
    str_flat(code, " @");
    str_add(code, vfunc->export_name);
    str_flat(code, "(");

    // Args
    for (int i = 0; i < argc; i++) {
        FuncArg *arg = array_get_index(args, i);
        if (i > 0)
            str_flat(code, ", ");
        str_add(code, ir_type(ir, arg->type));
        str_flat(code, " noundef");

        Type *type = arg->type;
        bool notnull = type->type == type_struct && type->is_pointer && type->nullable == false;
        if (notnull)
            str_flat(code, " nonnull");

        if(!is_extern) {
            str_flat(code, " ");
            str_add(code, arg->decl->ir_var);
        }
    }
    if (is_extern) {
        str_flat(code, ")\n");
    }else {
        str_flat(code, ")");
        if (vfunc->is_inline)
            str_flat(code, " alwaysinline");

        str_flat(code, " {\n");
    }
}

void ir_define_ext_func(IR* ir, Func* func) {
    if (!func)
        return;
    if (func->unit == ir->unit)
        return;
    if(!array_contains(ir->declared_funcs, func, arr_find_adr)) {
        Str *code = ir->code_extern;
        ir_func_definition(code, ir, func, true);
        array_push(ir->declared_funcs, func);
    }
}

char *ir_alloca(IR *ir, IRFunc* func, Type *type) {
    IRBlock *block = func->block_start;
    Str *code = block->code;

    str_preserve(code, 200);

    char bytes[20];

    char *var = ir_var(func);
    str_flat(code, "  ");
    str_add(code, var);
    str_flat(code, " = alloca ");
    str_add(code, ir_type(ir, type));
    str_flat(code, ", align ");
    str_add(code, ir_type_align(ir, type, bytes));
    str_flat(code, "\n");

    return var;
}

char *ir_alloca_by_size(IR *ir, IRFunc* func, char* size) {
    IRBlock *block = func->block_start;
    Str *code = block->code;

    char *var = ir_var(func);
    str_flat(code, "  ");
    str_add(code, var);
    str_flat(code, " = alloca i8, i32 ");
    str_add(code, size);
    str_flat(code, ", align 8\n");

    return var;
}

void ir_func_return_nothing(IR* ir) {
    Func* func = ir->func->func;
    if(!func->rett || type_is_void(func->rett)) {
        ir_func_return(ir, NULL, "void");
    } else {
        if(func->rett->is_pointer) {
            ir_func_return(ir, "ptr", "null");
        } else {
            char* type = ir_type(ir, func->rett);
            ir_func_return(ir, type, "0");
        }
    }
}

void ir_func_return(IR* ir, char* type, char* value) {

    IRFunc* func = ir->func;
    Func* vfunc = func->func;
    if(vfunc->scope_stack_reduce) {
        Scope* sub = vfunc->scope_stack_reduce;
        ir_write_ast(ir, sub);
    }
    
    Str* code = ir->block->code;
    str_flat(code, "  ret ");
    if(type) {
        str_add(code, type);
        str_flat(code, " ");
    }
    str_add(code, value);
    str_flat(code, "\n");
}


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
    int gc_count = 0;
    int gc_index = 0;

    ir->func = func;
    ir->block = func->block_code;
    Str *code = ir->block->code;

    // Arg vars
    Func* vfunc = func->func;
    Array *args = vfunc->args->values;
    for (int i = 0; i < args->length; i++) {
        FuncArg *arg = array_get_index(args, i);
        Decl* decl = arg->decl;
        // if(decl->is_gc) {
        //     gc_count++;
        // }
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
        if(decl->is_gc) {
            gc_count++;
        }
        if(decl->is_mut) {
            decl->ir_store_var = ir_alloca(ir, func, decl->type);
        }
    }

    // Start GC
    if(func->func == ir->b->func_main) {
        Func* gc_start = get_volt_class_func(b, "mem", "Gc", "start");
        ir_func_call(ir, ir_func_ptr(ir, gc_start), NULL, ir_type(ir, gc_start->rett), 0, 0);
        Func* stack_new = get_volt_class_func(b, "mem", "Stack", "new");
        char* stack_ob = ir_func_call(ir, ir_func_ptr(ir, stack_new), NULL, ir_type(ir, stack_new->rett), 0, 0);
        Global* g = get_volt_global(b, "mem", "stack");
        char* stack = ir_global(ir, g);
        ir_store(ir, g->type, stack, stack_ob);
    }

    // GC reserve
    char* reserve_adr = NULL;
    if(gc_count > 0) {
        func->gc_count = gc_count;
        // Get stack object
        Global* g = get_volt_global(b, "mem", "stack");
        Value* stack = vgen_ir_cached(alc, value_make(alc, v_global, g, g->type));
        char* lstack = ir_value(ir, vfunc->scope, stack);
        // Increase stack
        // Class* class = g->type->class;
        // ClassProp* prop = map_get(class->props, "stack_adr");
        // char* stack_adr = ir_class_pa(ir, class, lstack, prop);
        // char* stack_adr_val = ir_load(ir, prop->type, stack_adr);
        // char* add = ir_var(func);
        // char buf[100];
        // sprintf(buf, "  %s = getelementptr inbounds ptr, ptr %s, i32 %d\n", add, stack_adr_val, gc_count * 2);
        // str_add(code, buf);
        // ir_store(ir, prop->type, stack_adr, add);
        // char* reserve_adr = stack_adr_val;

        func->gc_stack = lstack;
        // func->gc_stack_adr = stack_adr;
        // func->gc_stack_adr_val = stack_adr_val;

        // // Reserve args
        Array* args = array_make(alc, gc_count);
        Value* amount = vgen_int(alc, gc_count, type_gen_number(alc, b, b->ptr_size, false, false));
        array_push(args, stack);
        array_push(args, amount);
        // Call reserve
        Array *values = ir_fcall_args(ir, scope, args);
        Func* reserve = get_volt_class_func(b, "mem", "Stack", "reserve");
        reserve_adr = ir_func_call(ir, ir_func_ptr(ir, reserve), values, ir_type(ir, reserve->rett), 0, 0);

        // Set decl IR values
        for (int i = 0; i < decls->length; i++) {
            Decl* decl = array_get_index(decls, i);
            if(decl->is_gc) {
                char lindex[10];
                itoa(gc_index, lindex, 10);
                gc_index += 2;
                char *var = ir_var(ir->func);
                str_flat(code, "  ");
                str_add(code, var);
                str_flat(code, " = getelementptr inbounds ptr, ptr ");
                str_add(code, reserve_adr);
                str_flat(code, ", i32 ");
                str_add(code, lindex);
                str_flat(code, "\n");

                decl->ir_store_var = var;
                ir_store(ir, decl->type, var, "null");
            }
        }
    }

    // Store arg values
    for (int i = 0; i < args->length; i++) {
        FuncArg *arg = array_get_index(args, i);
        Decl* decl = arg->decl;
        // if(decl->is_gc) {
        //     char lindex[10];
        //     itoa(gc_index, lindex, 10);
        //     gc_index += 2;
        //     char *var = ir_var(ir->func);
        //     str_flat(code, "  ");
        //     str_add(code, var);
        //     str_flat(code, " = getelementptr inbounds ptr, ptr ");
        //     str_add(code, reserve_adr);
        //     str_flat(code, ", i32 ");
        //     str_add(code, lindex);
        //     str_flat(code, "\n");

        //     decl->ir_store_var = var;
        //     // Store passed argument in storage var
        //     ir_store(ir, decl->type, decl->ir_store_var, decl->ir_var);
        // } else if (decl->is_mut) {
        // }
        if(decl->is_mut) {
            // Store passed argument in storage var
            ir_store(ir, decl->type, decl->ir_store_var, decl->ir_var);
        }
    }

    // AST
    ir->func = func;
    ir->block = func->block_code;

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
    if(!array_contains(ir->declared_funcs, func, arr_find_adr)) {
        Str *code = ir->code_extern;
        ir_func_definition(code, ir, func, true);
        array_push(ir->declared_funcs, func);
    }
}

char *ir_alloca(IR *ir, IRFunc* func, Type *type) {
    IRBlock *block = func->block_start;
    Str *code = block->code;

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


void ir_func_return(IR* ir, char* type, char* value) {
    IRFunc* func = ir->func;
    int gc_count = func->gc_count;
    if(gc_count > 0) {
        Allocator* alc = ir->alc;
        Build* b = ir->b;
        Scope *scope = func->func->scope;
        // Call reserve function
        Global* g = get_volt_global(b, "mem", "stack");
        Value* stack = vgen_ir_cached(alc, value_make(alc, v_global, g, g->type));
        // Pop args
        Array* args = array_make(alc, gc_count);
        Value* amount = vgen_int(alc, gc_count, type_gen_number(alc, b, b->ptr_size, false, false));
        array_push(args, stack);
        array_push(args, amount);
        // Call pop
        Array *values = ir_fcall_args(ir, scope, args);
        Func* pop = get_volt_class_func(b, "mem", "Stack", "pop");
        ir_func_call(ir, ir_func_ptr(ir, pop), values, ir_type(ir, pop->rett), 0, 0);

        // ir_store(ir, type_gen_volt(ir->alc, ir->b, "ptr"), func->gc_stack_adr, func->gc_stack_adr_val);
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

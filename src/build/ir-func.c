
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
    Func* vfunc = func->func;
    int gc_count = func_get_reserve_count(vfunc);
    func->gc_count = gc_count;

    Scope* gc_reserve = NULL;
    if(gc_count > 0) {
        Map *idfs = map_make(alc);
        Value *amount = vgen_int(alc, gc_count, type_gen_number(alc, b, b->ptr_size, false, false));
        Idf *idf = idf_make(alc, idf_value, amount);
        map_set(idfs, "amount", idf);
        Fc* fc = ir->fc;
        gc_reserve = gen_snippet_ast(alc, ir->fc, get_volt_snippet(ir->b, "mem", "reserve"), idfs, vfunc->scope);
        vfunc->scope_gc_pop = gen_snippet_ast(alc, ir->fc, get_volt_snippet(ir->b, "mem", "pop"), map_make(alc), vfunc->scope);
    }

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
        if(decl->is_mut) {
            decl->ir_store_var = ir_alloca(ir, func, decl->type);
        }
    }

    ir->func = func;
    ir->block = func->block_code;

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
    if(gc_reserve) {
        ir_write_ast(ir, gc_reserve);

        // Get stack object
        // Global* g = get_volt_global(b, "mem", "stack");
        // Value* stack = vgen_ir_cached(alc, value_make(alc, v_global, g, g->type));
        // char* lstack = ir_value(ir, vfunc->scope, stack);
        // // Increase stack
        // Class* class = g->type->class;
        // ClassProp* prop_stack_adr = map_get(class->props, "stack_adr");
        // char* stack_adr = ir_class_pa(ir, class, lstack, prop_stack_adr);
        // char* stack_adr_val = ir_load(ir, prop_stack_adr->type, stack_adr);
        // // Add
        // char* add = ir_var(func);
        // char buf[100];
        // sprintf(buf, "  %s = getelementptr inbounds ptr, ptr %s, i32 %d\n", add, stack_adr_val, gc_count * 2);
        // str_add(code, buf);
        // // Store
        // ir_store(ir, prop_stack_adr->type, stack_adr, add);
        // reserve_adr = stack_adr_val;

        // // Compare lowest
        // ClassProp* prop_next = map_get(class->props, "lowest_next");
        // char* next_var = ir_class_pa(ir, class, lstack, prop_stack_adr);
        // char* next_val = ir_load(ir, prop_next->type, stack_adr);
        // Type* type_next = prop_next->type;
        // char* type_next_ir = ir_type(ir, type_next);
        // char* comp = ir_compare(ir, op_lt, stack_adr_val, next_val, type_next_ir, type_next->is_signed, false);
        // IRBlock *block_if = ir_block_make(ir, ir->func, "if_gc_next_");
        // IRBlock *after = ir_block_make(ir, ir->func, "if_gc_next_");
        // char *lcond_i1 = ir_i1_cast(ir, comp);
        // ir_cond_jump(ir, lcond_i1, block_if, after);
        // ir->block = block_if;
        // code = block_if->code;
        // ir_store(ir, type_next, next_var, stack_adr_val);
        // ir->block = after;
        // code = after->code;

        // func->gc_stack = lstack;
        // func->gc_stack_adr = stack_adr;
        // func->gc_stack_adr_val = stack_adr_val;

        // // Reserve args
        // Array* args = array_make(alc, gc_count);
        // Value* amount = vgen_int(alc, gc_count, type_gen_number(alc, b, b->ptr_size, false, false));
        // array_push(args, stack);
        // array_push(args, amount);
        // // Call reserve
        // Array *values = ir_fcall_args(ir, scope, args);
        // Func* reserve = get_volt_class_func(b, "mem", "Stack", "reserve");
        // reserve_adr = ir_func_call(ir, ir_func_ptr(ir, reserve), values, ir_type(ir, reserve->rett), 0, 0);

        // Set decl IR values
        int gc_index = 0;
        Idf* idf = map_get(gc_reserve->identifiers, "STACK_ADR");
        Value* vc = idf->item;
        char* reserve_adr = ir_value(ir, scope, vc);
        Str *code = ir->block->code;

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
            }
        }
    }

    // Store arg values
    for (int i = 0; i < args->length; i++) {
        FuncArg *arg = array_get_index(args, i);
        Decl* decl = arg->decl;
        if(decl->is_mut) {
            // Store passed argument in storage var
            ir_store(ir, decl->type, decl->ir_store_var, decl->ir_var);
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
    if(func->func->scope_gc_pop) {
        Allocator* alc = ir->alc;
        Build* b = ir->b;

        ir_write_ast(ir, func->func->scope_gc_pop);

        // Map *idfs = map_make(alc);
        // Value *amount = vgen_int(alc, gc_count, type_gen_number(alc, b, b->ptr_size, false, false));
        // Idf *idf = idf_make(alc, idf_value, amount);
        // map_set(idfs, "amount", idf);
        // Fc* fc = ir->fc;
        // Scope* ast = gen_snippet_ast(alc, ir->fc, get_volt_snippet(ir->b, "mem", "pop"), idfs, ir->func->func->scope);
        // ir_write_ast(ir, ast);

        // Scope *scope = func->func->scope;
        // // Call reserve function
        // Global* g = get_volt_global(b, "mem", "stack");
        // Value* stack = vgen_ir_cached(alc, value_make(alc, v_global, g, g->type));
        // // Pop args
        // Array* args = array_make(alc, gc_count);
        // Value* amount = vgen_int(alc, gc_count, type_gen_number(alc, b, b->ptr_size, false, false));
        // array_push(args, stack);
        // array_push(args, amount);
        // // Call pop
        // Array *values = ir_fcall_args(ir, scope, args);
        // Func* pop = get_volt_class_func(b, "mem", "Stack", "pop");
        // ir_func_call(ir, ir_func_ptr(ir, pop), values, ir_type(ir, pop->rett), 0, 0);

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

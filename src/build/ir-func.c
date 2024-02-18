
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

    if(ir->b->verbose > 2)
        printf("> Func IR: %s\n", vfunc->export_name);

    // Start GC
    if(func->func == ir->b->func_main) {

        // Gc
        Func* gc_start = get_volt_class_func(b, "mem", "GcManager", "init");
        ir_func_call(ir, ir_func_ptr(ir, gc_start), NULL, ir_type(ir, gc_start->rett), 0, 0);
        Func* stack_new = get_volt_class_func(b, "mem", "Stack", "init");
        char* stack_ob = ir_func_call(ir, ir_func_ptr(ir, stack_new), NULL, ir_type(ir, stack_new->rett), 0, 0);

        // Set globals
        Array* strings = b->strings;
        Class* class = get_volt_class(ir->b, "type", "String");
        ClassProp* prop = map_get(class->props, "data");
        ClassProp* prop_len = map_get(class->props, "bytes");
        // ClassProp* prop_state = map_get(class->props, "GC_state");
        if(!prop || !prop_len) {
            build_err(ir->b, "Missing String.data or String.bytes");
        }
        Func *gc_init_bytes = get_volt_func(b, "mem", "gc_initialize_bytes");
        if (!gc_init_bytes) {
            build_err(ir->b, "Missing mem function: gc_initialize_bytes");
        }
        Type *type_u8 = type_gen_volt(ir->alc, b, "u8");
        Type *type_u16 = type_gen_volt(ir->alc, b, "u16");
        Type *type_ptr = type_gen_volt(ir->alc, b, "ptr");
        for (int i = 0; i < strings->length; i++) {
            str_preserve(ir->block->code, 200);
            VString* str = array_get_index(strings, i);
            char* pa = ir_class_pa(ir, class, str->ir_object_name, prop);
            ir_store(ir, pa, str->ir_body_name, "ptr", b->ptr_size);
            char* pa_len = ir_class_pa(ir, class, str->ir_object_name, prop_len);
            char* len = ir_int(ir, strlen(str->body));
            ir_store(ir, pa_len, len, "i16", b->ptr_size);
            // Make const
            // char* pa_state = ir_class_pa(ir, class, str->ir_object_name, prop_state);
            // ir_store(ir, pa_state, ir_int(ir, 14), "i8", b->ptr_size);
            char class_size[20];
            itoa(class->size, class_size, 10);
            Array *types = array_make(ir->alc, 4);
            array_push(types, type_ptr);
            array_push(types, type_u8);
            array_push(types, type_u8);
            array_push(types, type_u16);
            Array *values = array_make(ir->alc, 4);
            array_push(values, str->ir_object_name);
            array_push(values, "14");
            array_push(values, "0");
            array_push(values, class_size);
            Array *args = ir_fcall_ir_args(ir, values, types);
            ir_func_call(ir, ir_func_ptr(ir, gc_init_bytes), args, ir_type(ir, gc_init_bytes->rett), 0, 0);
        }
    }

    // GC reserve
    if(gc_reserve) {
        ir_write_ast(ir, gc_reserve);

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
    if(func->func->scope_gc_pop) {
        Allocator* alc = ir->alc;
        Build* b = ir->b;
        ir_write_ast(ir, func->func->scope_gc_pop);
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

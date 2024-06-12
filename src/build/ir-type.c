
#include "../all.h"

char *ir_type(IR *ir, Type *type) {
    //
    if (!type)
        return "void";
    if (type->is_pointer)
        return "ptr";

    Class *class = type->class;
    char name[256];
    char array_len[64];

    if (type_is_void(type)) {
        return "void";
    } else if (type->type == type_bool) {
        return "i1";
    } else if (type->type == type_error) {
        return "i32";
    } else if (type->type == type_struct) {

        ir_define_struct(ir, class);

        strcpy(name, "\%struct.");
        strcpy(name + 8, class->ir_name);
        return dups(ir->alc, name);

    } else if (type->type == type_int) {
        int bytes = type->size;
        return ir_type_int(ir, bytes);
    } else if (type->type == type_float) {
        int bytes = type->size;
        return ir_type_float(ir, bytes);
    } else if (type->type == type_static_array) {
        char* sub_type = ir_type(ir, type->array_type);
        strcpy(name, "[ ");
        strcat(name, itos(type->array_size, array_len, 10));
        strcat(name, " x ");
        strcat(name, sub_type);
        strcat(name, " ]");
        return dups(ir->alc, name);
    }

    printf("Type: %d\n", type->type);
    printf("Type: %s\n", type_to_str(type, name));
    printf("Unknown IR type (compiler bug)\n");
    raise(11);
    return NULL;
}

// char *ir_type_real(IR *ir, Type *type) {
//     Class *class = type->class;
//     if (class && type->type == type_struct) {
//         Str *result = str_make(ir->alc, 256);
//         int depth = 1;

//         ir_define_struct(ir, class);

//         char name[256];
//         strcpy(name, "\%struct.");
//         strcpy(name + 8, class->ir_name);
//         str_add(result, name);
//         //
//         while (depth > 0) {
//             str_flat(result, "*");
//             depth--;
//         }
//         //
//         return str_to_chars(ir->alc, result);
//     }

//     return ir_type(ir, type);
// }

char *ir_type_int(IR *ir, int bytes) {
    if (bytes == 1) {
        return "i8";
    } else if (bytes == 2) {
        return "i16";
    } else if (bytes == 4) {
        return "i32";
    } else if (bytes == 8) {
        return "i64";
    }

    printf("Namespace: %s\n", ir->unit->nsc->name);
    printf("Size: %d\n", bytes);
    die("Unsupported integer size (IR Generator)");
    return "";
}

char *ir_type_float(IR *ir, int bytes) {
    if (bytes == 4) {
        return "float";
    } else if (bytes == 8) {
        return "double";
    }
    printf("Namespace: %s\n", ir->unit->nsc->name);
    printf("Size: %d\n", bytes);
    die("Unsupported float size (IR Generator)");
    return "";
}

char *ir_type_align(IR *ir, Type *type, char* result) {
    int abytes = type->size;
    if (abytes > ir->b->ptr_size) {
        abytes = ir->b->ptr_size;
    }
    itos(abytes, result, 10);
    return result;
}

#include "../all.h"

v_i64 vnumber_value_i64(VNumber* num) {
    v_i64 val = (v_i64)num->value_uint;
    if(num->negative)
        val *= -1;
    return val;
}

Value* try_convert_number(Allocator* alc, Build* b, Value* val, Type* type) {
    if (val->type != v_number)
        return val;
    // Float
    if(val->rett->type == type_float) {
        if (type->type == type_float) {
            // Float -> float : adopt type
            val->rett = type;
        }
        // Float -> int : ignore
        return val;
    }

    // Integers
    VNumber* vn = val->item;
    if(number_fits_type(vn->value_uint, vn->negative, type)) {
        val->rett = type;
    }
    return val;
}

void match_value_types(Allocator* alc, Build* b, Value** v1_, Value** v2_) {
    //
    Value* v1 = *v1_;
    Value* v2 = *v2_;
    Type* t1 = v1->rett;
    Type* t2 = v2->rett;
    bool is_signed = t1->is_signed || t2->is_signed;
    bool is_float = t1->type == type_float || t2->type == type_float;
    int size = is_float ? max_num(t1->size, t2->size) : (max_num(t1->is_signed != is_signed ? t1->size * 2 : t1->size, t2->is_signed != is_signed ? t2->size * 2 : t2->size));
    Type* type = type_gen_number(alc, b, size, is_float, is_signed);
    if(!type)
        return;
    if(t1->type != type->type || t1->is_signed != type->is_signed || t1->size != type->size) {
        *v1_ = vgen_cast(alc, v1, type);
    }
    if(t2->type != type->type || t2->is_signed != type->is_signed || t2->size != type->size) {
        *v2_ = vgen_cast(alc, v2, type);
    }
}

bool number_fits_type(v_u64 val, bool negative, Type* type) {
    if (type->type != type_int)
        return false;
    if (type->size > sizeof(v_u64))
        return false;

    // negative value in unsigned type
    bool is_signed = type->is_signed;
    if(!is_signed && negative)
        return false;

    // check value
    v_u64 type_bits = type->size * 8;
    v_u64 max_bits = sizeof(v_u64) * 8;
    if(is_signed)
        type_bits--;
    v_u64 max = (v_u64)-1 >> (max_bits - type_bits);
    if (negative)
        max++;
    return val <= max;
}


#ifndef H_NUMBERS
#define H_NUMBERS

#include "typedefs.h"

v_i64 vnumber_value_i64(VNumber* num);
void match_value_types(Allocator* alc, Build* b, Value** v1_, Value** v2_);
Value* try_convert_number(Allocator* alc, Build* b, Value* val, Type* type);
bool number_fits_type(v_u64 val, bool negative, Type* type);

#endif

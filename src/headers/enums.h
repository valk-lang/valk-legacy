
enum LEX_TOKENS {
    tok_eof,
    tok_none,
    tok_space,
    tok_newline,
    tok_id,
    tok_at_word,
    tok_number,
    tok_string,
    tok_char_string,
    tok_op3,
    tok_op2, // 10
    tok_op1,
    tok_char,
    tok_scope_open,
    tok_scope_close,
    tok_pos,
    tok_cc,
};

enum ACCESS {
    act_public,
    act_private_fc,
    act_private_nsc,
    act_private_pkc,
    act_readonly_fc,
    act_readonly_nsc,
    act_readonly_pkc,
};

enum IDF {
    idf_func,
    idf_decl,
    idf_nsc,
    idf_scope,
    idf_class,
};

enum TYPES {
    type_void,
    type_ptr,
    type_struct,
    type_int,
    type_float,
    type_func,
    type_bool,
};

enum CLASSTYPES {
    ct_struct,
    ct_class,
    ct_ptr,
    ct_int,
    ct_float,
    ct_bool,
};

enum VALUES {
    v_decl,
    v_class_pa,
    v_ptrv,
    v_global,
    v_func_ptr,
    v_func_call,
    v_int,
    v_cast,
    v_string,
    v_op,
    v_compare,
    v_class_init,
    v_incr,
};

enum TOKENS {
    t_assign,
    t_statement,
    t_return,
    t_declare,
};

enum OPERATORS {
    op_add,
    op_sub,
    op_mul,
    op_div,
    op_mod,
    // Compare
    op_eq,
    op_ne,
    op_lt,
    op_gt,
    op_lte,
    op_gte,
    // Bit
    op_bit_and,
    op_bit_or,
    op_bit_xor,
    op_shl,
    op_shr,
};

enum SCOPES {
    sc_default,
    sc_func,
};
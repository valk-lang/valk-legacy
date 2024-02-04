
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
};

enum CLASSTYPES {
    ct_struct,
    ct_class,
    ct_ptr,
    ct_int,
    ct_float,
};

enum VALUES {
    v_decl,
    v_class_pa,
    v_ptrv,
    v_global,
    v_func_ptr,
    v_func_call,
    v_int,
};

enum TOKENS {
    t_assign,
    t_statement,
};
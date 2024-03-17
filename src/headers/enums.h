
enum LEX_TOKENS {
    tok_eof,
    tok_none,
    // Data tokens
    tok_data_pos = 2,
    tok_data_scope_end = 3,
    //
    tok_space,
    tok_newline,
    //
    tok_id,
    tok_number,
    tok_string,
    tok_char,
    tok_at_word,
    // Brackets
    tok_bracket_open,
    tok_bracket_close,
    tok_sq_bracket_open,
    tok_sq_bracket_close,
    tok_curly_open,
    tok_curly_close,
    tok_ltcurly_open,
    // Operators
    tok_plus,
    tok_sub,
    tok_mul,
    tok_div,
    tok_mod,
    // ++ --
    tok_plusplus,
    tok_subsub,
    // assign
    tok_eq,
    tok_plus_eq,
    tok_sub_eq,
    tok_mul_eq,
    tok_div_eq,
    // Compare
    tok_eqeq,
    tok_lt,
    tok_lte,
    tok_gt,
    tok_gte,
    tok_not_eq,
    // && ||
    tok_or,
    tok_and,
    // Bit ops
    tok_shl,
    tok_shr,
    tok_bit_or,
    tok_bit_and,
    tok_bit_xor,
    // ! ? : ; @ . ~ , #
    tok_not,
    tok_qmark,
    tok_colon,
    tok_semi,
    tok_at,
    tok_dot,
    tok_tilde,
    tok_comma,
    tok_hashtag,
    tok_triple_sub,
    tok_eqgt,
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
    idf_decl_overwrite,
    idf_nsc,
    idf_scope,
    idf_class,
    idf_global,
    idf_value_alias,
    idf_snippet,
    idf_value,
    idf_type,
    idf_cached_value,
    idf_error,
    idf_trait,
};

enum TYPES {
    type_void,
    type_ptr,
    type_struct,
    type_int,
    type_float,
    type_func, // 5
    type_bool,
    type_undefined,
    type_null,
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
    v_ptr_offset,
    v_global,
    v_func_ptr,
    v_func_call, // 5
    v_number,
    v_cast,
    v_string,
    v_op,
    v_compare, // 10
    v_class_init,
    v_incr,
    v_null,
    v_stack,
    v_atomic, // 15
    v_ptr_of,
    v_ir_cached,
    v_gc_link,
    v_gc_buffer,
    v_ir_value, // 20
    v_gc_get_table,
    v_value_scope,
    v_var,
    v_isset,
    v_and_or, // 25
    v_not,
    v_this_or_that,
};

enum TOKENS {
    t_assign,
    t_statement,
    t_return,
    t_declare, // 3
    t_if,
    t_while, // 5
    t_break,
    t_continue,
    t_throw,
    t_set_var,
    t_ast_scope,
    t_set_decl_store_var,
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
    //
    op_and,
    op_or,
};

enum SCOPES {
    sc_default,
    sc_func,
    sc_loop,
    sc_if,
};

enum SNIPPET_ARG_TYPES {
    snip_value,
    snip_type,
};
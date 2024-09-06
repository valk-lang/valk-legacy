
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
    // ! ? ?? : ; @ . ~ , # --- =>
    tok_not,
    tok_qmark,
    tok_qqmark,
    tok_colon,
    tok_semi,
    tok_at,
    tok_dot,
    tok_tilde,
    tok_comma,
    tok_hashtag,
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
    idf_type_alias,
    idf_snippet,
    idf_value,
    idf_type,
    idf_error,
    idf_trait,
    idf_macro,
    idf_macro_items,
    idf_macro_item,
    idf_cc_object_prop,
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
    type_static_array,
    type_error, // 10
    type_promise,
    type_multi,
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
    v_decl_overwrite,
    v_class_pa,
    v_ptrv,
    v_ptr_offset,
    v_global, // 5
    v_func_ptr,
    v_func_call, 
    v_number,
    v_cast,
    v_string, // 10
    v_op,
    v_compare, 
    v_class_init,
    v_incr,
    v_null, // 15
    v_stack,
    v_atomic,
    v_ptr_of,
    v_ir_cached,
    v_gc_link, // 20
    v_bufferd,
    v_ir_value, 
    v_UNUSED_1,
    v_value_scope,
    v_var, // 25
    v_isset,
    v_and_or,
    v_not,
    v_this_or_that,
    v_vscope, // 30
    v_null_alt_value,
    v_await,
    v_setjmp,
    v_longjmp,
    v_frameptr, // 35
    v_stackptr,
    v_memset,
    v_this_but_that,
    v_multi,
    v_errh, // 40
    v_phi,
    v_bit_lz,
    v_memcpy,
    v_undefined
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
    t_ast_scope, // 10
    t_set_return_value,
    t_each,
    t_return_vscope,
    t_yield,
    t_decl_set_store,
    t_decl_set_arg,
    t_disabled,
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
    sc_vscope,
};

enum SNIPPET_ARG_TYPES {
    snip_value,
    snip_type,
};

enum TARGET {
    os_linux,
    os_win,
    os_macos,
    os_bsd,
    os_other,
    arch_x86,
    arch_x64,
    arch_arm64,
    arch_other,
};

enum LINK {
    link_default,
    link_dynamic,
    link_static,
};

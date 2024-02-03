
enum TOKENS {
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
};

enum TYPES {
    type_void,
};
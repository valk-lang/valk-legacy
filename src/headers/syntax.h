
#ifndef H_SYNTAX
#define H_SYNTAX

#include "typedefs.h"

bool is_alpha_char(char c);
bool is_valid_varname_char(char c);
bool is_valid_varname_first_char(char c);
bool is_number(char c);
bool is_hex_char(char c);
bool is_octal_char(char c);
bool is_whitespace(char c);
bool is_newline(char c);
bool is_valid_varname(char *name);
bool is_valid_varname_all(char *name);
bool is_valid_number(char *str);
bool is_valid_hex_number(char *str);
bool is_valid_octal_number(char *str);
bool is_valid_macro_number(char *str);
bool ends_with(const char *str, const char *suffix);
bool starts_with(const char *a, const char *b);
char backslash_char(char ch);
bool str_is(const char* tkn, const char* comp);
bool str_in(char* tkn, char* comp);
char *string_replace_backslash_chars(Allocator *alc, char *body);
char* op_to_str(int op);
void char_to_hex(const unsigned char ch, char* buf);
char* itos(v_i64 val, char* buf, const int base);
v_i64 hex2int(char *hex);
v_i64 oct2int(char *oct);

#endif

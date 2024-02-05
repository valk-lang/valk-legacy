
#ifndef H_SYNTAX
#define H_SYNTAX

#include "typedefs.h"

bool is_alpha_char(char c);
bool is_valid_varname_char(char c);
bool is_number(char c);
bool is_hex_char(char c);
bool is_whitespace(char c);
bool is_newline(char c);
bool is_valid_varname(char *name);
bool is_valid_varname_all(char *name);
bool is_valid_number(char *str);
bool is_valid_hex_number(char *str);
bool is_valid_macro_number(char *str);
bool ends_with(const char *str, const char *suffix);
bool starts_with(const char *a, const char *b);
char backslash_char(char ch);
bool str_is(char* tkn, char* comp);
bool str_in(char* tkn, char* comp);
char *string_replace_backslash_chars(Allocator *alc, char *body);

#endif
